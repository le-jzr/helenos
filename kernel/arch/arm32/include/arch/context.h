/*
 * Copyright (c) 2007 Michal Kebrt
 * Copyright (c) 2023 Jiří Zárevúcky
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

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Thread context.
 */

#ifndef KERN_arm32_CONTEXT_H_
#define KERN_arm32_CONTEXT_H_

#include <align.h>
#include <assert.h>
#include <arch/stack.h>
#include <arch/context_struct.h>
#include <arch/regutils.h>
#include <panic.h>

/* Put one item onto the stack to support CURRENT and align it up. */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

_NO_TRACE static inline uintptr_t arm_current_mode()
{
	return current_status_reg_read() & STATUS_REG_MODE_MASK;
}

/**
 * Saves current context to the variable pointed to by `self`,
 * and restores the context denoted by `other`.
 *
 * When the `self` context is later restored by another call to
 * `context_swap()`, the control flow behaves as if the earlier call to
 * `context_swap()` just returned.
 */
_NO_TRACE static inline void context_swap(context_t *self, context_t *other)
{
	assert(self);
	assert(arm_current_mode() == SUPERVISOR_MODE);

	/*
	 * Explicitly store the arguments in r0/r1 so that we know which regs
	 * to clobber in asm.
	 */
	register uintptr_t r0 asm("r0") = (uintptr_t) self;
	register uintptr_t r1 asm("r1") = (uintptr_t) other;

	asm volatile (
	    /* FP cannot be used in clobbers, apparently. */
	    "push {fp} \n"

	    /* Clear LR and FP, in case we're restoring a new context. */
	    "mov lr, #0 \n"
	    "mov fp, #0 \n"

	    /* Store current SP and PC + 8. */
	    "stmia %[r0], {sp, pc} \n"
	    /* Restore saved SP and PC. */
	    "ldmia %[r1], {sp, pc} \n"

	    /* Restore FP. */
	    "pop {fp} \n"

	    /*
	     * Use r0/r1 as inputs, but also tell GCC they are overwritten by
	     * setting them as outputs. Clobber everything else to make GCC
	     * preserve those registers if needed.
	     */
	    : "=r" (r0),
	      "=r" (r1)
	    : [r0] "r" (r0),
	      [r1] "r" (r1)
	    : "lr", "ip", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
	      "cc", "memory"
	);
}

_NO_TRACE static inline void context_create(context_t *context,
    void (*fn)(void), void *stack_base, size_t stack_size)
{
	*context = (context_t) {
		.pc = (uintptr_t) fn,
		.sp = (uintptr_t) stack_base + stack_size - SP_DELTA,
	};
}

__attribute__((noreturn)) static inline void context_replace(void (*fn)(void),
    void *stack_base, size_t stack_size)
{
	assert(arm_current_mode() == SUPERVISOR_MODE);

	asm volatile (
	    "mov fp, #0 \n"
	    "mov lr, #0 \n"
	    "mov sp, %[sp] \n"
	    "mov pc, %[pc] \n"
	    :
	    : [pc] "r" ((uintptr_t) fn),
	      [sp] "r" ((uintptr_t) stack_base + stack_size - SP_DELTA)
	);

	unreachable();
}

#endif

/** @}
 */
