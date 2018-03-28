
#include <stdbool.h>
#include <printf.h>
#include <arch/cp15.h>
#include <arch/init.h>
#include <arch/pl310.h>
#include <arch/platform.h>

/* A number of cleanup operations need to be performed after
 * reset on armv7 processors, depending on the model.
 * These may include invalidating all processor-accessible caches,
 * be they separate or unified, and also branch predictors and
 * TLBs. All of these may need to be invalidated before they are
 * enabled.
 *
 * This function does not try to assume which invalidations are
 * necessary. Instead, it invalidates all accessible caches and
 * buffers that are not currently enabled. This is safe even
 * when an earlier bootloader already enabled some or all caches.
 *
 * Optionally, if `disable` is true, this function also cleans
 * and turns off all caches that are found to be
 */

#define PADDR_NULL ((uintptr_t) -1)

#ifndef L2_CACHE_BASE
#define L2_CACHE_BASE PADDR_NULL
#endif

static unsigned log2(unsigned val)
{
	unsigned log = 0;
	while (val >> log++);
	return log - 2;
}

#ifdef PROCESSOR_ARCH_armv7_a
static void dcache_invalidate_level(unsigned level)
{
	CSSELR_write(level << 1);
	const uint32_t ccsidr = CCSIDR_read();
	const unsigned sets = CCSIDR_SETS(ccsidr);
	const unsigned ways = CCSIDR_WAYS(ccsidr);
	const unsigned line_log = CCSIDR_LINESIZE_LOG(ccsidr);
	const unsigned set_shift = line_log;
	const unsigned way_shift = 32 - log2(ways);

	for (unsigned k = 0; k < ways; ++k) {
		for (unsigned j = 0; j < sets; ++j) {
			const uint32_t val = (level << 1) |
			    (j << set_shift) | (k << way_shift);
			DCISW_write(val);
		}
	}
}

static void dcache_clean_level(unsigned level)
{
	CSSELR_write(level << 1);
	const uint32_t ccsidr = CCSIDR_read();
	const unsigned sets = CCSIDR_SETS(ccsidr);
	const unsigned ways = CCSIDR_WAYS(ccsidr);
	const unsigned line_log = CCSIDR_LINESIZE_LOG(ccsidr);
	const unsigned set_shift = line_log;
	const unsigned way_shift = 32 - log2(ways);

	for (unsigned k = 0; k < ways; ++k) {
		for (unsigned j = 0; j < sets; ++j) {
			const uint32_t val = (level << 1) |
			    (j << set_shift) | (k << way_shift);
			DCCSW_write(val);
		}
	}
}

static void dcache_invalidate_all_armv7(void)
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

	dsb();
}

static void dcache_clean_all_armv7(void)
{
	const uint32_t cinfo = CLIDR_read();
	for (unsigned i = 0; i < 7; ++i) {
		switch (CLIDR_CACHE(i, cinfo))
		{
		case CLIDR_DCACHE_ONLY:
		case CLIDR_SEP_CACHE:
		case CLIDR_UNI_CACHE:
			dcache_clean_level(i);
		}
	}

	dsb();
}
#endif

/** invalidate all data and unified caches */
void dcache_invalidate_all(void)
{
#ifdef PROCESSOR_ARCH_armv7_a
	dcache_invalidate_all_armv7();
#else
	CIALL_write(0);
#endif
}

void dcache_clean_all(void)
{
#ifdef PROCESSOR_ARCH_armv7_a
	dcache_clean_all_armv7();
#else
	bool sep;
	if (MIDR_read() != CTR_read()) {
		sep = (CTR_read() & CTR_SEP_FLAG) == CTR_SEP_FLAG;
	} else {
		printf("Unknown cache type.\n");
		halt();
	}

	if (sep) {
		DCCALL_write(0);
	} else {
		CCALL_write(0);
	}
#endif
}

void enable_l2c(void)
{
	if (L2_CACHE_BASE == PADDR_NULL)
		return;

	void *base = (void *)L2_CACHE_BASE;
	int implementer;
	int cache_id;
	int part_number;
	int rtl_release;
	uint32_t control = pl310_read_reg1_control(base);

	pl310_read_cache_id(base, &implementer, &cache_id, &part_number, &rtl_release);

	printf("L2 cache: implementer = 0x%02x, cache_id = %d, part_number = 0x%02x, rtl_release = 0x%02x, control = 0x%08x\n",
	    implementer, cache_id, part_number, rtl_release, control);

	if (implementer != 0x41 /* ARM */ ||
	    part_number != 0x3 /* pl310 */) {
		printf("Unknown L2 cache.\n");
		return;
	}

	printf("Invalidating L2.\n");

	// Invalidate all ways.
	pl310_write_reg7_inv_way(base, 0xffff);

	// Wait until invalidation is complete.
	while (pl310_read_reg7_inv_way(base) != 0) {
		printf("Still invalidating.\n");
	}

	control |= 1;
	pl310_write_reg1_control(base, control);
	printf("L2 cache enabled.\n");

#if 0

	// Invalidate all ways.
	pl310_write_reg7_inv_way(base, 0xffff);

	// Wait until invalidation is complete.
	while (pl310_read_reg7_inv_way(base) != 0)
		;

	pl310_write_reg1_control(base, 0);
	printf("L2 cache disabled.\n");

	// Invalidate all ways.
	pl310_write_reg7_inv_way(base, 0xffff);

	// Wait until invalidation is complete.
	while (pl310_read_reg7_inv_way(base) != 0)
		;
#endif

}

void tlb_invalidate_all(void)
{
	ITLBIALL_write(0);
	DTLBIALL_write(0);
	TLBIALL_write(0);
	dsb();
	isb();
}

void icache_invalidate_all(void)
{
	// Invalidate instruction cache and branch predictors.
	ICIALLU_write(0);
	dsb();
	isb();
}

void enable_caches(void)
{
	uint32_t sctlr = SCTLR_read();

	if (!(sctlr & SCTLR_CACHE_EN_FLAG)) {
		// Invalidate all data caches. Necessary for armv7.
		dcache_invalidate_all();
	}

	icache_invalidate_all();

	// Enable caches.
	sctlr |= SCTLR_CACHE_EN_FLAG;
	sctlr |= SCTLR_BRANCH_PREDICT_EN_FLAG;
	sctlr |= SCTLR_INST_CACHE_EN_FLAG;

	SCTLR_write(sctlr);
	sctlr = SCTLR_read();

	if (sctlr & SCTLR_CACHE_EN_FLAG) {
		printf("Data and unified caches enabled.\n");
	} else {
		printf("Data and unified caches CANNOT be enabled.\n");
	}

	if (sctlr & SCTLR_INST_CACHE_EN_FLAG) {
		printf("Instruction caches enabled.\n");
	} else {
		printf("Instruction caches CANNOT be enabled.\n");
	}

	if (sctlr & SCTLR_BRANCH_PREDICT_EN_FLAG) {
		printf("Branch predictors enabled.\n");
	} else {
		printf("Branch predictors CANNOT be enabled.\n");
	}
}

void disable_caches(void)
{
	dsb();

	uint32_t sctlr = SCTLR_read();

	if (sctlr & SCTLR_CACHE_EN_FLAG) {
		// Clean all data caches.
		dcache_clean_all();
	}

	sctlr &= ~(SCTLR_CACHE_EN_FLAG |
	    SCTLR_BRANCH_PREDICT_EN_FLAG |
	    SCTLR_INST_CACHE_EN_FLAG);

	SCTLR_write(sctlr);
	sctlr = SCTLR_read();

	if (sctlr & SCTLR_CACHE_EN_FLAG) {
		printf("Data and unified caches CANNOT be disabled.\n");
	} else {
		printf("Data and unified caches disabled.\n");
		// Invalidate disabled caches.
		dcache_invalidate_all();
	}

	if (sctlr & SCTLR_INST_CACHE_EN_FLAG) {
		printf("Instruction caches CANNOT be disabled.\n");
	} else {
		printf("Instruction caches disabled.\n");
	}

	if (sctlr & SCTLR_BRANCH_PREDICT_EN_FLAG) {
		printf("Branch predictors CANNOT be disabled.\n");
	} else {
		printf("Branch predictors disabled.\n");
	}

	icache_invalidate_all();
}

void enable_mmu(void)
{
	// c3   - each two bits controls access to the one of domains (16)
	// 0b01 - behave as a client (user) of a domain
	DACR_write(0x55555555);

	tlb_invalidate_all();
	
	uint32_t sctlr = SCTLR_read();
#ifdef PROCESSOR_ARCH_armv6
	sctlr |= SCTLR_EXTENDED_PT_EN_FLAG;
#endif
#ifdef PROCESSOR_ARCH_armv7_a
	/* Turn off tex remap, RAZ/WI prior to armv7 */
	sctlr &= ~SCTLR_TEX_REMAP_EN_FLAG;
	/* Turn off accessed flag, RAZ/WI prior to armv7 */
	sctlr &= ~(SCTLR_ACCESS_FLAG_EN_FLAG | SCTLR_HW_ACCESS_FLAG_EN_FLAG);
#endif
	SCTLR_write(sctlr | SCTLR_MMU_EN_FLAG);
}

void disable_mmu(void)
{
	SCTLR_write(SCTLR_read() & ~SCTLR_MMU_EN_FLAG);
	tlb_invalidate_all();
}

static void wait_for_event() {
#ifdef PROCESSOR_ARCH_armv7_a
	asm volatile ( "wfe" );
#elif defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_arm926ej_s) || defined(PROCESSOR_arm920t)
	WFI_write(0);
#endif
}

void halt(void)
{
	while (true) {
		wait_for_event();
	}
}

void dsb(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	asm volatile ("dsb" ::: "memory");
#else
	CP15DSB_write(0);
#endif
}

void isb(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	asm volatile ("isb" ::: "memory");
#else
	CP15ISB_write(0);
#endif
}

