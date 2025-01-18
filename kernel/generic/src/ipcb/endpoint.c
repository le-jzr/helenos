
#include <ipc/new.h>

#include <stdatomic.h>
#include <stdalign.h>

#include <align.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <proc/thread.h>
#include <mem.h>

typedef struct ipc_buffer_weakref ipc_buffer_weakref_t;

/** Weak reference used by endpoints to access their parent buffer. */
struct ipc_buffer_weakref {
	refcount_t refcount;
	atomic_int access;
	_Atomic(ipc_buffer_t *) buffer;
};

/**
 * IPC buffer object.
 *
 * Used for receiving IPC messages. Every IPC endpoint is associated with
 * one such buffer. The owner of the buffer can read arriving messages from
 * it, and create endpoints that other tasks can use to send messages into it.
 */
struct ipc_buffer {
	kobj_t kobj;

	// constant after creation
	task_t *task;
	mem_t *mem;
	ipc_buffer_weakref_t *weakref;

	// Synchronize access by readers/writers.
	// Only one reader and one writer are allowed at a time.
	waitq_t read_queue;
	waitq_t write_queue;

	irq_spinlock_t lock;

	size_t size;
	size_t max_message_len;

	// All reservations whose generation is <= prefix_gen are
	// in the tail reservation area.
	size_t prefix_gen;
	// Last assigned generation for prefix area reservation.
	size_t gen_counter;

	// Portion of data starting at zero offset.
	size_t data_prefix_top;

	// Bytes reserved before data_tail_bottom, for guaranteed nonblocking
	// operation of some writes.
	size_t data_prefix_reservation_size;

	// Start of data in the middle of the buffer.
	// Also offset of the next read.
	size_t data_tail_bottom;

	// End of data in the middle of the buffer.
	// When data_tail_bottom equals data_tail_top after a read, the tail
	// is reset to point to prefix.
	size_t data_tail_top;

	// Bytes reserved at the end of the buffer, for guaranteed nonblocking
	// operation of some writes.
	size_t data_tail_reservation_size;

	// Size returned from the most recent call to ipc_buffer_read(),
	// used to release the right amount of memory in ipc_buffer_end_read().
	size_t current_read_size;

	// Used by writers and readers to wait for changes in the amount of
	// free space (or, conversely, available messages) in the buffer.
	// Only one thread can be waiting as such at any given time.
	thread_t *waiting_for_change;

	bool destroyed;
};

struct ipc_endpoint {
	// Endpoints don't have a separate lock since all data is either constant
	// or synchronized internally.
	kobj_t kobj;
	ipc_buffer_weakref_t *buffer;
	uintptr_t userdata;
	size_t max_len;

	size_t gen;
	atomic_size_t reservation;
};

static slab_cache_t *slab_ipc_buffer_cache;
static slab_cache_t *slab_ipc_endpoint_cache;
static slab_cache_t *slab_ipc_buffer_weakref_cache;

void ipc_buffer_initialize(void)
{
	slab_ipc_buffer_cache = slab_cache_create("ipc_buffer_t",
			sizeof(ipc_buffer_t), alignof(ipc_buffer_t), NULL, NULL, 0);
	slab_ipc_endpoint_cache = slab_cache_create("ipc_endpoint_t",
			sizeof(ipc_endpoint_t), alignof(ipc_endpoint_t), NULL, NULL, 0);
	slab_ipc_buffer_weakref_cache = slab_cache_create("ipc_buffer_weakref_t",
			sizeof(ipc_buffer_weakref_t), alignof(ipc_buffer_weakref_t),
			NULL, NULL, 0);
}

/* IPC buffer/endpoint implementation. */

static void weakref_free(void *wref)
{
	slab_free(slab_ipc_buffer_weakref_cache, wref);
}

const static kobj_class_t kobj_class_weakref = {
	.destroy = weakref_free,
};

#define KOBJ_CLASS_WEAKREF (&kobj_class_weakref)

static ipc_buffer_weakref_t *weakref_create(ipc_buffer_t *buffer)
{
	ipc_buffer_weakref_t *wref = slab_alloc(slab_ipc_buffer_weakref_cache, 0);
	kobj_initialize(&wref->kobj, KOBJ_CLASS_WEAKREF);
	atomic_init(&wref->access, 0);
	atomic_init(&wref->buffer, buffer);
	return wref;
}

static ipc_buffer_t *weakref_get(ipc_buffer_weakref_t *wref)
{
	if (!wref)
		return NULL;

	// Avoid incrementing access (and possibly delaying buffer destructor)
	// when the buffer is being destroyed.
	if (atomic_load_explicit(&wref->buffer, memory_order_relaxed) == NULL)
		return NULL;

	// Ensure that the buffer can't be deallocated while we're using it.

	// This fetch_add operation synchronizes with the same in weakref_destroy().
	// If this one happens first, then weakref_destroy() sees non-zero value
	// of access and consequently waits for weakref_put().
	// If the other happens first, then by acquire semantics here and release
	// semantics there, value of NULL is necessarily loaded from buffer.
	(void) atomic_fetch_add_explicit(&wref->access, 1, memory_order_acquire);

	ipc_buffer_t *b = atomic_load_explicit(&wref->buffer, memory_order_relaxed);

	// The buffer may have been destroyed already.
	if (!b)
		(void) atomic_fetch_sub_explicit(&wref->access, 1, memory_order_release);

	return b;
}

static void weakref_release(ipc_buffer_weakref_t *wref, ipc_buffer_t *buffer)
{
	// Just check to make sure we're in the right weakref.
	ipc_buffer_t *b = atomic_load_explicit(&wref->buffer, memory_order_relaxed);
	assert(b == buffer || b == NULL);

	// Synchronizes with atomic operations in weakref_destroy(), ensuring
	// any operations made by this thread until now are seen by the caller
	// of weakref_destroy().
	(void) atomic_fetch_sub_explicit(&wref->access, 1, memory_order_release);
}

static void weakref_put(ipc_buffer_weakref_t *wref)
{
	kobj_put(&wref->kobj);
}

static void weakref_destroy(ipc_buffer_weakref_t *wref)
{
	atomic_store_explicit(&wref->buffer, NULL, memory_order_relaxed);

	// One read with acq_rel semantics to make sure we properly synchronize
	// with both weakref_get() and weakref_put().
	if (atomic_fetch_add_explicit(&wref->access, 0, memory_order_acq_rel) == 0)
		return;

	// Wait for all functions using the reference to release it.
	// This is just acquire since we only need to synchronize with weakref_put.
	// We assume the caller already woke up/interrupted all that are sleeping.
	while (atomic_load_explicit(&wref->access, memory_order_acquire) > 0)
		spin_loop_body();

	weakref_put(wref);
}

struct message {
	uintptr_t total_bytes;
	uintptr_t handles;
	uintptr_t userdata;
	char data[];
};

static errno_t wait_for_data(ipc_buffer_t *buffer, deadline_t deadline)
{
	bool timed_out = false;

	while (buffer->data_tail_bottom == buffer->data_tail_top) {

		if (timed_out)
			return ETIMEOUT;

		if (!thread_wait_start())
			return EINTR;

		assert(buffer->waiting_for_change == NULL);
		buffer->waiting_for_change = THREAD;

		irq_spinlock_unlock(&buffer->lock, true);
		timed_out = thread_wait_finish(deadline);
		irq_spinlock_lock(&buffer->lock, true);

		assert(buffer->waiting_for_change == NULL
				|| buffer->waiting_for_change == THREAD);

		buffer->waiting_for_change = NULL;
	}

	return EOK;
}

// Returns offset to the next IPC message in the buffer.
// If no message is currently present, blocks until one arrives.
// Once the caller has finished reading the message, it must call
// ipc_buffer_end_read() to release the memory.
//
// Only one thread is allowed to be reading from the buffer at a time,
// to avoid complicated tracking of which buffer portions are ready for reuse.
// For short messages in a multi-threaded handling situation, it's recommended
// the caller immediately makes a local copy and calls ipc_buffer_end_read()
// before continuing to handle the message.
//
errno_t ipc_buffer_read(ipc_buffer_t *buffer, uintptr_t *out_offset, deadline_t deadline)
{
	errno_t rc = waitq_sleep_until_interruptible(&buffer->read_queue, deadline);
	if (rc != EOK)
		return rc;

	irq_spinlock_lock(&buffer->lock, true);

	rc = wait_for_data(buffer, deadline);

	if (rc == EOK) {
		assert(buffer->data_tail_bottom + sizeof(struct message) <= buffer->data_tail_top);
		assert(buffer->current_read_size == 0);

		buffer->current_read_size =
				mem_read_word(buffer->mem, buffer->data_tail_bottom);

		assert(buffer->current_read_size >= sizeof(struct message));
		assert(buffer->current_read_size <= buffer->data_tail_top);
		assert(buffer->data_tail_bottom <= buffer->data_tail_top - buffer->current_read_size);

		*out_offset = buffer->data_tail_bottom;
	}

	irq_spinlock_unlock(&buffer->lock, true);

	// Returning still holding read_queue token.
	// Will be released in buffer_end_read().

	return rc;
}

void ipc_buffer_end_read(ipc_buffer_t *buffer)
{
	// The code must keep in mind that this call may be called by userspace
	// without any association with buffer_read(). Kernel must not crash in
	// that case.

	irq_spinlock_lock(&buffer->lock, true);

	if (buffer->current_read_size == 0) {
		irq_spinlock_unlock(&buffer->lock, true);
		return;
	}

	buffer->data_tail_bottom += buffer->current_read_size;
	buffer->current_read_size = 0;

	assert(buffer->data_tail_bottom <= buffer->data_tail_top);

	if (buffer->data_tail_bottom >= buffer->data_tail_top) {
		// The tail has been emptied. Reset it to prefix.
		buffer->data_tail_bottom = 0;
		buffer->data_tail_top = buffer->data_prefix_top;
		buffer->data_prefix_top = 0;

		// Transfer prefix reservations.
		buffer->data_tail_reservation_size += buffer->data_prefix_reservation_size;
		buffer->data_prefix_reservation_size = 0;
		buffer->prefix_gen = buffer->gen_counter;
	}

	thread_t *waiting = buffer->waiting_for_change;
	buffer->waiting_for_change = NULL;

	irq_spinlock_unlock(&buffer->lock, true);

	if (waiting)
		thread_wakeup(waiting);

	waitq_wake_one(&buffer->read_queue);
}

static void ipc_buffer_destroy(void *arg)
{
	ipc_buffer_t *buffer = arg;

	// The buffer may still be accessed through endpoints.
	irq_spinlock_lock(&buffer->lock, true);
	// Mark buffer as undergoing destruction.
	buffer->destroyed = true;
	thread_t *waiting = buffer->waiting_for_change;
	buffer->waiting_for_change = NULL;
	irq_spinlock_unlock(&buffer->lock, true);

	// Wake up everyone.
	if (waiting)
		thread_wakeup(waiting);

	waitq_close(&buffer->read_queue);
	waitq_close(&buffer->write_queue);

	// Destroy weakref and wait for everyone currently accessing the buffer to finish.
	weakref_destroy(buffer->weakref);

	// Destroy/unref everything else.
	mem_put(buffer->mem);

	slab_free(slab_ipc_buffer_cache, buffer);
}

kobj_class_t kobj_class_ipc_buffer = {
	.destroy = ipc_buffer_destroy,
};

ipc_buffer_t *ipc_buffer_create(size_t size, size_t max_message_len)
{
	// Add internal header data to max_message_len.
	if (SIZE_MAX / max_message_len < 2)
		return NULL;

	max_message_len = ALIGN_UP(max_message_len + sizeof(struct message),
		alignof(struct message));

	ipc_buffer_t *b = slab_alloc(slab_ipc_buffer_cache, 0);
	if (!b)
		return NULL;

	memset(b, 0, sizeof(*b));

	b->weakref = weakref_create(b);
	if (!b->weakref) {
		slab_free(slab_ipc_buffer_cache, b);
		return NULL;
	}

	b->size = ALIGN_UP(size, PAGE_SIZE);
	b->mem = mem_create(b->size, PAGE_SIZE, AS_AREA_READ | AS_AREA_CACHEABLE);
	if (!b->mem) {
		kobj_put(&b->weakref->kobj);
		slab_free(slab_ipc_buffer_cache, b);
		return NULL;
	}

	kobj_initialize(&b->kobj, KOBJ_CLASS_IPC_BUFFER);
	irq_spinlock_initialize(&b->lock, "ipc_buffer_t::lock");

	b->max_message_len = ALIGN_UP(max_message_len, alignof(struct message));
	waitq_initialize(&b->read_queue);
	waitq_initialize(&b->write_queue);
	return b;
}

static inline size_t min_buffer_size(const struct ipc_write_data *data)
{
	return sizeof(struct message)
		+ data->handles_len * sizeof(data->handles[0])
		+ data->data1_len;
}

static inline size_t max_buffer_size_unaligned(const struct ipc_write_data *data)
{
	return sizeof(struct message)
			+ data->handles_len * sizeof(data->handles[0])
			+ data->data1_len
			+ data->data2_len;
}

static inline size_t max_buffer_size(const struct ipc_write_data *data)
{
	return ALIGN_UP(max_buffer_size_unaligned(data), alignof(struct message));
}

static errno_t write_internal(ipc_buffer_t *buffer,
		const struct ipc_write_data *data, size_t size, size_t buffer_offset)
{
	// TODO
	panic("unimplemented");
}

static bool buffer_try_write(ipc_buffer_t *buffer,
		const struct ipc_write_data *data, size_t *written, size_t max_len)
{
	size_t min_size = min_buffer_size(data);
	size_t wanted_size = min(max_buffer_size_unaligned(data), max_len);

	assert(buffer->data_prefix_reservation_size <= buffer->data_tail_bottom);
	assert(buffer->data_prefix_top <= buffer->data_tail_bottom - buffer->data_prefix_reservation_size);
	assert(buffer->data_tail_bottom <= buffer->data_tail_top);
	assert(buffer->data_tail_reservation_size <= buffer->size);
	assert(buffer->data_tail_top <= buffer->size - buffer->data_tail_reservation_size);

	size_t available_prefix = buffer->data_tail_bottom - buffer->data_prefix_top
			- buffer->data_prefix_reservation_size;
	size_t available_tail = buffer->size - buffer->data_tail_top
			- buffer->data_tail_reservation_size;

	// Prefer writing to the beginning of the buffer, but if tail would allow
	// more data being written, write there instead.

	if (available_prefix >= wanted_size
			|| (available_prefix >= min_size && available_prefix >= available_tail)) {
		size_t size = min(wanted_size, available_prefix);

		// Can fulfill right now.
		write_internal(buffer, data, size, buffer->data_prefix_top);
		buffer->data_prefix_top += size;
		*written = size - min_size;

		assert(buffer->data_prefix_top <= buffer->data_tail_bottom
				- buffer->data_prefix_reservation_size);
		return true;
	}

	if (available_tail >= min_size) {
		size_t size = min(wanted_size, available_tail);

		write_internal(buffer, data, size, buffer->data_tail_top);
		buffer->data_tail_top += size;
		*written = size - min_size;

		assert(buffer->data_tail_top <= buffer->size - buffer->data_tail_reservation_size);
		return true;
	}

	return false;
}

static ipc_buffer_t *endpoint_buffer_get(ipc_endpoint_t *ep)
{
	return weakref_get(ep->buffer);
}

static void endpoint_buffer_put(ipc_endpoint_t *ep, ipc_buffer_t *buffer)
{
	weakref_release(ep->buffer, buffer);
}

static size_t prefix_reservation_available(ipc_buffer_t *buffer)
{
	size_t reserve_bottom = buffer->data_tail_bottom - buffer->data_prefix_reservation_size;
	assert(reserve_bottom >= buffer->data_prefix_top);
	size_t free_space = reserve_bottom - buffer->data_prefix_top;

	assert(reserve_bottom >= buffer->max_message_len);
	size_t reservable_space = reserve_bottom - buffer->max_message_len;

	return min(free_space, reservable_space);
}

static size_t tail_reservation_available(ipc_buffer_t *buffer)
{
	size_t reserve_bottom = buffer->size - buffer->data_tail_reservation_size;
	assert(reserve_bottom >= buffer->data_tail_top);
	size_t free_space = reserve_bottom - buffer->data_tail_top;

	assert(reserve_bottom >= buffer->max_message_len);
	size_t reservable_space = reserve_bottom - buffer->max_message_len;

	return min(free_space, reservable_space);
}

ipc_endpoint_t *ipc_endpoint_create(ipc_buffer_t *buffer,
		uintptr_t userdata, size_t reserve, size_t max_message_len)
{
	if (SIZE_MAX / max_message_len < 2)
		return NULL;

	if (SIZE_MAX / reserve < 2)
		return NULL;

	ipc_endpoint_t *ep = slab_alloc(slab_ipc_endpoint_cache, 0);
	if (!ep)
		return NULL;

	// Add header data to reserve size and max_message_len.
	if (reserve > 0)
		reserve = ALIGN_UP(reserve + sizeof(struct message), alignof(struct message));

	if (max_message_len > 0)
		max_message_len = ALIGN_UP(max_message_len + sizeof(struct message),
			alignof(struct message));

	irq_spinlock_lock(&buffer->lock, true);

	if (max_message_len == 0)
		max_message_len = buffer->max_message_len;

	// Endpoint can request max_message_len that's greater than its buffer's.
	if (max_message_len > buffer->max_message_len) {
		irq_spinlock_unlock(&buffer->lock, true);
		slab_free(slab_ipc_endpoint_cache, ep);
		return NULL;
	}

	size_t gen;

	if (reserve <= prefix_reservation_available(buffer)) {
		buffer->gen_counter++;
		gen = buffer->gen_counter;
		buffer->data_prefix_reservation_size += reserve;
	} else if (reserve <= tail_reservation_available(buffer)) {
		gen = 0;
		buffer->data_tail_reservation_size += reserve;
	} else {
		// Not enough space to make a reservation.
		irq_spinlock_unlock(&buffer->lock, true);
		slab_free(slab_ipc_endpoint_cache, ep);
		return NULL;
	}

	irq_spinlock_unlock(&buffer->lock, true);

	kobj_initialize(&ep->kobj, KOBJ_CLASS_IPC_ENDPOINT);

	kobj_ref(&buffer->weakref->kobj);
	ep->buffer = buffer->weakref;

	ep->userdata = userdata;
	ep->max_len = max_message_len;
	ep->gen = gen;
	atomic_init(&ep->reservation, reserve);

	return ep;
}

errno_t ipc_endpoint_write(ipc_endpoint_t *ep,
		const struct ipc_write_data *data, size_t *written, deadline_t deadline)
{
	size_t needed_size = min_buffer_size(data);
	if (needed_size > ep->max_len) {
		// EP doesn't allow a request of this size.
		return EINVAL;
	}

	ipc_buffer_t *buffer = endpoint_buffer_get(ep);
	if (!buffer) {
		// The buffer has been destroyed.
		return EHANGUP;
	}

	size_t reservation = 0;

	// On most architectures, a relaxed load should be as cheap as non-atomic
	// memory load, so do that first and only make atomic exchange if needed.
	if (atomic_load_explicit(&ep->reservation, memory_order_relaxed) > 0) {
		// We don't care if we need all of it. First come wins all.
		reservation = atomic_exchange_explicit(&ep->reservation, 0, memory_order_relaxed);
	}

	bool reservation_used = needed_size <= reservation;
	bool nonblocking = (deadline == 0);
	// Skip queue if we have reserved capacity or can't wait.
	bool bypass_write_queue = reservation_used || nonblocking;

	if (!bypass_write_queue)
		waitq_sleep_until_interruptible(&buffer->write_queue, deadline);

	irq_spinlock_lock(&buffer->lock, true);

	assert(ep->max_len <= buffer->max_message_len);
	assert(buffer->max_message_len <= buffer->reserved_bottom);

	// Eat our reservation if we have one.
	buffer->reserved_bottom += reservation;

	assert(buffer->reserved_bottom <= buffer->size);

	bool timed_out = false;
	errno_t rc = EOK;

	while (!buffer_try_write(buffer, data, written, ep->max_len)) {
		// Not enough space available. Wait for reader to empty some of it.

		assert(!reservation_used);

		if (nonblocking || timed_out) {
			rc = ETIMEOUT;
			break;
		}

		if (buffer->destroyed) {
			// Buffer has been destroyed sometime after we got the reference.
			// Destructor is waiting for us to exit.
			rc = EHANGUP;
			break;
		}

		// We only bypass the write queue if we know for certain we won't wait.
		assert(!bypass_write_queue);

		// This must be true, because if there was a waiting writer,
		// he'd need to have the write token we own, and if there was
		// a waiting reader, that would mean the buffer is completely empty.
		assert(buffer->waiting_for_change == NULL);

		if (thread_wait_start()) {
			buffer->waiting_for_change = THREAD;
		} else {
			rc = EINTR;
			break;
		}

		irq_spinlock_unlock(&buffer->lock, true);
		timed_out = thread_wait_finish(deadline);
		irq_spinlock_lock(&buffer->lock, true);

		if (buffer->waiting_for_change != NULL) {
			// In case of timeout or interrupt.
			assert(buffer->waiting_for_change == THREAD);
			buffer->waiting_for_change = NULL;
		}
	}

	if (rc == EOK && buffer->waiting_for_change) {
		thread_wakeup(buffer->waiting_for_change);
		buffer->waiting_for_change = NULL;
	}

	irq_spinlock_unlock(&buffer->lock, true);

	// Let the next writer in.
	if (!bypass_write_queue)
		waitq_wake_one(&buffer->write_queue);

	endpoint_buffer_put(ep, buffer);

	return rc;
}
