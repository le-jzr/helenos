
#ifndef KERNEL_IPC_NEW_H_
#define KERNEL_IPC_NEW_H_

#include <errno.h>
#include <time/timeout.h>
#include <cap/cap.h>

typedef struct ipc_buffer ipc_buffer_t;
typedef struct ipc_endpoint ipc_endpoint_t;

extern kobj_class_t kobj_class_ipc_buffer;
#define KOBJ_CLASS_IPC_BUFFER (&kobj_class_ipc_buffer)

extern kobj_class_t kobj_class_ipc_endpoint;
#define KOBJ_CLASS_IPC_ENDPOINT (&kobj_class_ipc_endpoint)

typedef struct ipc_write_data {
	uintptr_t *handles;
	size_t handles_len;

	// The mandatory part of write.
	// A successful write will have written at least the handles and data1.
	uintptr_t data1;
	size_t data1_len;

	// The optional part of write.
	// A successful write will have written only as much of data2 as could fit
	// into the buffer (possibly even 0 bytes).
	// data1 and data2 do not have to be adjacent in memory.
	uintptr_t data2;
	size_t data2_len;

	deadline_t deadline;
} ipc_write_data_t;

void ipc_buffer_initialize(void);

errno_t ipc_buffer_read(ipc_buffer_t *, uintptr_t *, deadline_t);
void ipc_buffer_end_read(ipc_buffer_t *);
ipc_buffer_t *ipc_buffer_create(size_t, size_t);

ipc_endpoint_t *ipc_endpoint_create(ipc_buffer_t *buffer, uintptr_t userdata,
		size_t reserve, size_t max_message_len);
errno_t ipc_endpoint_write(ipc_endpoint_t *, const ipc_write_data_t *,
		size_t *, deadline_t);

#endif
