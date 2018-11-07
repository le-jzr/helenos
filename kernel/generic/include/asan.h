#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(__SANITIZE_ADDRESS__) || defined(__ASAN_IMPL__)

void asan_disable_lowmem(void);
void asan_check_alignment(uintptr_t addr, uintptr_t size);
void asan_mark_rw(uintptr_t addr, uintptr_t size, bool initialized);
void asan_mark_ro(uintptr_t addr, uintptr_t size);
void asan_mark_wo(uintptr_t addr, uintptr_t size);
void asan_mark_freed_frames(uintptr_t addr, uintptr_t size);
void asan_poison(uintptr_t addr, uintptr_t size);
void asan_enable(void);
void asan_init_shadow(void);
uintptr_t asan_shadow_to_kernel(uintptr_t);
uintptr_t asan_kernel_to_shadow(uintptr_t);

void __asan_load1_noabort(uintptr_t addr);
void __asan_store1_noabort(uintptr_t addr);
void __asan_load2_noabort(uintptr_t addr);
void __asan_store2_noabort(uintptr_t addr);
void __asan_load4_noabort(uintptr_t addr);
void __asan_store4_noabort(uintptr_t addr);
void __asan_load8_noabort(uintptr_t addr);
void __asan_store8_noabort(uintptr_t addr);
void __asan_load16_noabort(uintptr_t addr);
void __asan_store16_noabort(uintptr_t addr);
void __asan_loadN_noabort(uintptr_t addr, uintptr_t size);
void __asan_storeN_noabort(uintptr_t addr, uintptr_t size);
void __asan_handle_no_return(void);
void __sanitizer_ptr_cmp(void *a, void *b);
void __sanitizer_ptr_sub(void *a, void *b);
void __stack_chk_fail(void);

#else

#define asan_shadow_to_kernel(addr) (addr)
#define asan_mark_rw(addr, size, initialized) ((void) 0)

#endif

#if defined(__SANITIZE_ADDRESS__) && !defined(ASAN_SANITIZE_ALL)
#define ASAN_DISABLE __attribute__((no_sanitize_address))
#define ASAN_LOAD(addr, size) __asan_loadN_noabort((uintptr_t) (addr), (size))
#define ASAN_STORE(addr, size) __asan_storeN_noabort((uintptr_t) (addr), (size))
#define ASAN_ALIGNED(addr, align) asan_check_alignment((uintptr_t) (addr), (align))
#else
#define ASAN_DISABLE
#define ASAN_LOAD(addr, size)
#define ASAN_STORE(addr, size)
#define ASAN_ALIGNED(addr, align)
#endif
