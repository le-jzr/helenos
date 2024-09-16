
#ifndef LIBC_RTLD_ARCH_H_
#define LIBC_RTLD_ARCH_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "cc.h"

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

typedef struct {
	uint8_t width;
	uint16_t type;
} elf_rel_desc_t;

INTERNAL extern const elf_rel_desc_t arch_rel_list[];
INTERNAL extern const size_t         arch_rel_len;

#endif
