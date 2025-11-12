#include <_bits/errno.h>
#include <_bits/native.h>
#include <abi/cap.h>
#include <abi/ipc_b.h>
#include <adt/list.h>
#include <align.h>
#include <cap/cap.h>
#include <ipc_b.h>
#include <mem.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <panic.h>
#include <proc/task.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <str_error.h>
#include <synch/spinlock.h>
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

typedef struct  {
	link_t link;
	ipc_message_t data;
} ipc_linked_message_t;

typedef struct ipc_queue {
	/* Keep first. */
	kobject_t kobject;

	weakref_t *self_wref;

	/* Synchronizes pending and reserve list. */
	irq_spinlock_t lock;
	list_t pending;
	list_t reserve;

	/* Tied to length of the pending list. */
	waitq_t reader_waitq;
} ipc_queue_t;

struct ipc_endpoint {
	/* Keep first. */
	kobject_t kobject;

	uintptr_t tag;
	weakref_t *queue_ref;

	/* Synchronized by queue lock. */
	int reserves;
};

static slab_cache_t *slab_ipc_queue_cache;
static slab_cache_t *slab_ipc_endpoint_cache;
static slab_cache_t *slab_ipc_message_cache;

static ipc_linked_message_t *_message_alloc()
{
	return slab_alloc(slab_ipc_message_cache, FRAME_ATOMIC);
}

static void _message_free(ipc_linked_message_t *m)
{
	slab_free(slab_ipc_message_cache, m);
}

void ipc_queue_init(void)
{
	slab_ipc_queue_cache = slab_cache_create("ipc_queue_t",
		sizeof(ipc_queue_t), alignof(ipc_queue_t), NULL, NULL, 0);
	slab_ipc_endpoint_cache = slab_cache_create("ipc_endpoint_t",
		sizeof(ipc_endpoint_t), alignof(ipc_endpoint_t), NULL, NULL, 0);
	slab_ipc_message_cache = slab_cache_create("ipc_linked_message_t",
		sizeof(ipc_linked_message_t), alignof(ipc_linked_message_t),
		NULL, NULL, 0);
}

static void _queue_destroy(ipc_queue_t *q)
{
	if (q->self_wref)
		weakref_destroy(q->self_wref);

	ipc_linked_message_t *msg;

	while ((msg = list_pop(&q->reserve, ipc_linked_message_t, link)) != NULL)
		_message_free(msg);

	while ((msg = list_pop(&q->pending, ipc_linked_message_t, link)) != NULL)
		_message_free(msg);

	slab_free(slab_ipc_queue_cache, q);
}

/**
 *
 * @param size  Size of the buffer in bytes. Must be a multiple of PAGE_SIZE.
 * @return      Newly created queue or NULL if out of memory.
 */
ipc_queue_t *ipc_queue_create()
{
	ipc_queue_t *q = slab_alloc(slab_ipc_queue_cache, FRAME_ATOMIC);
	if (!q)
		return NULL;

	*q = (ipc_queue_t) {};

	irq_spinlock_initialize(&q->lock, "ipc_queue_t::lock");
	list_initialize(&q->pending);
	list_initialize(&q->reserve);

	q->self_wref = weakref_create(q);
	if (!q->self_wref) {
		_queue_destroy(q);
		return NULL;
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

void ipc_endpoint_put(ipc_endpoint_t *ep)
{
	kobject_put(&ep->kobject);
}

/** Reserve space for n messages in the queue.
 * If the space can be reserved, returns ipc_success.
 * Otherwise, returns ipc_e_no_memory.
 *
 * @param q
 * @return
 */
static ipc_retval_t _ipc_queue_reserve(ipc_queue_t *q, int n)
{
	LIST_INITIALIZE(tmp_list);

	for (int i = 0; i < n; i++) {
		auto m = _message_alloc();
		if (m == NULL) {
			while (!list_empty(&tmp_list))
				_message_free(list_pop(&tmp_list, ipc_linked_message_t, link));

			return ipc_e_no_memory;
		}

		list_append(&m->link, &tmp_list);
	}

	irq_spinlock_lock(&q->lock, true);
	list_concat(&q->reserve, &tmp_list);
	irq_spinlock_unlock(&q->lock, true);
	return ipc_success;
}

ipc_endpoint_t *ipcb_endpoint_create(ipc_queue_t *q, uintptr_t tag, int reserves)
{
	if (q == NULL)
		return NULL;

    if (reserves < 0 || reserves > IPC_ENDPOINT_MAX_RESERVES)
    	return NULL;

    ipc_endpoint_t *ep = slab_alloc(slab_ipc_endpoint_cache, FRAME_ATOMIC);
    if (!ep)
        return NULL;

    ep->queue_ref = weakref_ref(q->self_wref);
    if (!ep->queue_ref) {
        slab_free(slab_ipc_endpoint_cache, ep);
        return NULL;
    }

    auto rc = _ipc_queue_reserve(q, reserves);
    if (rc != ipc_success) {
    	weakref_put(ep->queue_ref);
     	slab_free(slab_ipc_endpoint_cache, ep);
      	return NULL;
    }

    kobject_initialize(&ep->kobject, KOBJECT_TYPE_IPC_ENDPOINT);
    ep->tag = tag;
    ep->reserves = reserves;

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
			ep = ipcb_endpoint_create(sender_q, ipc_get_arg(m, i).val, 1);
			if (!ep) {
				_deprocess_send(m);
				return ipc_e_no_memory;
			}

			ipc_set_arg_kobject(m, i, &ep->kobject);
			continue;

		case IPC_ARG_TYPE_ENDPOINT_2:
			ep = ipcb_endpoint_create(sender_q, ipc_get_arg(m, i).val, 2);
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

static ipc_retval_t _ipc_queue_send(ipc_queue_t *q, ipc_queue_t *sender_q,
	uintptr_t endpoint_tag,
	uspace_addr_t uspace_buffer, ipc_linked_message_t *m)
{
	assert(m != NULL);

	errno_t rc = copy_from_uspace(&m->data, uspace_buffer, sizeof(m->data));
	if (rc != EOK) {
		_message_free(m);
		return ipc_e_memory_fault;
	}

	ipc_retval_t ret = _process_send(sender_q, endpoint_tag, &m->data);
	if (ret != ipc_success) {
		_message_free(m);
		return ret;
	}

	irq_spinlock_lock(&q->lock, true);
	list_append(&m->link, &q->pending);
	irq_spinlock_unlock(&q->lock, true);

	return ipc_success;
}

static ipc_queue_t *_queue_from_cap(cap_handle_t cap)
{
	return (ipc_queue_t *) kobject_get(TASK, cap, KOBJECT_TYPE_IPC_QUEUE);
}

static ipc_endpoint_t *_endpoint_from_cap(cap_handle_t cap)
{
	return (ipc_endpoint_t *) kobject_get(TASK, cap, KOBJECT_TYPE_IPC_ENDPOINT);
}

sysarg_t sys_ipcb_send(sysarg_t return_queue_handle, sysarg_t endpoint_handle,
	uspace_addr_t uspace_msg)
{
	ipc_retval_t rc = ipc_success;
	auto return_q = _queue_from_cap((cap_handle_t) return_queue_handle);
	auto ep = _endpoint_from_cap((cap_handle_t) endpoint_handle);
	ipc_queue_t *dest_q = nullptr;
	ipc_linked_message_t *m = nullptr;

	if (!ep) {
		rc = ipc_e_invalid_argument;
		goto exit;
	}

	dest_q = weakref_hold(ep->queue_ref);
	if (!dest_q) {
		rc = ipc_e_destination_gone;
		goto exit;
	}

	irq_spinlock_lock(&dest_q->lock, true);
	if (ep->reserves > 0) {
		ep->reserves--;
		m = list_pop(&dest_q->reserve, ipc_linked_message_t, link);
	}
	irq_spinlock_unlock(&dest_q->lock, true);

	if (m == nullptr) {
		m = _message_alloc();
		if (m == nullptr) {
			rc = ipc_e_no_memory;
			goto exit;
		}
	}

	rc = _ipc_queue_send(dest_q, return_q, ep->tag, uspace_msg, m);

exit:
	if (return_q)
		ipc_queue_put(return_q);
	if (ep)
		ipc_endpoint_put(ep);
	if (dest_q)
		ipc_queue_put(dest_q);

	return rc;
}

/**
 * Destroy capabilities in the message and convert them back to kobject pointers.
 */
static void _deprocess_message(ipc_message_t *m, task_t *task)
{
    for (int i = 0; i < IPC_CALL_LEN; i++) {
        if (ipc_get_arg_type(m, i) != IPC_ARG_TYPE_OBJECT)
			continue;

        kobject_t *kobj = cap_destroy_any(task, ipc_get_arg(m, i).obj);
        assert(kobj != nullptr);
		ipc_set_arg_kobject(m, i, kobj);
    }
}

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
		_deprocess_message(m, task);
		return ipc_e_no_memory;
	}

	return ipc_success;
}


static ipc_retval_t _ipc_queue_read(ipc_queue_t *q,
	uspace_addr_t uspace_buffer, size_t *uspace_buffer_size)
{
	assert(*uspace_buffer_size >= sizeof(ipc_message_t));

	// TODO: read more than one message at a time,
	//       requires a bit more thought out waitq synchronization,
	//       currently the reader_waitq tokens match 1:1 to pending messages.

	*uspace_buffer_size = 0;

	// TODO: maybe we could turn this into a singly linked lock-free list?
	irq_spinlock_lock(&q->lock, true);
	ipc_linked_message_t *m = list_pop(&q->pending, ipc_linked_message_t, link);
	irq_spinlock_unlock(&q->lock, true);

	assert(m != NULL);

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

	_message_free(m);
	return ipc_success;
}

ipc_retval_t ipc_queue_read(ipc_queue_t *q,
	uspace_addr_t uspace_buffer, size_t *uspace_buffer_size, int timeout_usec)
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

	return _ipc_queue_read(q, uspace_buffer, uspace_buffer_size);
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
        ep = ipcb_endpoint_create(TASK->default_queue, tag, 0);
    } else {
        auto q = _queue_from_cap((cap_handle_t) queue_handle);
        ep = ipcb_endpoint_create(q, tag, 0);
        ipc_queue_put(q);
    }

    if (!ep) {
        cap_free(TASK, ep_cap);
        return 0;
    }

    cap_publish(TASK, ep_cap, &ep->kobject);
    return (sysarg_t) ep_cap;
}

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
   		ipc_endpoint_put(old_ep);
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
   		ipc_endpoint_put(ep);

    return (sysarg_t) cap;
}
