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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_B_H_
#define _LIBC_IPC_B_H_

#include <abi/ipc_b.h>
#include <stddef.h>
#include <adt/list.h>
#include <fibril.h>
#include <fibril_synch.h>

typedef struct ipcb_blob ipcb_blob_t;
typedef struct ipcb_endpoint ipcb_endpoint_t;
typedef struct ipcb_queue ipcb_queue_t;

ipcb_blob_t *ipcb_blob_create(const void *src, size_t len);
size_t ipcb_blob_get_len(const ipcb_blob_t *blob);
size_t ipcb_blob_read(const ipcb_blob_t *blob, void *dst, size_t len, size_t offset);
void ipcb_blob_put(ipcb_blob_t *blob);
void ipcb_blob_destroy(ipcb_blob_t *blob);

ipcb_queue_t *ipcb_queue_create(const char *name, size_t buffer_size);
void ipcb_queue_destroy(ipcb_queue_t *q);

typedef struct ipc_endpoint_class {
	void (*on_message)(void *self, ipc_message_t *msg);
	void (*on_destroy)(void *self);
} ipc_endpoint_class_t;

ipcb_endpoint_t *ipcb_endpoint_create(ipcb_queue_t *q,
	ipc_endpoint_class_t **data);
void ipcb_endpoint_put(ipcb_endpoint_t *ep);

typedef struct ipcb_call {
	const ipc_endpoint_class_t *class;
	fibril_event_t event;
	ipc_message_t response;
} ipcb_call_t;

typedef struct ipcb_call_cancellable {
	ipcb_call_t call;
	fibril_mutex_t mutex;
	cap_handle_t status;
	fibril_event_t status_initialized;
} ipcb_call_cancellable_t;

static inline ipc_message_t *ipcb_call_response(ipcb_call_t *call)
{
	return &call->response;
}

typedef enum ipc_call_result {
	ipc_call_result_success,

	/* Server didn't understand the message. */
	ipc_call_result_protocol_error,

	/* Server dropped the return endpoint or died before answering. */
	ipc_call_result_hungup,
} ipc_call_result_t;

[[gnu::access(write_only, 3)]]
void ipcb_call_start(ipcb_endpoint_t *ep, const ipc_message_t *m, ipcb_call_t *call);
[[gnu::access(write_only, 3)]]
void ipcb_call_start_cancellable(ipcb_endpoint_t *ep, ipc_message_t *m, ipcb_call_cancellable_t *call);
[[gnu::access(write_only, 2)]]
ipc_call_result_t ipcb_call_finish(ipcb_call_t *call, ipc_message_t *reply);
void ipcb_call_cancel(ipcb_call_cancellable_t *call);

[[gnu::access(write_only, 3)]]
ipc_call_result_t ipcb_call(ipcb_endpoint_t *ep, const ipc_message_t *m, ipc_message_t *reply);

void ipcb_answer(const ipc_message_t *call, const ipc_message_t *answer);
void ipcb_answer_protocol_error(const ipc_message_t *call);
void ipcb_set_cancel_handler(const ipc_message_t *call, void *handler);

void ipc_message_drop(const ipc_message_t *msg);

void cap_drop(cap_handle_t);

void ipcb_handle_messages(ipcb_queue_t *q, const struct timespec *expires);

#endif

/** @}
 */
