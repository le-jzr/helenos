/* This header file serves as a central location for platform variations
 * between various ARM boards. Do not put platform specific constants
 * anywhere else.
 */

/* Note that RAM_START and RAM_END must be aligned to 1 MB, i.e. the last five
 * hex digits must be zero (RAM_{START,END} % 0x100000 == 0).
 * BOOT_BASE has no such requirement.
 *
 * The loader maps memory in two stages.
 * First, it creates an identity mapping for the entire address space,
 * with caching enabled for the region between RAM_START and RAM_END.
 *
 * Then, after all the data is prepared for the kernel, part of the
 * identity mapping is overwritten, so that the region between RAM_START
 * and RAM_END is mapped at a fixed offset of 0x80000000.
 *
 * However, there is a catch. This new mapping must not overwrite the region of
 * identity mapping in which the bootloader is located, otherwise bad things
 * happen. This is not an issue for any of the existing platforms, but in case
 * where a new platform has RAM mapped at physical addresses > 0x80000000, this
 * needs to be ensured, e.g. by moving BOOT_BASE somewhere safe, and/or setting
 * RAM_END lower than the real end of memory. This has no effect on the kernel's
 * ability to utilize extra RAM above RAM_END.
 */

#if defined(MACHINE_beagleboardxm) || defined(MACHINE_beaglebone)

/* The start of usable RAM in physical address space. */
#define RAM_START  0x80000000U
#define RAM_END    0xc0000000U
/* Address where the boot stage image (this binary) starts. */
#define BOOT_BASE  0x80000000

#elif defined(MACHINE_gta02)

#define RAM_START  0x30000000U
#define RAM_END    0x38000000U
#define BOOT_BASE  0x30008000

#elif defined(MACHINE_raspberrypi)

#define RAM_START  0
#define RAM_END    0x20000000U
#define BOOT_BASE  0x00008000

#elif defined(MACHINE_integratorcp)

#define RAM_START  0
#define RAM_END    0x20000000U
#define BOOT_BASE  0

#elif defined(MACHINE_omnia)

#define RAM_START  0
/* Omnia can have either 1 or 2GB of memory, but it doesn't matter much
 * to the bootloader.
 */
#define RAM_END    0x40000000U

/* The default load offset in uboot. For convenience.
 * Could be 0, but we have at least 1 GB to work with, and kernel reclaims
 * the memory, so there's no point.
 */
#define BOOT_BASE  0x00800000

/* Define to 1 if RAM memory should be aliased at 0x80000000 by the boot
 * page table. This makes the corresponding portion of physical address space
 * inaccessible by the loader.
 */
#define KERNEL_REMAP 1

#else

#error RAM_START not defined

#endif

#ifdef PROCESSOR_cortex_a9
#ifdef MACHINE_omnia
#define L2_CACHE_BASE 0xf1008000
#else
#error Unspecified L2 cache register file base address.
#endif
#endif

/** Address where characters to be printed are expected. */

/** BeagleBoard-xM UART register address
 *
 * This is UART3 of AM/DM37x CPU
 */
#define BBXM_SCONS_THR          0x49020000
#define BBXM_SCONS_SSR          0x49020044

/* Check this bit before writing (tx fifo full) */
#define BBXM_THR_FULL           0x00000001

/** Beaglebone UART register addresses
 *
 * This is UART0 of AM335x CPU
 */
#define BBONE_SCONS_THR         0x44E09000
#define BBONE_SCONS_SSR         0x44E09044

/** Check this bit before writing (tx fifo full) */
#define BBONE_TXFIFO_FULL       0x00000001

/** GTA02 serial console UART register addresses.
 *
 * This is UART channel 2 of the S3C24xx CPU
 */
#define GTA02_SCONS_UTRSTAT	0x50008010
#define GTA02_SCONS_UTXH	0x50008020

/* Bits in UTXH register */
#define S3C24XX_UTXH_TX_EMPTY	0x00000004


/** IntegratorCP serial console output register */
#define ICP_SCONS_ADDR		0x16000000

/** Raspberry PI serial console registers */
#define BCM2835_UART0_BASE	0x20201000
#define BCM2835_UART0_DR	(BCM2835_UART0_BASE + 0x00)
#define BCM2835_UART0_FR	(BCM2835_UART0_BASE + 0x18)
#define BCM2835_UART0_ILPR	(BCM2835_UART0_BASE + 0x20)
#define BCM2835_UART0_IBRD	(BCM2835_UART0_BASE + 0x24)
#define BCM2835_UART0_FBRD	(BCM2835_UART0_BASE + 0x28)
#define BCM2835_UART0_LCRH	(BCM2835_UART0_BASE + 0x2C)
#define BCM2835_UART0_CR	(BCM2835_UART0_BASE + 0x30)
#define BCM2835_UART0_ICR	(BCM2835_UART0_BASE + 0x44)

#define BCM2835_UART0_FR_TXFF	(1 << 5)
#define BCM2835_UART0_LCRH_FEN	(1 << 4)
#define BCM2835_UART0_LCRH_WL8	((1 << 5) | (1 << 6))
#define BCM2835_UART0_CR_UARTEN	(1 << 0)
#define BCM2835_UART0_CR_TXE	(1 << 8)
#define BCM2835_UART0_CR_RXE	(1 << 9)
