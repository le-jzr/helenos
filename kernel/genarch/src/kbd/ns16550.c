/*
 * Copyright (C) 2001-2006 Jakub Jermar
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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	NS 16550 serial port / keyboard driver.
 */

#include <genarch/kbd/ns16550.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_sun.h>
#include <arch/drivers/kbd.h>
#include <arch/drivers/ns16550.h>
#include <ddi/irq.h>
#include <ipc/irq.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <console/console.h>
#include <interrupt.h>
#include <arch/interrupt.h>
#include <sysinfo/sysinfo.h>
#include <synch/spinlock.h>

#define LSR_DATA_READY	0x01

/** Structure representing the ns16550. */
static ns16550_t ns16550;

/** Structure for ns16550's IRQ. */
static irq_t ns16550_irq;

/*
 * These codes read from ns16550 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f		/* all keys up */

static void ns16550_suspend(chardev_t *);
static void ns16550_resume(chardev_t *);

static chardev_operations_t ops = {
	.suspend = ns16550_suspend,
	.resume = ns16550_resume,
	.read = ns16550_key_read
};

void ns16550_interrupt(void);

/** Initialize keyboard and service interrupts using kernel routine */
void ns16550_grab(void)
{
	ns16550_ier_write(&ns16550, IER_ERBFI);		/* enable receiver interrupt */
	
	while (ns16550_lsr_read(&ns16550) & LSR_DATA_READY)
		(void) ns16550_rbr_read(&ns16550);

	ns16550_irq.notif_cfg.notify = false;
}

/** Resume the former interrupt vector */
void ns16550_release(void)
{
	if (ns16550_irq.notif_cfg.answerbox)
		ns16550_irq.notif_cfg.notify = true;
}

/** Initialize ns16550.
 *
 * @param devno Device number.
 * @param inr Interrupt number.
 * @param vaddr Virtual address of device's registers.
 */
void ns16550_init(devno_t devno, inr_t inr, uintptr_t vaddr)
{
	chardev_initialize("ns16550_kbd", &kbrd, &ops);
	stdin = &kbrd;
	
	ns16550.devno = devno;
	ns16550.reg = (uint8_t *) vaddr;
	
	irq_initialize(&ns16550_irq);
	ns16550_irq.devno = devno;
	ns16550_irq.inr = inr;
	ns16550_irq.claim = ns16550_claim;
	ns16550_irq.handler = ns16550_irq_handler;
	irq_register(&ns16550_irq);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_NS16550);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, inr);
	sysinfo_set_item_val("kbd.address.virtual", NULL, vaddr);
	
	ns16550_grab();
}

/** Process ns16550 interrupt. */
void ns16550_interrupt(void)
{
	/* TODO
	 *
	 * ns16550 works in the polled mode so far.
	 */
}

/* Called from getc(). */
void ns16550_resume(chardev_t *d)
{
}

/* Called from getc(). */
void ns16550_suspend(chardev_t *d)
{
}

char ns16550_key_read(chardev_t *d)
{
	char ch;	

	while(!(ch = active_read_buff_read())) {
		uint8_t x;
		while (!(ns16550_lsr_read(&ns16550) & LSR_DATA_READY))
			;
		x = ns16550_rbr_read(&ns16550);
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				active_read_key_pressed(x);
		}
	}
	return ch;
}

/** Poll for key press and release events.
 *
 * This function can be used to implement keyboard polling.
 */
void ns16550_poll(void)
{
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&ns16550_irq.lock);

	if (ns16550_lsr_read(&ns16550) & LSR_DATA_READY) {
		if (ns16550_irq.notif_cfg.notify && ns16550_irq.notif_cfg.answerbox) {
			/*
			 * Send IPC notification.
			 */
			ipc_irq_send_notif(&ns16550_irq);
			spinlock_unlock(&ns16550_irq.lock);
			interrupts_restore(ipl);
			return;
		}
	}

	spinlock_unlock(&ns16550_irq.lock);
	interrupts_restore(ipl);

	while (ns16550_lsr_read(&ns16550) & LSR_DATA_READY) {
		uint8_t x;
		
		x = ns16550_rbr_read(&ns16550);
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}
}

irq_ownership_t ns16550_claim(void)
{
	return (ns16550_lsr_read(&ns16550) & LSR_DATA_READY);
}

void ns16550_irq_handler(irq_t *irq, void *arg, ...)
{
	panic("Not yet implemented, ns16550 works in polled mode.\n");
}

/** @}
 */
