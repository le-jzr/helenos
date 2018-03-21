/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genarchmm
 * @{
 */

/**
 * @file
 * @brief Virtual Address Translation for hierarchical 4-level page tables.
 */

#include <assert.h>
#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/km.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <arch/barrier.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <mem.h>
#include <align.h>
#include <macros.h>
#include <bitops.h>

static void pt_mapping_insert(as_t *, uintptr_t, uintptr_t, unsigned int);
static void pt_mapping_remove(as_t *, uintptr_t);
static bool pt_mapping_find(as_t *, uintptr_t, bool, pte_t *pte);
static void pt_mapping_update(as_t *, uintptr_t, bool, const pte_t *pte);
static void pt_mapping_make_global(uintptr_t, size_t);

page_mapping_operations_t pt_mapping_operations = {
	.mapping_insert = pt_mapping_insert,
	.mapping_remove = pt_mapping_remove,
	.mapping_find = pt_mapping_find,
	.mapping_update = pt_mapping_update,
	.mapping_make_global = pt_mapping_make_global
};

/** Map page to frame using hierarchical page tables.
 *
 * Map virtual address page to physical address frame
 * using flags.
 *
 * @param as    Address space to wich page belongs.
 * @param page  Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 *
 */
void pt_mapping_insert(as_t *as, uintptr_t page, uintptr_t frame,
    unsigned int flags)
{
	pte_t *ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);

	assert(page_table_locked(as));

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT) {
		pte_t *newpt = (pte_t *)
		    PA2KA(frame_alloc(PTL1_FRAMES, FRAME_LOWMEM, PTL1_SIZE - 1));
		memsetb(newpt, PTL1_SIZE, 0);
		SET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page), KA2PA(newpt));
		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page), PAGE_NEXT_LEVEL_PT);
	}

	pte_t *ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT) {
		pte_t *newpt = (pte_t *)
		    PA2KA(frame_alloc(PTL2_FRAMES, FRAME_LOWMEM, PTL2_SIZE - 1));
		memsetb(newpt, PTL2_SIZE, 0);
		SET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page), KA2PA(newpt));
		SET_PTL2_FLAGS(ptl1, PTL1_INDEX(page), PAGE_NEXT_LEVEL_PT);
	}

	pte_t *ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT) {
		pte_t *newpt = (pte_t *)
		    PA2KA(frame_alloc(PTL3_FRAMES, FRAME_LOWMEM, PTL2_SIZE - 1));
		memsetb(newpt, PTL2_SIZE, 0);
		SET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page), KA2PA(newpt));
		SET_PTL3_FLAGS(ptl2, PTL2_INDEX(page), PAGE_NEXT_LEVEL_PT);
	}

	pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	SET_FRAME_ADDRESS(ptl3, PTL3_INDEX(page), frame);
	SET_FRAME_FLAGS(ptl3, PTL3_INDEX(page), flags);
}

// Force the compiler to fully inline the recursion.
// The function is written in a way that makes it possible.
__attribute__((always_inline))
static inline _pt_mapping_remove_all(int level, paddr_t pt)
{
	// Never reached unless the page table is corrupted.
	if (level >= pt_num_levels())
		panic("Unexpected recursion in " __FUNC__ "().");

	int l = pt_num_levels() - level_todo;

	for (int i = 0; i < pt_levels[l].entries; i++) {
		paddr_t paddr;
		page_flags_t flags = pt_get_entry_by_index(l, PA2KA(pt), i, &paddr);

		// Just recursively free the allocated frames.
		// We don't need to bother with pt_coherence() here.
		if (flags & PAGE_NEXT_LEVEL_PT)
			_pt_mapping_remove_all(level + 1, paddr);
	}

	frame_free(pt, pt_levels[l].frames);
}

__attribute__((always_inline))
static inline _pt_mapping_remove_range(int level, paddr_t pt, vaddr_t vaddr, size_t size)
{
	

}

/** Remove mapping of page from hierarchical page tables.
 *
 * Remove any mapping of page within address space as.
 * TLB shootdown should follow in order to make effects of
 * this call visible.
 *
 * Empty page tables except PTL0 are freed.
 *
 * @param as   Address space to wich page belongs.
 * @param page Virtual address of the page to be demapped.
 *
 */
void pt_mapping_remove(as_t *as, uintptr_t page)
{
	assert(page_table_locked(as));

	/*
	 * First, remove the mapping, if it exists.
	 */

	pte_t *ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);
	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	pte_t *ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));
	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	pte_t *ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));
	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	/*
	 * Destroy the mapping.
	 * We need SET_FRAME_FLAGS for possible PT coherence maintenance.
	 * At least on ARM.
	 */
	SET_FRAME_FLAGS(ptl3, PTL3_INDEX(page), PAGE_NOT_PRESENT);

	/* ARM notably allows VIVT (Virtually Indexed Virtually Tagged)
	 * instruction caches, so we need to invalidate the page
	 * we unmapped.
	 * This is different from SET_FRAME_FLAGS(), which only
	 * ensures coherence of the page mapping entry, not of the
	 * memory being mapped/unmapped.
	 * Similar may be necessary on other plaftorms.
	 */
	pt_mapping_coherence(page, PAGE_SIZE);

	/*
	 * Second, free all empty tables along the way from PTL3 down to PTL0
	 * except those needed for sharing the kernel non-identity mappings.
	 */

	/* Check PTL3 */
	if (!PTL3_EMPTY(ptl3))
		return;

	/* Release the frame and remove PTL3 pointer from the parent
	 * table.
	 */
	if (PTL2_ENTRIES != 0) {
		SET_PTL3_FLAGS(ptl2, PTL2_INDEX(page), PAGE_NOT_PRESENT);
	} else if (PTL1_ENTRIES != 0) {
		SET_PTL2_FLAGS(ptl1, PTL1_INDEX(page), PAGE_NOT_PRESENT);
	} else {
		if (km_is_non_identity(page))
			return;

		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page), PAGE_NOT_PRESENT);
	}
	frame_free(KA2PA((uintptr_t) ptl3), PTL3_FRAMES);

	if (PTL2_ENTRIES != 0) {
		if (!PTL2_EMPTY(ptl2))
			return;

		/* Release the frame and remove PTL2 pointer from the parent
		 * table.
		 */
		if (PTL1_ENTRIES != 0) {
			SET_PTL2_FLAGS(ptl1, PTL1_INDEX(page), PAGE_NOT_PRESENT);
		} else {
			if (km_is_non_identity(page))
				return;

			SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page), PAGE_NOT_PRESENT);
		}
		frame_free(KA2PA((uintptr_t) ptl2), PTL2_FRAMES);

	}

	/* check PTL1, empty is still true */
	if (PTL1_ENTRIES != 0) {
		if (!PTL1_EMPTY(ptl1))
			return;

		/*
		 * PTL1 is empty.
		 * Release the frame and remove PTL1 pointer from the parent
		 * table.
		 */
		if (km_is_non_identity(page))
			return;

		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page), PAGE_NOT_PRESENT);
		frame_free(KA2PA((uintptr_t) ptl1), PTL1_FRAMES);
	}
}

void *pt_mapping_find_raw(as_t *as, vaddr_t vaddr, bool nolock)
{
	assert(nolock || page_table_locked(as));

	int last_level = pt_num_levels() - 1;

	paddr_t paddr = as->genarch.page_table;
	page_flags_t flags = PAGE_NEXT_LEVEL_PT;

	for (int l = 0; l < last_level; l++) {
		// TODO: Support large pages.
		if (flags != PAGE_NEXT_LEVEL_PT)
			return NULL;

		read_barrier();
		flags = pt_get_entry(l, PA2KA(paddr), &paddr);
	}

	if (flags == PAGE_NOT_PRESENT)
		return NULL;

	read_barrier();

	const size_t entry_size =
	    FRAMES2SIZE(pt_levels[last_level].frames) /
	    pt_levels[last_level].entries;

	return PA2KA(paddr) +
	    (entry_size * pt_index(last_level, vaddr));
}

page_flags_t pt_mapping_find(as_t *as, vaddr_t vaddr, bool nolock, paddr_t *paddr)
{
	assert(nolock || page_table_locked(as));

	paddr_t p = as->genarch.page_table;
	page_flags_t flags = PAGE_NEXT_LEVEL_PT;

	for (int l = 0; l < pt_num_levels; l++) {
		// TODO: Support large pages.
		if (flags != PAGE_NEXT_LEVEL_PT)
			return PAGE_NOT_PRESENT;

		read_barrier();
		vaddr_t vaddr = PA2KA(p);
		flags = pt_get_entry(l, vaddr, &p);
	}

	if (paddr != NULL)
		*paddr = p;

	return flags;
}


/** Update mapping for virtual page in hierarchical page tables.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param[in] pte  New PTE.
 */
void pt_mapping_update(as_t *as, uintptr_t page, bool nolock, const pte_t *pte)
{
	pte_t *t = pt_mapping_find_internal(as, page, nolock);
	if (!t)
		panic("Updating non-existent PTE");

	assert(PTE_VALID(t) == PTE_VALID(pte));
	assert(PTE_PRESENT(t) == PTE_PRESENT(pte));
	assert(PTE_GET_FRAME(t) == PTE_GET_FRAME(pte));
	assert(PTE_READABLE(t) == PTE_READABLE(pte));
	assert(PTE_WRITABLE(t) == PTE_WRITABLE(pte));
	assert(PTE_EXECUTABLE(t) == PTE_EXECUTABLE(pte));
	assert(PTE_CACHEABLE(t) == PTE_CACHEABLE(pte));

	*t = *pte;
}

/** Return the size of the region mapped by a single PTL0 entry.
 *
 * @return Size of the region mapped by a single PTL0 entry.
 */
static inline size_t ptl0_step_get(void)
{
	return 1UL << pt_levels[0].index_shift;
}

static inline int pt_num_levels()
{
	return sizeof(pt_levels) / sizeof(pt_levels[0]);
}

/** Make the mappings in the given range global across all address spaces.
 *
 * All PTL0 entries in the given range will be mapped to a next level page
 * table. The next level page table will be allocated and cleared.
 *
 * pt_mapping_remove() will never deallocate these page tables even when there
 * are no PTEs in them.
 *
 * @param as   Address space.
 * @param base Base address corresponding to the first PTL0 entry that will be
 *             altered by this function.
 * @param size Size in bytes defining the range of PTL0 entries that will be
 *             altered by this function.
 *
 */
void pt_mapping_make_global(vaddr_t base, size_t size)
{
	assert(size > 0);
	assert(pt_num_levels() > 1);

	vaddr_t ptl0 = PA2KA(AS_KERNEL->genarch.page_table);
	size_t ptl0_step = ptl0_step_get();
	size_t frames = pt_levels[1].frames;

	for (vaddr_t addr = ALIGN_DOWN(base, ptl0_step);
	    addr - 1 < (base - 1) + size;
	    addr += ptl0_step) {

		if (!(pt_get_entry(ptl0, NULL) & PAGE_NOT_PRESENT)) {
			assert(overlaps(addr, ptl0_step,
			    config.identity_base, config.identity_size));

			/*
			 * This PTL0 entry also maps the kernel identity region,
			 * so it is already global and initialized.
			 */
			continue;
		}

		vaddr_t l1 = PA2KA(frame_alloc(frames, FRAME_LOWMEM, 0));
		memsetb((void *) l1, FRAMES2SIZE(frames), 0);
		// Make sure the memset is visible to the page table walker.
		pt_coherence((void *) l1, FRAMES2SIZE(frames));

		pt_set_entry(0, ptl0, addr, l1,
		    PAGE_NEXT_LEVEL_PT | PAGE_GLOBAL), 
	}
}

/** @}
 */
