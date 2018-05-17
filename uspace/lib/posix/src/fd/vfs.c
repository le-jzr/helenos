#include "../internal/fd.h"

static ssize_t _vfs_read(void *self, void *buf, size_t nbyte);
static ssize_t _vfs_write(void *self, const void *buf, size_t nbyte);
static ssize_t _vfs_pread(void *self, void *buf, size_t nbyte, off_t offset);
static ssize_t _vfs_pwrite(void *self, const void *buf, size_t nbyte, off_t offset);
static int _vfs_close(void *self);
static int _vfs_sync(void *self);
static int _vfs_datasync(void *self);
static int _vfs_truncate(void *self);
static int _vfs_isatty(void *self);

static const struct __posix_fd_ops __vfs_fd_ops = {
	.read = _vfs_read,
	.write = _vfs_write,
	.pread = _vfs_pread,
	.pwrite = _vfs_pwrite,
	.close = _vfs_close,
	.sync = _vfs_sync,
	.datasync = _vfs_datasync,
	.truncate = _vfs_truncate,
	.isatty = _vfs_isatty,
};

static struct __posix_fd *_vfs_open_handler(const char *path, int posix_flags, int posix_mode)
{
	int flags = WALK_REGULAR;
	if (posix_flags & O_CREAT) {
		if (posix_flags & O_EXCL)
			flags |= WALK_MUST_CREATE;
		else
			flags |= WALK_MAY_CREATE;
	}

	int mode =
	    ((posix_flags & O_RDWR) ? MODE_READ | MODE_WRITE : 0) |
	    ((posix_flags & O_RDONLY) ? MODE_READ : 0) |
	    ((posix_flags & O_WRONLY) ? MODE_WRITE : 0) |
	    ((posix_flags & O_APPEND) ? MODE_APPEND : 0);

	int file;

	if (failed(vfs_lookup(pathname, flags, &file)))
		return -1;

	if (failed(vfs_open(file, mode))) {
		vfs_put(file);
		return -1;
	}

	if (posix_flags & O_TRUNC) {
		if (posix_flags & (O_RDWR | O_WRONLY)) {
			if (failed(vfs_resize(file, 0))) {
				vfs_put(file);
				return -1;
			}
		}
	}

	return file;
}
