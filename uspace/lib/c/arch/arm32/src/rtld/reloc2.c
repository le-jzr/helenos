
#include "../../../generic/private/rtld_arch.h"

/** Relocation types */
enum {
	R_ARM_ABS32         =  2,
	R_ARM_TLS_DTPMOD32  = 17,
	R_ARM_TLS_DTPOFF32  = 18,
	R_ARM_TLS_TPOFF32   = 19,
	R_ARM_COPY          = 20,
	R_ARM_GLOB_DAT      = 21,
	R_ARM_JUMP_SLOT     = 22,
	R_ARM_RELATIVE      = 23,
};

const elf_rel_desc_t arch_rel_list[] = {
	[R_ARM_ABS32]         = { 32, REL_BASE | REL_SYMVAL | REL_ADDEND },
	[R_ARM_TLS_DTPMOD32]  = { 32, REL_TLS | REL_DTPMOD },
	[R_ARM_TLS_DTPOFF32]  = { 32, REL_TLS | REL_SYMVAL | REL_ADDEND },
	[R_ARM_TLS_TPOFF32]   = { 32, REL_TLS | REL_TPOFF | REL_SYMVAL | REL_ADDEND },
	[R_ARM_COPY]          = { 32, REL_COPY },
	[R_ARM_GLOB_DAT]      = { 32, REL_BASE | REL_SYMVAL | REL_ADDEND },
	[R_ARM_JUMP_SLOT]     = { 32, REL_BASE | REL_SYMVAL },
	[R_ARM_RELATIVE]      = { 32, REL_BASE | REL_ADDEND },
};

const size_t arch_rel_len =
		sizeof(arch_rel_list) / sizeof(arch_rel_list[0]);
