
/** Sets the address of level 0 page table.
 *
 * @param pt Pointer to the page table to set.
 *
 * Page tables are always in cacheable memory.
 * Make sure the memory type is correct, and in sync with:
 * init_boot_pt (boot/arch/arm32/src/mm.c)
 * init_ptl0_section (boot/arch/arm32/src/mm.c)
 * set_pt_level1_flags (kernel/arch/arm32/include/arch/mm/page_armv6.h)
 */
void set_ptl0_addr(pte_t *pt)
{
	uint32_t val = (uint32_t)pt & TTBR_ADDR_MASK;
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	// FIXME: TTBR_RGN_WBWA_CACHE is unpredictable on ARMv6
	val |= TTBR_RGN_WBWA_CACHE | TTBR_C_FLAG;
#endif
	TTBR0_write(val);
}

void set_ptl1_addr(pte_t *pt, size_t i, uintptr_t address)
{
	pt[i].l0.coarse_table_addr = address >> 10;
	pt_coherence(&pt[i].l0);
}

void set_ptl3_addr(pte_t *pt, size_t i, uintptr_t address)
{
	pt[i].l1.frame_base_addr = address >> 12;
	pt_coherence(&pt[i].l1);
}
