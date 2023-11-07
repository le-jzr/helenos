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
#include <stdlib.h>
#include <str.h>
#include <uchar.h>
#include <vfs/vfs.h>
#include <wchar.h>
#include <macros.h>

/* We only access the generic fields here. */
struct _IO_FILE_user_data {
};

#include "../private/stdio.h"

static void _fflushbuf(FILE *stream);

/** Set stream buffer. */
int setvbuf(FILE *stream, void *buf, int mode, size_t size)
{
	if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
		errno = EINVAL;
		return -1;
	}

	if (stream->buffer != NULL) {
		errno = EINVAL;
		return -1;
	}

	stream->btype = mode;
	if (buf != NULL) {
		stream->buffer = buf;
		stream->buffer_end = stream->buffer + size;
		stream->buffer_head = stream->buffer;
		stream->buffer_tail = stream->buffer;
	}

	return 0;
}

/** Set stream buffer.
 *
 * When @p buf is NULL, the stream is set as unbuffered, otherwise
 * full buffering is enabled.
 */
void setbuf(FILE *stream, void *buf)
{
	if (buf == NULL) {
		setvbuf(stream, NULL, _IONBF, BUFSIZ);
	} else {
		setvbuf(stream, buf, _IOFBF, BUFSIZ);
	}
}

static void _use_backup_buffer(FILE *stream)
{
	if (stream->buffer == NULL) {
		stream->buffer = stream->backup_buffer;
		stream->buffer_end = stream->buffer + UNGETC_MAX;
		stream->buffer_head = stream->buffer;
		stream->buffer_tail = stream->buffer;
	}
}

/**
 * Try to allocate buffer for this stream.
 *
 * If allocation fails, or the stream is not supposed to have a buffer,
 * a small backup buffer is used to ensure ungetc() still works.
 * Allocation is retried before every operation if it has failed.
 *
 * @param stream
 */
static void _lazy_alloc_buffer(FILE *stream)
{
	if (stream->buffer_requested_size == 0) {
		_use_backup_buffer(stream);
		return;
	}

	if (stream->buffer != NULL && stream->buffer != stream->backup_buffer)
		return;

	void *buf = malloc(stream->buffer_requested_size);
	if (buf == NULL) {
		_use_backup_buffer(stream);
		return;
	}

	/* Make sure we transfer any ungetc data if we were using a backup buffer. */
	size_t buffer_used = stream->buffer_tail - stream->buffer_head;
	memcpy(buf, stream->buffer_head, buffer_used);

	stream->buffer = buf;
	stream->buffer_end = buf + stream->buffer_requested_size;
	stream->buffer_head = stream->buffer;
	stream->buffer_tail = stream->buffer + buffer_used;
	stream->allocated_buffer = true;
}

/** Write to a stream (unbuffered). */
static size_t _write(FILE *stream, const void *buf, size_t size)
{
	stream->need_sync = true;
	return stream->ops->write(stream, buf, size);
}

static bool _buffer_empty(FILE *stream)
{
	return stream->buffer_tail == stream->buffer_head;
}

static bool _buffer_full(FILE *stream)
{
	return stream->buffer_tail == stream->buffer_end;
}

static size_t _buffer_used(FILE *stream)
{
	return stream->buffer_tail - stream->buffer_head;
}

static size_t _buffer_free(FILE *stream)
{
	return stream->buffer_end - stream->buffer_tail;
}

static size_t _buffer_size(FILE *stream)
{
	return stream->buffer_end - stream->buffer;
}

/** Read some data in stream buffer.
 *
 * On error, stream error indicator is set and errno is set.
 */
static void _ffillbuf(FILE *stream)
{
	assert(_buffer_empty(stream));
	size_t nread = stream->ops->read(stream, stream->buffer, _buffer_size(stream));

	stream->buffer_head = stream->buffer;
	stream->buffer_tail = stream->buffer + nread;
	stream->buffer_state = _bs_read;
}

/** Write out stream buffer, do not sync stream. */
static void _fflushbuf(FILE *stream)
{
	if (_buffer_empty(stream) || stream->error)
		return;

	/* If buffer has prefetched read data, we need to seek back. */
	if (stream->buffer_state == _bs_read && stream->ops->seek)
		stream->ops->seek(stream, -(int64_t) _buffer_used(stream), SEEK_CUR);

	/* If buffer has unwritten data, we need to write them out. */
	if (stream->buffer_state == _bs_write) {
		while (!_buffer_empty(stream)) {
			stream->buffer_head += _write(stream, stream->buffer_head, _buffer_used(stream));

			/* On error stream error indicator and errno are set by _write */
			if (stream->error)
				return;
		}
	}

	stream->buffer_head = stream->buffer;
	stream->buffer_tail = stream->buffer;
	stream->buffer_state = _bs_empty;
}

static void _flush_to_newline(FILE *stream)
{
	uint8_t *endl = memrchr(stream->buffer_head, '\n', _buffer_used(stream));
	if (endl == NULL)
		return;

	while (stream->buffer_head <= endl) {
		size_t n = _write(stream, stream->buffer_head, _buffer_used(stream));
		if (n == 0)
			return;

		stream->buffer_head += n;
	}

	if (_buffer_empty(stream)) {
		stream->buffer_head = stream->buffer;
		stream->buffer_tail = stream->buffer;
		stream->buffer_state = _bs_empty;
	}
}

static size_t _read_from_buffer(FILE *stream, void *dest, size_t size)
{
	size_t n = min(_buffer_used(stream), size);
	memcpy(dest, stream->buffer_head, n);
	stream->buffer_head += n;
	return n;
}

static size_t _fread(FILE *stream, void *dest, size_t total)
{
	/* Empty the buffer first. */
	size_t nread = _read_from_buffer(stream, dest, total);

	/* If the read is longer than the buffer, read in directly first. */
	while (total - nread >= _buffer_size(stream)) {
		size_t n = stream->ops->read(stream, dest + nread, total - nread);
		if (n == 0)
			return nread;

		nread += n;
	}

	while (nread < total) {
		if (_buffer_empty(stream)) {
			_ffillbuf(stream);

			if (_buffer_empty(stream))
				/* error/eof/errno set by _ffillbuf() */
				break;
		}

		nread += _read_from_buffer(stream, dest + nread, total - nread);
	}

	return nread;
}

static void _before_read(FILE *stream)
{
	_lazy_alloc_buffer(stream);

	/* Make sure no data is pending write. */
	if (stream->buffer_state == _bs_write)
		_fflushbuf(stream);
}

/** Read from a stream.
 *
 * @param dest   Destination buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to read.
 * @param stream Pointer to the stream.
 *
 */
size_t fread(void *dest, size_t size, size_t nmemb, FILE *stream)
{
	if (size == 0 || nmemb == 0)
		return 0;

	flockfile(stream);
	_before_read(stream);
	size_t nread = _fread(stream, dest, size * nmemb);
	funlockfile(stream);
	return nread / size;
}

static size_t _write_to_buffer(FILE *stream, const void *src, size_t size)
{
	assert(stream->buffer_state != _bs_read);
	size_t n = min(_buffer_free(stream), size);
	memcpy(stream->buffer_tail, src, n);
	stream->buffer_tail += n;
	stream->buffer_state = _bs_write;
	return n;
}

static size_t _fwrite(FILE *stream, const void *buf, size_t total)
{
	size_t nwritten = 0;

	if (stream->error)
		return 0;

	/* First, fill & empty buffer. */
	if (!_buffer_empty(stream)) {
		nwritten += _write_to_buffer(stream, buf, total);
		if (nwritten == total)
			return nwritten;

		assert(_buffer_full(stream));

		_fflushbuf(stream);
		if (stream->error)
			return nwritten;
	}

	assert(_buffer_empty(stream));

	/*
	 * Now, with empty buffer, write directly to backend as long as the output
	 * exceeds buffer capacity.
	 */
	while (total - nwritten >= _buffer_size(stream)) {
		nwritten += _write(stream, buf + nwritten, total - nwritten);
		if (stream->error)
			return nwritten;
	}

	/* Write the remaining output to buffer. */
	if (nwritten < total)
		nwritten += _write_to_buffer(stream, buf + nwritten, total - nwritten);

	assert(nwritten == total);

	return nwritten;
}

static void _before_write(FILE *stream)
{
	_lazy_alloc_buffer(stream);

	/* Make sure buffer contains no prefetched data. */
	if (stream->buffer_state == _bs_read)
		_fflushbuf(stream);
}

static void _after_write(FILE* stream)
{
	/* If not buffered stream, ensure the buffer is clear on return. */
	if (!_buffer_empty(stream) && stream->btype == _IONBF)
		_fflushbuf(stream);
	else if (stream->btype == _IOLBF)
		_flush_to_newline(stream);
}

/** Write to a stream.
 *
 * @param buf    Source buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to write.
 * @param stream Pointer to the stream.
 *
 */
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream)
{
	if (size == 0 || nmemb == 0)
		return 0;

	flockfile(stream);
	_before_write(stream);
	size_t nwritten = _fwrite(stream, buf, size * nmemb);
	_after_write(stream);
	funlockfile(stream);
	return nwritten / size;
}

wint_t fputwc(wchar_t wc, FILE *stream)
{
	char buf[STR_BOUNDS(1)];
	size_t sz = 0;

	if (chr_encode(wc, buf, &sz, STR_BOUNDS(1)) != EOK) {
		errno = EILSEQ;
		return WEOF;
	}

	size_t wr = fwrite(buf, 1, sz, stream);
	if (wr < sz)
		return WEOF;

	return wc;
}

wint_t fputuc(char32_t wc, FILE *stream)
{
	char buf[STR_BOUNDS(1)];
	size_t sz = 0;

	if (chr_encode(wc, buf, &sz, STR_BOUNDS(1)) != EOK) {
		errno = EILSEQ;
		return WEOF;
	}

	size_t wr = fwrite(buf, 1, sz, stream);
	if (wr < sz)
		return WEOF;

	return wc;
}

wint_t putwchar(wchar_t wc)
{
	return fputwc(wc, stdout);
}

wint_t putuchar(char32_t wc)
{
	return fputuc(wc, stdout);
}

int fputc(int c, FILE *stream)
{
	unsigned char b;
	size_t wr;

	b = (unsigned char) c;
	wr = fwrite(&b, sizeof(b), 1, stream);
	if (wr < 1)
		return EOF;

	return b;
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int fputs(const char *str, FILE *stream)
{
	(void) fwrite(str, str_size(str), 1, stream);
	if (ferror(stream))
		return EOF;
	return 0;
}

int puts(const char *str)
{
	flockfile(stdout);
	_before_write(stdout);
	_fwrite(stdout, str, str_size(str));
	uint8_t b = '\n';
	_fwrite(stdout, &b, 1);
	bool error = ferror(stdout);
	funlockfile(stdout);

	return error ? EOF : 0;
}

int fgetc(FILE *stream)
{
	unsigned char c;
	if (fread(&c, sizeof(c), 1, stream) == 0)
		return EOF;
	else
		return (int) c;
}

char *fgets(char *str, int size, FILE *stream)
{
	flockfile(stream);
	_before_read(stream);

	int idx = 0;
	while (idx < size - 1) {
		unsigned char c;
		if (_fread(stream, &c, sizeof(c)) == 0)
			break;

		str[idx++] = c;

		if (c == '\n')
			break;
	}

	bool error = ferror(stream);

	funlockfile(stream);

	if (error || idx == 0)
		return NULL;

	str[idx] = '\0';
	return str;
}

int getchar(void)
{
	return fgetc(stdin);
}

int ungetc(int c, FILE *stream)
{
	if (c == EOF)
		return EOF;

	flockfile(stream);

	if (stream->buffer_state == _bs_write) {
		funlockfile(stream);
		return EOF;
	}

	if (_buffer_empty(stream)) {
		/* Maximize space for ungetc() */
		stream->buffer_head = stream->buffer_end;
		stream->buffer_tail = stream->buffer_end;
	}

	if (stream->buffer_head == stream->buffer) {
		/* No space for ungetc() */
		funlockfile(stream);
		return EOF;
	}

	stream->buffer_head--;
	stream->buffer_head[0] = (uint8_t) c;
	stream->eof = false;
	funlockfile(stream);
	return (uint8_t) c;
}

int fseek64(FILE *stream, off64_t offset, int whence)
{
	if (stream->ops->seek == NULL) {
		errno = EINVAL;
		return -1;
	}

	flockfile(stream);

	_fflushbuf(stream);
	if (!_buffer_empty()) {
		/* errno was set by _fflushbuf() */
		funlockfile(stream);
		return -1;
	}

	errno_t rc = stream->ops->seek(stream, offset, whence);
	if (rc != EOK) {
		errno = rc;
		stream->error = true;
	}

	funlockfile(stream);

	return rc == EOK ? 0 : -1;
}

off64_t ftell64(FILE *stream)
{
	if (stream->ops->tell == NULL) {
		errno = EINVAL;
		return EOF;
	}

	flockfile(stream);

	size_t buffer_bytes = _buffer_used(stream);
	off64_t pos = stream->ops->tell(stream);

	if (stream->buffer_state == _bs_read)
		pos -= buffer_bytes;
	else
		pos += buffer_bytes;

	funlockfile(stream);
	return pos;
}

int fseek(FILE *stream, long offset, int whence)
{
	return fseek64(stream, offset, whence);
}

long ftell(FILE *stream)
{
	off64_t off = ftell64(stream);

	/* The native position is too large for the C99-ish interface. */
	if (off > LONG_MAX) {
		errno = EOVERFLOW;
		return -1;
	}

	return off;
}

void rewind(FILE *stream)
{
	(void) fseek(stream, 0, SEEK_SET);
}

static int _fflush(FILE *stream)
{
	_fflushbuf(stream);
	if (!_buffer_empty()) {
		/* errno was set by _fflushbuf() */
		return EOF;
	}

	if (stream->need_sync) {
		errno_t rc = stream->ops->flush(stream);
		if (rc != EOK) {
			errno = rc;
			stream->error = true;
			return EOF;
		}

		stream->need_sync = false;
	}

	return 0;
}

int fflush(FILE *stream)
{
	flockfile(stream);
	int ret = _fflush(stream);
	funlockfile(stream);
	return ret;
}

int feof(FILE *stream)
{
	return stream->eof;
}

int ferror(FILE *stream)
{
	return stream->error;
}

void clearerr(FILE *stream)
{
	stream->eof = false;
	stream->error = false;
}

void flockfile(FILE *stream)
{
	if (stream->ops->lock)
		stream->ops->lock(stream);
}

int ftrylockfile(FILE *stream)
{
	if (stream->ops->try_lock) {
		return stream->ops->try_lock(stream) ? 0 : -1;
	} else {
		return 0;
	}
}

void funlockfile(FILE *stream)
{
	if (stream->ops->unlock)
		stream->ops->unlock(stream);
}

/** @}
 */
