/** Returns level 0 page table entry flags.
 *
 * @param pt Level 0 page table.
 * @param i  Index of the entry to return.
 *
 */
NO_TRACE static inline int get_pt_level0_flags(pte_t *pt, size_t i)
{
	pte_level0_t *p = &pt[i].l0;
	int np = (p->descriptor_type == PTE_DESCRIPTOR_NOT_PRESENT);

	return (np << PAGE_PRESENT_SHIFT) | (1 << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) | (1 << PAGE_WRITE_SHIFT) |
	    (1 << PAGE_EXEC_SHIFT) | (1 << PAGE_CACHEABLE_SHIFT);
}

/** Returns level 1 page table entry flags.
 *
 * @param pt Level 1 page table.
 * @param i  Index of the entry to return.
 *
 */
NO_TRACE static inline int get_pt_level1_flags(pte_t *pt, size_t i)
{
	pte_level1_t *p = &pt[i].l1;

	int dt = p->descriptor_type;
	int ap = p->access_permission_0;

	return ((dt == PTE_DESCRIPTOR_NOT_PRESENT) << PAGE_PRESENT_SHIFT) |
	    ((ap == PTE_AP_USER_RO_KERNEL_RW) << PAGE_READ_SHIFT) |
	    ((ap == PTE_AP_USER_RW_KERNEL_RW) << PAGE_READ_SHIFT) |
	    ((ap == PTE_AP_USER_RW_KERNEL_RW) << PAGE_WRITE_SHIFT) |
	    ((ap != PTE_AP_USER_NO_KERNEL_RW) << PAGE_USER_SHIFT) |
	    ((ap == PTE_AP_USER_NO_KERNEL_RW) << PAGE_READ_SHIFT) |
	    ((ap == PTE_AP_USER_NO_KERNEL_RW) << PAGE_WRITE_SHIFT) |
	    (1 << PAGE_EXEC_SHIFT) |
	    (p->bufferable << PAGE_CACHEABLE);
}

/** Sets flags of level 0 page table entry.
 *
 * @param pt    level 0 page table
 * @param i     index of the entry to be changed
 * @param flags new flags
 *
 */
NO_TRACE static inline void set_pt_level0_flags(pte_t *pt, size_t i, int flags)
{
	pte_level0_t *p = &pt[i].l0;

	if (flags & PAGE_NOT_PRESENT) {
		p->descriptor_type = PTE_DESCRIPTOR_NOT_PRESENT;
		/*
		 * Ensures that the entry will be recognized as valid when
		 * PTE_VALID_ARCH applied.
		 */
		p->should_be_zero = 1;
	} else {
		p->descriptor_type = PTE_DESCRIPTOR_COARSE_TABLE;
		p->should_be_zero = 0;
	}
}


/** Sets flags of level 1 page table entry.
 *
 * We use same access rights for the whole page. When page
 * is not preset we store 1 in acess_rigts_3 so that at least
 * one bit is 1 (to mark correct page entry, see #PAGE_VALID_ARCH).
 *
 * @param pt    Level 1 page table.
 * @param i     Index of the entry to be changed.
 * @param flags New flags.
 *
 */
NO_TRACE static inline void set_pt_level1_flags(pte_t *pt, size_t i, int flags)
{
	pte_level1_t *p = &pt[i].l1;

	if (flags & PAGE_NOT_PRESENT)
		p->descriptor_type = PTE_DESCRIPTOR_NOT_PRESENT;
	else
		p->descriptor_type = PTE_DESCRIPTOR_SMALL_PAGE;

	p->cacheable = p->bufferable = (flags & PAGE_CACHEABLE) != 0;

	/* default access permission */
	p->access_permission_0 = p->access_permission_1 =
	    p->access_permission_2 = p->access_permission_3 =
	    PTE_AP_USER_NO_KERNEL_RW;

	if (flags & PAGE_USER)  {
		if (flags & PAGE_READ) {
			p->access_permission_0 = p->access_permission_1 =
			    p->access_permission_2 = p->access_permission_3 =
			    PTE_AP_USER_RO_KERNEL_RW;
		}
		if (flags & PAGE_WRITE) {
			p->access_permission_0 = p->access_permission_1 =
			    p->access_permission_2 = p->access_permission_3 =
			    PTE_AP_USER_RW_KERNEL_RW;
		}
	}
}

NO_TRACE static inline void set_pt_level0_present(pte_t *pt, size_t i)
{
	pte_level0_t *p = &pt[i].l0;

	p->should_be_zero = 0;
	write_barrier();
	p->descriptor_type = PTE_DESCRIPTOR_COARSE_TABLE;
}


NO_TRACE static inline void set_pt_level1_present(pte_t *pt, size_t i)
{
	pte_level1_t *p = &pt[i].l1;

	p->descriptor_type = PTE_DESCRIPTOR_SMALL_PAGE;
}
