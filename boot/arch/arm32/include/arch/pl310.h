#pragma once

#define READ__ 1
#define WRITE__ 2

#define RO READ__
#define WO WRITE__
#define RW (READ__ | WRITE__)

#define __reg(offset__, access__, name__) \
	static inline uint32_t pl310_read_##name__(void *base) { return (access__ & READ__) ? *(volatile uint32_t *)(void*)(base + offset__) : 0; } \
	static inline void pl310_write_##name__(void *base, uint32_t val__) { if (access__ & WRITE__) *(volatile uint32_t *)(void*)(base + offset__) = val__; }

#include "pl310_regs.in"

#undef RO
#undef WO
#undef RW
#undef __reg

static inline int pl310_get_bits(uint32_t *val, int nbits) {
	int bits = *val & ((1 << nbits) - 1);
	*val >>= nbits;
	return bits;
}

static inline void pl310_read_cache_id(void *base, int *implementer, int *cache_id, int *part_number, int *rtl_release) {
	uint32_t reg = pl310_read_reg0_cache_id(base);
	*rtl_release = pl310_get_bits(&reg, 6);
	*part_number = pl310_get_bits(&reg, 4);
	*cache_id = pl310_get_bits(&reg, 6);
	(void) pl310_get_bits(&reg, 8); // reserved
	*implementer = pl310_get_bits(&reg, 8);
}
