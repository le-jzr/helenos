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

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CONTEXT_H_
#define KERN_sparc64_CONTEXT_H_

#include <arch/stack.h>
#include <arch/context_struct.h>
#include <typedefs.h>
#include <align.h>
#include <trace.h>
#include <arch.h>
#include <panic.h>

#define SP_DELTA  (STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE)

extern void context_replace_arch(uintptr_t pc, uintptr_t sp);
extern void context_swap_arch(context_t *self, context_t *other);

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
	if (!self) {
		context_t dummy;
		context_swap_arch(&dummy, other);
	} else {
		context_swap_arch(self, other);
	}
}

_NO_TRACE static inline void context_create(context_t *context,
    void (*fn)(void), void *stack_base, size_t stack_size)
{
	uintptr_t stack_top = (uintptr_t) stack_base + stack_size - SP_DELTA;

	*context = (context_t) {
		.pc = (uintptr_t) fn - 8,
		.sp = stack_top - STACK_BIAS,
		.fp = -STACK_BIAS,
	};
}

__attribute__((noreturn)) static inline void context_replace(void (*fn)(void),
    void *stack_base, size_t stack_size)
{
	uintptr_t stack_top = (uintptr_t) stack_base + stack_size - SP_DELTA;

	context_replace_arch((uintptr_t) fn - 8, stack_top - STACK_BIAS);
	unreachable();
}

#endif

/** @}
 */
