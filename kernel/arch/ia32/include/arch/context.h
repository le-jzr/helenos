/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CONTEXT_H_
#define KERN_ia32_CONTEXT_H_

#include <typedefs.h>
#include <arch/context_struct.h>
#include <trace.h>
#include <arch.h>
#include <panic.h>

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
	uintptr_t dummy1;
	uintptr_t dummy2;

	void *dummy_sp;

	asm volatile (
	    /* Stash uspace thread pointer. */
	    "movl %%gs:0, %%eax\n"
	    "pushl %%eax\n"

	    /* Stash rbp (can't be put in clobbers list). */
	    "pushl %%ebp\n"

	    /* Call the snippet below to save PC on stack. */
	    "call 1f\n"

	    /* Returned from call. Skip to end. */
	    "jmp 2f\n"

	    /* Save current stack pointer. */
	    "1: movl %%esp, (%%edi)\n"

	    /* Restore stack pointer of the other context. */
	    "movl (%%esi), %%esp\n"

	    /* Return to the PC at the top of new stack. */
	    "ret\n"

	    /* Landing site for jump after return. */
	    /* Restore rbp. */
	    "2: popl %%ebp\n"

	    /* Restore uspace thread pointer. */
	    "popl %%eax\n"
	    "movl %%eax, %%gs:0\n"

	    /*
	     * Indicates to GCC that rdi and rsi are overwritten,
	     * since a register cannot be in the input list and clobber list
	     * at the same time, but overlapping output with input is ok.
	     */
	    : "=D" (dummy1), "=S" (dummy2)

	      /*
	       * Pass destination for stack pointer save in rdi.
	       * Pass source for stack pointer restore in rsi.
	       */
	    : "D" (self ? &self->sp : &dummy_sp), "S" (&other->sp)

	      /*
	       * Clobber all registers except edi/esi and ebp.
	       * This will make GCC preserve the registers that contain any live data.
	       * "cc" indicates flags may change.
	       * "memory" indicates memory can change.
	       */
	    : "cc", "memory", "eax", "ebx", "ecx", "edx");
}

extern void context_trampoline(void);

_NO_TRACE static inline void context_create(context_t *context,
    void (*fn)(void), void *stack_base, size_t stack_size)
{
	context->sp = stack_base + stack_size - 2 * sizeof(uintptr_t);

	/* Trampoline for context_swap() to "return" to. */
	((void **) context->sp)[0] = context_trampoline;
	/* Function to call from the trampoline. */
	((void **) context->sp)[1] = fn;
}

__attribute__((noreturn)) static inline void context_replace(void (*fn)(void),
    void *stack_base, size_t stack_size)
{
	void *sp = stack_base + stack_size - 2 * sizeof(uintptr_t);

	/* Trampoline for context_swap() to "return" to. */
	((void **) sp)[0] = context_trampoline;
	/* Function to call from the trampoline. */
	((void **) sp)[1] = fn;

	asm volatile (
	    "movl %0, %%esp\n"
	    "ret\n"
	    :: "r" (sp));

	unreachable();
}

#endif

/** @}
 */
