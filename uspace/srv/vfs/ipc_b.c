
#include "vfs.h"

#include <abi/ipc_b.h>
#include <ipc_b.h>
#include <protocol/core.h>
#include <stdlib.h>

/*
 * At first, we emulate the original API by keeping a file descriptor mapping in
 * each VFS instance object. A new instance is created by calling the VFS
 * singleton object registered in the root service.
 *
 * This is meant to be a stopgap. Eventually, each file descriptor should be
 * an individual IPC object served by the FS servers directly, with no central
 * VFS server at all.
 */

struct _vfs_instance {

};

struct _vfs_boxed_handle {
	ipc_endpoint_ops_t *ops;
	vfs_node_t *node;
	int permissions;
};

struct _vfs_node {
	ipc_endpoint_ops_t *ops;
};

struct _vfs_open_file {
	ipc_endpoint_ops_t *ops;

	vfs_node_t *node;

	int permissions;
	bool open_read;
	bool open_write;

	/** Append on write. */
	bool append;

	unsigned refcnt;
};

static void vfs_in_clone(ipc_message_t *call)
{
	int oldfd = ipc_get_val(call, 2);
	int newfd = ipc_get_val(call, 3);
	bool desc = ipc_get_val(call, 4);

	int outfd = -1;
	errno_t rc = vfs_op_clone(oldfd, newfd, desc, &outfd);
	async_answer_1(req, rc, outfd);
}


static void _instance_on_message(void *self, ipc_message_t *call)
{
	struct _vfs_instance *instance = self;

	// Standard arguments are [0] = return endpoint, [1] = protocol method number.
	// The remaining four arguments depend on the method.

	if (ipc_get_arg_type(call, 0) != IPC_ARG_TYPE_OBJECT)
		return;

	if (ipc_get_arg_type(call, 1) != IPC_ARG_TYPE_VAL) {
		ipcb_answer_protocol_error(call);
		return;
	}

	auto method = ipc_get_arg(call, 1).val;

	switch (method) {
	case VFS_IN_CLONE:
		vfs_in_clone(&call);
		break;
	case VFS_IN_FSPROBE:
		vfs_in_fsprobe(&call);
		break;
	case VFS_IN_FSTYPES:
		vfs_in_fstypes(&call);
		break;
	case VFS_IN_MOUNT:
		vfs_in_mount(&call);
		break;
	case VFS_IN_OPEN:
		vfs_in_open(&call);
		break;
	case VFS_IN_PUT:
		vfs_in_put(&call);
		break;
	case VFS_IN_READ:
		vfs_in_read(&call);
		break;
	case VFS_IN_REGISTER:
		vfs_register(&call);
		cont = false;
		break;
	case VFS_IN_RENAME:
		vfs_in_rename(&call);
		break;
	case VFS_IN_RESIZE:
		vfs_in_resize(&call);
		break;
	case VFS_IN_STAT:
		vfs_in_stat(&call);
		break;
	case VFS_IN_STATFS:
		vfs_in_statfs(&call);
		break;
	case VFS_IN_SYNC:
		vfs_in_sync(&call);
		break;
	case VFS_IN_UNLINK:
		vfs_in_unlink(&call);
		break;
	case VFS_IN_UNMOUNT:
		vfs_in_unmount(&call);
		break;
	case VFS_IN_WAIT_HANDLE:
		vfs_in_wait_handle(&call);
		break;
	case VFS_IN_WALK:
		vfs_in_walk(&call);
		break;
	case VFS_IN_WRITE:
		vfs_in_write(&call);
		break;
	default:
		async_answer_0(&call, ENOTSUP);
		break;
	}
}

static void _instance_on_destroy(void *self)
{
	assert(self);

	struct _vfs_instance *instance = self;
	// TODO
	free(instance);
}

static ipc_endpoint_ops_t _instance_ops = {
	.on_message = _instance_on_message,
	.on_destroy = _instance_on_destroy,
};


#if 0

static ipc_endpoint_t *_vfs_instantiate()
{
	panic("TODO");
}

static void _main_on_message(void *self, ipc_message_t *call)
{
	if (call->flags != ipc_message_flags_1(0, IPC_ARG_TYPE_OBJECT)) {
		ipcb_answer_protocol_error(call);
		return;
	}

	auto ep = _vfs_instantiate();

	ipc_message_t reply = {};


	ipcb_answer(const ipc_message_t *call, const ipc_message_t *answer)
	panic("TODO");
}

static void _main_on_destroy(void *self)
{
	exit(EXIT_SUCCESS);
}

void vfs_ipcb_register()
{
	auto main_endpoint = ipc_endpoint_create(NULL, uintptr_t tag, int reserves)
}

#endif
