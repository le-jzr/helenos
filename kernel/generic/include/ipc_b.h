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

#include <errno.h>
#include <time/timeout.h>
#include <cap/cap.h>

#define IPC_BLOB_SIZE_LIMIT 65536

typedef struct ipc_blob ipc_blob_t;
typedef struct ipc_buffer ipc_buffer_t;
typedef struct ipc_endpoint ipc_endpoint_t;

extern kobject_ops_t ipc_blob_kobject_ops;
extern kobject_ops_t ipc_buffer_kobject_ops;
extern kobject_ops_t ipc_endpoint_kobject_ops;

typedef struct ipc_write_data {
	uintptr_t *handles;
	size_t handles_len;

	// The mandatory part of write.
	// A successful write will have written at least the handles and data1.
	uintptr_t data1;
	size_t data1_len;

	// The optional part of write.
	// A successful write will have written only as much of data2 as could fit
	// into the buffer (possibly even 0 bytes).
	// data1 and data2 do not have to be adjacent in memory.
	uintptr_t data2;
	size_t data2_len;

	deadline_t deadline;
} ipc_write_data_t;

void ipc_blob_init(void);

ipc_blob_t *ipc_blob_create(uspace_addr_t, sysarg_t);
sysarg_t sys_blob_create(uspace_addr_t, sysarg_t);
sys_errno_t sys_blob_read(cap_handle_t, sysarg_t, sysarg_t, uspace_addr_t);
sys_errno_t sys_blob_destroy(cap_handle_t);

void ipc_buffer_initialize(void);

errno_t ipc_buffer_read(ipc_buffer_t *, uintptr_t *, deadline_t);
void ipc_buffer_end_read(ipc_buffer_t *);
ipc_buffer_t *ipc_buffer_create(size_t, size_t);

ipc_endpoint_t *ipc_endpoint_create(ipc_buffer_t *buffer, uintptr_t userdata,
		size_t reserve, size_t max_message_len);
errno_t ipc_endpoint_write(ipc_endpoint_t *, const ipc_write_data_t *,
		size_t *, deadline_t);

#endif

/** @}
 */
