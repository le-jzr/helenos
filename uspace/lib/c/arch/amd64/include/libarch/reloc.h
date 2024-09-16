
/** Relocation types */
enum {
	R_X86_64_64 = 1,
	R_X86_64_PC32 = 2,
	R_X86_64_COPY = 5,
	R_X86_64_GLOB_DAT = 6,
	R_X86_64_JUMP_SLOT = 7,
	R_X86_64_RELATIVE = 8,
	R_X86_64_DTPMOD64 = 16,
	R_X86_64_DTPOFF64 = 17,
	R_X86_64_TPOFF64 = 18,
};

typedef struct {
	uint8_t width;
	uint16_t type;
} RelDesc;

enum RelType {
	REL_ADDEND  = 1 << 0,
	REL_BASE    = 1 << 1,
	REL_PLACE   = 1 << 2,
	REL_SYMVAL  = 1 << 3,
	REL_SYMSZ   = 1 << 4,
	REL_DTPMOD  = 1 << 5,
	REL_DTPOFF  = 1 << 6,
	REL_TPOFF   = 1 << 7,
	REL_COPY    = 1 << 8,
};

// For the special snowflake named ARM.
#define RELA_IMPLICIT_ADDEND 0

#define RELOC_DEF { \
	[R_X86_64_64]    = { 64, REL_SYMVAL | REL_ADDEND }, \
	[R_X86_64_PC32]  = { 32, REL_SYMVAL | REL_ADDEND | REL_PLACE }, \
	[R_X86_64_GLOB_DAT]  = { 64, REL_SYMVAL }, \
	[R_X86_64_JUMP_SLOT] = { 64, REL_SYMVAL }, \
	[R_X86_64_RELATIVE]  = { 64, REL_BASE | REL_ADDEND }, \
	[R_X86_64_DTPMOD64] = { 64, REL_DTPMOD }, \
	[R_X86_64_DTPOFF64] = { 64, REL_DTPOFF }, \
	[R_X86_64_COPY]     = { 0, REL_COPY }, \
}

//[R_X86_64_SIZE64]   = { 64, REL_SYMSZ | REL_ADDEND },
//[R_X86_64_SIZE32]   = { 32, REL_SYMSZ | REL_ADDEND },
//[R_X86_64_PC16]  = { 16, REL_SYMVAL | REL_ADDEND | REL_PLACE },
//[R_X86_64_PC8]   = {  8, REL_SYMVAL | REL_ADDEND | REL_PLACE },
//[R_X86_64_PC64]  = { 64, REL_SYMVAL | REL_ADDEND | REL_PLACE },
//[R_X86_64_8]     = {  8, REL_SYMVAL | REL_ADDEND },
//[R_X86_64_16]    = { 16, REL_SYMVAL | REL_ADDEND },
//[R_X86_64_32S]   = { 32, REL_SYMVAL | REL_ADDEND },
//[R_X86_64_32]    = { 32, REL_SYMVAL | REL_ADDEND },
