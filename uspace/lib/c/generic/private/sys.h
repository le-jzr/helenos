/*
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_SYS_H_
#define _LIBC_PRIVATE_SYS_H_

#include <assert.h>
#include <stdlib.h>
#include <abi/syscall.h>
#include <libc.h>
#include <stdint.h>
#include <stdio.h>
#include <abi/mm/as.h>
#include <errno.h>
#include <abi/proc/task.h>
#include <string.h>
#include <task.h>

#define KOBJ_NULL NULL
#define MEM_NULL ((mem_handle_t) NULL)

typedef void *kobj_handle_t;
typedef struct mem_handle *mem_handle_t;

static inline _Noreturn void panic(const char *str)
{
	printf("panic: %s\n", str);
	abort();
}

static inline mem_handle_t sys_mem_create(size_t size, size_t align, int flags)
{
	return (mem_handle_t) __SYSCALL3(SYS_MEM_CREATE, size, align, flags);
}

static inline errno_t sys_mem_change_flags(mem_handle_t mem, int flags)
{
	return (errno_t) __SYSCALL2(SYS_MEM_CHANGE_FLAGS, (sysarg_t) mem, flags);
}

static inline task_handle_t sys_task_self(void)
{
	return (task_handle_t) __SYSCALL0(SYS_TASK_SELF);
}

static inline task_handle_t sys_task_create(const char *name)
{
	return (task_handle_t) __SYSCALL2(SYS_TASK_CREATE, (sysarg_t) name, strlen(name));
}

static inline errno_t sys_task_mem_map(task_handle_t task, mem_handle_t mem, size_t offset, size_t size, uintptr_t *vaddr, int flags)
{
	return (errno_t) __SYSCALL6(SYS_TASK_MEM_MAP, (sysarg_t) task, (sysarg_t) mem, offset, size, (sysarg_t) vaddr, flags);
}

static inline errno_t sys_task_mem_remap(task_handle_t task, uintptr_t vaddr, size_t size, int flags)
{
	return (errno_t) __SYSCALL4(SYS_TASK_MEM_REMAP, (sysarg_t) task, (sysarg_t) vaddr, size, flags);
}

static inline errno_t sys_task_mem_unmap(task_handle_t task, uintptr_t vaddr, size_t size)
{
	return (errno_t) __SYSCALL3(SYS_TASK_MEM_UNMAP, (sysarg_t) task, (sysarg_t) vaddr, size);
}

static inline errno_t sys_task_connect(task_handle_t task, cap_phone_handle_t *phone)
{
	return (errno_t) __SYSCALL2(SYS_TASK_CONNECT, (sysarg_t) task, (sysarg_t) phone);
}

static inline void *sys_mem_map(mem_handle_t mem, size_t offset, size_t size, void *vaddr, int flags)
{
	uintptr_t addr = (uintptr_t) vaddr;
	errno_t rc = sys_task_mem_map(0, mem, offset, size, &addr, flags);
	return rc == EOK ? (void *) addr : NULL;
}

static inline errno_t sys_mem_remap(void *vaddr, size_t size, int flags)
{
	return sys_task_mem_remap(0, (uintptr_t) vaddr, size, flags);
}

static inline void sys_mem_unmap(void *vaddr, size_t size)
{
	sys_task_mem_unmap(0, (uintptr_t) vaddr, size);
}

static inline void sys_kobj_put(void *kobj)
{
	errno_t rc = (errno_t) __SYSCALL1(SYS_KOBJ_PUT, (sysarg_t) kobj);
	assert(rc == EOK);
}

static inline errno_t sys_task_mem_set(task_handle_t task, uintptr_t dst, int byte, size_t size)
{
	return (errno_t) __SYSCALL4(SYS_TASK_MEM_SET, (sysarg_t) task, dst, byte, size);
}

static inline errno_t sys_task_mem_write(task_handle_t task, uintptr_t dst, const void *src, size_t size)
{
	return (errno_t) __SYSCALL4(SYS_TASK_MEM_WRITE, (sysarg_t) task, dst, (sysarg_t) src, size);
}

static inline errno_t sys_task_thread_start(task_handle_t task, const char *name, uintptr_t pc, uintptr_t stack_base, uintptr_t stack_size)
{
	return (errno_t) __SYSCALL6(SYS_TASK_THREAD_START, (sysarg_t) task, (sysarg_t) name, strlen(name), pc, stack_base, stack_size);
}

static inline task_id_t sys_task_get_id_2(task_handle_t task)
{
	task_id_t tid = 0;
	__SYSCALL2(SYS_TASK_GET_ID_2, (sysarg_t) task, (sysarg_t) &tid);
	return tid;
}

static inline errno_t sys_task_wait(task_handle_t task, int *status)
{
	return (errno_t) __SYSCALL2(SYS_TASK_WAIT, (sysarg_t) task, (sysarg_t) status);
}

static inline uintptr_t sys_vaddr_limit(void)
{
	// TODO: actually get the correct value from somewhere
	// Currently, we use the lowest common denominator out of blatant laziness.
	return ((uintptr_t) INT32_MAX) + 1;
}

errno_t elf_load_file2(const char *name, const char *cwd, const char *const args[], int file, task_handle_t *out_task, int, int, int);
errno_t elf_load_file_name2(const char *path, const char *cwd, const char *const args[], task_handle_t *out_task, int, int, int);

#endif

/** @}
 */
