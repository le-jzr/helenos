/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_CONTEXT_H_
#define KERN_ia64_CONTEXT_H_

#include <typedefs.h>
#include <arch/register.h>
#include <align.h>
#include <arch/stack.h>
#include <arch/context_struct.h>
#include <trace.h>
#include <arch.h>
#include <arch/faddr.h>
#include <panic.h>

/*
 * context_save_arch() and context_restore_arch() are both leaf procedures.
 * No need to allocate scratch area.
 *
 * One item is put onto the stack to support CURRENT.
 */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

extern void *__gp;

extern int context_save_arch(context_t *ctx) __attribute__((returns_twice));
extern void context_restore_arch(context_t *ctx) __attribute__((noreturn));
extern void context_replace_arch(uintptr_t pc, uintptr_t sp, uintptr_t bsp,
    uintptr_t gp, uintptr_t fpsr) __attribute__((noreturn));

/**
 * Saves current context to the variable pointed to by `self`,
 * and restores the context denoted by `other`.
 *
 * When the `self` context is later restored by another call to
 * `context_swap()`, the control flow behaves as if the earlier call to
 * `context_swap()` just returned.
 *
 * If `self` is NULL, the currently running context is thrown away.
 */
_NO_TRACE static inline void context_swap(context_t *self, context_t *other)
{
	if (!self || context_save_arch(self))
		context_restore_arch(other);
}

_NO_TRACE static inline void context_create(context_t *context,
    void (*fn)(void), void *stack_base, size_t stack_size)
{
	/* RSE stack starts at the bottom of memory stack, hence the division by 2. */

	*context = (context_t) {
		.pc = FADDR(fn),
		.sp = ((uintptr_t) stack_base) + ALIGN_UP((stack_size / 2), STACK_ALIGNMENT) - SP_DELTA,
		.bsp = ((uintptr_t) stack_base) + ALIGN_UP((stack_size / 2), REGISTER_STACK_ALIGNMENT),
		.ar_fpsr = FPSR_TRAPS_ALL | FPSR_SF1_CTRL,
		.r1 = (uintptr_t) &__gp,
	};
}

__attribute__((noreturn)) static inline void context_replace(void (*fn)(void),
    void *stack_base, size_t stack_size)
{
	uintptr_t sp = ((uintptr_t) stack_base) + ALIGN_UP((stack_size / 2), STACK_ALIGNMENT) - SP_DELTA;
	uintptr_t bsp = ((uintptr_t) stack_base) + ALIGN_UP((stack_size / 2), REGISTER_STACK_ALIGNMENT);

	context_replace_arch(FADDR(fn), sp, bsp, (uintptr_t) &__gp,
	    FPSR_TRAPS_ALL | FPSR_SF1_CTRL);
	unreachable();
}

#endif

/** @}
 */
