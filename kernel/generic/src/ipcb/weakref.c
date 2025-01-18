
#include <ipc_b.h>

#include <stdalign.h>
#include <stdatomic.h>

#include <lib/refcount.h>
#include <mm/slab.h>
#include <proc/thread.h>

static slab_cache_t *slab_weakref_cache;

/** Weak reference used by endpoints to access their parent buffer. */
struct weakref {
	atomic_refcount_t refcount;
	atomic_int access;
	_Atomic(void *) inner;

	IRQ_SPINLOCK_DECLARE(destroyer_lock);
	thread_t *destroyer;
};

void weakref_init(void)
{
	slab_weakref_cache = slab_cache_create("ipc_buffer_weakref_t",
		sizeof(weakref_t), alignof(weakref_t), NULL, NULL, 0);
}

weakref_t *weakref_create(void *inner)
{
	weakref_t *ref = slab_alloc(slab_weakref_cache, FRAME_ATOMIC);
	if (!ref)
		return NULL;

	refcount_init(&ref->refcount);
	atomic_init(&ref->access, 1);
	atomic_init(&ref->inner, inner);
	atomic_init(&ref->destroyer, NULL);
	return ref;
}

weakref_t *weakref_ref(weakref_t *ref)
{
	refcount_up(&ref->refcount);
	return ref;
}

void weakref_put(weakref_t *ref)
{
	if (refcount_down(&ref->refcount))
		slab_free(slab_weakref_cache, ref);
}

void *weakref_hold(weakref_t *ref)
{
	if (!ref)
		return NULL;

	/* Ensure that the inner object can't be deallocated while we're using it. */
	if (atomic_fetch_add_explicit(&ref->access, 1, memory_order_acquire) == 0) {
		/* The weakref has already been destroyed. */
		atomic_fetch_sub_explicit(&ref->access, 1, memory_order_relaxed);
		return NULL;
	}

	void *p = atomic_load_explicit(&ref->inner, memory_order_relaxed);
	if (!p)
		weakref_release(ref);

	return p;
}

void weakref_release(weakref_t *ref)
{
	/*
	 * Synchronizes with weakref_destroy(), ensuring any operations made by
	 * this thread until now are seen by its caller.
	 */
	if (atomic_fetch_sub_explicit(&ref->access, 1, memory_order_acq_rel) == 1) {

		/* Ensure we only do this once, the first time access count falls to 0. */
		irq_spinlock_lock(&ref->destroyer_lock, true);
		thread_t *thread = ref->destroyer;
		ref->destroyer = NULL;
		irq_spinlock_unlock(&ref->destroyer_lock, true);

		if (thread)
			thread_wakeup(thread);
	}
}

/** Sets ref->inner to NULL and wait for anyone still using it to finish. */
void weakref_destroy(weakref_t *ref)
{
	atomic_store_explicit(&ref->inner, NULL, memory_order_relaxed);

	irq_spinlock_lock(&ref->destroyer_lock, true);

	// A decrement with acq_rel semantics to make sure we properly synchronize
	// with both weakref_hold() and weakref_release().
	if (atomic_fetch_sub_explicit(&ref->access, 1, memory_order_acq_rel) > 1) {
		// Someone is still using it. Sleep until they wake us up.
		(void) thread_wait_start();
		ref->destroyer = THREAD;
		irq_spinlock_unlock(&ref->destroyer_lock, true);
		thread_wait_finish(DEADLINE_NEVER);
	} else {
		irq_spinlock_unlock(&ref->destroyer_lock, true);
	}

	weakref_put(ref);
}
