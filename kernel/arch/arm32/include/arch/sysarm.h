#pragma once

#include <stdbool.h>
#include <stdint.h>

// Enable high interrupt vectors if supported, and returns whether enabled.
extern bool sysarm_high_vectors_enable(void);

extern void sysarm_tlb_invalidate_all(void);

extern void sysarm_memory_barrier(void);
extern void sysarm_write_memory_barrier(void);
extern void sysarm_read_memory_barrier(void);

extern void sysarm_flush_modified_code_range(uintptr_t from, uintptr_t to);
extern void sysarm_wait_for_event(void);
