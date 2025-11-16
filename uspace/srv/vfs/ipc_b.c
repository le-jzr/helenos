
#include "vfs.h"

#include <abi/ipc_b.h>
#include <ipc/loc.h>
#include <ipc/vfs.h>
#include <ipc_b.h>
#include <protocol/core.h>
#include <stdlib.h>
#include <vfs/canonify.h>
#include <vfs/vfs.h>

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
	ipc_endpoint_ops_t *ops;
	vfs_client_data_t *vfs_data;
};

struct _vfs_boxed_handle {
	ipc_endpoint_ops_t *ops;
	vfs_node_t *node;
	int permissions;
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



static errno_t vfs_in_fsprobe(vfs_client_data_t *vfs_data, service_id_t service_id, const char *fs_name, vfs_fs_probe_info_t *info)
{
	return vfs_op_fsprobe(fs_name, service_id, info);
}

static errno_t vfs_in_fstypes(vfs_client_data_t *vfs_data, ipc_blob_t **fstypes)
{
	vfs_fstypes_t fstypes_data;
	errno_t rc = vfs_get_fstypes(&fstypes_data);
	if (rc == EOK) {
		*fstypes = ipc_blob_create(fstypes_data.buf, fstypes_data.size);
		vfs_fstypes_free(&fstypes_data);
	}
	return rc;
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
		vfs_in_clone(instance, call);
		break;
	case VFS_IN_FSPROBE:
		vfs_in_fsprobe(instance, call);
		break;
	case VFS_IN_FSTYPES:
		vfs_in_fstypes(instance, call);
		break;
	case VFS_IN_MOUNT:
		vfs_in_mount(instance, call);
		break;
	case VFS_IN_OPEN:
		vfs_in_open(instance, call);
		break;
	case VFS_IN_PUT:
		vfs_in_put(instance, call);
		break;
	case VFS_IN_READ:
		vfs_in_read(instance, call);
		break;
	case VFS_IN_REGISTER:
		vfs_register(instance, call);
		cont = false;
		break;
	case VFS_IN_RENAME:
		vfs_in_rename(instance, call);
		break;
	case VFS_IN_RESIZE:
		vfs_in_resize(instance, call);
		break;
	case VFS_IN_STAT:
		vfs_in_stat(instance, call);
		break;
	case VFS_IN_STATFS:
		vfs_in_statfs(instance, call);
		break;
	case VFS_IN_SYNC:
		vfs_in_sync(instance, call);
		break;
	case VFS_IN_UNLINK:
		vfs_in_unlink(instance, call);
		break;
	case VFS_IN_UNMOUNT:
		vfs_in_unmount(instance, call);
		break;
	case VFS_IN_WAIT_HANDLE:
		vfs_in_wait_handle(instance, call);
		break;
	case VFS_IN_WALK:
		vfs_in_walk(instance, call);
		break;
	case VFS_IN_WRITE:
		vfs_in_write(instance, call);
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
