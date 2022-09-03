/*
 * Copyright (c) 2007 Michal Kebrt
 * Copyright (c) 2007 Pavel Jancik
 * Copyright (c) 2022 Jiří Zárevúcky
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

#include "ras_page.h"
#include "../../../generic/private/cc.h"

/* This definition of ras_page will preempt the definition in shared libc,
 * ensuring that a value can be written here before dynamic relocations are
 * processed. Dynamic relocations are still necessary for code in libc
 * (namely, implementations of atomics) to be able to access this variable.
 *
 * Since this is always linked into the main executable, this is always
 * the definition dynamically linked to references in libc.
 */
PROTECTED volatile unsigned *__libc_arch_ras_page;

/* User-space task entry point */
extern _Noreturn void _start(void);
/* Architecture-generic entry point. */
extern _Noreturn void __c_start(void *);

__attribute__((used))
static void c_start(void *pcb, void *ras)
{
	/* We do this in C rather than assembly to make sure we address the variable correctly.
	 * There is only one correct way to address the variable of course, since it's PROTECTED
	 * and defined in this file, but if something does break, it will hopefully be more obvious
	 * this way than the various silent or obscure ways it can be broken when the reference
	 * is hardcoded in assembly.
	 */
	__libc_arch_ras_page = ras;

	__c_start(pcb);
}

/* Naked function: GCC only generates the symbol itself,
 * no prologue/epilogue assembly code is produced by the compiler.
 */
__attribute__((naked))
void _start(void)
{
	asm volatile (
		/* Get PCB pointer from stack. */
		"sub sp, sp, #4 \n"
		"pop {r0} \n"

		/* RAS address is in r2 */
		"mov r1, r2 \n"

		/* Create the first stack frame. */
		"mov fp, #0 \n"
		"mov ip, sp \n"
		"push {fp, ip, lr, pc} \n"
		"sub fp, ip, #4 \n"

		"bl c_start \n"
	);
}
