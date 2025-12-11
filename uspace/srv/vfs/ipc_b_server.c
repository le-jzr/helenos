#include <stddef.h>
#include <stdlib.h>
void vfs_instance_t_handle_message(const ipcb_message_t *msg, vfs_instance_t *instance)
{
switch (ipcb_get_val_1(msg) {

/* clone :: [{'name': 'oldfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'newfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'desc', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'bool'}, {'name': 'outfd', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_CLONE: // vfs_op_clone
{
	// TODO: check message type and detect protocol mismatch
	int oldfd = ipcb_get_val2(&msg);
	int newfd = ipcb_get_val3(&msg);
	bool desc = ipcb_get_val4(&msg);
	int outfd;
	errno_t rc = vfs_op_clone(oldfd, newfd, desc, &outfd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, outfd);
	ipcb_send_answer(&msg, answer);
}

/* fsprobe :: [{'name': 'service_id', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'service_id_t'}, {'name': 'fs_name', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}, {'name': 'info', 'indirect': True, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'vfs_fs_probe_info_t'}] */

case VFS_IN_FSPROBE: // vfs_in_fsprobe
{
	// TODO: check message type and detect protocol mismatch
	service_id_t service_id = ipcb_get_val2(&msg);
	size_t fs_name_slice = ipcb_get_val_3(&msg);
	size_t fs_name_len = ipcb_slice_len(fs_name_slice);
	void *fs_name = calloc(fs_name_len, 1);
	if (fs_name == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipc_blob_read_4(&msg, fs_name, fs_name_slice);
	fs_name[fs_name_len - 1] = '\0';

	vfs_fs_probe_info_t info;
	errno_t rc = vfs_in_fsprobe(service_id, fs_name, &info);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);

	struct __attribute__((packed)) {
		vfs_fs_probe_info_t info;
	} _outdata = {
		.info = info,
	};
	ipc_blob_write_1(&answer, &_outdata, sizeof(_outdata));

	ipcb_send_answer(&msg, answer);
	free(fs_name);
}

/* fstypes :: [{'name': 'fstypes', 'indirect': True, 'in': False, 'out': True, 'wide': False, 'type': None}] */

case VFS_IN_FSTYPES: // vfs_in_fstypes
{
	// TODO: check message type and detect protocol mismatch
	size_t fstypes_slice = ipcb_get_val_2(&msg);
	size_t fstypes_len = ipcb_slice_len(fstypes_slice);
	void *fstypes = calloc(fstypes_len, 1);
	if (fstypes == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipcb_buffer_t fstypes_obj = ipc_get_obj_3(msg);

	errno_t rc = vfs_in_fstypes(fstypes, fstypes_len);
	ipcb_buffer_write(fstypes_obj, fstypes_slice, fstypes, fstypes_len);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
	free(fstypes);
}

/* mount :: [{'name': 'mpfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'service_id', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'service_id_t'}, {'name': 'flags', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'unsigned'}, {'name': 'instance', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'unsigned'}, {'name': 'opts', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}, {'name': 'fs_name', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}, {'name': 'outfd', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_MOUNT: // vfs_op_mount
{
	// TODO: check message type and detect protocol mismatch
	struct __attribute__((packed)) {
		int mpfd;
		service_id_t service_id;
		unsigned flags;
		unsigned instance;
		size_t opts_slice;
		size_t fs_name_slice;
	} _indata;

	ipc_blob_read_2(&msg, &_indata, sizeof(_indata));

	size_t opts_len = ipcb_slice_len(_indata.opts_slice);
	void *opts = calloc(opts_len, 1);
	if (opts == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipc_blob_read_3(&msg, opts, opts_slice);
	opts[opts_len - 1] = '\0';

	size_t fs_name_len = ipcb_slice_len(_indata.fs_name_slice);
	void *fs_name = calloc(fs_name_len, 1);
	if (fs_name == nullptr) {
		ipcb_answer_nomem(msg);
		free(opts);
		return;
	}

	ipc_blob_read_4(&msg, fs_name, fs_name_slice);
	fs_name[fs_name_len - 1] = '\0';

	int outfd;
	errno_t rc = vfs_op_mount(_indata.mpfd, _indata.service_id, _indata.flags, _indata.instance, opts, fs_name, &outfd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, outfd);
	ipcb_send_answer(&msg, answer);
	free(opts);
	free(fs_name);
}

/* open :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'mode', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_OPEN: // vfs_op_open
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	int mode = ipcb_get_val3(&msg);
	errno_t rc = vfs_op_open(fd, mode);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
}

/* put :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_PUT: // vfs_op_put
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	errno_t rc = vfs_op_put(fd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
}

/* read :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'pos', 'indirect': False, 'in': True, 'out': False, 'wide': True, 'object': False, 'type': 'aoff64_t'}, {'name': 'buffer', 'indirect': True, 'in': False, 'out': True, 'wide': False, 'type': None}, {'name': 'read', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'size_t'}] */

case VFS_IN_READ: // vfs_op_read_direct
{
	// TODO: check message type and detect protocol mismatch
	struct __attribute__((packed)) {
		int fd;
		aoff64_t pos;
		size_t buffer_slice;
	} _indata;

	ipc_blob_read_2(&msg, &_indata, sizeof(_indata));

	size_t buffer_len = ipcb_slice_len(_indata.buffer_slice);
	void *buffer = calloc(buffer_len, 1);
	if (buffer == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipcb_buffer_t buffer_obj = ipc_get_obj_3(msg);

	size_t read;
	errno_t rc = vfs_op_read_direct(_indata.fd, _indata.pos, buffer, buffer_len, &read);
	ipcb_buffer_write(buffer_obj, _indata.buffer_slice, buffer, buffer_len);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, read);
	ipcb_send_answer(&msg, answer);
	free(buffer);
}

/* rename :: [{'name': 'basefd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'old', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}, {'name': 'new', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}] */

case VFS_IN_RENAME: // vfs_in_rename
{
	// TODO: check message type and detect protocol mismatch
	struct __attribute__((packed)) {
		int basefd;
		size_t old_slice;
		size_t new_slice;
	} _indata;

	ipc_blob_read_2(&msg, &_indata, sizeof(_indata));

	size_t old_len = ipcb_slice_len(_indata.old_slice);
	void *old = calloc(old_len, 1);
	if (old == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipc_blob_read_3(&msg, old, old_slice);
	old[old_len - 1] = '\0';

	size_t new_len = ipcb_slice_len(_indata.new_slice);
	void *new = calloc(new_len, 1);
	if (new == nullptr) {
		ipcb_answer_nomem(msg);
		free(old);
		return;
	}

	ipc_blob_read_4(&msg, new, new_slice);
	new[new_len - 1] = '\0';

	errno_t rc = vfs_in_rename(_indata.basefd, old, new);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
	free(old);
	free(new);
}

/* resize :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'size', 'indirect': False, 'in': True, 'out': False, 'wide': True, 'object': False, 'type': 'int64_t'}] */

case VFS_IN_RESIZE: // vfs_op_resize
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	int64_t size = ipcb_get_val64_3(&msg);
	errno_t rc = vfs_op_resize(fd, size);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
}

/* stat :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'data', 'indirect': True, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'vfs_stat_t'}] */

case VFS_IN_STAT: // vfs_op_stat_direct
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	vfs_stat_t data;
	errno_t rc = vfs_op_stat_direct(fd, &data);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);

	struct __attribute__((packed)) {
		vfs_stat_t data;
	} _outdata = {
		.data = data,
	};
	ipc_blob_write_1(&answer, &_outdata, sizeof(_outdata));

	ipcb_send_answer(&msg, answer);
}

/* statfs :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'data', 'indirect': True, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'vfs_statfs_t'}] */

case VFS_IN_STATFS: // vfs_op_statfs_direct
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	vfs_statfs_t data;
	errno_t rc = vfs_op_statfs_direct(fd, &data);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);

	struct __attribute__((packed)) {
		vfs_statfs_t data;
	} _outdata = {
		.data = data,
	};
	ipc_blob_write_1(&answer, &_outdata, sizeof(_outdata));

	ipcb_send_answer(&msg, answer);
}

/* sync :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_SYNC: // vfs_op_sync
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	errno_t rc = vfs_op_sync(fd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
}

/* unlink :: [{'name': 'parentfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'expectfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'path', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}] */

case VFS_IN_UNLINK: // vfs_op_unlink
{
	// TODO: check message type and detect protocol mismatch
	int parentfd = ipcb_get_val2(&msg);
	int expectfd = ipcb_get_val3(&msg);
	size_t path_slice = ipcb_get_val_4(&msg);
	size_t path_len = ipcb_slice_len(path_slice);
	void *path = calloc(path_len, 1);
	if (path == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipc_blob_read_5(&msg, path, path_slice);
	path[path_len - 1] = '\0';

	errno_t rc = vfs_op_unlink(parentfd, expectfd, path);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
	free(path);
}

/* unmount :: [{'name': 'mpfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_UNMOUNT: // vfs_op_unmount
{
	// TODO: check message type and detect protocol mismatch
	int mpfd = ipcb_get_val2(&msg);
	errno_t rc = vfs_op_unmount(mpfd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_send_answer(&msg, answer);
}

/* wrap_handle :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'handle', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': True, 'type': 'vfs_wrapped_handle_t'}] */

case VFS_IN_WRAP_HANDLE: // vfs_in_wrap_handle
{
	// TODO: check message type and detect protocol mismatch
	int fd = ipcb_get_val2(&msg);
	vfs_wrapped_handle_t handle;
	errno_t rc = vfs_in_wrap_handle(fd, &handle);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, handle);
	ipcb_send_answer(&msg, answer);
}

/* unwrap_handle :: [{'name': 'handle', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': True, 'type': 'vfs_wrapped_handle_t'}, {'name': 'high_fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'bool'}, {'name': 'fd', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_UNWRAP_HANDLE: // vfs_in_unwrap_handle
{
	// TODO: check message type and detect protocol mismatch
	vfs_wrapped_handle_t handle = ipcb_get_obj2(&msg);
	bool high_fd = ipcb_get_val3(&msg);
	int fd;
	errno_t rc = vfs_in_unwrap_handle(handle, high_fd, &fd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, fd);
	ipcb_send_answer(&msg, answer);
}

/* walk :: [{'name': 'parentfd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'flags', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'path', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'str'}, {'name': 'fd', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'int'}] */

case VFS_IN_WALK: // vfs_op_walk
{
	// TODO: check message type and detect protocol mismatch
	int parentfd = ipcb_get_val2(&msg);
	int flags = ipcb_get_val3(&msg);
	size_t path_slice = ipcb_get_val_4(&msg);
	size_t path_len = ipcb_slice_len(path_slice);
	void *path = calloc(path_len, 1);
	if (path == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipc_blob_read_5(&msg, path, path_slice);
	path[path_len - 1] = '\0';

	int fd;
	errno_t rc = vfs_op_walk(parentfd, flags, path, &fd);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, fd);
	ipcb_send_answer(&msg, answer);
	free(path);
}

/* write :: [{'name': 'fd', 'indirect': False, 'in': True, 'out': False, 'wide': False, 'object': False, 'type': 'int'}, {'name': 'pos', 'indirect': False, 'in': True, 'out': False, 'wide': True, 'object': False, 'type': 'aoff64_t'}, {'name': 'buffer', 'indirect': True, 'in': True, 'out': False, 'wide': False, 'type': None}, {'name': 'written', 'indirect': False, 'in': False, 'out': True, 'wide': False, 'object': False, 'type': 'size_t'}] */

case VFS_IN_WRITE: // vfs_op_write_direct
{
	// TODO: check message type and detect protocol mismatch
	struct __attribute__((packed)) {
		int fd;
		aoff64_t pos;
		size_t buffer_slice;
	} _indata;

	ipc_blob_read_2(&msg, &_indata, sizeof(_indata));

	size_t buffer_len = ipcb_slice_len(_indata.buffer_slice);
	void *buffer = calloc(buffer_len, 1);
	if (buffer == nullptr) {
		ipcb_answer_nomem(msg);
		return;
	}

	ipc_blob_read_3(&msg, buffer, buffer_slice);

	size_t written;
	errno_t rc = vfs_op_write_direct(_indata.fd, _indata.pos, buffer, buffer_len, &written);
	ipcb_message_t answer = ipcb_start_answer(&msg, rc);
	ipcb_set_val_1(&answer, written);
	ipcb_send_answer(&msg, answer);
	free(buffer);
}
