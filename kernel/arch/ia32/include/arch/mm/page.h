/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_PAGE_H_
#define KERN_ia32_PAGE_H_

#include <arch/mm/frame.h>
#include <stdbool.h>
#include <trace.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#define PT_ENTRIES 1024

#define PTE_P		(1 << 0)
#define PTE_RW		(1 << 1)

#define PDE_P		(1 << 0)
#define PDE_RW		(1 << 1)
#define PDE_4M		(1 << 7)

#ifdef __ASSEMBLER__

#define KA2PA(x)  ((x) - 0x80000000)
#define PA2KA(x)  ((x) + 0x80000000)

#else /* __ASSEMBLER__ */

#define KA2PA(x)  (((uintptr_t) (x)) - UINT32_C(0x80000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT32_C(0x80000000))

#include <mm/mm.h>
#include <arch/interrupt.h>
#include <stddef.h>
#include <stdint.h>

/* Page fault error codes. */

/** When bit on this position is 0, the page fault was caused by a not-present
 * page.
 */
#define PFERR_CODE_P		(1 << 0)

/** When bit on this position is 1, the page fault was caused by a write. */
#define PFERR_CODE_RW		(1 << 1)

/** When bit on this position is 1, the page fault was caused in user mode. */
#define PFERR_CODE_US		(1 << 2)

/** When bit on this position is 1, a reserved bit was set in page directory. */
#define PFERR_CODE_RSVD		(1 << 3)

/** Page Table Entry. */
union pte {
	uint32_t raw;
	struct {
		unsigned present : 1;
		unsigned writeable : 1;
		unsigned uaccessible : 1;
		unsigned page_write_through : 1;
		unsigned page_cache_disable : 1;
		unsigned accessed : 1;
		unsigned dirty : 1;
		unsigned pat : 1;
		unsigned global : 1;
		unsigned available : 3;
		unsigned frame_address : 20;
	} __attribute__ ((packed));
};

typedef struct {
	int entries;
	int frames;
	int index_shift;
	int index_width;
} ptl_desc_t;

typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;
typedef unsigned page_flags_t;

typedef union pte pte_t;
typedef volatile uint32_t pt_t[PT_ENTRIES];

/*
 * Implementation of generic multi-level page table interface.
 * IA-32 has 2-level page tables.
 */

static const ptl_desc_t pt_levels[2] = {
	{
		.entries = PT_ENTRIES,
		.frames = 1,
		.index_shift = 22,
		.index_width = 10,
	},
	{
		.entries = PT_ENTRIES,
		.frames = 1,
		.index_shift = 12,
		.index_width = 10,
	}
};

NO_TRACE static inline int pt_index(int l, vaddr_t vaddr)
{
	int shift = pt_levels[l].index_shift;
	int width = pt_levels[l].index_width;
	return (vaddr >> shift) & ((1 << width) - 1);
}

NO_TRACE static inline page_flags_t pt_get_entry_by_index(int l, pt_t pt, int index, paddr_t *paddr)
{
	pte_t p;
	p.raw = pt[index];

	if (paddr != NULL)
		*paddr = p.frame_address << 12;

	// TODO: Support NX bit and large pages.

	return PAGE_FLAGS(
		.present = p.present,
		.next_level = (l == 0),
		.read = 1,
		.write = p.writeable,
		.execute = 1,
		.kernel_only = !p.uaccessible,
		.global = p.global,
		.cacheable = !p.page_cache_disable,
	);
}

NO_TRACE static inline page_flags_t pt_get_entry(int l, pt_t pt, vaddr_t vaddr, paddr_t *paddr)
{
	return pt_get_entry_by_index(l, pt, pt_index(l, vaddr), paddr);
}

NO_TRACE static inline void pt_set_entry_by_index(int l, pt_t pt, int index, paddr_t paddr, page_flags_t flags)
{
	pte_t p = { .raw = 0 };

	p.frame_address = paddr >> 12;
	p.present = !(flags & PAGE_NOT_PRESENT);
	p.page_cache_disable = !(flags & PAGE_CACHEABLE);
	p.uaccessible = (flags & PAGE_USER) != 0;
	p.writeable = (flags & _PAGE_WRITE) != 0;
	p.global = (flags & PAGE_GLOBAL) != 0;

	pt[index] = p.raw;

	// Compiler fence.
	// Probably unnecessary, with the page table marked volatile.
	asm volatile ("" ::: "memory");
}

NO_TRACE static inline void pt_set_entry(int l, pt_t pt, vaddr_t vaddr, paddr_t paddr, page_flags_t flags)
{
	pt_set_entry_by_index(l, pt, pt_index(l, vaddr), paddr, flags);
}

extern void page_arch_init(void);
extern void page_fault(unsigned int, istate_t *);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
