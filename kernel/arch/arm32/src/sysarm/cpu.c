static inline unsigned log2(unsigned val)
{
	unsigned log = 0;
	--val;
	while (val) {
		++log;
		val >>= 1;
	}
	return log;
}

static unsigned dcache_ways(unsigned level);
static unsigned dcache_sets(unsigned level);
static unsigned dcache_linesize_log(unsigned level);


/** Implementers (vendor) names */
static const char * implementer(unsigned id)
{
	switch (id) {
	case 0x41: return "ARM Limited";
	case 0x44: return "Digital Equipment Corporation";
	case 0x4d: return "Motorola, Freescale Semiconductor Inc.";
	case 0x51: return "Qualcomm Inc.";
	case 0x56: return "Marvell Semiconductor Inc.";
	case 0x69: return "Intel Corporation";
	}
	return "Unknown implementer";
}

/** Architecture names */
static const char * architecture_string(cpu_arch_t *arch)
{
	static const char *arch_data[] = {
		"ARM",       /* 0x0 */
		"ARMv4",       /* 0x1 */
		"ARMv4T",      /* 0x2 */
		"ARMv5",       /* 0x3 */
		"ARMv5T",      /* 0x4 */
		"ARMv5TE",     /* 0x5 */
		"ARMv5TEJ",    /* 0x6 */
		"ARMv6"        /* 0x7 */
	};
	if (arch->arch_num < (sizeof(arch_data) / sizeof(arch_data[0])))
		return arch_data[arch->arch_num];
	else
		return arch_data[0];
}


/** Retrieves processor identification from CP15 register 0.
 *
 * @param cpu Structure for storing CPU identification.
 * See page B4-1630 of ARM Architecture Reference Manual.
 */
static void arch_cpu_identify(cpu_arch_t *cpu)
{
	const uint32_t ident = MIDR_read();

	cpu->imp_num = (ident >> MIDR_IMPLEMENTER_SHIFT) & MIDR_IMPLEMENTER_MASK;
	cpu->variant_num = (ident >> MIDR_VARIANT_SHIFT) & MIDR_VARIANT_MASK;
	cpu->arch_num = (ident >> MIDR_ARCHITECTURE_SHIFT) & MIDR_ARCHITECTURE_MASK;
	cpu->prim_part_num = (ident >> MIDR_PART_NUMBER_SHIFT) & MIDR_PART_NUMBER_MASK;
	cpu->rev_num = (ident >> MIDR_REVISION_SHIFT) & MIDR_REVISION_MASK;

	// TODO CPUs with arch_num == 0xf use CPUID scheme for identification
	cpu->dcache_levels = dcache_levels();

	for (unsigned i = 0; i < cpu->dcache_levels; ++i) {
		cpu->dcache[i].ways = dcache_ways(i);
		cpu->dcache[i].sets = dcache_sets(i);
		cpu->dcache[i].way_shift = 31 - log2(cpu->dcache[i].ways);
		cpu->dcache[i].set_shift = dcache_linesize_log(i);
		cpu->dcache[i].line_size = 1 << dcache_linesize_log(i);
		printf("Found DCache L%u: %u-way, %u sets, %u byte lines "
		    "(shifts: w%u, s%u)\n", i + 1, cpu->dcache[i].ways,
		    cpu->dcache[i].sets, cpu->dcache[i].line_size,
		    cpu->dcache[i].way_shift, cpu->dcache[i].set_shift);
	}
}

/** See chapter B4.1.19 of ARM Architecture Reference Manual */
static unsigned dcache_linesize_log(unsigned level)
{
#ifdef PROCESSOR_ARCH_armv7_a
	CSSELR_write((level & CCSELR_LEVEL_MASK) << CCSELR_LEVEL_SHIFT);
	const uint32_t ccsidr = CCSIDR_read();
	return CCSIDR_LINESIZE_LOG(ccsidr);
#endif
	return 0;

}

/** See chapter B4.1.19 of ARM Architecture Reference Manual */
static unsigned dcache_ways(unsigned level)
{
#ifdef PROCESSOR_ARCH_armv7_a
	CSSELR_write((level & CCSELR_LEVEL_MASK) << CCSELR_LEVEL_SHIFT);
	const uint32_t ccsidr = CCSIDR_read();
	return CCSIDR_WAYS(ccsidr);
#endif
	return 0;
}

/** See chapter B4.1.19 of ARM Architecture Reference Manual */
static unsigned dcache_sets(unsigned level)
{
#ifdef PROCESSOR_ARCH_armv7_a
	CSSELR_write((level & CCSELR_LEVEL_MASK) << CCSELR_LEVEL_SHIFT);
	const uint32_t ccsidr = CCSIDR_read();
	return CCSIDR_SETS(ccsidr);
#endif
	return 0;
}

unsigned dcache_levels(void)
{
	unsigned levels = 0;
#ifdef PROCESSOR_ARCH_armv7_a
	const uint32_t val = CLIDR_read();
	for (unsigned i = 0; i < 8; ++i) {
		const unsigned ctype = CLIDR_CACHE(i, val);
		switch (ctype) {
		case CLIDR_DCACHE_ONLY:
		case CLIDR_SEP_CACHE:
		case CLIDR_UNI_CACHE:
			++levels;
		default:
			(void)0;
		}
	}
#endif
	return levels;
}

static void dcache_clean_manual(unsigned level, bool invalidate,
    unsigned ways, unsigned sets, unsigned way_shift, unsigned set_shift)
{

	for (unsigned i = 0; i < ways; ++i) {
		for (unsigned j = 0; j < sets; ++j) {
			const uint32_t val =
			    ((level & 0x7) << 1) |
			    (j << set_shift) | (i << way_shift);
			if (invalidate)
				DCCISW_write(val);
			else
				DCCSW_write(val);
		}
	}
}

void dcache_flush(void)
{
	/* See ARM Architecture Reference Manual ch. B4.2.1 p. B4-1724 */
	const unsigned levels = dcache_levels();
	for (unsigned i = 0; i < levels; ++i) {
		const unsigned ways = dcache_ways(i);
		const unsigned sets = dcache_sets(i);
		const unsigned way_shift = 32 - log2(ways);
		const unsigned set_shift = dcache_linesize_log(i);
		dcache_clean_manual(i, false, ways, sets, way_shift, set_shift);
	}
}

void dcache_flush_invalidate(void)
{
	/* See ARM Architecture Reference Manual ch. B4.2.1 p. B4-1724 */
	const unsigned levels = dcache_levels();
	for (unsigned i = 0; i < levels; ++i) {
		const unsigned ways = dcache_ways(i);
		const unsigned sets = dcache_sets(i);
		const unsigned way_shift = 32 - log2(ways);
		const unsigned set_shift = dcache_linesize_log(i);
		dcache_clean_manual(i, true, ways, sets, way_shift, set_shift);
	}
}


void cpu_dcache_flush(void)
{
	for (unsigned i = 0; i < CPU->arch.dcache_levels; ++i)
		dcache_clean_manual(i, false,
		    CPU->arch.dcache[i].ways, CPU->arch.dcache[i].sets,
		    CPU->arch.dcache[i].way_shift, CPU->arch.dcache[i].set_shift);
}

void cpu_dcache_flush_invalidate(void)
{
	const unsigned levels =  dcache_levels();
	for (unsigned i = 0; i < levels; ++i)
		dcache_clean_manual(i, true,
		    CPU->arch.dcache[i].ways, CPU->arch.dcache[i].sets,
		    CPU->arch.dcache[i].way_shift, CPU->arch.dcache[i].set_shift);
}

void icache_invalidate(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	ICIALLU_write(0);
#else
	ICIALL_write(0);
#endif
}

#if !defined(PROCESSOR_ARCH_armv7_a)
static bool cache_is_unified(void)
{
	if (MIDR_read() != CTR_read()) {
		/* We have the CTR register */
		return (CTR_read() & CTR_SEP_FLAG) != CTR_SEP_FLAG;
	} else {
		panic("Unknown cache type");
	}
}
#endif

void dcache_invalidate(void)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	dcache_flush_invalidate();
#else
	if (cache_is_unified())
		CIALL_write(0);
	else
		DCIALL_write(0);
#endif
}

void dcache_clean_mva_pou(uintptr_t mva)
{
#if defined(PROCESSOR_ARCH_armv7_a)
	DCCMVAU_write(mva);
#else
	if (cache_is_unified())
		CCMVA_write(mva);
	else
		DCCMVA_write(mva);
#endif
}

