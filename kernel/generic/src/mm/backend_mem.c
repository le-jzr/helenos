/*
 * Copyright (c) 2022 Jiří Zárevúcky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_generic_mm
 * @{
 */

/**
 * @file
 * @brief	Backend for address space areas backed by mem_t.
 *
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <align.h>
#include <genarch/mm/page_ht.h>
#include <genarch/mm/page_pt.h>
#include <mem.h>
#include <mm/as.h>
#include <mm/km.h>
#include <mm/page.h>
#include <mm/reserve.h>

static bool _create(as_area_t *);
static void _destroy(as_area_t *);

static bool _is_resizable(as_area_t *);
static bool _is_shareable(as_area_t *);

static int _page_fault(as_area_t *, uintptr_t, pf_access_t);
static void _frame_free(as_area_t *, uintptr_t, uintptr_t, pte_t *);

mem_backend_t mem_backend = {
	.create = _create,
	.resize = NULL,
	.share = NULL,
	.destroy = _destroy,

	.is_resizable = _is_resizable,
	.is_shareable = _is_shareable,

	.page_fault = _page_fault,
	.frame_free = _frame_free,

	.create_shared_data = NULL,
	.destroy_shared_data = NULL,
};

bool _create(as_area_t *area)
{
	return true;
}

void _destroy(as_area_t *area)
{
	mem_put(area->backend_data.mem);
	area->backend_data.mem = NULL;
}

bool _is_resizable(as_area_t *area)
{
	return false;
}

bool _is_shareable(as_area_t *area)
{
	return false;
}

int _page_fault(as_area_t *area, uintptr_t upage, pf_access_t access)
{
	assert(page_table_locked(AS));
	assert(mutex_locked(&area->lock));
	assert(IS_ALIGNED(upage, PAGE_SIZE));

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	size_t mem_offset = area->backend_data.mem_offset + (upage - area->base);

	bool write = (access == PF_ACCESS_WRITE);
	bool cow = area->backend_data.mem_cow;

	// Look up frame in the memory span.
	uintptr_t frame = mem_lookup(area->backend_data.mem, mem_offset, true);

	if (!frame) {
		// Failed allocating a frame.
		return AS_PF_SILENT;
	}

	bool new_mapping = true;

	if (write && cow) {
		// Copy-on-write page.
		// We allocate a fresh frame and fill it in from the template.

		if (!reserve_try_alloc(1))
			return AS_PF_SILENT;

		uintptr_t new_frame;
		uintptr_t dest_kpage = km_temporary_page_get(&new_frame, FRAME_NO_RESERVE);
		assert(new_frame != 0);
		assert(dest_kpage != 0);

		// Temporarily map the source page.
		uintptr_t src_kpage = km_map(frame, PAGE_SIZE, PAGE_SIZE, PAGE_READ | PAGE_CACHEABLE);
		assert(src_kpage != 0);
		memcpy((void *) dest_kpage, (void *) src_kpage, PAGE_SIZE);
		km_unmap(src_kpage, PAGE_SIZE);

		km_temporary_page_put(dest_kpage);
		frame = new_frame;

		// Check whether we need to remove an existing non-writable mapping.
		assert(frame != 0);

		pte_t pte;
		if (page_mapping_find(AS, upage, false, &pte) && PTE_PRESENT(&pte)) {
			assert(!PTE_WRITABLE(&pte));

			// Remove the mapping.
			ipl_t ipl = tlb_shootdown_start(TLB_INVL_PAGES, AS->asid, upage, 1);
			page_mapping_remove(AS, upage);
			tlb_invalidate_pages(AS->asid, upage, 1);
			as_invalidate_translation_cache(AS, upage, 1);
			tlb_shootdown_finalize(ipl);

			new_mapping = false;
		}
	}

	unsigned flags = as_area_get_flags(area);
	if (cow && !write)
		flags &= ~PAGE_WRITE;

	/*
	 * Map 'upage' to 'frame'.
	 * Note that TLB shootdown is not attempted as only new information is
	 * being inserted into page tables.
	 */
	page_mapping_insert(AS, upage, frame, flags);
	if (new_mapping && !used_space_insert(&area->used_space, upage, 1))
		panic("Cannot insert used space, page = 0x%" PRIxPTR ".", upage);

	return AS_PF_OK;
}

void _frame_free(as_area_t *area, uintptr_t page, uintptr_t frame, pte_t *pte)
{
	assert(page_table_locked(area->as));
	assert(mutex_locked(&area->lock));

	// No COW means there's no frames that belong to us.
	// Everything is referenced indirectly through mem_t,
	// and will be freed when the mem_t is destroyed.
	if (!area->backend_data.mem_cow)
		return;

	if (PTE_WRITABLE(pte)) {
		// If the area is copy-on-write, free frames that are writable in the page table,
		// as those are the local copies.
		frame_free(frame, 1);
	}
}

/** @}
 */
