/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdio.h>

#include <errno.h>
#include <io/kio.h>
#include <stdlib.h>
#include <vfs/inbox.h>
#include <vfs/vfs_sess.h>
#include <vfs/vfs.h>

#include "../private/io.h"
#include "../private/stdio.h"

static size_t stdio_kio_read(void *, size_t, size_t, FILE *);
static size_t stdio_kio_write(const void *, size_t, size_t, FILE *);
static int stdio_kio_flush(FILE *);

static size_t stdio_vfs_read(void *, size_t, size_t, FILE *);
static size_t stdio_vfs_write(const void *, size_t, size_t, FILE *);
static int stdio_vfs_flush(FILE *);
static errno_t stdio_vfs_close(FILE *);

/** KIO stream ops */
static __stream_ops_t stdio_kio_ops = {
	.read = stdio_kio_read,
	.write = stdio_kio_write,
	.flush = stdio_kio_flush,
	.close = NULL,
};

/** VFS stream ops */
static __stream_ops_t stdio_vfs_ops = {
	.read = stdio_vfs_read,
	.write = stdio_vfs_write,
	.flush = stdio_vfs_flush,
	.close = stdio_vfs_close,
};

static FILE stdin_null = {
	.fd = -1,
	.pos = 0,
	.error = true,
	.eof = true,
	.ops = &stdio_vfs_ops,
	.arg = NULL,
	.sess = NULL,
	.btype = _IONBF,
	.buf = NULL,
	.buf_size = 0,
	.buf_head = NULL,
	.buf_tail = NULL,
	.buf_state = _bs_empty
};

static FILE stdout_kio = {
	.fd = -1,
	.pos = 0,
	.error = false,
	.eof = false,
	.ops = &stdio_kio_ops,
	.arg = NULL,
	.sess = NULL,
	.btype = _IOLBF,
	.buf = NULL,
	.buf_size = BUFSIZ,
	.buf_head = NULL,
	.buf_tail = NULL,
	.buf_state = _bs_empty
};

static FILE stderr_kio = {
	.fd = -1,
	.pos = 0,
	.error = false,
	.eof = false,
	.ops = &stdio_kio_ops,
	.arg = NULL,
	.sess = NULL,
	.btype = _IONBF,
	.buf = NULL,
	.buf_size = 0,
	.buf_head = NULL,
	.buf_tail = NULL,
	.buf_state = _bs_empty
};

FILE *stdin = NULL;
FILE *stdout = NULL;
FILE *stderr = NULL;

static LIST_INITIALIZE(files);

void __stdio_init(void)
{
	/*
	 * The first three standard file descriptors are assigned for compatibility.
	 * This will probably be removed later.
	 */
	int infd = inbox_get("stdin");
	if (infd >= 0) {
		int stdinfd = -1;
		(void) vfs_clone(infd, -1, false, &stdinfd);
		assert(stdinfd == 0);
		vfs_open(stdinfd, MODE_READ);
		stdin = fdopen(stdinfd, "r");
	} else {
		stdin = &stdin_null;
		list_append(&stdin->link, &files);
	}

	int outfd = inbox_get("stdout");
	if (outfd >= 0) {
		int stdoutfd = -1;
		(void) vfs_clone(outfd, -1, false, &stdoutfd);
		assert(stdoutfd <= 1);
		while (stdoutfd < 1)
			(void) vfs_clone(outfd, -1, false, &stdoutfd);
		vfs_open(stdoutfd, MODE_APPEND);
		stdout = fdopen(stdoutfd, "a");
	} else {
		stdout = &stdout_kio;
		list_append(&stdout->link, &files);
	}

	int errfd = inbox_get("stderr");
	if (errfd >= 0) {
		int stderrfd = -1;
		(void) vfs_clone(errfd, -1, false, &stderrfd);
		assert(stderrfd <= 2);
		while (stderrfd < 2)
			(void) vfs_clone(errfd, -1, false, &stderrfd);
		vfs_open(stderrfd, MODE_APPEND);
		stderr = fdopen(stderrfd, "a");
	} else {
		stderr = &stderr_kio;
		list_append(&stderr->link, &files);
	}
}

void __stdio_done(void)
{
	while (!list_empty(&files)) {
		FILE *file = list_get_instance(list_first(&files), FILE, link);
		fclose(file);
	}
}

static bool parse_mode(const char *fmode, int *mode, bool *create, bool *excl,
    bool *truncate)
{
	/* Parse mode except first character. */
	const char *mp = fmode;

	if (*mp++ == '\0') {
		errno = EINVAL;
		return false;
	}

	if ((*mp == 'b') || (*mp == 't'))
		mp++;

	bool plus;
	if (*mp == '+') {
		mp++;
		plus = true;
	} else {
		plus = false;
	}

	bool ex;
	if (*mp == 'x') {
		mp++;
		ex = true;
	} else {
		ex = false;
	}

	if (*mp != '\0') {
		errno = EINVAL;
		return false;
	}

	*create = false;
	*truncate = false;
	*excl = false;

	/* Parse first character of fmode and determine mode for vfs_open(). */
	switch (fmode[0]) {
	case 'r':
		*mode = plus ? MODE_READ | MODE_WRITE : MODE_READ;
		if (ex) {
			errno = EINVAL;
			return false;
		}
		break;
	case 'w':
		*mode = plus ? MODE_READ | MODE_WRITE : MODE_WRITE;
		*create = true;
		*excl = ex;
		if (!plus)
			*truncate = true;
		break;
	case 'a':
		/* TODO: a+ must read from beginning, append to the end. */
		if (plus) {
			errno = ENOTSUP;
			return false;
		}

		if (ex) {
			errno = EINVAL;
			return false;
		}

		*mode = MODE_APPEND | (plus ? MODE_READ | MODE_WRITE : MODE_WRITE);
		*create = true;
		break;
	default:
		errno = EINVAL;
		return false;
	}

	return true;
}

static void _setvbuf(FILE *stream)
{
	/* FIXME: Use more complex rules for setting buffering options. */

	switch (stream->fd) {
	case 1:
		setvbuf(stream, NULL, _IOLBF, BUFSIZ);
		break;
	case 0:
	case 2:
		setvbuf(stream, NULL, _IONBF, 0);
		break;
	default:
		setvbuf(stream, NULL, _IOFBF, BUFSIZ);
	}
}

/** Open a stream.
 *
 * @param path Path of the file to open.
 * @param mode Mode string, (r|w|a)[b|t][+][x].
 *
 */
FILE *fopen(const char *path, const char *fmode)
{
	int mode;
	bool create;
	bool excl;
	bool truncate;

	if (!parse_mode(fmode, &mode, &create, &excl, &truncate))
		return NULL;

	/* Open file. */
	FILE *stream = malloc(sizeof(FILE));
	if (stream == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	int flags = WALK_REGULAR;
	if (create && excl)
		flags |= WALK_MUST_CREATE;
	else if (create)
		flags |= WALK_MAY_CREATE;
	int file;
	errno_t rc = vfs_lookup(path, flags, &file);
	if (rc != EOK) {
		errno = rc;
		free(stream);
		return NULL;
	}

	rc = vfs_open(file, mode);
	if (rc != EOK) {
		errno = rc;
		vfs_put(file);
		free(stream);
		return NULL;
	}

	if (truncate) {
		rc = vfs_resize(file, 0);
		if (rc != EOK) {
			errno = rc;
			vfs_put(file);
			free(stream);
			return NULL;
		}
	}

	stream->fd = file;
	stream->pos = 0;
	stream->error = false;
	stream->eof = false;
	stream->ops = &stdio_vfs_ops;
	stream->arg = NULL;
	stream->sess = NULL;
	stream->need_sync = false;
	_setvbuf(stream);
	stream->ungetc_chars = 0;

	list_append(&stream->link, &files);

	return stream;
}

FILE *fdopen(int fd, const char *mode)
{
	/* Open file. */
	FILE *stream = malloc(sizeof(FILE));
	if (stream == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	stream->fd = fd;
	stream->pos = 0;
	stream->error = false;
	stream->eof = false;
	stream->ops = &stdio_vfs_ops;
	stream->arg = NULL;
	stream->sess = NULL;
	stream->need_sync = false;
	_setvbuf(stream);
	stream->ungetc_chars = 0;

	list_append(&stream->link, &files);

	return stream;
}

static int _fclose_nofree(FILE *stream)
{
	errno_t rc = EOK;

	fflush(stream);

	if (stream->ops->close)
		rc = stream->ops->close(stream);

	list_remove(&stream->link);

	if (rc != EOK) {
		errno = rc;
		return EOF;
	}

	return 0;
}

int fclose(FILE *stream)
{
	int rc = _fclose_nofree(stream);

	if ((stream != &stdin_null) &&
	    (stream != &stdout_kio) &&
	    (stream != &stderr_kio))
		free(stream);

	return rc;
}

FILE *freopen(const char *path, const char *mode, FILE *stream)
{
	FILE *nstr;

	if (path == NULL) {
		/* Changing mode is not supported */
		return NULL;
	}

	(void) _fclose_nofree(stream);
	nstr = fopen(path, mode);
	if (nstr == NULL) {
		free(stream);
		return NULL;
	}

	list_remove(&nstr->link);
	*stream = *nstr;
	list_append(&stream->link, &files);

	free(nstr);

	return stream;
}

int fileno(FILE *stream)
{
	if (stream->ops != &stdio_vfs_ops) {
		errno = EBADF;
		return EOF;
	}

	return stream->fd;
}

async_sess_t *vfs_fsession(FILE *stream, iface_t iface)
{
	if (stream->fd >= 0) {
		if (stream->sess == NULL)
			stream->sess = vfs_fd_session(stream->fd, iface);

		return stream->sess;
	}

	return NULL;
}

errno_t vfs_fhandle(FILE *stream, int *handle)
{
	if (stream->fd >= 0) {
		*handle = stream->fd;
		return EOK;
	}

	return ENOENT;
}

/** Read from KIO stream. */
static size_t stdio_kio_read(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	stream->eof = true;
	return 0;
}

/** Write to KIO stream. */
static size_t stdio_kio_write(const void *buf, size_t size, size_t nmemb,
    FILE *stream)
{
	errno_t rc;
	size_t nwritten;

	rc = kio_write(buf, size * nmemb, &nwritten);
	if (rc != EOK) {
		stream->error = true;
		nwritten = 0;
	}

	return nwritten / size;
}

/** Flush KIO stream. */
static int stdio_kio_flush(FILE *stream)
{
	kio_update();
	return 0;
}

/** Read from VFS stream. */
static size_t stdio_vfs_read(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	errno_t rc;
	size_t nread;

	if (size == 0 || nmemb == 0)
		return 0;

	rc = vfs_read(stream->fd, &stream->pos, buf, size * nmemb, &nread);
	if (rc != EOK) {
		errno = rc;
		stream->error = true;
	} else if (nread == 0) {
		stream->eof = true;
	}

	return (nread / size);
}

/** Write to VFS stream. */
static size_t stdio_vfs_write(const void *buf, size_t size, size_t nmemb,
    FILE *stream)
{
	errno_t rc;
	size_t nwritten;

	rc = vfs_write(stream->fd, &stream->pos, buf, size * nmemb, &nwritten);
	if (rc != EOK) {
		errno = rc;
		stream->error = true;
	}

	return nwritten / size;
}

/** Flush VFS stream. */
static int stdio_vfs_flush(FILE *stream)
{
	errno_t rc;

	rc = vfs_sync(stream->fd);
	if (rc != EOK) {
		errno = rc;
		return EOF;
	}

	return 0;
}

static errno_t stdio_vfs_close(FILE *stream)
{
	if (stream->sess != NULL)
		async_hangup(stream->sess);

	if (stream->fd >= 0)
		return vfs_put(stream->fd);

	return EOK;
}

/** @}
 */
