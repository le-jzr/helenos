
#define __ASAN_IMPL__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <panic.h>
#include <print.h>
#include <stacktrace.h>
#include <asan.h>
#include <arch/mm/km.h>
#include <align.h>
#include <mem.h>
#include <mm/km.h>

#define KASAN_STACK_LEFT       0xF1
#define KASAN_STACK_MID        0xF2
#define KASAN_STACK_RIGHT      0xF3
#define KASAN_STACK_PARTIAL    0xF4
#define KASAN_USE_AFTER_SCOPE  0xF8

extern uint8_t ktext_start[];
extern uint8_t ktext_end[];
extern uint8_t kdata_start[];
extern uint8_t krodata_end[];
extern uint8_t symbol_table[];
extern uint8_t kdata_end[];

static bool _lowmem_disable = false;

// FIXME: thread local
static bool _asan_disable = false;

static bool _asan_enabled = false;
static bool _asan_shadow_enabled = false;

static bool _asan_fatal = true;

static inline uintptr_t _kmem_start(void)
{
	// XXX: We are making the simplifying assumption that kernel uses
	//      the top 2^N bytes of the virtual address space for some N.
	//      So we can easily calculate the base from the size of shadow
	//      memory.

	// FIXME: Would be safer to just provide constants for the kernel
	//        memory range, since it's known statically. If the range is
	//        not static (e.g. on RISC-V we could dynamically decide the
	//        usable address width), then we would need more changes anyway,
	//        as the shadow memory size and location would have to change
	//        to match.

	return ((uintptr_t) 0) - KM_SHADOW_SIZE * 8;
}

static inline uintptr_t _kernel_to_shadow(uintptr_t addr)
{
	return KM_SHADOW_START + (addr - _kmem_start()) / 8;
}

static inline uintptr_t _shadow_to_kernel(uintptr_t addr)
{
	return _kmem_start() + (addr - KM_SHADOW_START) * 8;
}

uintptr_t asan_kernel_to_shadow(uintptr_t addr)
{
	return _kernel_to_shadow(addr);
}

uintptr_t asan_shadow_to_kernel(uintptr_t addr)
{
	return _shadow_to_kernel(addr);
}

#define _asan_error(...) \
	do { \
		bool dis = _asan_disable; \
		_asan_disable = true; \
		if (_asan_fatal) { \
			panic(__VA_ARGS__); \
		} else { \
			printf(__VA_ARGS__); \
			stack_trace(); \
		} \
		_asan_disable = dis; \
	} while (0);

/**
 * This function makes asan reject any future accesses to lower half of
 * the address space. During early boot, lower half is uses as an identity
 * mapping of the physical memory, so we don't do this straight away.
 */
void asan_disable_lowmem(void)
{
	_lowmem_disable = true;
}

#define FLAG_NO_READ      0x40
#define FLAG_NO_WRITE     0x20
#define FLAG_INITIALIZED  0x10

void asan_enable(void)
{
	_asan_enabled = true;
}

void asan_init_shadow(void)
{
	uintptr_t end = config.stack_base + config.stack_size;

	asan_mark_rw(PA2KA(0), end - PA2KA(0), true);

	_asan_shadow_enabled = true;
}

void asan_poison(uintptr_t addr, uintptr_t size)
{
	assert(size > 0);

	/* Area must start and end at 8-byte boundary. */
	assert(addr % 8 == 0);
	assert(size % 8 == 0);

	uintptr_t saddr = _kernel_to_shadow(addr);
	uintptr_t ssize = size / 8;

	bool dis = _asan_disable;
	_asan_disable = true;
	km_shadow_poke(saddr, ssize, 0xffffffffu);
	memset((void *) saddr, 0xff, ssize);
	_asan_disable = dis;
}

void asan_mark_freed_frames(uintptr_t addr, uintptr_t size)
{
	asan_poison(addr, size);
}

static void asan_mark(uintptr_t addr, uintptr_t size, int flags)
{
	assert(size > 0);

	/* Area must start at 8-byte boundary. */
	assert(addr % 8 == 0);

	printf("Making: %p - %p (%zu bytes)\n", (void *) addr,
	    (void *) (addr + size), size);

	uintptr_t off = size - ALIGN_DOWN(size, 8);

	uintptr_t saddr = _kernel_to_shadow(addr);
	uintptr_t ssize = ALIGN_UP(size, 8) / 8;

	bool dis = _asan_disable;
	_asan_disable = true;

	km_shadow_poke(saddr, ssize, 0xffffffffu);
	memset((void *) saddr, 0x00 | flags, size / 8);
	if (off > 0)
		((uint8_t *) saddr)[size / 8] = (uint8_t) off | flags;

	_asan_disable = dis;
}

void asan_mark_rw(uintptr_t addr, uintptr_t size, bool initialized)
{
	asan_mark(addr, size, initialized ? FLAG_INITIALIZED : 0);
}

void asan_mark_ro(uintptr_t addr, uintptr_t size)
{
	asan_mark(addr, size, FLAG_NO_WRITE | FLAG_INITIALIZED);
}

void asan_mark_wo(uintptr_t addr, uintptr_t size)
{
	asan_mark(addr, size, FLAG_NO_READ);
}

static inline void _asan_check_alignment(uintptr_t addr, uintptr_t size)
{
	if (_asan_disable || !_asan_enabled)
		return;

	if (addr % size == 0)
		return;

	_asan_error("Misaligned memory access: %p, %zu\n", (void *) addr, size);
}

void asan_check_alignment(uintptr_t addr, uintptr_t size)
{
	_asan_check_alignment(addr, size);
}

__attribute__((no_sanitize_address))
static inline void _asan_access(uintptr_t addr, uintptr_t size)
{
	if (_asan_disable || !_asan_enabled || size == 0)
		return;

	/*
	 * Since we are working with unsigned variables, explicit overflow
	 * check is in order.
	 */
	assert(addr + size > addr);

	if ((intptr_t) addr >= 0) {
		if (!_lowmem_disable)
			return;

		_asan_error("Kernel memory access to lower half of memory: "
		    "%p, %zu\n", (void *) addr, size);
	}

	if (addr < KM_SHADOW_START + KM_SHADOW_SIZE &&
	    addr + size > KM_SHADOW_START) {

		/*
		 * No code should access shadow memory unless
		 * _asan_disable is true.
		 */

		_asan_error("Access to shadow memory outside asan.\n");
	}

	if (addr < (uintptr_t) ktext_end) {

		if (addr + size > (uintptr_t) ktext_start) {
			_asan_error("Kernel data access inside .text section: "
			    "%p, %zu\n", (void *) addr, size);
		} else {
#if 0
			_asan_error("Kernel memory access below ktext: "
			    "%p, %zu\n", (void *) addr, size);
#endif
		}
	}

	if (!_asan_shadow_enabled)
		return;

	uintptr_t saddr = _kernel_to_shadow(addr);
	uintptr_t ssize = _kernel_to_shadow(ALIGN_UP(addr + size, 8)) - saddr;

	if (saddr - KM_SHADOW_START + ssize > KM_SHADOW_SIZE)
		panic("Access outside the shadow memory range.\n");

	uint8_t *s = (uint8_t *) saddr;

	_asan_disable = true;

	for (uintptr_t i = 0; i < ssize; i++) {
		if (s[i] != 0)
			continue;

		_asan_error("Access to inaccessible memory: %p, %zu\n",
		    (void *) addr, size);
	}

	_asan_disable = false;
}

__attribute__((no_sanitize_address))
static inline void _asan_read(uintptr_t addr, uintptr_t size)
{
	if (_asan_disable || !_asan_enabled)
		return;

	// TODO: check that the memory is initialized
}

__attribute__((no_sanitize_address))
static inline void _asan_write(uintptr_t addr, uintptr_t size)
{
	if (_asan_disable || !_asan_enabled)
		return;

	if (addr < (uintptr_t) krodata_end && addr + size > (uintptr_t) kdata_start) {
		_asan_error("Kernel data write inside .rodata section: "
		    "%p, %zu\n", (void *) addr, size);
	}

	if (addr < (uintptr_t) kdata_end && addr + size > (uintptr_t) symbol_table) {
		_asan_error("Kernel data write inside symbol table section: "
		    "%p, %zu\n", (void *) addr, size);
	}

//	uintptr_t top = p + size;
}

void __asan_load1_noabort(uintptr_t addr)
{
	_asan_access(addr, 1);
}

void __asan_store1_noabort(uintptr_t addr)
{
	_asan_access(addr, 1);
	_asan_write(addr, 1);
}

void __asan_load2_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 2);
	_asan_access(addr, 2);
}

void __asan_store2_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 2);
	_asan_access(addr, 2);
	_asan_write(addr, 2);
}

void __asan_load4_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 4);
	_asan_access(addr, 4);
}

void __asan_store4_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 4);
	_asan_access(addr, 4);
	_asan_write(addr, 4);
}

void __asan_load8_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 8);
	_asan_access(addr, 8);
}

void __asan_store8_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 8);
	_asan_access(addr, 8);
	_asan_write(addr, 8);
}

void __asan_load16_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 16);
	_asan_access(addr, 16);
}

void __asan_store16_noabort(uintptr_t addr)
{
	_asan_check_alignment(addr, 16);
	_asan_access(addr, 16);
	_asan_write(addr, 16);
}

void __asan_loadN_noabort(uintptr_t addr, uintptr_t size)
{
	_asan_access(addr, size);
}

void __asan_storeN_noabort(uintptr_t addr, uintptr_t size)
{
	_asan_access(addr, size);
	_asan_write(addr, size);
}

void __asan_handle_no_return(void)
{
}

void __sanitizer_ptr_cmp(void *a, void *b)
{
}

void __sanitizer_ptr_sub(void *a, void *b)
{
}

void __stack_chk_fail(void)
{
}

