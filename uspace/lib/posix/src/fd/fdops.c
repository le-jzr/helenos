
static struct {
	fibril_mutex_t mutex;
	struct __posix_fd **entries;
	int len;
	int lowest_free;
} _posix_fds = {
	.mutex = FIBRIL_MUTEX_INIT,
	.entries = NULL,
	.len = 0,
	.lowest_free = 0,
};

/* Make sure we can't overflow size_t with allocations. */
#if LONG_MAX == INT64_MAX
#define _LONG_SIZE 8
#else
#define _LONG_SIZE 4
#endif

#if OPEN_MAX > (LONG_MAX / _LONG_SIZE)
#define _OPEN_MAX (LONG_MAX / _LONG_SIZE)
#else
#define _OPEN_MAX OPEN_MAX
#endif

int _grow_fds(int min_len)
{
	int old_len = _posix_fds.len;
	int new_len;
	if (_OPEN_MAX / 2 < old_len) {
		new_len = _OPEN_MAX;
	} else {
		new_len = max(old_len * 2, min_len);
	}

	struct __posix_fd **new_fds = realloc(_posix_fds.entries, new_len * sizeof(struct __posix_fd *));

	/* Try to allocate at least min_len if we can't get the preferred size. */
	if (new_fds == NULL) {
		new_len = min_len;
		new_fds = realloc(fds, new_len * sizeof(struct __posix_fd *));
	}

	if (new_fds == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Clear out the newly allocated entries. */
	for (int i = old_len; i < new_len; i++) {
		new_fds[i] = NULL;
	}
	
	_posix_fds.entries = new_fds;
	_posix_fds.len = new_len;
	return 0;
}

static int _alloc_fd_locked_2(struct __posix_fd *f, int fd)
{
	assert(fd >= 0);
	assert(fd < _posix_fds.len);
	assert(_posix_fds.entries[fd] == NULL);
	
	if (fd == _posix_fds.lowest_free) {
		_posix_fds.lowest_free += 1;
	}
	
	_posix_fds.entries[fd] = f;
	atomic_fetch_add(&f->refcnt, 1);
	return fd;
}

static int _alloc_fd_locked(struct __posix_fd *f, int min_fd)
{
	int len = _posix_fds.len;
	struct posix_fd **fds = _posix_fds.entries;

	if (min_fd >= _OPEN_MAX) {
		errno = EBADF;
		return -1;
	}

	bool is_lowest = false;

	if (min_fd <= _posix_fds.lowest_free) {
		min_fd = _posix_fds.lowest_free;
		is_lowest = true;
	}

	while (min_fd < len && fds[min_fd] != NULL) {
		min_fd++;
	}

	if (min_fd >= len) {
		if (len >= _OPEN_MAX) {
			errno = EMFILE;
			return -1;
		}

		if (_grow_fds(min_fd + 1) < 0) {
			return -1;
		}
	}

	if (is_lowest) {
		_posix_fds.lowest_free = min_fd + 1;
	}

	return _alloc_fd_locked_2(f, min_fd);
}

int __alloc_fd(struct __posix_fd *f)
{
	fibril_mutex_lock(&_posix_fds.mutex);
	int retval = _alloc_fd_locked(f);
	fibril_mutex_unlock(&_posix_fds.mutex);
	return retval;
}

static struct __posix_fd *_get_file_locked(int fd)
{
	if (fd < 0 || fd >= _posix_fds.len) {
		errno = EBADF;
		return NULL;
	}

	struct __posix_fd *f = _posix_fds.entries[fd];
	if (f == NULL) {
		errno = EBADF;
		return NULL;
	}

	atomic_fetch_add(&f->refcnt, 1);
	return f;
}

static struct __posix_fd *_take_file_locked(int fd)
{
	if (fd < 0 || fd >= _posix_fds.len) {
		errno = EBADF;
		return NULL;
	}

	struct __posix_fd *f = _posix_fds.entries[fd];
	if (f == NULL) {
		errno = EBADF;
		return NULL;
	}

	_posix_fds.entries[fd] = NULL;
	if (fd < _posix_fds.lowest_free) {
		_posix_fds.lowest_free = fd;
	}
	return f;
}

static struct __posix_fd *_take_file(int fd)
{
	fibril_mutex_lock(&_posix_fds.mutex);
	struct __posix_fd *f = _take_file_locked(fd);
	fibril_mutex_unlock(&_posix_fds.mutex);
	return f;
}

static int _put_file(struct __posix_fd *f)
{
	if (f == NULL) {
		return -1;
	}

	if (atomic_fetch_sub(&f->refcnt, 1) == 1) {
		return f->__ops->close(f);
	}

	return 0;
}

static int _with_file(int fd, void (*op)(struct __posix_fd *, void *), void *data)
{
	fibril_mutex_lock(&_posix_fds.mutex);
	struct __posix_fd *f = _get_file_locked(fd);
	fibril_mutex_unlock(&_posix_fds.mutex);

	if (f == NULL) {
		return -1;
	}

	op(f, data);
	return _put_file(f);
}

static int _dup(int fd, int fd2)
{
	int newfd = -1;

	fibril_mutex_lock(&_posix_fds.mutex);
	struct __posix_fd *f = _get_file_locked(fd);
	if (f != NULL) {
		newfd = _alloc_fd_locked(f, fd2);
	}
	fibril_mutex_unlock(&_posix_fds.mutex);

	_put_file(f);
	return newfd;
}

static int _dup2(int fd, int fd2)
{
	int newfd = -1;
	struct __posix_fd *f = NULL;
	struct __posix_fd *f2 = NULL;

	fibril_mutex_lock(&_posix_fds.mutex);
	f = _get_file_locked(fd);
	if (f != NULL) {
		errno_t save_errno = errno;
		f2 = _take_file(fd2);
		errno = save_errno;

		newfd = _alloc_fd_locked(f, fd2);
		assert(newfd == -1 || newfd == fd2);
	}
	fibril_mutex_unlock(&_posix_fds.mutex);

	_put_file(f2);
	_put_file(f);
	return newfd;
}

struct _read_data {
	ssize_t retval;
	void *buf;
	size_t nbyte;
	off_t offset;
};

struct _write_data {
	ssize_t retval;
	const void *buf;
	size_t nbyte;
	off_t offset;
};

static void _isatty(struct __posix_fd *f, void *data) {
	*(int *)data = f->__ops->is(f, __POSIX_FD_TTY);
}

static void _read(struct __posix_fd *f, void *data) {
	struct _read_data *rdata = data;
	rdata->retval = f->__ops->read(f, rdata->buffer, rdata->length, -1);
}

static void _pread(struct __posix_fd *f, void *data) {
	struct _read_data *rdata = data;

	if (!f->__ops->is(f, __POSIX_FD_SEEKABLE)) {
		errno = ESPIPE;
		return;
	}

	if (rdata->offset < 0) {
		errno = EINVAL;
		return;
	}

	rdata->retval = f->__ops->pread(f, rdata->buffer, rdata->length, rdata->offset);
}

static void _write(struct __posix_fd *f, void *data) {
	struct _write_data *wdata = data;
	wdata->retval = f->__ops->write(f, wdata->buffer, wdata->length, -1);
}

static void _pwrite(struct __posix_fd *f, void *data) {
	struct _write_data *wdata = data;

	if (!f->__ops->is(f, __POSIX_FD_SEEKABLE)) {
		errno = ESPIPE;
		return;
	}

	if (wdata->offset < 0) {
		errno = EINVAL;
		return;
	}

	wdata->retval = f->__ops->write(f, wdata->buffer, wdata->length, wdata->offset);
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/close.html
 *
 * The behavior of close() is similar to that of most other platforms,
 * in that the file descriptor is _always_ deallocated, even when an
 * error occurs and a failure is reported. For further discussion of this
 * behavior, see e.g. the Linux Programmer's Manual (`man 2 close` on
 * a Linux system).
 */
int close(int fd)
{
	return _put_file(_take_file(fd));
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/isatty.html
 */
int isatty(int fd)
{
	int ret = 0;
	_with_file(fd, _isatty, &ret);
	return ret;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html
 */
ssize_t read(int fd, void *buf, size_t nbyte)
{
	struct _read_data data = {
		.retval = -1,
		.buf = buf,
		.nbyte = nbyte,
		.offset = -1,
	};

	_with_file(fd, _read, &data);
	return data.retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html
 */
ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset)
{
	struct _read_data data = {
		.retval = -1,
		.buf = buf,
		.nbyte = nbyte,
		.offset = offset,
	};

	_with_file(fd, _pread, &data);
	return data.retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html
 */
ssize_t write(int fd, const void *buf, size_t nbyte)
{
	struct _write_data data = {
		.retval = -1,
		.buf = buf,
		.nbyte = nbyte,
		.offset = -1,
	};

	_with_file(fd, _write, &data);
	return data.retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html
 */
ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset)
{
	struct _write_data data = {
		.retval = -1,
		.buf = buf,
		.nbyte = nbyte,
		.offset = offset,
	};

	_with_file(fd, _pwrite, &data);
	return data.retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/lseek.html
 */
off_t lseek(int fd, off_t offset, int whence)
{
	switch (whence) {
	case SEEK_SET:
	case SEEK_CUR:
	case SEEK_END:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	struct _seek_data data = {
		.retval = -1,
		.offset = offset,
		.whence = whence,
	};

	_with_file(fd, _seek, &data);
	return data.retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/fsync.html
 */
int fsync(int fd)
{
	int retval = -1;
	_with_file(fd, _sync, &retval);
	return retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/ftruncate.html
 */
int ftruncate(int fd, off_t length)
{
	struct _truncate_data data = {
		.retval = -1,
		.length = length,
	}
	
	_with_file(fd, _truncate, &data);
	return data.retval;
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/dup.html
 */
int dup(int fd)
{
	return _dup(fd, 0);
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/dup.html
 */
int dup2(int fd, int fd2)
{
	return _dup2(fd, fd2);
}

/** http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html
 */
int fcntl(int fd, int cmd, ...)
{
	int flags;

	switch (cmd) {
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
		va_list ap;
		va_start(ap, cmd);
		int fd2 = va_arg(ap, int);
		va_end(ap);
		
		return _dup(fd, fd2);
	case F_GETFD:
		/* FD_CLOEXEC is not supported. There are no other flags. */
		return 0;
	case F_SETFD:
		/* FD_CLOEXEC is not supported. Ignore arguments and report success. */
		return 0;
	case F_GETFL:
		/* File status flags (i.e. O_APPEND) are currently private to the
		 * VFS server so it cannot be easily retrieved. */
		flags = 0;
		/* File access flags are currently not supported for file descriptors.
		 * Provide full access. */
		flags |= O_RDWR;
		return flags;
	case F_SETFL:
		/* File access flags are currently not supported for file descriptors.
		 * Ignore arguments and report success. */
		return 0;
	case F_GETOWN:
	case F_SETOWN:
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/* Signals (SIGURG) and file locks are not supported. */
		errno = ENOTSUP;
		return -1;
	default:
		/* Unknown command */
		errno = EINVAL;
		return -1;
	}
}
