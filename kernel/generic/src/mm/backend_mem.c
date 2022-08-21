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

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#include <align.h>
#include <mem.h>
#include <mm/as.h>
#include <mm/page.h>
#include <mm/km.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>

static bool _create(as_area_t *);
static bool _resize(as_area_t *, size_t);
static void _share(as_area_t *);
static void _destroy(as_area_t *);

static bool _is_resizable(as_area_t *);
static bool _is_shareable(as_area_t *);

static int _page_fault(as_area_t *, uintptr_t, pf_access_t);
static void _frame_free(as_area_t *, uintptr_t, uintptr_t, pte_t *);

static atomic_uintptr_t zero_frame = ATOMIC_VAR_INIT(0);

mem_backend_t mem_backend = {
	.create = _create,
	.resize = _resize,
	.share = _share,
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

bool _resize(as_area_t *area, size_t new_pages)
{
	return false;
}

void _share(as_area_t *area)
{
	panic("not shareable");
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

static uintptr_t get_zero_frame(void)
{
	uintptr_t frame = atomic_load_explicit(&zero_frame, memory_order_acquire);
	if (frame == 0) {
		// Lazily allocate the first time we need it.
		uintptr_t new_zero_frame;
		uintptr_t kpage = km_temporary_page_get(&new_zero_frame, 0);
		assert(new_zero_frame != 0);
		assert(kpage != 0);
		memset((void *) kpage, 0, PAGE_SIZE);
		km_temporary_page_put(kpage);

		// Atomic compare-exchange in case another thread did it first.
		if (!atomic_compare_exchange_strong_explicit(&zero_frame, &frame, new_zero_frame,
				memory_order_acq_rel, memory_order_acquire))
			frame_free(new_zero_frame, 1);
	}
	return frame;
}

int _page_fault(as_area_t *area, uintptr_t upage, pf_access_t access)
{
	assert(page_table_locked(AS));
	assert(mutex_locked(&area->lock));
	assert(IS_ALIGNED(upage, PAGE_SIZE));

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	int64_t mem_offset = area->backend_data.mem_offset + (upage - area->base);

	bool write = (access == PF_ACCESS_WRITE);
	bool cow = area->backend_data.mem_cow;
	bool copy = write && cow;
	bool alloc = write && !cow;

	// Look up frame in the memory span.
	uintptr_t frame = mem_lookup(area->backend_data.mem, mem_offset, alloc);

	if (frame == 0 && alloc) {
		// Failed allocating a frame.
		return AS_PF_FAULT;
	}

	if (copy) {
		// Copy-on-write page.
		// We allocate a fresh frame and fill it in from the template.

		uintptr_t new_frame;
		uintptr_t dest_kpage = km_temporary_page_get(&new_frame, 0);
		assert(new_frame != 0);
		assert(dest_kpage != 0);

		if (frame == 0) {
			// We're "copying" an unallocated (i.e. never touched) page,
			// so it's just filled with zeroes.
			memsetb((void *)dest_kpage, PAGE_SIZE, 0);
		} else {
			// Temporarily map the source page.
			uintptr_t src_kpage = km_map(frame, PAGE_SIZE, PAGE_SIZE, 0);
			assert(src_kpage != 0);
			memcpy((void *) dest_kpage, (void *) src_kpage, PAGE_SIZE);
			km_unmap(src_kpage, PAGE_SIZE);
		}

		km_temporary_page_put(dest_kpage);
		frame = new_frame;
	}

	// If true, then this is the page mapping's final form, and won't change on write.
	bool final = write || (frame != 0 && !cow);
	bool new_mapping = true;

	// Check whether we need to remove an existing non-writable mapping.
	if (write) {
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

	// If the frame is not allocated yet, we use a singleton zero-filled page.
	if (frame == 0)
		frame = get_zero_frame();

	assert(frame != 0);

	// FIXME: Why do we have so many different ways to say READ/WRITE/EXEC? It's error prone as hell.
	unsigned flags = as_area_get_flags(area);
	if (!final)
		flags &= ~PAGE_WRITE;

	// Map 'upage' to 'frame'./
	// Note that TLB shootdown is not attempted as only new information is
	// being inserted into page tables.
	page_mapping_insert(AS, upage, frame, flags);
	if (new_mapping && !used_space_insert(&area->used_space, upage, 1))
		panic("Cannot insert used space, page = 0x%"PRIxPTR".", upage);

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
