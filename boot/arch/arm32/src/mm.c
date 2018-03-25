/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32boot
 * @{
 */
/** @file
 * @brief Memory management used while booting the kernel.
 */

#include <stdbool.h>
#include <stdint.h>
#include <printf.h>
#include <arch/asm.h>
#include <arch/mm.h>
#include <arch/cp15.h>
#include <arch/init.h>

/** Check if caching can be enabled for a given memory section.
 *
 * Memory areas used for I/O are excluded from caching.
 *
 * @param section	The section number.
 *
 * @return	whether the given section can be mapped as cacheable.
*/
static inline bool section_cacheable(pfn_t section)
{
	const unsigned long address = section << PTE_SECTION_SHIFT;
#if RAM_START == 0
	return address < RAM_END;
#else
	return address >= RAM_START && address < RAM_END;
#endif
}

/** Initialize "section" page table entry.
 *
 * Will be readable/writable by kernel with no access from user mode.
 * Will belong to domain 0.
 * Cache or buffering may be enabled for addresses corresponding to physical
 * RAM, but are disabled for all other areas.
 *
 * @param pte   Section entry to initialize.
 * @param frame First frame in the section (frame number).
 *
 * @note If frame is not 1 MB aligned, first lower 1 MB aligned frame will be
 *       used.
 *
 */
static void init_ptl0_section(pte_level0_section_t* pte,
    pfn_t frame)
{
	pte->descriptor_type = PTE_DESCRIPTOR_SECTION;
	pte->xn = 0;
	pte->domain = 0;
	pte->should_be_zero_1 = 0;
	pte->access_permission_0 = PTE_AP_USER_NO_KERNEL_RW;
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	/*
	 * Keeps this setting in sync with memory type attributes in:
	 * init_boot_pt (boot/arch/arm32/src/mm.c)
	 * set_pt_level1_flags (kernel/arch/arm32/include/arch/mm/page_armv6.h)
	 * set_ptl0_addr (kernel/arch/arm32/include/arch/mm/page.h)
	 */
	pte->tex = section_cacheable(frame) ? 5 : 0;
	pte->c = 0;
	pte->b = 1;
#else
	pte->tex = 0;
	pte->c = section_cacheable(frame);
	pte->b = section_cacheable(frame);
#endif
	pte->access_permission_1 = 0;
#if defined(PROCESSOR_ARCH_armv7_a)
	pte->shareable = 1;
#else
	pte->shareable = 0;
#endif
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	pte->non_global = 1;
#else
	pte->non_global = 0;
#endif
	pte->should_be_zero_2 = 0;
	pte->non_secure = 0;
	pte->section_base_addr = frame;
}

/** Initialize page table used while booting the kernel. */
static void init_boot_pt(void)
{
	/*
	 * Create 1:1 virtual-physical mapping.
	 *
	 * Optionally, the physical memory (RAM_START to RAM_END) is
	 * aliased at offset 0x80000000. This has the result that
	 * physical mappings in this region are inaccessible to the
	 * loader. On platforms that currently use it, this is not
	 * a problem, but a more sophisticated solution may need to
	 * be devised in the future.
	 */
	for (pfn_t page = 0; page < PTL0_ENTRIES; ++page) {
		pfn_t frame = page;
#if KERNEL_REMAP
		static const unsigned ram_vstart = RAM_OFFSET >> 20;
		static const unsigned ram_vend = (RAM_OFFSET + (RAM_END - RAM_START)) >> 20;
		if (page >= ram_vstart && page < ram_vend) {
			frame = page - ram_vstart + (RAM_START >> 20);
		}
#endif
		init_ptl0_section(&boot_pt[page], frame);
	}

	/*
	 * Tell MMU page might be cached. Keeps this setting in sync
	 * with memory type attributes in:
	 * init_ptl0_section (boot/arch/arm32/src/mm.c)
	 * set_pt_level1_flags (kernel/arch/arm32/include/arch/mm/page_armv6.h)
	 * set_ptl0_addr (kernel/arch/arm32/include/arch/mm/page.h)
	 */
	uint32_t val = (uint32_t)boot_pt & TTBR_ADDR_MASK;
#if defined(PROCESSOR_ARCH_armv7_a)
	val |= TTBR_RGN_WBWA_CACHE | TTBR_C_FLAG;
#elif defined(PROCESSOR_ARCH_armv6)
	val |= TTBR_RGN_WB_CACHE | TTBR_C_FLAG;
#endif
	TTBR0_write(val);
}

/** Start the MMU - initialize page table and enable paging. */
void mmu_start(void)
{
	disable_mmu();
	init_boot_pt();
	enable_mmu();
}

/** @}
 */
