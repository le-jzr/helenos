/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_CONTEXT_H_
#define KERN_mips32_CONTEXT_H_

#include <align.h>
#include <arch/stack.h>
#include <arch/context_struct.h>
#include <trace.h>
#include <arch.h>
#include <arch/faddr.h>
#include <panic.h>

/*
 * Put one item onto the stack to support CURRENT and align it up.
 */
#define SP_DELTA  (ABI_STACK_FRAME + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

#define context_set(ctx, pc, stack, size) \
    context_set_generic(ctx, pc, stack, size)

#define context_set_generic(ctx, _pc, stack, size) \
	do { \
		(ctx)->pc = (uintptr_t) (_pc); \
		(ctx)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
	} while (0)

extern int context_save_arch(context_t *ctx) __attribute__((returns_twice));
extern void context_restore_arch(context_t *ctx) __attribute__((noreturn));

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
	*context = (context_t) { 0 };
	context_set(context, FADDR(fn), stack_base, stack_size);
}

__attribute__((noreturn)) static inline void context_replace(void (*fn)(void),
    void *stack_base, size_t stack_size)
{
	context_t ctx;
	context_create(&ctx, fn, stack_base, stack_size);
	context_swap(NULL, &ctx);
	unreachable();
}

#endif

/** @}
 */
