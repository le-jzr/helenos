struct _open_handler {
	char *prefix;
	__fd_open_handler_t handler;
};

struct _open_handlers {
	fibril_mutex_t mutex;
	struct _open_handler *e;
	int len;
} _handlers;

static const char PATH_SEPARATOR = '/';

int _strcmp(const char *a, const char *b) {
	if (a == NULL)
		return b == NULL ? 0 : -1;

	if (b == NULL)
		return 1;

	return strcmp(a, b);
}

char *_canonify(const char *path)
{
	char *d = strdup(path);
	if (d == NULL)
		return NULL;

	char *nd = canonify(d, NULL);
	if (nd == NULL) {
		free(d);
		return NULL;
	}

	/* Ensure the string is at the start of the buffer. */
	if (nd != d) {
		char *dt = d;
		while (true) {
			*dt = *nd;
			if (*dt == '\0')
				break;
			dt++;
			nd++;
		}
	}

	return d;
}

bool _path_starts_with(const char *path, const char *prefix)
{
	if (prefix == NULL)
		return true;

	if (path == NULL)
		return false;

	while (true) {
		if (*prefix == '\0')
			return *path == '\0' || *path == PATH_SEPARATOR;

		if (*prefix != *path)
			return false;

		path++;
		prefix++;
	}
}

/**
 * Implements http://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html
 *
 * The behavior of this function can be modified using `__fd_set_open_handler()`.
 */
int open(const char *pathname, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT) {
		va_list args;
		va_start(args, flags);
		mode = va_arg(args, mode_t);
		va_end(args);
		(void) mode;
	}

	if (((flags & (O_RDONLY | O_WRONLY | O_RDWR)) == 0) ||
	    ((flags & (O_RDONLY | O_WRONLY)) == (O_RDONLY | O_WRONLY)) ||
	    ((flags & (O_RDONLY | O_RDWR)) == (O_RDONLY | O_RDWR)) ||
	    ((flags & (O_WRONLY | O_RDWR)) == (O_WRONLY | O_RDWR))) {
		assert(!"Invalid arguments to open().");
		errno = EINVAL;
		return -1;
	}

	// FIXME: This won't work with symlinks.
	char *path = _canonify(pathname);
	if (path == NULL) {
		errno = ENOMEM;
		return -1;
	}

	__fd_open_handler_t handler = NULL;

	fibril_mutex_lock(_handlers.mutex);

	for (int i = 0; i < _handlers.len; i++) {
		if (_path_starts_with(path, _handlers.e[i].prefix)) {
			handler = _handlers.e[i].handler;
			break;
		}
	}

	fibril_mutex_unlock(_handlers.mutex);

	if (handler == NULL) {
		free(path);
		errno = ENOENT;
		return -1;
	}

	struct __posix_fd *fptr = handler(path, flags, mode);
	free(path);
	if (fptr == NULL) {
		errno = ENOENT;
		return -1;
	}

	int fd = __alloc_fd(fptr);
	__put_file(fptr);
	return fd;
}

static errno_t _set_handler_locked(char *prefix, __fd_open_handler_t handler)
{
	for (int i = 0; i < _handlers.len; i++) {
		if (_strcmp(prefix, _handlers.e[i].prefix) == 0) {
			_handlers.e[i].handler == handler;
			free(prefix);
			return EOK;
		}
	}

	struct _open_handler *new =
	    realloc(_handlers.e, (_handlers.len + 1) * sizeof(*_handlers.e));
	if (new == NULL) {
		free(prefix);
		return ENOMEM;
	}

	struct _open_handler tmp;

	new[len] = { .prefix = prefix, .handler = handler };

	/* Reorder the array (reverse lexicographic order). */
	for (int i = len - 1; i > 0; i--) {
		if (_strcmp(new[i].prefix, new[i + 1].prefix) >= 0)
			break;

		tmp = new[i];
		new[i] = new[i+1];
		new[i+1] = tmp;
	}

	_handlers.e = new;
	_handlers.len = len + 1;
	return EOK;
}

errno_t __fd_set_open_handler(const char *prefix, __fd_open_handler_t handler)
{
	char *s = _canonify(prefix);
	if (s == NULL)
		return ENOMEM;

	fibril_mutex_lock(_handlers.mutex);
	errno_t rc = _set_handler_locked(s, handler);
	fibril_mutex_unlock(_handlers.mutex);
	return rc;
}


