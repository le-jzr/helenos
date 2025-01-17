
#include <kobj/kobj.h>
#include <synch/spinlock.h>

typedef struct ipc_buffer {
	kobj_t kobj;
	SPINLOCK_DECLARE(lock);
	void *data;
	size_t data_size;
} ipc_buffer_t;

static slab_cache_t *slab_ipc_buffer_cache;

void ipc_buffer_initialize(void)
{
	slab_ipc_buffer_cache = slab_cache_create("ipc_buffer_t",
			sizeof(ipc_buffer_t), alignof(ipc_buffer_t), NULL, NULL, 0);
}

static void destroy_buffer(void *arg)
{
	ipc_buffer_t *buffer = arg;
	if (buffer->data)
		free(buffer->data);
	slab_free(slab_ipc_buffer_cache, buffer);
}

static kobj_class_t kobj_class_ipc_buffer = {
	.destroy = destroy_buffer,
};

static ipc_buffer_t *create_buffer(void *data, size_t data_size)
{
	ipc_buffer_t *buffer = slab_alloc(slab_ipc_buffer_cache, 0);
	if (!buffer)
		return NULL;

	kobj_initialize(&buffer->kobj, &kobj_class_ipc_buffer);
	irq_spinlock_initialize(&buffer->lock, "ipc_buffer_t.lock");
	buffer->data = data;
	buffer->data_size = data_size;
}

sys_errno_t sys_buffer_create(uspace_addr_t data, sysarg_t data_size,
    uspace_ptr_kobj_handle_t out_handle)
{
	void *b = malloc(data_size);
	if (!b)
		return ENOMEM;

	errno_t rc = copy_from_uspace(b, data, data_size);
	if (rc != EOK) {
		free(b);
		return rc;
	}

	ipc_buffer_t *buffer = create_buffer(b, data_size);
	if (!buffer) {
		free(b);
		return ENOMEM;
	}

	kobj_handle_t handle = kobj_table_insert(&TASK->kobj_table, buffer);
	if (!handle) {
		kobj_put(buffer);
		return ENOMEM;
	}

	rc = copy_to_uspace(out_handle, &handle, sizeof(handle));
	if (rc != EOK) {
		kobj_put(kobj_table_remove(&TASK->kobj_table, handle));
		return rc;
	}

	return EOK;
}

sys_errno_t sys_buffer_read(kobj_handle_t buffer_handle,
    sysarg_t offset, sysarg_t size, uspace_addr_t dest)
{
	ipc_buffer_t *buffer = kobj_table_lookup(&TASK->kobj_table,
	    buffer_handle, &kobj_class_ipc_buffer);
	if (!buffer)
		return ENOENT;

	errno_t rc = EOK;
	irq_spinlock_lock(&buffer->lock, true);
	if (buffer->data) {
		if (buffer->data_size < offset + size) {
			rc = ERANGE;
		} else {
			rc = copy_to_uspace(dest, buffer->data + offset, size);
		}
	} else {
		rc = EINVAL;
	}
	irq_spinlock_unlock(&buffer->lock, true);

	kobj_put(buffer);
	return rc;
}

// TODO: associate buffer with the creating task for accounting purposes
//       and destroy owned buffers when task exits.

/**
 * Deallocate the internal memory of the buffer (further reads will return
 * error), and destroy the handle. This function exists to make it possible
 * for buffer's creator to free the memory even when a malicious/buggy recipient
 * holds onto a reference beyond the expected lifetime of the object.
 */
sys_errno_t sys_buffer_destroy(kobj_handle_t buffer_handle)
{
	ipc_buffer_t *buffer = kobj_table_lookup(&TASK->kobj_table,
	    buffer_handle, &kobj_class_ipc_buffer);
	if (!buffer)
		return ENOENT;

	irq_spinlock_lock(&buffer->lock, true);
	if (buffer->data) {
		free(buffer->data);
		buffer->data = NULL;
	}
	irq_spinlock_unlock(&buffer->lock, true);

	/* First drop the reference from lookup. */
	kobj_put(buffer);

	/*
	 * Then destroy the handle.
	 * Note that the two arguments to kobj_put may differ if proxy is involved.
	 */
	kobj_put(kobj_table_remove(&TASK->kobj_table, buffer_handle));
}
