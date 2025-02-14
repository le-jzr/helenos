/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

/** @addtogroup protocol
 * @{
 */

#ifndef _LIBC_PROTOCOL_FILE_H_
#define _LIBC_PROTOCOL_FILE_H_

typedef enum {
	// TODO
} ipc_file_open_flags_t;

typedef struct ipc_node_ops {

} ipc_node_ops_t;

typedef struct ipc_file_ops {
	void (*read)(void *data, ipc_buffer_t *buf, size_t req, ipc_endpoint_t *ret),
	void (*reopen)(void *data, ipc_file_open_flags_t flags, ipc_endpoint_t *ret),
	void (*resize)(void *data, size_t size, ipc_endpoint_t *ret),
	void (*stat)(void *data, ipc_endpoint_t *ret),
	sync,
	write,
} ipc_file_ops_t;

size_t ipc_file_read(ipc_file_t *, void *dst, size_t dst_len);

#endif

/** @}
 */
