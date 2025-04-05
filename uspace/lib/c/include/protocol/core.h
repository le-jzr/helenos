#pragma once

#include <abi/ipc_b.h>
#include <stddef.h>
#include <stdint.h>

#define IPC_QUEUE_DEFAULT ((ipc_queue_t *) nullptr)

typedef struct ipc_queue ipc_queue_t;
typedef struct ipc_blob ipc_blob_t;
typedef struct ipc_buffer ipc_buffer_t;
typedef struct ipc_endpoint ipc_endpoint_t;
typedef struct ipc_mem ipc_mem_t;

ipc_queue_t *ipc_queue_create(const char *name, size_t buffer_size);
void ipc_queue_reserve(ipc_queue_t *q, int msgs);
void ipc_queue_destroy(ipc_queue_t *q);
ipc_retval_t ipc_queue_read(ipc_queue_t *q, ipc_message_t *msg, size_t n);

ipc_blob_t *ipc_blob_create(const void *src, size_t src_len);
void ipc_blob_read(const ipc_blob_t *blob, void *dst, size_t len, size_t offset);
void ipc_blob_put(ipc_blob_t *blob);

ipc_buffer_t *ipc_buffer_create(size_t len);
void ipc_buffer_write(ipc_buffer_t *buf, const void *src, size_t len, size_t offset);
void ipc_buffer_consume(ipc_buffer_t *buf, void *dst, size_t len, size_t offset);
ipc_blob_t *ipc_buffer_finalize(ipc_buffer_t *buf);
void ipc_buffer_put(ipc_buffer_t *buf);

typedef enum ipc_mem_flags {
    ipc_mem_ro,
    ipc_mem_rw,
    ipc_mem_cow,
} ipc_mem_flags_t;

ipc_mem_t *ipc_mem_create(const void *src, size_t src_len, ipc_mem_flags_t flags);
void *ipc_mem_map(ipc_mem_t *mem, ipc_mem_flags_t flags);
void ipc_mem_unmap(ipc_mem_t *mem, void *vaddr);
void ipc_mem_put(ipc_mem_t *mem);

typedef struct ipc_endpoint_ops {
	void (*on_message)(void *self, ipc_message_t *msg);
	void (*on_destroy)(void *self);
} ipc_endpoint_ops_t;

ipc_endpoint_t *ipc_endpoint_create(ipc_queue_t *q, void *epdata);
void ipc_endpoint_put(ipc_endpoint_t *ep);
ipc_retval_t ipc_send(ipc_endpoint_t *ep, const ipc_message_t *msg);
