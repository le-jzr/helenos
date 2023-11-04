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

/* We only access the generic fields here. */
struct _IO_FILE_user_data {
};

#include "../private/stdio.h"

static void _ffillbuf(FILE *stream);
static void _fflushbuf(FILE *stream);

/** Set stream buffer. */
int setvbuf(FILE *stream, void *buf, int mode, size_t size)
{
	if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF)
		return -1;

	stream->btype = mode;
	stream->buf = buf;
	stream->buf_size = size;
	stream->buf_head = stream->buf;
	stream->buf_tail = stream->buf;
	stream->buf_state = _bs_empty;

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

/** Allocate stream buffer. */
static int _fallocbuf(FILE *stream)
{
	assert(stream->buf == NULL);

	stream->buf = malloc(stream->buf_size);
	if (stream->buf == NULL) {
		errno = ENOMEM;
		return EOF;
	}

	stream->buf_head = stream->buf;
	stream->buf_tail = stream->buf;
	return 0;
}

/** Read from a stream (unbuffered).
 *
 * @param buf    Destination buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to read.
 * @param stream Pointer to the stream.
 *
 * @return Number of elements successfully read. On error this is less than
 *         nmemb, stream error indicator is set and errno is set.
 */
static size_t _fread(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	return stream->ops->read(buf, size, nmemb, stream);
}

/** Write to a stream (unbuffered).
 *
 * @param buf    Source buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to write.
 * @param stream Pointer to the stream.
 *
 * @return Number of elements successfully written. On error this is less than
 *         nmemb, stream error indicator is set and errno is set.
 */
static size_t _fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream)
{
	size_t nwritten;

	if (size == 0 || nmemb == 0)
		return 0;

	nwritten = stream->ops->write(buf, size, nmemb, stream);

	if (nwritten > 0)
		stream->need_sync = true;

	return (nwritten / size);
}

static bool _buffer_empty(FILE *stream)
{
	return stream->buf_head == stream->buf_tail;
}

/** Read some data in stream buffer.
 *
 * On error, stream error indicator is set and errno is set.
 */
static void _ffillbuf(FILE *stream)
{
	assert(_buffer_empty(stream));

	size_t nread = _fread(stream->buf, 1, stream->buf_size, stream);

	stream->buf_head = stream->buf + nread;
	stream->buf_tail = stream->buf;
	stream->buf_state = _bs_read;
}

/** Write out stream buffer, do not sync stream. */
static void _fflushbuf(FILE *stream)
{
	size_t bytes_used;

	if ((!stream->buf) || (stream->btype == _IONBF) || (stream->error))
		return;

	bytes_used = stream->buf_head - stream->buf_tail;

	/* If buffer has prefetched read data, we need to seek back. */
	if (bytes_used > 0 && stream->buf_state == _bs_read && stream->ops->seek)
		stream->ops->seek(stream, -(int64_t) bytes_used, SEEK_CUR);

	/* If buffer has unwritten data, we need to write them out. */
	if (bytes_used > 0 && stream->buf_state == _bs_write) {
		(void) _fwrite(stream->buf_tail, 1, bytes_used, stream);
		/* On error stream error indicator and errno are set by _fwrite */
		if (stream->error)
			return;
	}

	stream->buf_head = stream->buf;
	stream->buf_tail = stream->buf;
	stream->buf_state = _bs_empty;
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
	uint8_t *dp;
	size_t bytes_left;
	size_t now;
	size_t data_avail;
	size_t total_read;
	size_t i;

	if (size == 0 || nmemb == 0)
		return 0;

	bytes_left = size * nmemb;
	total_read = 0;
	dp = (uint8_t *) dest;

	/* Bytes from ungetc() buffer */
	while (stream->ungetc_chars > 0 && bytes_left > 0) {
		*dp++ = stream->ungetc_buf[--stream->ungetc_chars];
		++total_read;
		--bytes_left;
	}

	/* If not buffered stream, read in directly. */
	if (stream->btype == _IONBF) {
		total_read += _fread(dest, 1, bytes_left, stream);
		return total_read / size;
	}

	/* Make sure no data is pending write. */
	if (stream->buf_state == _bs_write)
		_fflushbuf(stream);

	/* Perform lazy allocation of stream buffer. */
	if (stream->buf == NULL) {
		if (_fallocbuf(stream) != 0)
			return 0; /* Errno set by _fallocbuf(). */
	}

	while ((!stream->error) && (!stream->eof) && (bytes_left > 0)) {
		if (stream->buf_head == stream->buf_tail)
			_ffillbuf(stream);

		if (stream->error || stream->eof) {
			/* On error errno was set by _ffillbuf() */
			break;
		}

		data_avail = stream->buf_head - stream->buf_tail;

		if (bytes_left > data_avail)
			now = data_avail;
		else
			now = bytes_left;

		for (i = 0; i < now; i++) {
			dp[i] = stream->buf_tail[i];
		}

		dp += now;
		stream->buf_tail += now;
		bytes_left -= now;
		total_read += now;
	}

	return (total_read / size);
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
	uint8_t *data;
	size_t bytes_left;
	size_t now;
	size_t buf_free;
	size_t total_written;
	size_t i;
	uint8_t b;
	bool need_flush;

	if (size == 0 || nmemb == 0)
		return 0;

	/* If not buffered stream, write out directly. */
	if (stream->btype == _IONBF) {
		now = _fwrite(buf, size, nmemb, stream);
		fflush(stream);
		return now;
	}

	/* Make sure buffer contains no prefetched data. */
	if (stream->buf_state == _bs_read)
		_fflushbuf(stream);

	/* Perform lazy allocation of stream buffer. */
	if (stream->buf == NULL) {
		if (_fallocbuf(stream) != 0)
			return 0; /* Errno set by _fallocbuf(). */
	}

	data = (uint8_t *) buf;
	bytes_left = size * nmemb;
	total_written = 0;
	need_flush = false;

	while ((!stream->error) && (bytes_left > 0)) {
		buf_free = stream->buf_size - (stream->buf_head - stream->buf);
		if (bytes_left > buf_free)
			now = buf_free;
		else
			now = bytes_left;

		for (i = 0; i < now; i++) {
			b = data[i];
			stream->buf_head[i] = b;

			if ((b == '\n') && (stream->btype == _IOLBF))
				need_flush = true;
		}

		data += now;
		stream->buf_head += now;
		buf_free -= now;
		bytes_left -= now;
		total_written += now;
		stream->buf_state = _bs_write;

		if (buf_free == 0) {
			/* Only need to drain buffer. */
			_fflushbuf(stream);
			if (!stream->error)
				need_flush = false;
		}
	}

	if (need_flush)
		fflush(stream);

	return (total_written / size);
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
	if (fputs(str, stdout) < 0)
		return EOF;
	return putchar('\n');
}

int fgetc(FILE *stream)
{
	char c;

	/* This could be made faster by only flushing when needed. */
	if (stdout)
		fflush(stdout);
	if (stderr)
		fflush(stderr);

	if (fread(&c, sizeof(char), 1, stream) < sizeof(char))
		return EOF;

	return (int) c;
}

char *fgets(char *str, int size, FILE *stream)
{
	int c;
	int idx;

	idx = 0;
	while (idx < size - 1) {
		c = fgetc(stream);
		if (c == EOF)
			break;

		str[idx++] = c;

		if (c == '\n')
			break;
	}

	if (ferror(stream))
		return NULL;

	if (idx == 0)
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

	if (stream->ungetc_chars >= UNGETC_MAX)
		return EOF;

	stream->ungetc_buf[stream->ungetc_chars++] =
	    (uint8_t)c;

	stream->eof = false;
	return (uint8_t)c;
}

int fseek64(FILE *stream, off64_t offset, int whence)
{
	if (stream->ops->seek == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (stream->error)
		return -1;

	_fflushbuf(stream);
	if (stream->error) {
		/* errno was set by _fflushbuf() */
		return -1;
	}

	stream->ungetc_chars = 0;

	errno_t rc = stream->ops->seek(stream, offset, whence);

	if (rc != EOK) {
		errno = rc;
		stream->error = true;
		return -1;
	}

	return 0;
}

off64_t ftell64(FILE *stream)
{
	if (stream->ops->tell == NULL) {
		errno = EINVAL;
		return EOF;
	}

	if (stream->error)
		return EOF;

	_fflushbuf(stream);
	if (stream->error) {
		/* errno was set by _fflushbuf() */
		return EOF;
	}

	return stream->ops->tell(stream) - stream->ungetc_chars;
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

int fflush(FILE *stream)
{
	if (stream->error)
		return EOF;

	_fflushbuf(stream);
	if (stream->error) {
		/* errno was set by _fflushbuf() */
		return EOF;
	}

	if (stream->need_sync) {
		/**
		 * Better than syncing always, but probably still not the
		 * right thing to do.
		 */
		if (stream->ops->flush(stream) == EOF)
			return EOF;

		stream->need_sync = false;
	}

	return 0;
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

/** @}
 */
