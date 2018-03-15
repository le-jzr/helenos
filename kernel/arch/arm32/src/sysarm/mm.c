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

#include <arch/l2cache.h>

#ifdef PROCESSOR_ARCH_armv7_a
static unsigned log2(unsigned val)
{
	unsigned log = 0;
	while (val >> log++);
	return log - 2;
}

static void dcache_invalidate_level(unsigned level)
{
	CSSELR_write(level << 1);
	const uint32_t ccsidr = CCSIDR_read();
	const unsigned sets = CCSIDR_SETS(ccsidr);
	const unsigned ways = CCSIDR_WAYS(ccsidr);
	const unsigned line_log = CCSIDR_LINESIZE_LOG(ccsidr);
	const unsigned set_shift = line_log;
	const unsigned way_shift = 32 - log2(ways);

	for (unsigned k = 0; k < ways; ++k)
		for (unsigned j = 0; j < sets; ++j) {
			const uint32_t val = (level << 1) |
			    (j << set_shift) | (k << way_shift);
			DCISW_write(val);
		}
}

/** invalidate all dcaches -- armv7 */
static void cache_invalidate(void)
{
	const uint32_t cinfo = CLIDR_read();
	for (unsigned i = 0; i < 7; ++i) {
		switch (CLIDR_CACHE(i, cinfo))
		{
		case CLIDR_DCACHE_ONLY:
		case CLIDR_SEP_CACHE:
		case CLIDR_UNI_CACHE:
			dcache_invalidate_level(i);
		}
	}
	asm volatile ( "dsb\n" );
	ICIALLU_write(0);
	asm volatile ( "dsb\n" );
	asm volatile ( "isb\n" );

#ifdef L2_CACHE_BASE
	void *base = (void *)L2_CACHE_BASE;
	int implementer;
	int cache_id;
	int part_number;
	int rtl_release;
	uint32_t control = read_reg1_control(base);

	read_cache_id(base, &implementer, &cache_id, &part_number, &rtl_release);

	printf("L2 cache present: implementer = 0x%02x, cache_id = %d, part_number = 0x%02x, rtl_release = 0x%02x, control = 0x%08x\n",
	    implementer, cache_id, part_number, rtl_release, control);

	// Invalidate all ways.
	write_reg7_inv_way(base, 0xffff);

	// Wait until invalidation is complete.
	while (read_reg7_inv_way(base) != 0)
		;
#endif
}
#endif

/** Disable the MMU */
static void disable_paging(void)
{
	asm volatile (
		"mrc p15, 0, r0, c1, c0, 0\n"
		"bic r0, r0, #1\n"
		"mcr p15, 0, r0, c1, c0, 0\n"
		::: "r0"
	);
}

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
	pte->tex = 0; // section_cacheable(frame) ? 5 : 0;
	// FIXME: what is intended by this?
	pte->cacheable = 0; // section_cacheable(frame) ? 0 : 0;
	pte->bufferable = 0; // section_cacheable(frame) ? 1 : 1;
#else
	pte->bufferable = section_cacheable(frame);
	pte->cacheable = section_cacheable(frame);
	pte->tex = 0;
#endif
	pte->access_permission_1 = 0;
	pte->shareable = 0;
	pte->non_global = 1;
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
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	// FIXME: TTBR_RGN_WBWA_CACHE is unpredictable on ARMv6
	val |= TTBR_RGN_WBWA_CACHE | TTBR_C_FLAG;
#endif
	TTBR0_write(val);
}

static void enable_paging(void)
{
	/*
	 * c3   - each two bits controls access to the one of domains (16)
	 * 0b01 - behave as a client (user) of a domain
	 */
	asm volatile (
		/* Behave as a client of domains */
		"ldr r0, =0x55555555\n"
		"mcr p15, 0, r0, c3, c0, 0\n"

		/* Current settings */
		"mrc p15, 0, r0, c1, c0, 0\n"

		/* Enable ICache, DCache, BPredictors and MMU,
		 * we disable caches before jumping to kernel
		 * so this is safe for all archs.
		 * Enable VMSAv6 the bit (23) is only writable on ARMv6.
		 * (and QEMU)
		 */
#ifdef PROCESSOR_ARCH_armv6
//		"ldr r1, =0x00801805\n"
#else
//		"ldr r1, =0x00001805\n"
#endif

// XXX: caching disabled for testing
		"ldr r1, =0x00000001\n"

		"orr r0, r0, r1\n"

		/* Invalidate the TLB content before turning on the MMU.
		 * ARMv7-A Reference manual, B3.10.3
		 */
		"mcr p15, 0, r0, c8, c7, 0\n"

		// XXX: missing sync?

		/* Store settings, enable the MMU */
		"mcr p15, 0, r0, c1, c0, 0\n"
		::: "r0", "r1"
	);
}

/** Start the MMU - initialize page table and enable paging. */
void mmu_start(void)
{
	disable_paging();
#ifdef PROCESSOR_ARCH_armv7_a
	/* Make sure we run in memory code when caches are enabled,
	 * make sure we read memory data too. This part is ARMv7 specific as
	 * ARMv7 no longer invalidates caches on restart.
	 * See chapter B2.2.2 of ARM Architecture Reference Manual p. B2-1263*/
	cache_invalidate();
#endif
	init_boot_pt();
	enable_paging();
}

/** @}
 */
