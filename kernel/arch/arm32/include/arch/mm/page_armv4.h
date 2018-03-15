/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Paging related declarations.
 */

#ifndef KERN_arm32_PAGE_armv4_H_
#define KERN_arm32_PAGE_armv4_H_

#ifndef KERN_arm32_PAGE_H_
#error "Do not include arch specific page.h directly use generic page.h instead"
#endif

/* Macros for querying the last-level PTE entries. */
#define PTE_VALID_ARCH(pte) \
	(((pte_t *) (pte))->l0.should_be_zero != 0 || PTE_PRESENT_ARCH(pte))
#define PTE_PRESENT_ARCH(pte) \
	(((pte_t *) (pte))->l0.descriptor_type != 0)
#define PTE_GET_FRAME_ARCH(pte) \
	(((uintptr_t) ((pte_t *) (pte))->l1.frame_base_addr) << FRAME_WIDTH)
#define PTE_WRITABLE_ARCH(pte) \
	(((pte_t *) (pte))->l1.access_permission_0 == PTE_AP_USER_RW_KERNEL_RW)
#define PTE_EXECUTABLE_ARCH(pte) \
	1

#ifndef __ASSEMBLER__

/** Level 0 page table entry. */
typedef struct {
	/* 0b01 for coarse tables, see below for details */
	unsigned descriptor_type : 2;
	unsigned impl_specific : 3;
	unsigned domain : 4;
	unsigned should_be_zero : 1;

	/* Pointer to the coarse 2nd level page table (holding entries for small
	 * (4KB) or large (64KB) pages. ARM also supports fine 2nd level page
	 * tables that may hold even tiny pages (1KB) but they are bigger (4KB
	 * per table in comparison with 1KB per the coarse table)
	 */
	unsigned coarse_table_addr : 22;
} ATTRIBUTE_PACKED pte_level0_t;

/** Level 1 page table entry (small (4KB) pages used). */
typedef struct {

	/* 0b10 for small pages */
	unsigned descriptor_type : 2;
	unsigned bufferable : 1;
	unsigned cacheable : 1;

	/* access permissions for each of 4 subparts of a page
	 * (for each 1KB when small pages used */
	unsigned access_permission_0 : 2;
	unsigned access_permission_1 : 2;
	unsigned access_permission_2 : 2;
	unsigned access_permission_3 : 2;
	unsigned frame_base_addr : 20;
} ATTRIBUTE_PACKED pte_level1_t;

typedef union {
	pte_level0_t l0;
	pte_level1_t l1;
} pte_t;

/* Level 1 page tables access permissions */

/** User mode: no access, privileged mode: no access. */
#define PTE_AP_USER_NO_KERNEL_NO	0

/** User mode: no access, privileged mode: read/write. */
#define PTE_AP_USER_NO_KERNEL_RW	1

/** User mode: read only, privileged mode: read/write. */
#define PTE_AP_USER_RO_KERNEL_RW	2

/** User mode: read/write, privileged mode: read/write. */
#define PTE_AP_USER_RW_KERNEL_RW	3


/* pte_level0_t and pte_level1_t descriptor_type flags */

/** pte_level0_t and pte_level1_t "not present" flag (used in descriptor_type). */
#define PTE_DESCRIPTOR_NOT_PRESENT	0

/** pte_level0_t coarse page table flag (used in descriptor_type). */
#define PTE_DESCRIPTOR_COARSE_TABLE	1

/** pte_level1_t small page table flag (used in descriptor type). */
#define PTE_DESCRIPTOR_SMALL_PAGE	2

#define pt_coherence_m(pt, count) \
do { \
	for (unsigned i = 0; i < count; ++i) \
		dcache_clean_mva_pou((uintptr_t)(pt + i)); \
	read_barrier(); \
} while (0)

extern int get_pt_level0_flags(pte_t *pt, size_t i);
extern int get_pt_level1_flags(pte_t *pt, size_t i);
extern void set_pt_level0_flags(pte_t *pt, size_t i, int flags);
extern void set_pt_level1_flags(pte_t *pt, size_t i, int flags);
extern void set_pt_level0_present(pte_t *pt, size_t i);
extern void set_pt_level1_present(pte_t *pt, size_t i);
extern void page_arch_init(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
