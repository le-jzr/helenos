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

#include <stddef.h>
#include <ipc/ipc.h>

typedef enum ipc_node_method {
	ipc_node_copy,
	ipc_node_fsprobe,
	ipc_node_fstypes,
	ipc_node_mount,
	ipc_node_move,
	ipc_node_open,
	ipc_node_stat,
	ipc_node_statfs,
	ipc_node_sync,
	ipc_node_unlink,
	ipc_node_unmount,
	ipc_node_walk,
} ipc_node_method_t;

typedef enum ipc_file_method {
	ipc_file_method_read,
	ipc_file_method_write,
	ipc_file_method_reopen,
	ipc_file_method_resize,
	ipc_file_method_stat,
	ipc_file_method_sync,
} ipc_file_method_t;

typedef enum {
	ipc_file_open_regular,
	// TODO
} ipc_file_open_flags_t;

typedef struct ipc_node_ops {

} ipc_node_ops_t;

typedef struct ipc_file ipc_file_t;

typedef struct ipc_file_ops {
	void (*read)(void *data, ipc_buffer_t *buf, size_t req, ipc_endpoint_t *ret);
	void (*write)(void *data, ipc_blob_t *buf, size_t req, ipc_endpoint_t *ret);
	void (*reopen)(void *data, ipc_file_open_flags_t flags, ipc_endpoint_t *ret);
	void (*resize)(void *data, size_t size, ipc_endpoint_t *ret);
	void (*stat)(void *data, ipc_endpoint_t *ret);
	//sync,
	//write,
} ipc_file_ops_t;

size_t ipc_file_read(ipc_file_t *, int64_t offset, void *dst, size_t dst_len, size_t min_len);
size_t ipc_file_write(ipc_file_t *, int64_t offset, const void *src, size_t src_len, size_t min_len);

ipc_fsnode_mount_result_t ipc_fsnode_bind(ipc_fsnode_t *mp, ipc_fsnode_t *mountee);
ipc_fsnode_t *ipc_fsnode_unbind(ipc_fsnode_t *mp);
ipc_fsnode_attach_result_t ipc_fsnode_attach(const char *fs_name, service_id_t serv, const char *opts,
    unsigned int flags, unsigned int instance, ipc_fsnode_t **out_root);

#endif

/** @}
 */
