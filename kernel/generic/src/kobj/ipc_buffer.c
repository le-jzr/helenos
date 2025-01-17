
#include <synch/spinlock.h>
#include <stdatomic.h>
#include <kobj/kobj.h>

typedef struct ipc_message {
	link_t link;
	uintptr_t endpoint_tag;
	uintptr_t args[IPC_CALL_LEN];
	int handle_count;
} ipc_message_t;

/** Weak reference used by endpoints to access their parent buffer. */
struct weakref {
	atomic_refcount_t refcount;
	SPINLOCK_DECLARE(lock);
	ipc_queue_t *queue;
};

struct weakref *weakref_ref(struct weakref *ref)
{
	if (ref)
		refcount_up(&ref->refcount);
	return ref;
}

void weakref_put(struct weakref *ref)
{
	if (ref && refcount_down(&ref->refcount)) {
		assert(ref->queue == NULL);
		slab_free(slab_weakref, ref);
	}
}

typedef struct {
	kobj_t kobj;
	task_t *owner;
	struct weakref *wref;
	waitq_t waitq;
	list_t list;
} ipc_queue_t;

struct ipc_endpoint {
	kobj_t kobj;
	uintptr_t tag;
	struct weakref *ref;
};

static void ipc_endpoint_destroy(void *arg)
{
	ipc_endpoint_t *ep = arg;
	weakref_put(ep->ref);
	slab_free(slab_ipc_endpoint, ep);
}

static kobj_class_t kobj_class_endpoint = {
	.destroy = ipc_endpoint_destroy,
};

static void ipc_buffer_destroy(ipc_buffer_t *buffer)
{
	/* Close the waitqueue. */
	waitq_close(&buffer->waitq);

	/* Invalidate weak references. */
	irq_spinlock_lock(&buffer->wref->lock, true);
	buffer->wref->buffer = NULL;
	irq_spinlock_unlock(&buffer->wref->lock, true);

	weakref_put(buffer->wref);

	task_release(buffer->owner);
}

sys_errno_t sys_ipc_buffer_create()
{

}

static errno_t ipc_endpoint_create(kobj_handle_t queue_handle, uintptr_t tag, ipc_endpoint_t **out_ep)
{
	ipc_queue_t *queue = kobj_table_lookup(&TASK->kobj_table, queue_handle, KOBJ_CLASS_QUEUE);
	if (!queue)
		return ENOENT;

	ipc_endpoint_t *ep = slab_alloc(slab_ipc_endpoint, 0);
	if (!ep) {
		kobj_put(queue);
		return ENOMEM;
	}

	kobj_initialize(&ep->kobj, &kobj_class_endpoint);
	ep->tag = tag;
	assert(queue->wref != NULL);
	ep->ref = weakref_ref(queue->wref);

	*out_ep = ep;
	return EOK;
}

sys_errno_t sys_ipc_endpoint_create(sysarg_t queue_handle, sysarg_t tag,
	uspace_addr_t out_endpoint_handle)
{
	ipc_endpoint_t *ep;
	errno_t rc = ipc_endpoint_create(queue_handle, tag, &ep);
	if (rc != EOK)
		return rc;

	kobj_handle_t ep_handle = kobj_table_insert(&TASK->kobj_table, ep);
	if (!ep_handle) {
		kobj_put(ep);
		return ENOMEM;
	}

	errno_t rc = copy_to_uspace(out_endpoint_handle, &ep_handle, sizeof(ep_handle));
	if (rc != EOK) {
		kobj_put(kobj_table_remove(&TASK->kobj_table, ep_handle);
		return rc;
	}

	return EOK;
}

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
