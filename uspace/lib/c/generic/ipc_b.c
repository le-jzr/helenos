
#include <abi/ipc_b.h>
#include <abi/syscall.h>
#include <ipc_b.h>
#include <libc.h>
#include <stdio.h>
#include <stdlib.h>
#include <libarch/config.h>
#include <align.h>
#include <str.h>

#include "private/fibril.h"

#define __unused __attribute__((unused))

#define DEBUG(fmt, ...) fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__)

struct ipcb_call_in {
	const ipc_message_t *msg;
	ipc_endpoint_t *return_ep;
};

#define _heap_wrap(obj) ({ \
	typeof(obj) *mem = malloc(sizeof(obj)); \
	*mem = (obj); \
	mem; \
})

__unused
static ipc_retval_t _sys_ipc_send(ipc_endpoint_t *ep, const ipc_message_t *m,
	cap_handle_t *return_q, uintptr_t return_tag, int reserves)
{
	panic("unimplemented");
}

__unused
static ipc_retval_t _sys_ipc_receive(cap_handle_t q, ipc_message_t *m)
{
	panic("unimplemented");
}

static cap_handle_t _sys_ipc_queue_create(uintptr_t name, size_t len,
	size_t buffer_size)
{
	panic("unimplemented");
}

__unused
static void _sys_ipc_queue_destroy(cap_handle_t queue)
{
	panic("unimplemented");
}

static inline cap_handle_t _queue_handle(ipc_queue_t *q)
{
	return (cap_handle_t) q;
}

static inline ipc_queue_t *_queue_from_handle(cap_handle_t handle)
{
	return (ipc_queue_t *) handle;
}

// TODO
#if 0

static void _sys_ipc_send_slow(ipc_endpoint_t *ep, const ipc_message_t *m)
{
	ipc_message_t m = *m;
	ipc_endpoint_t *ep;



	/* Normally, this is all done in kernel if it can be done without blocking. */
	for (int i = 0; i < IPC_CALL_LEN; i++) {
		switch (ipc_get_arg_type(&m, i)) {
		case IPC_ARG_TYPE_VAL:
		case IPC_ARG_TYPE_CAP:
			continue;

		/* The argument is an endpoint tag.
		 * A new endpoint with this tag is created and sent.
		 * The sending task doesn't get a capability to the created endpoint.
		 * The endpoint gets 1 or 2 reservations and is invalidated when exhausted.
		 */
		IPC_ARG_TYPE_ENDPOINT_1:
			_ipc_endpoint_reserve
			ep = _ipc_endpoint_create_reserved()


		IPC_ARG_TYPE_ENDPOINT_2:
		/* The argument is a capability. */
		/* The argument is a capability and is automatically dropped on send. */
		IPC_ARG_TYPE_CAP_AUTODROP,
		}

		panic("Bad argument type");
	}
}

/*
 * Does the same thing as _sys_ipc_call(), but ensures the operation cannot
 * fail.
 */
static void _make_call_slow(ipc_endpoint_t *ep, const ipc_message_t *m,
	ipc_queue_t *return_q, uintptr_t return_tag, int reserves)

#endif


static void _make_call(ipc_endpoint_t *ep, const ipc_message_t *m,
	ipc_queue_t *return_q, uintptr_t return_tag, int reserves)
{
	assert(ipc_get_arg(m, 0).val == 0);

	ipc_retval_t rc = _sys_ipc_send(ep, m, _queue_handle(return_q), return_tag, reserves);
	switch (rc) {
	case ipc_success:
		return;

	// TODO: recover from resource exhaustion conditions
	case ipc_e_no_memory:
	case ipc_e_reserve_failed:
	case ipc_e_limit_exceeded:
		panic("Out of space (unimplemented recovery path).");

	case ipc_e_invalid_argument:
		panic("Invalid argument to _make_call().");
	case ipc_e_memory_fault:
		panic("Invalid pointer to _make_call().");

	case ipc_e_timed_out:
	case ipc_e_interrupted_thread:
	case ipc_e_destination_gone:
		/* Fallthrough */
	}

	panic("Unexpected return from SYS_IPC_CALL");
}

static void _reply_on_message(void *self, ipc_message_t *msg)
{
	ipcb_call_t *call = self;

	if (call->response.endpoint_tag != 0) {
		DEBUG("Unexpected extra reply.");
		ipc_message_drop(msg);
		return;
	}

	call->response = *msg;
}

static void _reply_on_destroy(void *self)
{
	ipcb_call_t *call = self;
	fibril_notify(&call->event);
}

static const ipc_endpoint_ops_t return_class = {
	.on_message = _reply_on_message,
	.on_destroy = _reply_on_destroy,
};

static void _set_status_cap(ipcb_call_cancellable_t *call, ipc_message_t *msg)
{
	if (ipc_get_arg_type(msg, 1) != IPC_ARG_TYPE_OBJECT) {
		DEBUG("Received invalid status setup message.");
		ipc_message_drop(msg);
		return;
	}

	auto obj = ipc_get_arg(msg, 1).obj;
	ipc_set_arg(msg, 1, 0);
	ipc_message_drop(msg);

	if (obj == CAP_NIL) {
		DEBUG("Received invalid status setup message.");
		return;
	}

	fibril_mutex_lock(&call->mutex);
	bool assigned = (call->status == CAP_NIL);
	if (assigned)
		call->status = obj;
	fibril_mutex_unlock(&call->mutex);

	if (assigned) {
		fibril_notify(&call->status_initialized);
	} else {
		DEBUG("Received unexpected extra status setup message.");
		ipc_object_put(obj);
	}
}

static void _reply_on_message_cancellable(void *self, ipc_message_t *msg)
{
	ipcb_call_cancellable_t *call = self;

	if (call->call.response.endpoint_tag != 0) {
		DEBUG("Unexpected extra reply.");
		ipc_message_drop(msg);
		return;
	}

	if (msg->flags & IPC_MESSAGE_FLAG_STATUS) {
		_set_status_cap(call, msg);
		return;
	}

	call->call.response = *msg;
}

static void _reply_on_destroy_cancellable(void *self)
{
	ipcb_call_cancellable_t *call = self;
	fibril_notify(&call->call.event);
	fibril_notify(&call->status_initialized);
}

static const ipc_endpoint_ops_t return_class_cancellable = {
	.on_message = _reply_on_message_cancellable,
	.on_destroy = _reply_on_destroy_cancellable,
};


/*
 * Make an uncancellable call.
 */
void ipcb_call_start(ipc_endpoint_t *ep, const ipc_message_t *m, ipcb_call_t *call)
{
	assert(ipc_get_arg(m, 0).val == 0);
	assert(!(m->flags & IPC_MESSAGE_FLAG_PROTOCOL_ERROR));

	*call = (ipcb_call_t) {
		.class = &return_class,
		.event = FIBRIL_EVENT_INIT,
	};

	_make_call(ep, m, NULL, (uintptr_t) call, 1);

	if (m->flags & IPC_MESSAGE_FLAG_OBJECT_DROPPED) {
		//free(ep);
	}
}

ipc_call_result_t ipcb_call_finish(ipcb_call_t *call, ipc_message_t *reply)
{
	fibril_wait_for(&call->event);

	/* If we never got any reply message. */
	if (call->response.endpoint_tag == 0)
		return ipc_call_result_hungup;

	if (call->response.flags & IPC_MESSAGE_FLAG_PROTOCOL_ERROR) {
		assert(reply->flags == IPC_MESSAGE_FLAG_PROTOCOL_ERROR);
		return ipc_call_result_protocol_error;
	}

	call->response.endpoint_tag = 0;
	*reply = call->response;
	return ipc_call_result_success;
}

ipc_call_result_t ipcb_call(ipc_endpoint_t *ep, const ipc_message_t *m, ipc_message_t *reply)
{
	ipcb_call_t call;
	ipcb_call_start(ep, m, &call);
	return ipcb_call_finish(&call, reply);
}

/*
 *
 */
void ipcb_call_start_cancellable(ipc_endpoint_t *ep,
	ipc_message_t *m, ipcb_call_cancellable_t *call)
{
	assert(ipc_get_arg(m, 0).val == 0);
	assert(!(m->flags & IPC_MESSAGE_FLAG_PROTOCOL_ERROR));

	m->flags |= IPC_MESSAGE_FLAG_STATUS;

	*call = (ipcb_call_cancellable_t) {
		.call = {
			.class = &return_class_cancellable,
			.event = FIBRIL_EVENT_INIT,
		},
	};

	_make_call(ep, m, NULL, (uintptr_t) call, 2);

	if (m->flags & IPC_MESSAGE_FLAG_OBJECT_DROPPED) {
		//free(ep);
	}
}

/*
 * Signals cancellation to the call's recipient.
 */
void ipcb_call_cancel(ipcb_call_cancellable_t *call)
{
	panic("unimplemented");
}

ipc_queue_t *ipc_queue_create(const char *name, size_t buffer_size)
{
	assert(buffer_size >= PAGE_SIZE);
	assert(buffer_size % PAGE_SIZE == 0);

	cap_handle_t handle =
		_sys_ipc_queue_create((uintptr_t) name, str_size(name), buffer_size);

	// TODO: handle memory exhaustion

	if (handle == CAP_NIL)
		panic("out of memory");

	return _queue_from_handle(handle);
}

void ipc_queue_destroy(ipc_queue_t *q)
{
	if (!q)
		return;

	_sys_ipc_queue_destroy(_queue_handle(q));
	free(q);
}

/**
 * @param epdata  Must start with an `ipc_endpoint_ops_t *` field.
 */
ipc_endpoint_t *ipc_endpoint_create(ipc_queue_t *q, void *epdata)
{
    auto ep = __SYSCALL2(SYS_IPCB_ENDPOINT_CREATE, (sysarg_t) q, (sysarg_t) epdata);
    if (ep == 0)
        panic("unimplemented");

    return (ipc_endpoint_t *) ep;
}

void ipc_endpoint_put(ipc_endpoint_t *ep)
{
	panic("unimplemented");
}

void ipcb_answer(const ipc_message_t *call, const ipc_message_t *msg)
{
	panic("unimplemented");
}

void ipcb_answer_protocol_error(const ipc_message_t *call)
{
	ipc_message_t msg = {
		.flags = IPC_MESSAGE_FLAG_PROTOCOL_ERROR |
			IPC_MESSAGE_FLAG_OBJECT_DROPPED,
	};

	ipcb_answer(call, &msg);
}

void ipcb_set_cancel_handler(const ipc_message_t *call, void *handler)
{
	panic("unimplemented");
}

void ipc_message_drop(const ipc_message_t *msg)
{
	for (int i = 0; i < IPC_CALL_LEN; i++) {
		if (ipc_get_arg_type(msg, i) != IPC_ARG_TYPE_OBJECT) {
			assert(ipc_get_arg_type(msg, i) == IPC_ARG_TYPE_VAL);
			continue;
		}

		auto obj = ipc_get_arg(msg, i).obj;
		if (obj != NULL)
			ipc_object_put(obj);
	}
}

typedef struct {
	const ipc_endpoint_ops_t *class;
} _return_endpoint_t;

static inline const ipc_endpoint_ops_t *_class_from_ep_tag(uintptr_t tag)
{
	assert(tag != 0);
	return *(const ipc_endpoint_ops_t **) (void *) tag;
}

void ipcb_handle_messages(ipc_queue_t *q, const struct timespec *expires)
{
	// TODO: timeouts

	ipc_message_t msg;
	ipc_retval_t rc = _sys_ipc_receive(_queue_handle(q), &msg);

	do {
		switch (rc) {
		case ipc_success:
			continue;
		case ipc_e_timed_out:
			return;
		case ipc_e_invalid_argument:
			panic("Invalid argument to _sys_ipc_receive()");
		case ipc_e_memory_fault:
			panic("Invalid buffer address to _sys_ipc_receive()");
		case ipc_e_no_memory:
			panic("TODO: OOM handling");
		case ipc_e_limit_exceeded:
		case ipc_e_interrupted_thread:
		case ipc_e_reserve_failed:
		case ipc_e_destination_gone:
			break;
		}

		panic("unexpected retval from _sys_ipc_receive()");
	} while (false);

	auto tag = msg.endpoint_tag;

	if (msg.flags & IPC_MESSAGE_FLAG_AUTOMATIC_MESSAGE) {
		if (msg.flags & IPC_MESSAGE_FLAG_OBJECT_DROPPED) {
			_class_from_ep_tag(tag)->on_destroy((void *) tag);
			return;
		}

		panic("TODO");
	}

	bool dropped = (msg.flags & IPC_MESSAGE_FLAG_OBJECT_DROPPED);
	msg.flags &= ~IPC_MESSAGE_FLAG_OBJECT_DROPPED;

	auto class = _class_from_ep_tag(tag);
	class->on_message((void *) tag, &msg);

	if (dropped)
		class->on_destroy((void *) tag);
}

void ipc_call_long_1(const ipc_endpoint_t *ep,
	ipc_message_t *reply, sysarg_t arg1, const void *data, size_t data_len)
{
	panic("unimplemented");
}

void ipc_object_put(ipc_object_t *obj)
{
    panic("unimplemented");
}

ipc_blob_t *ipc_blob_create(const void *src, size_t src_len)
{
    panic("unimplemented");
}
