/*
 * Copyright (c) 2011 Martin Decky
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

#ifndef _LIBC_PRIVATE_STDIO_H_
#define _LIBC_PRIVATE_STDIO_H_

#include <adt/list.h>
#include <stdio.h>
#include <async.h>
#include <stddef.h>
#include <offset.h>

/** Maximum characters that can be pushed back by ungetc() */
#define UNGETC_MAX 1

/** Stream operations */
typedef struct {
	/** Read from stream */
	size_t (*read)(FILE *stream, void *buf, size_t size);
	/** Write to stream */
	size_t (*write)(FILE *stream, const void *buf, size_t size);
	/** Seek on the stream */
	errno_t (*seek)(FILE *stream, int64_t offset, int whence);
	/** Retrieve current stream offset */
	int64_t (*tell)(FILE *stream);
	/** Close stream */
	errno_t (*close)(FILE *stream);
	/** Flush stream */
	int (*flush)(FILE *stream);
} __stream_ops_t;

enum __buffer_state {
	/** Buffer is empty */
	_bs_empty,

	/** Buffer contains data to be written */
	_bs_write,

	/** Buffer contains prefetched data for reading */
	_bs_read
};

struct _IO_FILE {
	/** Linked list pointer. */
	link_t link;

	/** Stream operations */
	__stream_ops_t *ops;

	/** Buffer */
	uint8_t *buf;

	/** Buffer I/O pointer */
	uint8_t *buf_head;

	/** Points to end of occupied space when in read mode. */
	uint8_t *buf_tail;

	/** Buffer size */
	size_t buf_size;

	/** Error indicator. */
	int error;

	/** End-of-file indicator. */
	int eof;

	/**
	 * Non-zero if the stream needs sync on fflush(). XXX change
	 * console semantics so that sync is not needed.
	 */
	int need_sync;

	/** Number of pushed back characters */
	int ungetc_chars;

	/** Buffering type */
	enum __buffer_type btype;

	/** Buffer state */
	enum __buffer_state buf_state;

	/** Pushed back characters */
	uint8_t ungetc_buf[UNGETC_MAX];

	/** Non-generic userdata. Contents defined by each user. */
	struct _IO_FILE_user_data user;
};

#endif

/** @}
 */
