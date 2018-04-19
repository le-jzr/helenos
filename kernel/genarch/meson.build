
if config_acpi
	src += files('src/acpi/acpi.c', 'genarch/src/acpi/madt.c')
endif

if config_page_pt
	src += files('src/mm/page_pt.c', 'src/mm/as_pt.c')
endif

if config_page_ht
	src += files('src/mm/page_ht.c', 'src/mm/as_ht.c')
endif

if config_asid
	src += files('src/mm/asid.c')
endif

if config_asid_fifo
	src += files('src/mm/asid_fifo.c')
endif

if config_softint
	src += files('src/softint/division.c', 'src/softint/multiplication.c')
endif

if config_fb
	src += files('src/fb/font-8x16.c', 'src/fb/fb.c', 'src/fb/bfb.c')
endif

if config_dsrlnin
	src += files('src/drivers/dsrln/dsrlnin.c')
endif

if config_dsrlnout
	src += files('src/drivers/dsrln/dsrlnout.c')
endif

if config_i8042
	src += files('src/drivers/i8042/i8042.c')
endif

ifeq ($(CONFIG_NS16550),y)
GENARCH_SOURCES += \
	genarch/src/drivers/ns16550/ns16550.c
endif

ifeq ($(CONFIG_PL011_UART),y)
GENARCH_SOURCES += \
	genarch/src/drivers/pl011/pl011.c
endif

ifeq ($(CONFIG_S3C24XX_IRQC),y)
GENARCH_SOURCES += \
	genarch/src/drivers/s3c24xx/irqc.c
endif

ifeq ($(CONFIG_S3C24XX_UART),y)
GENARCH_SOURCES += \
	genarch/src/drivers/s3c24xx/uart.c
endif

ifeq ($(CONFIG_OMAP_UART),y)
GENARCH_SOURCES += \
	genarch/src/drivers/omap/uart.c
endif

ifeq ($(CONFIG_AM335X_TIMERS),y)
GENARCH_SOURCES += \
	genarch/src/drivers/am335x/timer.c
endif

ifeq ($(CONFIG_BCM2835_MAILBOX),y)
GENARCH_SOURCES += \
	genarch/src/drivers/bcm2835/mbox.c
endif

ifeq ($(CONFIG_VIA_CUDA),y)
GENARCH_SOURCES += \
	genarch/src/drivers/via-cuda/cuda.c
endif

ifeq ($(CONFIG_PC_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd.c \
	genarch/src/kbrd/scanc_pc.c
endif

ifeq ($(CONFIG_AT_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd_at.c \
	genarch/src/kbrd/scanc_at.c
endif

ifeq ($(CONFIG_SUN_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd.c \
	genarch/src/kbrd/scanc_sun.c
endif

ifeq ($(CONFIG_MAC_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd.c \
	genarch/src/kbrd/scanc_mac.c
endif

ifeq ($(CONFIG_SRLN),y)
GENARCH_SOURCES += \
	genarch/src/srln/srln.c
endif

ifeq ($(CONFIG_OFW_TREE),y)
GENARCH_SOURCES += \
	genarch/src/ofw/ofw_tree.c
endif

ifeq ($(CONFIG_OFW_PCI),y)
GENARCH_SOURCES += \
	genarch/src/ofw/ebus.c \
	genarch/src/ofw/pci.c  \
	genarch/src/ofw/sbus.c \
	genarch/src/ofw/upa.c
endif

ifeq ($(CONFIG_MULTIBOOT), y)
GENARCH_SOURCES += \
	genarch/src/multiboot/common.c \
	genarch/src/multiboot/multiboot.c \
	genarch/src/multiboot/multiboot2.c

GENARCH_AUTOCHECK_HEADERS += \
	genarch/include/genarch/multiboot/multiboot_memmap_struct.h \
	genarch/include/genarch/multiboot/multiboot_info_struct.h
endif

ifeq ($(CONFIG_EGA), y)
GENARCH_SOURCES += \
	genarch/src/drivers/ega/ega.c
endif

ifeq ($(CONFIG_IOMAP_BITMAP), y)
GENARCH_SOURCES += \
	genarch/src/ddi/ddi-bitmap.c
endif

ifeq ($(CONFIG_IOMAP_DUMMY), y)
GENARCH_SOURCES += \
	genarch/src/ddi/ddi-dummy.c
endif

