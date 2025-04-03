/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

#ifndef KERN_IPC_B_H_
#define KERN_IPC_B_H_

#include <abi/ipc_b.h>

#include <cap/cap.h>
#include <errno.h>
#include <ipc/ipc.h>
#include <time/timeout.h>

typedef struct ipc_blob ipc_blob_t;
typedef struct ipc_queue ipc_queue_t;
typedef struct ipc_endpoint ipc_endpoint_t;

extern kobject_ops_t ipc_blob_kobject_ops;
extern kobject_ops_t ipc_queue_kobject_ops;
extern kobject_ops_t ipc_endpoint_kobject_ops;

void weakref_init(void);
void ipc_blob_init(void);
void ipc_queue_init(void);

typedef struct weakref weakref_t;
weakref_t *weakref_create(void *inner);
weakref_t *weakref_ref(weakref_t *ref);
void weakref_put(weakref_t *ref);
void *weakref_hold(weakref_t *ref);
void weakref_release(weakref_t *ref);
void weakref_destroy(weakref_t *ref);

ipc_blob_t *ipc_blob_create(uspace_addr_t, sysarg_t);
sysarg_t sys_blob_create(uspace_addr_t, sysarg_t);
sys_errno_t sys_blob_read(cap_handle_t, sysarg_t, sysarg_t, uspace_addr_t);
sys_errno_t sys_blob_destroy(cap_handle_t);

ipc_queue_t *ipc_queue_create(size_t);
ipc_retval_t ipc_queue_reserve(ipc_queue_t *, size_t);
ipc_retval_t ipc_queue_read(ipc_queue_t *q,
    uspace_addr_t uspace_buffer, size_t *uspace_buffer_size,
	size_t *reservations_granted, int timeout_usec);

ipc_endpoint_t *ipc_endpoint_create(ipc_queue_t *q, uintptr_t tag, int reserves);

sys_errno_t sys_ipc_endpoint_create(sysarg_t queue_handle, sysarg_t tag,
	uspace_addr_t out_endpoint_handle);

#endif

/** @}
 */
