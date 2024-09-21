
#ifndef COMMON_STDIO_FILE_H_
#define COMMON_STDIO_FILE_H_

#include <stdbool.h>
#include <stddef.h>
#include <adt/list.h>

typedef size_t file_stream_handle_t;
typedef size_t file_lock_handle_t;

/* Operations used to implement <stdio.h> */
struct stdio_file_ops {
	/*
	 * Read up to `size` bytes from the stream designated by `f`,
	 * and return the number of bytes read. At least one byte must be read
	 * by a successful invocation, but it may be less than `size` bytes.
	 *
	 * Return 0 if and only if no bytes have been read.
	 * If an error occured, set `errno` to an appropriate value in addition to
	 * returning the number of bytes read. Do not change `errno` if returning
	 * 0 due to an end-of-file condition.
	 */
	size_t (*read)(file_stream_handle_t f, void *buf, size_t size, bool *error);

	/*
	 * Write up to `size` bytes to the stream designated by `f`,
	 * and return the number of bytes written. At least one byte must be written
	 * by a successful invocation, but it may be less than `size` bytes.
	 *
	 * Return 0 if no bytes have been written due to an error.
	 * If an error occured, set `errno` to an appropriate value in addition to
	 * returning 0.
	 */
	size_t (*write)(file_stream_handle_t f, const void *buf, size_t size, bool *error);

	int (*seek)(file_stream_handle_t f, long offset, int whence);

	/*
	 * lock/try_lock/unlock a mutex for synchronization.
	 * These functions guard every use of read, write and flush,
	 * and also ensure every big operation (such as printf()) occurs atomically.
	 *
	 * The lock must be reentrant (aka recursive).
	 */
	void (*lock)(file_lock_handle_t l);
	bool (*try_lock)(file_lock_handle_t l);
	void (*unlock)(file_lock_handle_t l);

	/*
	 * Flush the underlying stream, if appropriate.
	 * If the operation could not be finished due to an error,
	 * write an appropriate error code to `errno` and return -1.
	 * Otherwise, return 0.
	 */
	int (*flush)(file_stream_handle_t f);

	/* Close/destroy/deallocate the underlying stream and lock, as appropriate. */
	void (*close)(file_stream_handle_t f, file_lock_handle_t l);
};

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
	const struct stdio_file_ops *ops;

	/** Underlying stream. */
	file_stream_handle_t stream_handle;

	/** Underlying lock. */
	file_lock_handle_t lock_handle;

	/** Error indicator. */
	bool error;

	/** End-of-file indicator. */
	bool eof;

	/** Buffering type */
	enum __buffer_type btype;

	/** Buffer state */
	enum __buffer_state buffer_state;

	/** Buffer */
	uint8_t *buffer;
	uint8_t *buffer_end;
	uint8_t *buffer_head;
	uint8_t *buffer_tail;

	bool allocated_buffer;

	size_t position_offset;
};

#endif /* COMMON_STDIO_FILE_H_ */
