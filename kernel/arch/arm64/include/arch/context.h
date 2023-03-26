/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Thread context.
 */

#ifndef KERN_arm64_CONTEXT_H_
#define KERN_arm64_CONTEXT_H_

#include <assert.h>
#include <align.h>
#include <arch/context_struct.h>
#include <arch/stack.h>
#include <trace.h>
#include <arch.h>
#include <panic.h>

/* Put one item onto the stack to support CURRENT and align it up. */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

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

	register uintptr_t x0 asm("x0") = (uintptr_t) self;
	register uintptr_t x1 asm("x1") = (uintptr_t) other;

	asm volatile (
	    /* Save FP and LR on stack. */
	    "sub sp, sp, #16 \n"
	    "stp fp, lr, [sp] \n"

	    /* Clear FP and LR, in case we're swapping to a new context. */
	    "mov fp, #0 \n"
	    "mov lr, #0 \n"

	    /* Set x2 to PC address just past the branch below. */
	    "adr x2, 1f \n"
	    "mov x3, sp \n"

	    /* Write the SP and PC values to context. */
	    "stp x3, x2, [%[x0]] \n"
	    /* Read the SP and PC values from the other context. */
	    "ldp x3, x2, [%[x1]] \n"

	    /* Set stack pointer and branch to new PC. */
	    "mov sp, x3 \n"
	    "br x2 \n"

	    /* We arrive here when we return from another swap. */
	    /* Restore FP and LR from stack. */
	    "1: ldp fp, lr, [sp] \n"
	    "add sp, sp, #16 \n"

	    : "=r" (x0),
	      "=r" (x1)
	    : [x0] "r" (x0),
	      [x1] "r" (x1)
	    : "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12",
	      "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20", "x21", "x22",
	      "x23", "x24", "x25", "x26", "x27", "x28", "cc", "memory"
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
	asm volatile (
	    "mov lr, #0 \n"
	    "mov fp, #0 \n"
	    "mov sp, %[sp] \n"
	    "br %[pc] \n"
	    :
	    : [pc] "r" ((uintptr_t) fn),
	      [sp] "r" ((uintptr_t) stack_base + stack_size - SP_DELTA)
	    : "memory"
	);

	unreachable();
}

#endif

/** @}
 */
