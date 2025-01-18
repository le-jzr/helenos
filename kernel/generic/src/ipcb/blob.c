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

#include <ipc_b.h>

#include <mm/slab.h>
#include <proc/task.h>
#include <stdalign.h>
#include <stdlib.h>
#include <synch/spinlock.h>
#include <syscall/copy.h>

/**
 * IPC blob is just a chunk of immutable data slapped into a kobject and
 * transferable between tasks. It can be created, read, and destroyed.
 */
struct ipc_blob {
	/* Keep first */
	kobject_t kobject;

	IRQ_SPINLOCK_DECLARE(lock);
	void *data;
	size_t data_size;
};

static slab_cache_t *slab_ipc_blob_cache;

void ipc_blob_init(void)
{
	slab_ipc_blob_cache = slab_cache_create("ipc_blob_t",
			sizeof(ipc_blob_t), alignof(ipc_blob_t), NULL, NULL, 0);
}

static void destroy_blob(kobject_t *arg)
{
	ipc_blob_t *blob = (ipc_blob_t *) arg;
	if (blob->data)
		free(blob->data);
	slab_free(slab_ipc_blob_cache, blob);
}

kobject_ops_t ipc_blob_kobject_ops = {
	.destroy = destroy_blob,
};

static ipc_blob_t *create_blob(void *data, size_t data_size)
{
	ipc_blob_t *blob = slab_alloc(slab_ipc_blob_cache, 0);
	if (!blob)
		return NULL;

	kobject_initialize(&blob->kobject, KOBJECT_TYPE_IPC_BLOB);
	irq_spinlock_initialize(&blob->lock, "ipc_blob_t.lock");
	blob->data = data;
	blob->data_size = data_size;
	return blob;
}

ipc_blob_t *ipc_blob_create(uspace_addr_t data, sysarg_t data_size)
{
	if (data_size > IPC_BLOB_SIZE_LIMIT)
		return NULL;

	void *b = malloc(data_size);
	if (!b)
		return NULL;

	errno_t rc = copy_from_uspace(b, data, data_size);
	if (rc != EOK) {
		free(b);
		return NULL;
	}

	ipc_blob_t *blob = create_blob(b, data_size);
	if (!blob) {
		free(b);
		return NULL;
	}

	return blob;
}

sysarg_t sys_blob_create(uspace_addr_t data, sysarg_t data_size)
{
	ipc_blob_t *blob = ipc_blob_create(data, data_size);
	if (!blob)
		return (sysarg_t) CAP_NIL;

	cap_handle_t handle = cap_create(TASK, &blob->kobject);
	if (handle == CAP_NIL)
		kobject_put(&blob->kobject);

	return (sysarg_t) handle;
}

sys_errno_t sys_blob_read(cap_handle_t blob_handle,
    sysarg_t offset, sysarg_t size, uspace_addr_t dest)
{
	ipc_blob_t *blob =
	    (ipc_blob_t *) kobject_get(TASK, blob_handle, KOBJECT_TYPE_IPC_BLOB);
	if (!blob)
		return ENOENT;

	errno_t rc = EOK;
	irq_spinlock_lock(&blob->lock, true);
	if (blob->data) {
		if (blob->data_size < size || blob->data_size - size < offset) {
			rc = ERANGE;
		} else {
			rc = copy_to_uspace(dest, blob->data + offset, size);
		}
	} else {
		rc = EINVAL;
	}
	irq_spinlock_unlock(&blob->lock, true);

	kobject_put(&blob->kobject);
	return rc;
}

// TODO: some form of memory accounting is necessary

/**
 * Deallocate the internal memory of the blob (further reads will return
 * error), and destroy the handle. This function exists to make it possible
 * for blob's creator to free the memory even when a buggy recipient
 * holds onto a reference beyond the expected lifetime of the object.
 */
sys_errno_t sys_blob_destroy(cap_handle_t blob_handle)
{
	ipc_blob_t *blob =
	    (ipc_blob_t *) cap_destroy(TASK, blob_handle, KOBJECT_TYPE_IPC_BLOB);
	if (!blob)
		return ENOENT;

	irq_spinlock_lock(&blob->lock, true);
	void *data = blob->data;
	blob->data = NULL;
	irq_spinlock_unlock(&blob->lock, true);

	if (data)
		free(data);

	return EOK;
}

/** @}
 */
