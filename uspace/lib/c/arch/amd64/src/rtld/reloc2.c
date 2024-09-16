
#include "../../../generic/private/rtld_arch.h"

/** Relocation types */
enum {
	R_X86_64_NONE      =  0,
	R_X86_64_64        =  1, // S + A
	R_X86_64_PC32      =  2, // S + A - P
	R_X86_64_GOT32     =  3, // G + A
	R_X86_64_PLT32     =  4, // L + A - P
	R_X86_64_COPY      =  5,
	R_X86_64_GLOB_DAT  =  6, // S
	R_X86_64_JUMP_SLOT =  7, // S
	R_X86_64_RELATIVE  =  8, // B + A
	R_X86_64_GOTPCREL  =  9, // G + GOT + A - P
	R_X86_64_32        = 10, // S + A
	R_X86_64_32S       = 11, // S + A
	R_X86_64_16        = 12, // S + A
	R_X86_64_PC16      = 13, // S + A - P
	R_X86_64_8         = 14, // S + A
	R_X86_64_PC8       = 15, // S + A - P
	R_X86_64_DTPMOD64  = 16,
	R_X86_64_DTPOFF64  = 17,
	R_X86_64_TPOFF64   = 18,
	R_X86_64_TLSGD     = 19,
	R_X86_64_TLSLD     = 20,
	R_X86_64_DTPOFF32  = 21,
	R_X86_64_GOTTPOFF  = 22,
	R_X86_64_TPOFF32   = 23,
	R_X86_64_PC64      = 24, // S + A - P
	R_X86_64_GOTOFF64  = 25, // S + A - GOT
	R_X86_64_GOTPC32   = 26, // GOT + A - P
	R_X86_64_SIZE32    = 32, // Z + A
	R_X86_64_SIZE64    = 33, // Z + A
	R_X86_64_GOTPC32_TLSDESC = 34,
	R_X86_64_TLSDESC_CALL    = 35,
	R_X86_64_IRELATIVE       = 37, // indirect (B + A)
	R_X86_64_RELATIVE64      = 38, // B + A
	R_X86_64_GOTPCRELX       = 41, // G + GOT + A - P
	R_X86_64_REX_GOTPCRELX   = 42, // G + GOT + A - P
};

const elf_rel_desc_t arch_rel_list[] = {
	[R_X86_64_8]         = {  8, REL_BASE | REL_SYMVAL | REL_ADDEND },
	[R_X86_64_16]        = { 16, REL_BASE | REL_SYMVAL | REL_ADDEND },
	[R_X86_64_32]        = { 32, REL_BASE | REL_SYMVAL | REL_ADDEND },
	[R_X86_64_64]        = { 64, REL_BASE | REL_SYMVAL | REL_ADDEND },

	[R_X86_64_PC8]       = {  8, REL_BASE | REL_SYMVAL | REL_ADDEND | REL_PLACE },
	[R_X86_64_PC16]      = { 16, REL_BASE | REL_SYMVAL | REL_ADDEND | REL_PLACE },
	[R_X86_64_PC32]      = { 32, REL_BASE | REL_SYMVAL | REL_ADDEND | REL_PLACE },
	[R_X86_64_PC64]      = { 64, REL_BASE | REL_SYMVAL | REL_ADDEND | REL_PLACE },

	[R_X86_64_SIZE32]    = { 32, REL_SYMSZ | REL_ADDEND },
	[R_X86_64_SIZE64]    = { 64, REL_SYMSZ | REL_ADDEND },

	[R_X86_64_GLOB_DAT]  = { 64, REL_BASE | REL_SYMVAL },
	[R_X86_64_JUMP_SLOT] = { 64, REL_BASE | REL_SYMVAL },
	[R_X86_64_RELATIVE]  = { 64, REL_BASE | REL_ADDEND },

	[R_X86_64_COPY]      = {  0, REL_COPY },

	[R_X86_64_DTPMOD64]  = { 64, REL_TLS | REL_DTPMOD },
	[R_X86_64_DTPOFF64]  = { 64, REL_TLS | REL_SYMVAL | REL_ADDEND },
	[R_X86_64_TPOFF64]   = { 64, REL_TLS | REL_TPOFF | REL_SYMVAL | REL_ADDEND },
};

const size_t arch_rel_len =
		sizeof(arch_rel_list) / sizeof(arch_rel_list[0]);
