/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** @addtogroup arm32turrisomnia
 * @{
 */
/** @file
 *  @brief Turris Omnia platform driver.
 */

#include <arch/exception.h>
#include <assert.h>
#include <genarch/srln/srln.h>
#include <interrupt.h>
#include <ddi/ddi.h>
#include <mm/km.h>
#include <console/console.h>
#include <arch/mach/turrisomnia/turrisomnia.h>
#include <genarch/drivers/ns16550/ns16550.h>
#include <genarch/drivers/gicv1/gicv1.h>

#define OMNIA_MEMORY_START       0x00000000      /* physical */
#define OMNIA_MEMORY_SIZE        0x40000000      /* 1 GB */
// TODO: Support 2GB variant.

#define UART_BASE 0xf1012000
#define UART_SIZE 0x6000
#define UART_REG_SHIFT 2

#define GIC_BASE 0xf100C000
#define GIC_SIZE 0x2000


// Cortex-A9 MPCore TRM
#define GLOBAL_TIMER_INR 27
#define PIC_FIQ_SUMMARY_INR 28
#define PRIVATE_TIMER_INR 29
#define WATCHDOG_TIMER_INR 30
#define PIC_IRQ_SUMMARY_INR 31
// ARMADA 38x Functional Specification
#define UART0_INR 44
#define UART1_INR 45
#define RTC_INR 53

#define LAST_INR 191

static void omnia_init(void);
static void omnia_timer_irq_start(void);
static void omnia_cpu_halt(void);
static void omnia_get_memory_extents(uintptr_t *start, size_t *size);
static void omnia_irq_exception(unsigned int exc_no, istate_t *istate);
static void omnia_frame_init(void);
static void omnia_output_init(void);
static void omnia_input_init(void);
static size_t omnia_get_irq_count(void);
static const char *omnia_get_platform_name(void);

volatile gicv1_distributor_t *gicv1_distributor;
volatile gicv1_cpu_interface_t *gicv1_cpu;

// TODO
static struct omnia {
	void *uart_base;
	ioport8_t *uart0;
	ns16550_instance_t *indev0;
	outdev_t *outdev0;
	
	ioport8_t *uart1;
} omnia;

struct arm_machine_ops omnia_machine_ops = {
	.machine_init = omnia_init,
	.machine_timer_irq_start = omnia_timer_irq_start,
	.machine_cpu_halt = omnia_cpu_halt,
	.machine_get_memory_extents = omnia_get_memory_extents,
	.machine_irq_exception = omnia_irq_exception,
	.machine_frame_init = omnia_frame_init,
	.machine_output_init = omnia_output_init,
	.machine_input_init = omnia_input_init,
	.machine_get_irq_count = omnia_get_irq_count,
	.machine_get_platform_name = omnia_get_platform_name,
};

static void omnia_init(void)
{
	// TODO: Disable watchdog somewhere at the end.
	omnia.uart_base = (void *) km_map(UART_BASE, UART_SIZE, PAGE_NOT_CACHEABLE);
	assert(omnia.uart_base != NULL);
	omnia.uart0 = (ioport8_t *) omnia.uart_base;
	omnia.uart1 = omnia.uart0 + 0x100;
	
	uintptr_t gic = km_map(GIC_BASE, GIC_SIZE, PAGE_NOT_CACHEABLE);
	gicv1_distributor = (gicv1_distributor_t *) (gic + 0x1000);
	gicv1_cpu = (gicv1_cpu_interface_t *) (gic + 0x100);
	
	// XXX: enable all
	for (int i = 0; i < 32; i++) {
		gicv1_distributor->iser[i] = 0xffffffff;
	}
	
	gicv1_dist_enable();
	gicv1_cpu->icr = 1;
	
	printf("IC distributor enabled: %d\n", gicv1_dist_is_enabled());

	// TODO
}

#if 0
static irq_ownership_t omnia_timer_irq_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void omnia_timer_irq_handler(irq_t *irq)
{
	// TODO
	// TODO: ack
	#if 0
	spinlock_unlock(&irq->lock);
	clock();
	spinlock_lock(&irq->lock);
	#endif
}
#endif

static void omnia_timer_irq_start(void)
{
	//printf("Unimplemented omnia_timer_irq_start().\n");
	// TODO
}

static void omnia_cpu_halt(void)
{
	while (1);
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void omnia_get_memory_extents(uintptr_t *start, size_t *size)
{
	*start = OMNIA_MEMORY_START;
	*size  = OMNIA_MEMORY_SIZE;
}

static void omnia_irq_exception(unsigned int exc_no, istate_t *istate)
{
	printf("Unimplemented omnia_irq_exception(%d).\n", exc_no);
	uint32_t ir = gicv1_cpu->iar;
	printf("Exception: 0x%08x\n", ir);
	gicv1_cpu->eoir = ir;
	// TODO
}

static void omnia_frame_init(void)
{
	//printf("Unimplemented omnia_frame_init().\n");
}

static void dummy_cir(void *arg, inr_t inr) {
	(void) arg;
	(void) inr;
	// TODO
}

static void omnia_output_init(void)
{
	if (omnia.outdev0 == NULL) {
		omnia.indev0 = ns16550_init(omnia.uart0, UART_REG_SHIFT, UART0_INR, dummy_cir, NULL,
		    &omnia.outdev0);
		assert(omnia.outdev0 != NULL);
	}

	stdout_wire(omnia.outdev0);
}

static void omnia_input_init(void)
{
	// TODO
}

size_t omnia_get_irq_count(void)
{
	return LAST_INR + 1;
}

const char *omnia_get_platform_name(void)
{
	return "turrisomnia";
}

/**
 * @}
 */

