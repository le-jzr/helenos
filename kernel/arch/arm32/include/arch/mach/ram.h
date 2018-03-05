#pragma once

/* XXX: Keep this in sync with boot/arch/arm32/include/arch/platform.h */

#if defined(MACHINE_beagleboardxm) || defined(MACHINE_beaglebone)

/* The start of usable RAM in physical address space. */
#define RAM_START  0x80000000
#define RAM_END    0xc0000000
/* Address where the boot stage image (this binary) starts. */
#define BOOT_BASE  0x80000000

#elif defined(MACHINE_gta02)

#define RAM_START  0x30000000
#define RAM_END    0x38000000
#define BOOT_BASE  0x30008000

#elif defined(MACHINE_raspberrypi)

#define RAM_START  0
#define RAM_END    0x20000000
#define BOOT_BASE  0x00008000

#elif defined(MACHINE_integratorcp)

#define RAM_START  0
#define RAM_END    0x20000000
#define BOOT_BASE  0

#elif defined(MACHINE_omnia)

#define RAM_START  0
/* Omnia can have either 1 or 2GB of memory.
 * Either way, nothing else is mapped in the lower half.
 */
#define RAM_END    0x40000000
/* The default load offset in uboot. For convenience.
 * Could be 0, but we have at least 1 GB to work with, and kernel reclaims
 * the memory, so there's no point.
 */
#define BOOT_BASE  0x00800000

#else

#error RAM_START not defined

#endif
