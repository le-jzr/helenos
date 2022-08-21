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

#define KOBJ_NULL ((kobj_handle_t) NULL)
#define TASK_NULL ((task_handle_t) NULL)
#define MEM_NULL ((mem_handle_t) NULL)

#define MEM_MAP_FAILED ((uintptr_t) -1)

typedef struct kobj_handle *kobj_handle_t;
typedef struct task_handle *task_handle_t;
typedef struct mem_handle *mem_handle_t;

#define unimplemented() { assert(!"unimplemented"); abort(); }

static inline _Noreturn void panic(const char *str)
{
	printf("panic: %s\n", str);
	abort();
}

static inline mem_handle_t sys_mem_create(size_t size, int flags)
{
	unimplemented();
}

static inline errno_t sys_mem_change_flags(mem_handle_t mem, int flags)
{
	unimplemented();
}

static inline task_handle_t sys_task_self(void)
{
	unimplemented();
}

static inline task_handle_t sys_task_create(const char *name)
{
	unimplemented();
}

static inline uintptr_t sys_task_mem_map(task_handle_t task, mem_handle_t mem, size_t offset, size_t size, uintptr_t vaddr, int flags)
{
	unimplemented();
}

static inline void *sys_mem_map(mem_handle_t mem, size_t offset, size_t size, void *vaddr, int flags)
{
	unimplemented();
}

static inline errno_t sys_mem_remap(void *vaddr, size_t size, int flags)
{
	unimplemented();
}

static inline void sys_mem_unmap(void *vaddr, size_t size)
{
	unimplemented();
}

static inline void sys_kobj_put(void *kobj)
{
	unimplemented();
}

static inline errno_t sys_task_mem_set(task_handle_t task, uintptr_t dst, int byte, size_t size)
{
	unimplemented();
}

errno_t elf_load_file2(const char *name, int file, task_handle_t *out_task);
errno_t elf_load_file_name2(const char *path, task_handle_t *out_task);

#endif

/** @}
 */
