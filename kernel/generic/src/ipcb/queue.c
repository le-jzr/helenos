#include "_bits/errno.h"
#include "abi/cap.h"
#include "abi/ipc_b.h"
#include "adt/list.h"
#include "cap/cap.h"
#include "mm/slab.h"
#include <ipc_b.h>

#include <stdatomic.h>
#include <stdalign.h>

#include <align.h>
#include <synch/spinlock.h>
#include <mem.h>
#include <proc/task.h>
#include <str_error.h>
#include <syscall/copy.h>

#define DEBUG(f, ...) printf("IPC(" __FILE__ ":%d) " f, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define unimplemented() panic("unimplemented")

/*
 * Ideal outcome:
 *  - perfect asynchronicity, meaning that the ONLY way to block waiting for
 *    other task's action should be ipc_queue_read().
 *  - Ability to time-out/cancel an asynchronous action, with any residual
 *    resource cost being the responsibility of the slow party. At the same
 *    time, it should be impossible for a bad task to crash a server via
 *    resource exhaustion.
 *    Separate time-out facility is probably unnecessary, we can mimic it
 *    via cancellation.
 *    Cancellation means we probably need request-response protocol integrated
 *    kernel-side, rather than simple unidirectional message passing.
 */

/*
 * Linked message structure used in the pending and free lists.
 * Allocated and freed with the queue.
 */
typedef struct  {
	link_t link;
	ipc_message_t data;
} ipc_linked_message_t;

/*
 * Dynamically allocated message structure that is used to send a message when
 * the destination queue's buffer is full. The structure is emptied and
 * returned to sender as soon as space frees in the queue.
 */
typedef struct {
	link_t link;
	ipc_message_t data;
	weakref_t *parent_queue;
} ipc_dynamic_message_t;

enum {
	/* Initial number of free structures in buffer, and also the maximum.
	 * More can be dynamically allocated when they are being used, but they
	 * won't be kept around free over this number.
	 * Not too big, since most of that will be laying unused most of the time,
	 * and the size of the buffer will only matter during oom conditions when
	 * dynamically allocating more will not be possible, in which case we
	 * absolutely don't want a ton of unused memory lying around.
	 */
	IPC_DYNAMIC_MESSAGE_BUFFER_DEFAULT_SIZE = 8,
};

typedef struct ipc_queue {
	/* Keep first. */
	kobject_t kobject;

	weakref_t *self_wref;

	/* Synchronizes just the fields immediately after. */
	irq_spinlock_t free_dynamic_lock;
	list_t free_dynamic;
	size_t free_dynamic_count;
	list_t reserve_dynamic;
	size_t reserve_dynamic_count;
	size_t reserve_dynamic_requested;

	/* Synchronizes everything below. */
	irq_spinlock_t lock;

	list_t pending_dynamic;
	list_t pending;
	list_t free;

	list_t pages;

	size_t free_count;
	size_t reserved;
	size_t reserve_unclaimed;
	size_t reserve_requested;

	/* Tied to length of the pending list. */
	waitq_t reader_waitq;
} ipc_queue_t;

struct ipc_endpoint {
	/* Keep first. */
	kobject_t kobject;

	uintptr_t tag;
	weakref_t *queue_ref;
};

static slab_cache_t *slab_ipc_queue_cache;
static slab_cache_t *slab_ipc_endpoint_cache;
static slab_cache_t *slab_page_cache;
static slab_cache_t *slab_ipc_dynamic_message_cache;

static void _dynamic_message_free(ipc_dynamic_message_t *dyn)
{
	ipc_queue_t *q = weakref_hold(dyn->parent_queue);
	if (!q) {
		/* The parent queue no longer exists. */
		weakref_put(dyn->parent_queue);
		slab_free(slab_ipc_dynamic_message_cache, dyn);
		return;
	}

	irq_spinlock_lock(&q->free_dynamic_lock, true);

	bool overbudget =
		q->free_dynamic_count >= IPC_DYNAMIC_MESSAGE_BUFFER_DEFAULT_SIZE;

	if (overbudget) {
	    assert(q->reserve_dynamic_requested == 0);
	} else {
	    /* Keep a few free ones cached locally. */

		if (q->reserve_dynamic_requested > 0) {
			assert(q->free_dynamic_count == 0);
			q->reserve_dynamic_requested--;

			q->reserve_dynamic_count++;
			list_append(&dyn->link, &q->reserve_dynamic);
		} else {
			q->free_dynamic_count++;
			list_append(&dyn->link, &q->free_dynamic);
		}
	}

	irq_spinlock_unlock(&q->free_dynamic_lock, true);
	weakref_release(dyn->parent_queue);

	if (overbudget) {
		weakref_put(dyn->parent_queue);
		slab_free(slab_ipc_dynamic_message_cache, dyn);
	}
}

/**
 * Free a message buffer that belongs to this queue.
 * If some pending reservations were fulfilled, *reservations_granted is increased.
 */
static void _release_message_buffer(ipc_queue_t *q, ipc_linked_message_t *m,
	size_t *reservations_granted)
{
	irq_spinlock_lock(&q->lock, true);

	/*
     * If there are pending dynamically allocated message buffers,
     * the released buffers are first used to free them.
     */

	ipc_dynamic_message_t *dyn = list_pop(&q->pending_dynamic,
		ipc_dynamic_message_t, link);

	if (dyn) {
		irq_spinlock_unlock(&q->lock, true);

		m->data = dyn->data;
		_dynamic_message_free(dyn);

		irq_spinlock_lock(&q->lock, true);

		list_append(&m->link, &q->pending);
	} else {
		list_append(&m->link, &q->free);

		if (q->reserve_requested > 0) {
			q->reserve_requested--;
			q->reserve_unclaimed++;
			(*reservations_granted)++;
		} else {
			q->free_count++;
		}
	}

	irq_spinlock_unlock(&q->lock, true);
}

void ipc_queue_init(void)
{
	slab_ipc_queue_cache = slab_cache_create("ipc_queue_t",
		sizeof(ipc_queue_t), alignof(ipc_queue_t), NULL, NULL, 0);
	slab_ipc_endpoint_cache = slab_cache_create("ipc_endpoint_t",
		sizeof(ipc_endpoint_t), alignof(ipc_endpoint_t), NULL, NULL, 0);
	slab_page_cache = slab_cache_create("ipc_queue_t::page",
		PAGE_SIZE, 0, NULL, NULL, 0);
	slab_ipc_dynamic_message_cache = slab_cache_create("ipc_dynamic_message_t",
		sizeof(ipc_dynamic_message_t), alignof(ipc_dynamic_message_t),
		NULL, NULL, 0);
}

typedef struct __attribute__((may_alias)) {
	char data[PAGE_SIZE - sizeof(link_t)];
	link_t link;
} _dummy_page_t;

_Static_assert(sizeof(_dummy_page_t) == PAGE_SIZE);

static link_t *_page_link(void *page)
{
	return &((_dummy_page_t *) page)->link;
}

static void _insert_page(ipc_queue_t *q, void *page)
{
	list_append(_page_link(page), &q->pages);

	assert(page < (void *) _page_link(page));
	size_t page_size = (void *) _page_link(page) - page;
	assert(page_size > sizeof(ipc_linked_message_t));
	assert(page_size < PAGE_SIZE);

	int n = page_size / sizeof(ipc_linked_message_t);
	ipc_linked_message_t *buckets = page;

	for (int i = 0; i < n; i++)
		list_append(&buckets[i].link, &q->free);

	q->free_count += n;
}

static void _queue_destroy(ipc_queue_t *q)
{
	if (q->self_wref)
		weakref_destroy(q->self_wref);

	while (!list_empty(&q->free_dynamic)) {
		ipc_dynamic_message_t *dyn = list_pop(&q->free_dynamic,
			ipc_dynamic_message_t, link);

		assert(dyn->parent_queue == q->self_wref);

		weakref_put(dyn->parent_queue);
		slab_free(slab_ipc_dynamic_message_cache, dyn);
	}

	while (!list_empty(&q->pages)) {
		void *page = list_pop(&q->pages, _dummy_page_t, link);
		slab_free(slab_page_cache, page);
	}

	slab_free(slab_ipc_queue_cache, q);
}

/**
 *
 * @param size  Size of the buffer in bytes. Must be a multiple of PAGE_SIZE.
 * @return      Newly created queue or NULL if out of memory.
 */
ipc_queue_t *ipc_queue_create(size_t size)
{
	assert(size >= PAGE_SIZE);
	assert(IS_ALIGNED(size, PAGE_SIZE));

	ipc_queue_t *q = slab_alloc(slab_ipc_queue_cache, FRAME_ATOMIC);
	if (!q)
		return NULL;

	*q = (ipc_queue_t) {};

	irq_spinlock_initialize(&q->lock, "ipc_queue_t::lock");
	irq_spinlock_initialize(&q->free_dynamic_lock,
		"ipc_queue_t::free_dynamic_lock");
	list_initialize(&q->free_dynamic);
	list_initialize(&q->pending_dynamic);
	list_initialize(&q->pending);
	list_initialize(&q->free);
	list_initialize(&q->pages);

	q->self_wref = weakref_create(q);
	if (!q->self_wref) {
		_queue_destroy(q);
		return NULL;
	}

	size_t page_count = size / PAGE_SIZE;
	for (size_t i = 0; i < page_count; i++) {
		void *page = slab_alloc(slab_page_cache, FRAME_ATOMIC);
		if (!page) {
			_queue_destroy(q);
			return NULL;
		}

		_insert_page(q, page);
	}

	kobject_initialize(&q->kobject, KOBJECT_TYPE_IPC_QUEUE);
	return q;
}

static void _queue_kobj_destroy(kobject_t *kobj)
{
    assert(list_empty(&kobj->caps_list));

    auto q = (ipc_queue_t *) kobj;
    _queue_destroy(q);
}

const kobject_ops_t ipc_queue_kobject_ops = {
    .destroy = _queue_kobj_destroy,
};

void ipc_queue_put(ipc_queue_t *q)
{
    kobject_put(&q->kobject);
}

static ipc_retval_t _ipc_queue_reserve(ipc_queue_t *q, size_t n)
{
	if (q->reserve_requested > SIZE_MAX - n)
		return ipc_e_limit_exceeded;

	if (q->free_count >= n && q->reserve_requested == 0) {
		q->free_count -= n;
		q->reserve_unclaimed += n;
		return ipc_success;
	} else {
		q->reserve_requested += n;
		return ipc_reserve_pending;
	}
}

/** Reserve space for n messages in the queue.
 * If the space can be reserved immediately, returns ipc_success.
 * Otherwise, returns ipc_reserve_pending, or ipc_limit_exceeded
 * if too many reservations have been requested.
 *
 * @param q
 * @return
 */
ipc_retval_t ipc_queue_reserve(ipc_queue_t *q, size_t n)
{
	if (n <= 0)
		return ipc_e_invalid_argument;

	irq_spinlock_lock(&q->lock, true);
	ipc_retval_t rc = _ipc_queue_reserve(q, n);
	irq_spinlock_unlock(&q->lock, true);
	return rc;
}

ipc_endpoint_t *ipc_endpoint_create(ipc_queue_t *q, uintptr_t tag, int reserves)
{
    assert(reserves == 0);

    ipc_endpoint_t *ep = slab_alloc(slab_ipc_endpoint_cache, FRAME_ATOMIC);
    if (!ep)
        return NULL;

    ep->queue_ref = weakref_ref(q->self_wref);
    if (!ep->queue_ref) {
        slab_free(slab_ipc_endpoint_cache, ep);
        return NULL;
    }

    kobject_initialize(&ep->kobject, KOBJECT_TYPE_IPC_ENDPOINT);
    ep->tag = tag;

    return ep;
}

static void _ipc_endpoint_destroy(kobject_t *kobj)
{
    assert(list_empty(&kobj->caps_list));

    auto ep = (ipc_endpoint_t *) kobj;
    weakref_put(ep->queue_ref);
    slab_free(slab_ipc_endpoint_cache, ep);
}

const kobject_ops_t ipc_endpoint_kobject_ops = {
    .destroy = _ipc_endpoint_destroy,
};

static void _deprocess_send(ipc_message_t *m)
{
	for (int i = 0; i < IPC_CALL_LEN; i++) {
		if (ipc_get_arg_type(m, i) == IPC_ARG_TYPE_KOBJECT)
			kobject_put(ipc_get_arg(m, i).ptr);
	}
}

static inline void ipc_set_arg_kobject(ipc_message_t *m, int i, kobject_t *kobj)
{
	ipc_set_arg(m, i, (ipc_arg_t) (void *) kobj, IPC_ARG_TYPE_KOBJECT);
}

/**
 * Preprocess all the different object argument types.
 * After processing, arg type fields all contain either IPC_ARG_TYPE_VAL,
 * or IPC_ARG_TYPE_OBJECT, with the latter type's argument having been
 * converted to kobject_t reference.
 *
 * @param sender_q
 * @param tag
 * @param m
 * @return ipc_success
 * @return ipc_e_invalid_argument
 * @return ipc_e_no_memory
 */
static ipc_retval_t _process_send(ipc_queue_t *sender_q, uintptr_t tag,
	ipc_message_t *m)
{
	if (m->endpoint_tag != 0) {
		DEBUG("Sending message with nonzero endpoint tag.\n");
		return ipc_e_invalid_argument;
	}

	if ((m->flags & IPC_MESSAGE_FLAG_PROTOCOL_ERROR) && m->flags != IPC_MESSAGE_FLAG_PROTOCOL_ERROR) {
		DEBUG("Sending invalid protocol error message.\n");
		return ipc_e_invalid_argument;
	}

	m->endpoint_tag = tag;
	int autodrop = 0;
	ipc_endpoint_t *ep;

	for (int i = 0; i < IPC_CALL_LEN; i++) {
		ipc_arg_type_t type = ipc_get_arg_type(m, i);

		switch (type) {
		case IPC_ARG_TYPE_VAL:
			continue;

		case IPC_ARG_TYPE_ENDPOINT_1:
			ep = ipc_endpoint_create(sender_q, ipc_get_arg(m, i).val, 1);
			if (!ep) {
				_deprocess_send(m);
				return ipc_e_no_memory;
			}

			ipc_set_arg_kobject(m, i, &ep->kobject);
			continue;

		case IPC_ARG_TYPE_ENDPOINT_2:
			ep = ipc_endpoint_create(sender_q, ipc_get_arg(m, i).val, 2);
			if (!ep) {
				_deprocess_send(m);
				return ipc_e_no_memory;
			}

			ipc_set_arg_kobject(m, i, &ep->kobject);
			continue;

		case IPC_ARG_TYPE_OBJECT:
			kobject_t *kobj = kobject_get_any(TASK, ipc_get_arg(m, i).obj);
			if (!kobj) {
				_deprocess_send(m);
				DEBUG("Trying to send an invalid capability.\n");
				return ipc_e_invalid_argument;
			}

			ipc_set_arg_kobject(m, i, kobj);
			continue;

		case IPC_ARG_TYPE_OBJECT_AUTODROP:
			/* We need all allocations done before we start working on these. */
			autodrop++;
			continue;

		case IPC_ARG_TYPE_NONE:
		case IPC_ARG_TYPE_KOBJECT:
			/* Fallthrough */
		}

		/* If we fell off the switch(), the type is wrong. */
		_deprocess_send(m);
		DEBUG("Invalid argument type: %d\n", type);
		return ipc_e_invalid_argument;
	}

	if (autodrop) {
		for (int i = 0; i < IPC_CALL_LEN; i++) {
			if (ipc_get_arg_type(m, i) == IPC_ARG_TYPE_OBJECT_AUTODROP) {
				/*
				 * We don't guarantee any particular state of the autodrop caps
				 * when returning ipc_e_invalid_argument. The userspace should
				 * just panic when it happens, because it's always a bug.
				 * We exploit this leeway here to avoid locking the caps twice,
				 * once for retrieval and second for removal.
				 *
				 * We do this in a separate loop because failure in creating
				 * endpoints (ipc_e_no_memory) is recoverable.
				 */

				// TODO: Only lock the capabilities only once, and retrieve
				//       all the objects at once, atomically.

				kobject_t *kobj = cap_destroy_any(TASK, ipc_get_arg(m, i).obj);
				if (!kobj) {
					_deprocess_send(m);
					DEBUG("Trying to send+drop an invalid capability.\n");
					return ipc_e_invalid_argument;
				}

				ipc_set_arg_kobject(m, i, kobj);
			}
		}
	}

	return ipc_success;
}

// FIXME
__attribute__((unused))
static ipc_retval_t _ipc_queue_send_reserved(ipc_queue_t *q, ipc_queue_t *sender_q,
	uintptr_t endpoint_tag,
	uspace_addr_t uspace_buffer, size_t uspace_buffer_size,
	size_t *reservations_granted)
{
	/* A write with a reservation can't wait. */
	irq_spinlock_lock(&q->lock, true);

	assert(q->reserved > 0);
	q->reserved--;

	ipc_linked_message_t *m = list_pop(&q->free, ipc_linked_message_t, link);
	assert(m);

	irq_spinlock_unlock(&q->lock, true);

	errno_t rc = copy_from_uspace(&m->data, uspace_buffer, sizeof(m->data));
	if (rc != EOK) {
		_release_message_buffer(q, m, reservations_granted);
		return ipc_e_memory_fault;
	}

	ipc_retval_t ret = _process_send(sender_q, endpoint_tag, &m->data);
	if (ret != ipc_success) {
		_release_message_buffer(q, m, reservations_granted);
		return ret;
	}

	irq_spinlock_lock(&q->lock, true);
	list_append(&m->link, &q->pending);
	irq_spinlock_unlock(&q->lock, true);

	return ipc_success;
}

#if 0

ipc_retval_t _ipc_queue_write_unreserved(ipc_queue_t *q,
	uspace_addr_t uspace_buffer, size_t uspace_buffer_size, bool alloc)
{

	irq_spinlock_lock(&q->lock, true);
	ipc_linked_message_t *m = list_pop(&q->free, ipc_linked_message_t, link);
	if (m) {
		q->free_count--;
	}
	irq_spinlock_unlock(&q->lock, true);

	if (!m) {

	}
}

#endif

/**
 * Preprocess message retrieved from queue before sending it to userspace.
 * It only contains IPC_ARG_TYPE_VAL and IPC_ARG_TYPE_KOBJECT entries before
 * processing. IPC_ARG_TYPE_KOBJECT entries are converted to newly allocated
 * capabilities (IPC_ARG_TYPE_OBJECT) in the recipient task.
 *
 * If capabilities cannot be allocated for every object in the message,
 * ipc_e_no_memory is returned and the message is restored to the original
 * state.
 *
 * Otherwise, ipc_success is returned and the message contains only VAL and
 * OBJECT entries.
 *
 * @param m     Message to process.
 * @param task  Task to allocate capabilities for.
 */
static ipc_retval_t _preprocess_message(ipc_message_t *m, task_t *task)
{
	for (int i = 0; i < IPC_CALL_LEN; i++) {
		ipc_arg_type_t type = ipc_get_arg_type(m, i);
		switch (type) {
		default:
			panic("Bad arg type %d in message retrieved from queue.", type);
		case IPC_ARG_TYPE_VAL:
			continue;
		case IPC_ARG_TYPE_KOBJECT:
		}

		cap_handle_t cap = cap_create(task, ipc_get_arg(m, i).ptr);
		if (cap != CAP_NIL) {
			ipc_set_arg(m, i, (ipc_arg_t) cap, IPC_ARG_TYPE_OBJECT);
			continue;
		}

		/* Failed allocating capabilities, restore original values. */
		for (int j = 0; j < i; j++) {
			if (ipc_get_arg_type(m, j) != IPC_ARG_TYPE_OBJECT)
				continue;

			kobject_t *kobj = cap_destroy_any(TASK, ipc_get_arg(m, j).obj);
			ipc_set_arg_kobject(m, j, kobj);
		}

		return ipc_e_no_memory;
	}

	return ipc_success;
}

/**
 * Destroy capabilities in the message and convert them back to kobject pointers.
 */
static void _deprocess_message(ipc_message_t *m, task_t *task)
{
    for (int i = 0; i < IPC_CALL_LEN; i++) {
        if (ipc_get_arg_type(m, i) != IPC_ARG_TYPE_OBJECT)
			continue;

        kobject_t *kobj = cap_destroy_any(TASK, ipc_get_arg(m, i).obj);
        assert(kobj != nullptr);
		ipc_set_arg_kobject(m, i, kobj);
    }
}

static ipc_retval_t _ipc_queue_read(ipc_queue_t *q,
	uspace_addr_t uspace_buffer, size_t *uspace_buffer_size,
	size_t *reservations_granted)
{
	assert(!list_empty(&q->pending));
	assert(*uspace_buffer_size > sizeof(ipc_message_t));

	// TODO: read more than one message at a time,
	//       requires a bit more thought out waitq synchronization,
	//       currently the reader_waitq tokens match 1:1 to pending messages.

	*uspace_buffer_size = 0;

	// TODO: maybe we could turn this into a singly linked lock-free list?
	irq_spinlock_lock(&q->lock, true);
	ipc_linked_message_t *m = list_pop(&q->pending, ipc_linked_message_t, link);
	irq_spinlock_unlock(&q->lock, true);

	/* Turn kobject references into caps. */
	ipc_retval_t rc = _preprocess_message(&m->data, TASK);
	if (rc != ipc_success) {
		irq_spinlock_lock(&q->lock, true);
		list_prepend(&m->link, &q->pending);
		irq_spinlock_unlock(&q->lock, true);
		waitq_wake_one(&q->reader_waitq);
		return rc;
	}

	// TODO: pass the whole message in a vector register
	//       instead of using copy_to/from_uspace()?
	// The whole structure is 256b/512b depending on pointer size, so should fit
	// into register on anything with a vector extension.
	if (copy_to_uspace(uspace_buffer, &m->data, sizeof(m->data)) != EOK) {
		_deprocess_message(&m->data, TASK);
		irq_spinlock_lock(&q->lock, true);
		list_prepend(&m->link, &q->pending);
		irq_spinlock_unlock(&q->lock, true);
		waitq_wake_one(&q->reader_waitq);
		return ipc_e_memory_fault;
	}

	*uspace_buffer_size = sizeof(ipc_message_t);

	_release_message_buffer(q, m, reservations_granted);
	return ipc_success;
}

ipc_retval_t ipc_queue_read(ipc_queue_t *q,
	uspace_addr_t uspace_buffer, size_t *uspace_buffer_size,
	size_t *reservations_granted, int timeout_usec)
{
    assert(q != NULL);

	if (*uspace_buffer_size < sizeof(ipc_message_t))
		return ipc_e_invalid_argument;

	errno_t rc;

	if (timeout_usec < 0) {
		rc = _waitq_sleep_timeout(&q->reader_waitq, 0,
			SYNCH_FLAGS_INTERRUPTIBLE);
	} else {
		rc = _waitq_sleep_timeout(&q->reader_waitq, timeout_usec,
			SYNCH_FLAGS_NON_BLOCKING | SYNCH_FLAGS_INTERRUPTIBLE);
	}

	switch (rc) {
	default:
		panic("Unhandled error code %s in ipc_queue_read()\n", str_error_name(rc));
		break;
	case EINTR:
		return ipc_e_interrupted_thread;
	case ETIMEOUT:
		return ipc_e_timed_out;
	case EOK:
	}

	return _ipc_queue_read(q, uspace_buffer, uspace_buffer_size,
		reservations_granted);
}


////////////////////////////////////////////////////////////////////////////////

sysarg_t sys_ipcb_endpoint_create(sysarg_t queue_handle, sysarg_t tag,
	uspace_addr_t out_endpoint_handle)
{
    cap_handle_t ep_cap;
    if (cap_alloc(TASK, &ep_cap) != EOK)
        return 0;

    ipc_endpoint_t *ep;

    if (queue_handle == 0) {
        ep = ipc_endpoint_create(TASK->default_queue, tag, 0);
    } else {
        auto q = kobject_get(TASK, (cap_handle_t) queue_handle, KOBJECT_TYPE_IPC_QUEUE);
        ep = ipc_endpoint_create((ipc_queue_t *) q, tag, 0);
        kobject_put(q);
    }

    if (!ep) {
        cap_free(TASK, ep_cap);
        return 0;
    }

    cap_publish(TASK, ep_cap, &ep->kobject);
    return (sysarg_t) ep_cap;
}

#if 0

sys_errno_t sys_ipc_call(kobj_handle_t endpoint_handle,
    kobj_handle_t return_queue_handle, sysarg_t return_ep_tag,
    sysarg_t handle_count, uspace_ptr_uintptr_t argptr)
{
	ipc_message_t *msg = slab_alloc(slab_ipc_message, 0);
	if (!msg)
		return ENOMEM;

	link_initialize(&msg->link);
	msg->handle_count = handle_count;

	errno_t rc = copy_from_uspace(&msg->args, argptr, sizeof(msg->args));
	if (rc != EOK) {
		slab_free(slab_ipc_message, msg);
		return rc;
	}

	if (msg->args[0] != 0) {
		/* Arg 0 is replaced with a newly created endpoint. */
		slab_free(slab_ipc_message, msg);
		return EINVAL;
	}

	ipc_endpoint_t *ep = kobj_table_lookup(&TASK->kobj_table, endpoint_handle, KOBJ_CLASS_ENDPOINT);
	if (!ep) {
		slab_free(slab_ipc_message, msg);
		return ENOENT;
	}

	msg->endpoint_tag = ep->tag;

	irq_spinlock_lock(&ep->ref->lock, true);
	ipc_queue_t *queue = ep->ref->queue;
	if (!ep->ref->queue) {
		irq_spinlock_unlock(&ep->ref->lock, true);
		kobj_put(ep);
		slab_free(slab_ipc_message, msg);
		return EHANGUP;
	}

	/* create return endpoint */
	ipc_queue_t *retq = kobj_table_lookup(&TASK->kobj_table, return_queue_handle, KOBJ_CLASS_QUEUE);


	/* translate handles */
	for (int i = 1; i < handle_count; i++) {
		kobj_t *kobj = kobj_table_shallow_lookup(&TASK->kobj_table, msg->args[i]);
		if (kobj)
			msg->args[i] = kobj_table_insert(&queue->owner->kobj_table, kobj);
		else
			msg->args[i] = 0;
	}

	list_append(&msg->link, &ep->ref->queue->list);
	waitq_wake_one(&ep->ref->queue->waitq);
	irq_spinlock_unlock(&ep->ref->lock, true);
	kobj_put(ep);
	return EOK;
}

sys_errno_t sys_ipc_send(sysarg_t endpoint_handle, sysarg_t handle_count,
	sysarg_t data_size, uspace_addr_t data)
{
	ipc_message_t *msg = slab_alloc(slab_ipc_message, 0);
	if (!msg)
		return ENOMEM;

	link_initialize(&msg->link);
	msg->handle_count = handle_count;

	errno_t rc = copy_from_uspace(&msg->args, argptr, sizeof(msg->args));
	if (rc != EOK) {
		slab_free(slab_ipc_message, msg);
		return rc;
	}

	ipc_endpoint_t *ep = kobj_table_lookup(&TASK->kobj_table, endpoint_handle, KOBJ_CLASS_ENDPOINT);
	if (!ep) {
		slab_free(slab_ipc_message, msg);
		return ENOENT;
	}

	msg->endpoint_tag = ep->tag;

	irq_spinlock_lock(&ep->ref->lock, true);
	ipc_queue_t *queue = ep->ref->queue;
	if (!ep->ref->queue) {
		irq_spinlock_unlock(&ep->ref->lock, true);
		kobj_put(ep);
		slab_free(slab_ipc_message, msg);
		return EHANGUP;
	}

	/* translate handles */
	for (int i = 0; i < handle_count; i++) {
		kobj_t *kobj = kobj_table_shallow_lookup(&TASK->kobj_table, msg->args[i]);
		if (kobj)
			msg->args[i] = kobj_table_insert(&queue->owner->kobj_table, kobj);
		else
			msg->args[i] = 0;
	}

	list_append(&msg->link, &ep->ref->queue->list);
	waitq_wake_one(&ep->ref->queue->waitq);
	irq_spinlock_unlock(&ep->ref->lock, true);
	kobj_put(ep);
	return EOK;
}

sys_errno_t sys_ipc_receive(kobj_handle_t buffer_handle)
{
	ipc_buffer_t *buffer = kobj_table_lookup(&TASK->kobj_table, buffer_handle, KOBJ_CLASS_BUFFER);
	if (!buffer)
		return ENOENT;

	waitq_sleep_interruptible(&buffer->waitq);

	irq_spinlock_lock(&buffer->wref->lock, true);
	assert(buffer->wref->buffer == buffer);
	ipc_message_t *msg = list_pop(&buffer->queue, ipc_message_t, link);
	irq_spinlock_unlock(&buffer->wref->lock);
	kobj_put(buffer);

	// TODO: process message
}

#endif

// TODO: move elsewhere

static IRQ_SPINLOCK_DECLARE(_root_lock);
static ipc_endpoint_t *_root_ep;

sys_errno_t sys_ipcb_ns_set(sysarg_t ep_cap)
{
    auto ep = (ipc_endpoint_t *)
        kobject_get(TASK, (cap_handle_t) ep_cap, KOBJECT_TYPE_IPC_ENDPOINT);

    if (!ep)
        return EINVAL;

    irq_spinlock_lock(&_root_lock, true);
    auto old_ep = _root_ep;
    _root_ep = ep;
    irq_spinlock_unlock(&_root_lock, true);

    if (old_ep)
        kobject_put(&old_ep->kobject);
    return EOK;
}

sysarg_t sys_ipcb_ns_get()
{
    irq_spinlock_lock(&_root_lock, true);
    auto ep = _root_ep;
    kobject_add_ref(&ep->kobject);
    irq_spinlock_unlock(&_root_lock, true);

    auto cap = cap_create(TASK, &ep->kobject);
    if (cap == CAP_NIL)
        kobject_put(&ep->kobject);

    return (sysarg_t) cap;
}
