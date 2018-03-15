static inline void clean_dcache_poc(void *address, size_t size)
{
	const uintptr_t addr = (uintptr_t) address;

#if !defined(PROCESSOR_ARCH_armv7_a)
	bool sep;
	if (MIDR_read() != CTR_read()) {
		sep = (CTR_read() & CTR_SEP_FLAG) == CTR_SEP_FLAG;
	} else {
		printf("Unknown cache type.\n");
		halt();
	}
#endif

	for (uintptr_t a = ALIGN_DOWN(addr, CP15_C7_MVA_ALIGN); a < addr + size;
	    a += CP15_C7_MVA_ALIGN) {
#if defined(PROCESSOR_ARCH_armv7_a)
		DCCMVAC_write(a);
#else
		if (sep)
			DCCMVA_write(a);
		else
			CCMVA_write(a);
#endif
	}
}

