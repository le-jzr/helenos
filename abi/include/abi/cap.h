/*
 * Copyright (c) 2017 Jakub Jermar
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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_CAP_H_
#define _ABI_CAP_H_

#include <stdbool.h>
#include <stdint.h>

typedef void *cap_handle_t;

typedef struct {
} *cap_call_handle_t;

typedef struct {
} *cap_phone_handle_t;

typedef struct {
} *cap_irq_handle_t;

typedef struct {
} *cap_waitq_handle_t;

typedef struct {
} *cap_mem_handle_t;

typedef struct {
} *cap_data_handle_t;

typedef struct {
} *cap_endpoint_handle_t;

typedef struct {
} *cap_buffer_handle_t;

static cap_handle_t const CAP_NIL = 0;

static inline bool cap_handle_valid(cap_handle_t handle)
{
	return handle != CAP_NIL;
}

static inline intptr_t cap_handle_raw(cap_handle_t handle)
{
	return (intptr_t) handle;
}

#endif

/** @}
 */
