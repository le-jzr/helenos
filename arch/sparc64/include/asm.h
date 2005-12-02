/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __sparc64_ASM_H__
#define __sparc64_ASM_H__

#include <arch/types.h>
#include <config.h>

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of IPL.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_enable(void) {
}

/** Disable interrupts.
 *
 * Disable interrupts and return previous
 * value of IPL.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_disable(void) {
}

/** Restore interrupt priority level.
 *
 * Restore IPL.
 *
 * @param ipl Saved interrupt priority level.
 */
static inline void interrupts_restore(ipl_t ipl) {
}

/** Return interrupt priority level.
 *
 * Return IPL.
 *
 * @return Current interrupt priority level.
 */
static inline ipl_t interrupts_read(void) {
}

/** Return base address of current stack.
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 */
static inline __address get_stack_base(void)
{
	__address v;
	
	__asm__ volatile ("and %%o6, %1, %0\n" : "=r" (v) : "r" (~(STACK_SIZE-1)));
	
	return v;
}

/** Read Trap Base Address register.
 *
 * @return Current value in TBA.
 */
static inline __u64 tba_read(void)
{
	__u64 v;
	
	__asm__ volatile ("rdpr %%tba, %0\n" : "=r" (v));
	
	return v;
}

/** Write Trap Base Address register.
 *
 * @param New value of TBA.
 */
static inline void tba_write(__u64 v)
{
	__asm__ volatile ("wrpr %0, %1, %%tba\n" : : "r" (v), "i" (0));
}


void cpu_halt(void);
void cpu_sleep(void);
void asm_delay_loop(__u32 t);

#endif
