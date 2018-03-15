
typedef arch {
	arm_unknown = 0,
	arm_obsolete, // ARMv3 or older
	armv4,
	armv4t,
	armv5,
	armv5t,
	armv5te,
	armv5tej,
	armv6,
	armv7a,
} arch_t;



typedef struct {

} midr_t;

typedef struct {
	uint32_t midr;
	arch_t architecture;
} cpuinfo_t;

cpuinfo_t cpuinfo;

enum {
	implementer_arm = 0x41,
};

void cpuid_architecture() {

	uint32_t midr = MIDR_read();

	cpuinfo.midr.implementer = (midr >> MIDR_IMPLEMENTER_SHIFT) & MIDR_IMPLEMENTER_MASK;
	cpuinfo.midr.variant = (midr >> MIDR_VARIANT_SHIFT) & MIDR_VARIANT_MASK;
	cpuinfo.midr.architecture = (midr >> MIDR_ARCHITECTURE_SHIFT) & MIDR_ARCHITECTURE_MASK;
	cpuinfo.midr.primary_part = (midr >> MIDR_PART_NUMBER_SHIFT) & MIDR_PART_NUMBER_MASK;
	cpuinfo.midr.revision = (midr >> MIDR_REVISION_SHIFT) & MIDR_REVISION_MASK;
	
	if (cpuinfo.midr.implementer == implementer_arm) {
		if (cpuinfo.midr.primary_part >> 8 == 0) {
			// armv2 or armv3
			cpuinfo.architecture = arm_obsolete;
			return;
		}
		
		if (cpuinfo.midr.primary_part >> 8 == 0x7) {
			if (midr & (1<<23)) {
				cpuinfo.architecture = armv4t;
			} else {
				// armv3
				cpuinfo.architecture = arm_obsolete;
			}
			return;
		}
	}
	
	switch (cpuinfo.midr.architecture) {
	case 0x1: cpuinfo.architecture = armv4;    break;
	case 0x2: cpuinfo.architecture = armv4t;   break;
	case 0x3: cpuinfo.architecture = armv5;    break;
	case 0x4: cpuinfo.architecture = armv5t;   break;
	case 0x5: cpuinfo.architecture = armv5te;  break;
	case 0x6: cpuinfo.architecture = armv5tej; break;
	case 0x7: cpuinfo.architecture = armv6;    break;
	case 0xf: /* cpuid */                      break;
	default:  fatal("Unknown cpu architecture.");
	}

	if (cpuinfo.architecture != arm_unknown)
		return;

	
}

void cpuid() {
	memset(&cpuinfo, 0, sizeof(cpuinfo));
}
