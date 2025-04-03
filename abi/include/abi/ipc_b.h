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

#ifndef _ABI_IPC_B_H_
#define _ABI_IPC_B_H_

#include <assert.h>
#include <panic.h>
#include <stdint.h>

#define IPC_MESSAGE_ARGS 6
#define IPC_BLOB_SIZE_LIMIT 65536

/* 'ipc_object_t *' is the type of userspace capability handles. */
typedef struct ipc_object ipc_object_t;

typedef enum ipc_arg_type {
	IPC_ARG_TYPE_NONE,
	/* Just a plain integer. */
	IPC_ARG_TYPE_VAL,
	/* The argument is an endpoint tag.
	 * A new endpoint with this tag is created and sent.
	 * The sending task doesn't get a capability to the created endpoint.
	 */
	IPC_ARG_TYPE_ENDPOINT_1,
	IPC_ARG_TYPE_ENDPOINT_2,
	/* The argument is a capability. */
	IPC_ARG_TYPE_OBJECT,
	/* The argument is a capability and is automatically dropped on send. */
	IPC_ARG_TYPE_OBJECT_AUTODROP,

#ifdef KERNEL
	/* Only for kernel. */
	IPC_ARG_TYPE_KOBJECT,
#endif
} ipc_arg_type_t;

/* IPC message flags */
enum {
	//  |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 |
	//  |  ARG0_TYPE        |  ARG1_TYPE        |  ARG2_TYPE        |
	//
	//  | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 |
	//  |  ARG3_TYPE        |  ARG4_TYPE        |  ARG5_TYPE        |
	//
	//  | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
	//  | PE | RR | OD | CC | AM |

	/*
	 * Set in a reply message if the server doesn't recognize the message
	 * it's replying to.
	 */
	IPC_MESSAGE_FLAG_PROTOCOL_ERROR = 1 << 24,

	/*
	 * Automatic message sent to itself when a requested reservation
	 * is available but there's no pending message in the queue.
	 */
	IPC_MESSAGE_FLAG_RESERVATION_RELEASED = 1 << 25,

	/*
	 * Automatic message sent to the endpoint's owner when all references
	 * to it are destroyed.
	 */
	IPC_MESSAGE_FLAG_OBJECT_DROPPED = 1 << 26,

	/*
	 * Flag set in initial message to signal caller wants a status endpoint.
	 * Also set in a reply message if it carries a status endpoint instead
	 * of final result. The caller may signal desire to cancel by dropping
	 * this endpoint, or make calls on it to ask for status information if
	 * the callee's protocol supports it.
	 */
	IPC_MESSAGE_FLAG_STATUS = 1 << 27,

	/*
	 * Set whenever the message is synthetic and not explicitly sent by
	 * any task. Can be combined with IPC_MESSAGE_FLAG_RESERVATION_RELEASED
	 * or IPC_MESSAGE_FLAG_OBJECT_DROPPED.
	 */
	IPC_MESSAGE_FLAG_AUTOMATIC_MESSAGE = 1 << 28,
};

static inline uintptr_t ipc_message_flags_2(uintptr_t flags, ipc_arg_type_t type0, ipc_arg_type_t type1)
{
	return flags | type0 | (type1 << 4);
}

typedef enum ipc_retval {
	ipc_success,
	ipc_reserve_pending,

	ipc_e_timed_out,
	ipc_e_no_memory,
	ipc_e_limit_exceeded,
	ipc_e_interrupted_thread,
	ipc_e_invalid_argument,
	ipc_e_memory_fault,
	ipc_e_reserve_failed,
} ipc_retval_t;

typedef union ipc_arg {
	uintptr_t val;
	ipc_object_t *obj;
	void *ptr;
} ipc_arg_t __attribute__((__transparent_union__));

/*
 * Raw IPC message data.
 */
typedef struct ipc_message {
	uintptr_t endpoint_tag;
	uintptr_t flags;
	ipc_arg_t args[IPC_MESSAGE_ARGS];
} ipc_message_t;

static inline ipc_arg_type_t ipc_get_arg_type(const ipc_message_t *m, int arg)
{
	assert(arg >= 0 && arg < IPC_MESSAGE_ARGS);
	return (m->flags >> (arg << 2)) & 0xf;
}

static inline ipc_arg_t ipc_get_arg(const ipc_message_t *m, int arg)
{
	assert(arg >= 0 && arg < IPC_MESSAGE_ARGS);
	return m->args[arg];
}

static inline void ipc_set_arg(ipc_message_t *m, int arg,
	ipc_arg_t val, ipc_arg_type_t type)
{
	assert((type & 0xf) == type);

	m->flags &= ~(0xf << (arg << 2));
	m->flags |= type << (arg << 2);
	m->args[arg] = val;
}

static inline void __ipc_message_prepend(ipc_message_t *msg, ipc_arg_t arg, ipc_arg_type_t type)
{
    panic("unimplemented");
}

#endif

/** @}
 */
