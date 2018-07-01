/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2007 Jakub Jermar
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#include <adt/list.h>
#include <fibril.h>
#include <stack.h>
#include <tls.h>
#include <stdlib.h>
#include <as.h>
#include <context.h>
#include <futex.h>
#include <assert.h>
#include <mem.h>
#include <str.h>
#include <abi/proc/thread.h>
#include <libarch/faddr.h>
#include <macros.h>
#include "private/fibril.h"
#include "private/thread.h"
#include "private/fibril.h"

/** Member of timeout_list. */
typedef struct {
	link_t link;
	struct timeval expires;
	fibril_event_t *event;
} _timeout_t;

typedef enum {
	SWITCH_FROM_DEAD,
	SWITCH_FROM_HELPER,
	SWITCH_FROM_YIELD,
} _switch_type_t;

/* This futex serializes access to global data. */
static futex_t fibril_futex = FUTEX_INITIALIZER;

static futex_t ready_semaphore = FUTEX_INITIALIZE(0);

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(fibril_list);
static LIST_INITIALIZE(timeout_list);

/* Only used as unique markers for triggered events. */
static fibril_t _fibril_event_triggered;
static fibril_t _fibril_event_timed_out;
#define _EVENT_INITIAL   (NULL)
#define _EVENT_TRIGGERED (&_fibril_event_triggered)
#define _EVENT_TIMED_OUT (&_fibril_event_timed_out)

#ifdef CONFIG_UNLIMITED_THREADS
static _Atomic int threads_balance = INT_MIN;
#else
static _Atomic int threads_balance = 0;
#endif

static _Atomic int fibrils_balance = 0;

static inline int atomic_int_get(_Atomic int *a)
{
	return __atomic_load_n(a, __ATOMIC_RELAXED);
}

static inline void atomic_int_add(_Atomic int *a, int b)
{
	(void) __atomic_add_fetch(a, b, __ATOMIC_RELAXED);
}

/** Function that spans the whole life-cycle of a lightweight fibril.
 *
 * Each begins execution in this function. Then the function implementing
 * the fibril logic is called.  After its return, the return value is saved.
 * The fibril then switches to another fibril, which cleans up after it.
 *
 */
static void _fibril_main(void)
{
	/* fibril_futex is locked when a lightweight fibril is started. */
	futex_unlock(&fibril_futex);

	fibril_t *f = fibril_self();

	/* Call the implementing function. */
	fibril_exit(f->func(f->arg));

	/* Not reached */
}

/** Allocate a fibril structure and TCB, but don't do anything else with it. */
fibril_t *fibril_alloc(void)
{
	tcb_t *tcb = tls_make();
	if (!tcb)
		return NULL;

	fibril_t *fibril = calloc(1, sizeof(fibril_t));
	if (!fibril) {
		tls_free(tcb);
		return NULL;
	}

	tcb->fibril_data = fibril;
	fibril->tcb = tcb;
	return fibril;
}

/**
 * Set up pointer to thread-local storage and put the fibril into fibril_list.
 * The fibril structure must be allocated via fibril_alloc().
 *
 * @param fibril  The fibril structure allocated for running fibril.
 * @return        Same as input.
 */
fibril_t *fibril_setup(fibril_t *fibril)
{
	if (!fibril)
		return NULL;

	__tcb_set(fibril->tcb);

	futex_lock(&fibril_futex);
	list_append(&fibril->all_link, &fibril_list);
	futex_unlock(&fibril_futex);

	return fibril;
}

/**
 * Destroy a fibril structure allocated by fibril_alloc().
 * It does not matter whether fibril_setup() has been called on it.
 */
void fibril_teardown(fibril_t *fibril)
{
	if (link_in_use(&fibril->all_link)) {
		futex_lock(&fibril_futex);
		list_remove(&fibril->all_link);
		futex_unlock(&fibril_futex);
	}
	tls_free(fibril->tcb);
	free(fibril);
}

static errno_t _helper_fibril_fn(void *arg);

static void _spawn_threads_if_needed(void)
{
	if (fibril_self()->rmutex_locks > 0) {
		/* Can't spawn threads now. */
		return;
	}

	while (true) {
		if (atomic_int_get(&fibrils_balance) >= 0)
			return;

		if (atomic_int_get(&threads_balance) >= 0)
			return;

		/*
		 * `fibrils_balance < 0` means there are more active fibrils than
		 * threads. `threads_balance < 0` means there are fewer active threads
		 * than the maximum set.
		 */

		// FIXME: Bit of a race condition here.
		//        We might accidentally spawn more threads than the set maximum.
		//        It doesn't actually hurt anything though. We can fix it later with CAS.
		atomic_int_add(&fibrils_balance, +1);
		atomic_int_add(&threads_balance, +1);

		if (!fibril_run_heavy(_helper_fibril_fn, NULL,
		    "lightweight_runner", PAGE_SIZE)) {

			/* Failed to create. */
			atomic_int_add(&fibrils_balance, -1);
			atomic_int_add(&threads_balance, -1);
			return;
		}
	}
}

/**
 * Event notification with a given reason.
 *
 * @param reason  Reason of the notification.
 *                Can be either _EVENT_TRIGGERED or _EVENT_TIMED_OUT.
 */
static fibril_t *_fibril_trigger_internal(fibril_event_t *event, fibril_t *reason)
{
	assert(reason != _EVENT_INITIAL);
	assert(reason == _EVENT_TIMED_OUT || reason == _EVENT_TRIGGERED);

	futex_assert_is_locked(&fibril_futex);

	if (event->fibril == _EVENT_INITIAL) {
		event->fibril = reason;
		return NULL;
	}

	if (event->fibril == _EVENT_TIMED_OUT) {
		assert(reason == _EVENT_TRIGGERED);
		event->fibril = reason;
		return NULL;
	}

	if (event->fibril == _EVENT_TRIGGERED) {
		/* Already triggered. Nothing to do. */
		return NULL;
	}

	fibril_t *f = event->fibril;
	event->fibril = reason;

	assert(f->sleep_event == event);
	return f;
}

static fibril_t *_ready_list_pop(const struct timeval *expires, bool locked)
{
	if (locked)
		futex_assert_is_locked(&fibril_futex);
	else
		futex_assert_is_not_locked(&fibril_futex);

	errno_t rc = futex_down_timeout(&ready_semaphore, expires);

	if (rc != EOK)
		return NULL;

	if (!locked)
		futex_lock(&fibril_futex);
	fibril_t *f = list_pop(&ready_list, fibril_t, link);
	if (!locked)
		futex_unlock(&fibril_futex);
	assert(f);
	return f;
}

static fibril_t *_ready_list_pop_nonblocking(bool locked)
{
	struct timeval tv = {0};
	return _ready_list_pop(&tv, locked);
}

static void _ready_list_push(fibril_t *f)
{
	futex_assert_is_locked(&fibril_futex);

	atomic_int_add(&fibrils_balance, -1);

	/* Enqueue in ready_list. */
	list_append(&f->link, &ready_list);
	futex_up(&ready_semaphore);
}

static void _restore_fibril(fibril_t *f)
{
	if (!f)
		return;

	futex_assert_is_locked(&fibril_futex);

	if (f->is_heavy)
		futex_up(&f->heavy_blocking_sem);
	else
		_ready_list_push(f);
}

/** Fire all timeouts that expired. */
static struct timeval *_handle_expired_timeouts(struct timeval *next_timeout)
{
	struct timeval tv;
	getuptime(&tv);

	futex_lock(&fibril_futex);

	while (!list_empty(&timeout_list)) {
		link_t *cur = list_first(&timeout_list);
		_timeout_t *to = list_get_instance(cur, _timeout_t, link);

		if (tv_gt(&to->expires, &tv)) {
			*next_timeout = to->expires;
			futex_unlock(&fibril_futex);
			_spawn_threads_if_needed();
			return next_timeout;
		}

		list_remove(&to->link);

		_restore_fibril(_fibril_trigger_internal(
		    to->event, _EVENT_TIMED_OUT));
	}

	futex_unlock(&fibril_futex);
	_spawn_threads_if_needed();
	return NULL;
}

/**
 * Clean up after a dead fibril from which we restored context, if any.
 * Called after a switch is made and fibril_futex is unlocked.
 */
static void _fibril_cleanup_dead(void)
{
	fibril_t *srcf = fibril_self();
	if (!srcf->clean_after_me)
		return;

	void *stack = srcf->clean_after_me->stack;
	assert(stack);
	as_area_destroy(stack);
	fibril_teardown(srcf->clean_after_me);
	srcf->clean_after_me = NULL;
}

/** Switch to a fibril. */
static void _fibril_switch_to(_switch_type_t type, fibril_t *dstf, bool locked)
{
	assert(fibril_self()->rmutex_locks == 0);

	if (!locked)
		futex_lock(&fibril_futex);
	else
		futex_assert_is_locked(&fibril_futex);

	fibril_t *srcf = fibril_self();
	assert(srcf);
	assert(dstf);

	switch (type) {
	case SWITCH_FROM_YIELD:
		_ready_list_push(srcf);
		break;
	case SWITCH_FROM_DEAD:
		dstf->clean_after_me = srcf;
		break;
	case SWITCH_FROM_HELPER:
		break;
	}

	atomic_int_add(&fibrils_balance, +1);

	dstf->thread_ctx = srcf->thread_ctx;
	srcf->thread_ctx = NULL;

	/* Just some bookkeeping to allow better debugging of futex locks. */
	futex_give_to(&fibril_futex, dstf);

	/* Swap to the next fibril. */
	context_swap(&srcf->ctx, &dstf->ctx);

	assert(srcf == fibril_self());
	assert(srcf->thread_ctx);

	if (!locked) {
		/* Must be after context_swap()! */
		futex_unlock(&fibril_futex);
		_fibril_cleanup_dead();
	}
}

/**
 * Main function for a helper fibril.
 * The helper fibril executes on threads in the lightweight fibril pool when
 * there is no fibril ready to run. Its only purpose is to block until
 * another fibril is ready, or a timeout expires.
 *
 * There is at most one helper fibril per thread.
 *
 */
static errno_t _helper_fibril_fn(void *arg)
{
	/* Set itself as the thread's own context. */
	fibril_self()->thread_ctx = fibril_self();

	(void) arg;

	struct timeval next_timeout;
	while (true) {
		struct timeval *to = _handle_expired_timeouts(&next_timeout);
		fibril_t *f = _ready_list_pop(to, false);
		if (f)
			_fibril_switch_to(SWITCH_FROM_HELPER, f, false);
	}

	return EOK;
}

/** Create a new fibril.
 *
 * @param func Implementing function of the new fibril.
 * @param arg Argument to pass to func.
 * @param stksz Stack size in bytes.
 *
 * @return 0 on failure or TLS of the new fibril.
 *
 */
fid_t fibril_create_generic(errno_t (*func)(void *), void *arg, size_t stksz)
{
	fibril_t *fibril;

	fibril = fibril_alloc();
	if (fibril == NULL)
		return 0;

	fibril->stack_size = (stksz == FIBRIL_DFLT_STK_SIZE) ?
	    stack_size_get() : stksz;
	fibril->stack = as_area_create(AS_AREA_ANY, fibril->stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (fibril->stack == AS_MAP_FAILED) {
		fibril_teardown(fibril);
		return 0;
	}

	fibril->func = func;
	fibril->arg = arg;

	context_create_t sctx = {
		.fn = _fibril_main,
		.stack_base = fibril->stack,
		.stack_size = fibril->stack_size,
		.tls = fibril->tcb,
	};

	context_create(&fibril->ctx, &sctx);
	return (fid_t) fibril;
}

/** Destroy a lightweight fibril that is not running.
 *
 * Free resources of a fibril that has been created with fibril_create()
 * but never started using fibril_start().
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 */
void fibril_destroy(fibril_t *fibril)
{
	assert(!fibril->is_running);
	assert(!fibril->is_heavy);

	assert(fibril->stack);
	as_area_destroy(fibril->stack);
	fibril_teardown(fibril);
}

/**
 * fibril_wait_timeout() in a heavy fibril.
 */
static errno_t _wait_timeout_heavy(fibril_event_t *event, const struct timeval *expires)
{
	fibril_t *srcf = fibril_self();
	event->fibril = srcf;

	futex_unlock(&fibril_futex);

	/* Block on the internal semaphore. */
	errno_t rc = futex_down_composable(&srcf->heavy_blocking_sem, expires);

	if (rc != EOK) {
		/* On failure, we need to check the event again. */
		futex_lock(&fibril_futex);

		assert(event->fibril != _EVENT_INITIAL);
		assert(event->fibril != _EVENT_TIMED_OUT);
		assert(event->fibril == srcf || event->fibril == _EVENT_TRIGGERED);

		bool triggered = (event->fibril == _EVENT_TRIGGERED);
		event->fibril = _EVENT_INITIAL;
		futex_unlock(&fibril_futex);

		if (triggered)
			return EOK;

		/* No wakeup incoming (see futex_down_composable()). */
		futex_up(&srcf->heavy_blocking_sem);
	}

	return rc;
}

/**
 * Same as `fibril_wait_for()`, except with a timeout.
 *
 * It is guaranteed that timing out cannot cause another thread's
 * `fibril_notify()` to be lost. I.e. the function returns success if and
 * only if `fibril_notify()` was called after the last call to
 * wait/wait_timeout returned, and before the call timed out.
 *
 * @return ETIMEOUT if timed out. EOK otherwise.
 */
errno_t fibril_wait_timeout(fibril_event_t *event, const struct timeval *expires)
{
	assert(fibril_self()->rmutex_locks == 0);

	if (!fibril_self()->thread_ctx) {
		fibril_self()->thread_ctx =
		    fibril_create_generic(_helper_fibril_fn, NULL, PAGE_SIZE);
		if (!fibril_self()->thread_ctx)
			return ENOMEM;
	}

	futex_lock(&fibril_futex);

	if (event->fibril == _EVENT_TRIGGERED) {
		event->fibril = _EVENT_INITIAL;
		futex_unlock(&fibril_futex);
		return EOK;
	}

	assert(event->fibril == _EVENT_INITIAL);

	fibril_t *srcf = fibril_self();
	if (srcf->is_heavy)
		return _wait_timeout_heavy(event, expires);

	fibril_t *dstf = NULL;

	/*
	 * We cannot block here waiting for another fibril becoming
	 * ready, since that would require unlocking the fibril_futex,
	 * and that in turn would allow another thread to restore
	 * the source fibril before this thread finished switching.
	 *
	 * Instead, we switch to an internal "helper" fibril whose only
	 * job is to wait for an event, freeing the source fibril for
	 * wakeups. There is always one for each running thread.
	 */

	dstf = _ready_list_pop_nonblocking(true);
	if (!dstf) {
		dstf = srcf->thread_ctx;
		assert(dstf);
	}

	_timeout_t timeout = { 0 };
	if (expires) {
		timeout.expires = *expires;
		timeout.event = event;
		list_append(&timeout.link, &timeout_list);
	}

	event->fibril = srcf;
	srcf->sleep_event = event;

	_fibril_switch_to(SWITCH_FROM_HELPER, dstf, true);

	assert(event->fibril != srcf);
	assert(event->fibril != _EVENT_INITIAL);
	assert(event->fibril == _EVENT_TIMED_OUT || event->fibril == _EVENT_TRIGGERED);

	list_remove(&timeout.link);
	errno_t rc = (event->fibril == _EVENT_TIMED_OUT) ? ETIMEOUT : EOK;
	event->fibril = _EVENT_INITIAL;

	futex_unlock(&fibril_futex);
	_fibril_cleanup_dead();
	return rc;
}

void fibril_wait_for(fibril_event_t *event)
{
	assert(fibril_self()->rmutex_locks == 0);

	(void) fibril_wait_timeout(event, NULL);
}

void fibril_notify(fibril_event_t *event)
{
	futex_lock(&fibril_futex);
	_restore_fibril(_fibril_trigger_internal(event, _EVENT_TRIGGERED));
	futex_unlock(&fibril_futex);
	_spawn_threads_if_needed();
}

/** Start a fibril that has not been running yet. */
void fibril_start(fibril_t *fibril)
{
	futex_lock(&fibril_futex);
	assert(!fibril->is_running);
	fibril->is_running = true;

	if (!link_in_use(&fibril->all_link))
		list_append(&fibril->all_link, &fibril_list);

	_restore_fibril(fibril);

	futex_unlock(&fibril_futex);
	_spawn_threads_if_needed();
}

/** Start a fibril that has not been running yet. (obsolete) */
void fibril_add_ready(fibril_t *fibril)
{
	fibril_start(fibril);
}

/**
 * Switch to another fibril, if one is ready to run.
 * Has no effect on a heavy fibril.
 */
void fibril_yield(void)
{
	if (fibril_self()->rmutex_locks > 0)
		return;

	if (fibril_self()->is_heavy)
		// TODO: thread yield?
		return;

	fibril_t *f = _ready_list_pop_nonblocking(false);
	if (f)
		_fibril_switch_to(SWITCH_FROM_YIELD, f, false);
}

/**
 * Obsolete, use fibril_self().
 *
 * @return ID of the currently running fibril.
 */
fid_t fibril_get_id(void)
{
	return (fid_t) fibril_self();
}

/** @return the currently running fibril. */
fibril_t *fibril_self(void)
{
	fibril_t *self = __tcb_get()->fibril_data;

	/* Sanity checks. */
	assert(self);
	assert(self->tcb);
	assert(self->tcb->fibril_data == self);

	return self;
}

/** Terminate current thread.
 *
 * @param status Exit status. Currently not used.
 *
 */
static _Noreturn void _sys_thread_exit(int status)
{
	__SYSCALL1(SYS_THREAD_EXIT, (sysarg_t) status);
	__builtin_unreachable();
}

/**
 * Exit a fibril. Never returns.
 *
 * @param retval  Value to return from fibril_join() called on this fibril.
 */
_Noreturn void fibril_exit(long retval)
{
	// TODO: implement fibril_join() and remember retval
	(void) retval;

	if (fibril_self()->is_heavy) {
		/* Thread exit. */
		// FIXME: Proper cleanup of thread stack requires sys_thread_join().
		fibril_teardown(fibril_self());
		_sys_thread_exit(0);
		/* Not reached. */
	}

	fibril_t *f = _ready_list_pop_nonblocking(false);
	if (!f)
		f = fibril_self()->thread_ctx;

	_fibril_switch_to(SWITCH_FROM_DEAD, f, false);
	__builtin_unreachable();
}

void __thread_main(uspace_arg_t *uarg)
{
	fibril_t *f = fibril_setup(uarg->uspace_thread_arg);
	assert(f);

	fibril_exit(f->func(f->arg));
}

static errno_t _sys_thread_create(uspace_arg_t *uarg,
    const char *name)
{
	thread_id_t tid;
	return __SYSCALL4(SYS_THREAD_CREATE, (sysarg_t) uarg,
	    (sysarg_t) name, (sysarg_t) str_size(name), (sysarg_t) &tid);
}

static errno_t _thread_create(fibril_t *f, const char *name)
{
	assert(!f->is_running);
	assert(!f->is_heavy);

	/* Make heap thread safe. */
	malloc_enable_multithreaded();

	f->is_heavy = true;

	f->uarg.uspace_entry = (void *) FADDR(__thread_entry);
	f->uarg.uspace_stack = f->stack;
	f->uarg.uspace_stack_size = f->stack_size;
	f->uarg.uspace_thread_function = NULL;
	f->uarg.uspace_thread_arg = f;
	f->uarg.uspace_uarg = &f->uarg;

	return _sys_thread_create(&f->uarg, name);
}

fid_t fibril_run_heavy(errno_t (*func)(void *), void *arg, const char *name, size_t stack_size)
{
	assert(fibril_self()->rmutex_locks == 0);

	fid_t f = fibril_create_generic(func, arg, stack_size);
	if (!f)
		return f;

	errno_t rc = _thread_create(f, name);
	if (rc != EOK) {
		fibril_destroy(f);
		return 0;
	}

	return f;
}

void fibril_detach(fid_t f)
{
	// TODO: detached state is currently default
}

#if 0

/** Get current thread ID. */
static void thread_id_t _sys_thread_get_id(void)
{
	thread_id_t thread_id;
	(void) __SYSCALL1(SYS_THREAD_GET_ID, (sysarg_t) &thread_id);
	return thread_id;
}

/** Wait unconditionally for specified number of microseconds. */
int sys_thread_usleep(useconds_t usec)
{
	(void) __SYSCALL1(SYS_THREAD_USLEEP, usec);
	return 0;
}

/** Wait unconditionally for specified number of seconds. */
unsigned int sys_thread_sleep(unsigned int sec)
{
	/* Sleep in 1000 second steps to support full argument range. */

	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;

		sys_thread_usleep(period * 1000000);
		sec -= period;
	}

	return 0;
}

#endif

/**
 * Spawn a given number of threads for the thread pool, immediately, and
 * unconditionally. This is meant to be used for tests and debugging.
 * Normal operation should just use `fibril_enable_multithreaded()`.
 *
 * @param threads  Number of threads to spawn.
 */
void fibril_force_add_threads(int threads)
{
	assert(fibril_self()->rmutex_locks == 0);

	for (int i = 0; i < threads; i++) {
		if (!fibril_run_heavy(_helper_fibril_fn, NULL,
		    "lightweight_runner", PAGE_SIZE))
			break;

		atomic_int_add(&fibrils_balance, +1);
	}
}

/**
 * Opt-in to multithreaded lightweight fibrils.
 * Currently breaks some servers. Eventually, should be the default.
 */
void fibril_enable_multithreaded(void)
{
	/* CONFIG_UNLIMITED_THREADS removes the limit unconditionally. */
#ifndef CONFIG_UNLIMITED_THREADS
	// TODO: Base the choice on the number of CPUs instead of a fixed value.
	atomic_int_add(&threads_balance, -4);
#endif
}

void __fibrils_init(void)
{
	/* Empty for now. */
}

void fibril_usleep(suseconds_t timeout)
{
	struct timeval expires;
	getuptime(&expires);
	tv_add_diff(&expires, timeout);

	fibril_event_t event = FIBRIL_EVENT_INIT;
	fibril_wait_timeout(&event, &expires);
}

void fibril_sleep(unsigned int sec)
{
	struct timeval expires;
	getuptime(&expires);
	expires.tv_sec += sec;

	fibril_event_t event = FIBRIL_EVENT_INIT;
	fibril_wait_timeout(&event, &expires);
}

/** @}
 */
