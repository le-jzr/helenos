#pragma once

void tlb_invalidate_all(void);
void icache_invalidate_all(void);
void dcache_invalidate_all(void);
void dcache_clean_all(void);

void enable_caches(void);
void disable_caches(void);
void enable_mmu(void);
void disable_mmu(void);

void enable_l2c(void);
void disable_l2c(void);

void halt(void);

void dsb(void);
void isb(void);
