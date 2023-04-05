/*
 * Copyright (c) 2016 Martin Decky
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

/** @addtogroup kernel_riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_CONTEXT_H_
#define KERN_riscv64_CONTEXT_H_

#include <arch/context_struct.h>
#include <trace.h>
#include <arch.h>
#include <panic.h>

#define SP_DELTA  16

extern void context_swap(context_t *self, context_t *other);

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
	register uintptr_t pc asm("a0") = (uintptr_t) fn;
	register uintptr_t sp asm("a1") =
	    (uintptr_t) stack_base + stack_size - SP_DELTA;

	asm volatile (
	    "mv gp, zero \n"
	    "mv s0, zero \n"  /* fp */
	    "mv tp, zero \n"
	    "mv ra, zero \n"
	    "mv sp, %[sp] \n"

	    "jr %[pc] \n"

	    :
	    : [pc] "r" (pc), [sp] "r" (sp)
	    : "memory"
	);

	unreachable();
}

#endif

/** @}
 */
