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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_MEM_H_
#define KERN_MEM_H_

#include <stddef.h>
#include <stdint.h>
#include <cc.h>

#ifdef CONFIG_LTO
#define DO_NOT_DISCARD ATTRIBUTE_USED
#else
#define DO_NOT_DISCARD
#endif

#define memset(dst, val, cnt)  __builtin_memset((dst), (val), (cnt))
#define memcpy(dst, src, cnt)  __builtin_memcpy((dst), (src), (cnt))
#define memcmp(s1, s2, cnt)    __builtin_memcmp((s1), (s2), (cnt))

extern void memsetb(void *, size_t, uint8_t)
    __attribute__((nonnull(1)));
extern void memsetw(void *, size_t, uint16_t)
    __attribute__((nonnull(1)));
extern void *memmove(void *, const void *, size_t)
    __attribute__((nonnull(1, 2))) DO_NOT_DISCARD;

extern void *mem_alloc(size_t alignment, size_t size) __attribute__((malloc));
extern void *mem_realloc(void *old_ptr, size_t alignment, size_t old_size,
    size_t new_size);
extern void mem_free(void *ptr, size_t alignment, size_t size);

#define make(type) \
	((type *) mem_alloc(_Alignof(type), sizeof(type)))

#define make_array(type, items) \
	((type *) mem_alloc(_Alignof(type), sizeof(type) * (items)))

#define resize_array(ptr, old_items, new_items) \
	((typeof(*(ptr)) *) mem_realloc(ptr, alignof(*(ptr)), \
	    (old_items) * sizeof(*(ptr)), \
	    (new_items) * sizeof(*(ptr))))

#define delete(ptr) \
	mem_free((ptr), _Alignof(*(ptr)), sizeof(*(ptr)))

#define delete_array(ptr, items) \
	mem_free((ptr), _Alignof(*(ptr)), sizeof(*(ptr)) * (items))

#endif

/** @}
 */
