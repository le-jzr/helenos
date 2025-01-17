/*
 * Copyright (c) 2024 Jiří Zárevúcky
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

/** @addtogroup libc
 * @{
 */

#ifndef _LIBC_KOBJ_H_
#define _LIBC_KOBJ_H_

#include <abi/syscall.h>
#include <libc.h>

typedef struct kobject_mem *mem_t;

static inline void sys_kobject_put(void *arg)
{
	__SYSCALL1(SYS_KOBJECT_PUT, (sysarg_t) arg);
}

static inline mem_t sys_mem_create(size_t size, void *template)
{
	return (mem_t) __SYSCALL2(SYS_MEM_CREATE, size, (sysarg_t) template);
}

static inline errno_t sys_mem_map(mem_t mem, void *vaddr, uintptr_t offset, size_t size)
{
	return (errno_t) __SYSCALL4(SYS_MEM_MAP, (sysarg_t) mem, (sysarg_t) vaddr, offset, size);
}

#endif /* _LIBC_KOBJ_H_ */

/** @}
 */
