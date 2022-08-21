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

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_MM_MEM_H_
#define KERN_MM_MEM_H_

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <kobj.h>

// Always 64b regardless of architecture for simplicity.
typedef uint64_t phys_addr_t;
typedef struct mem mem_t;

extern const kobj_class_t kobj_class_mem;
#define KOBJ_CLASS_MEM (&kobj_class_mem)

void mem_init(void);

mem_t *mem_create(uint64_t size, size_t page_size, int flags);
void mem_put(mem_t *);

errno_t mem_change_flags(mem_t *, int);
bool mem_flags_valid(int);
phys_addr_t mem_lookup(mem_t *mem, uint64_t offset, bool alloc);
uintptr_t mem_read_word(mem_t *mem, uint64_t offset);
errno_t mem_write_from_uspace(mem_t *mem, uint64_t offset, uintptr_t src, size_t size);

uint64_t mem_size(mem_t *);
int mem_flags(mem_t *);

void mem_range_ref(uint64_t offset, size_t size, int flags);
void mem_range_unref(uint64_t offset, size_t size, int flags);

#endif

/** @}
 */
