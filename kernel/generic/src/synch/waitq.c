/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2022 Jiří Zárevúcky
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

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief Wait queue.
 *
 * Wait queue is the basic synchronization primitive upon which all
 * other synchronization primitives build.
 *
 * It allows threads to wait for an event in first-come, first-served
 * fashion. Conditional operation as well as timeouts and interruptions
 * are supported.
 *
 */

#include <assert.h>
#include <errno.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <proc/thread.h>
#include <proc/scheduler.h>
#include <arch/asm.h>
#include <typedefs.h>
#include <time/timeout.h>
#include <arch.h>
#include <context.h>
#include <adt/list.h>
#include <arch/cycle.h>
#include <mem.h>

/** Initialize wait queue
 *
 * Initialize wait queue.
 *
 * @param wq Pointer to wait queue to be initialized.
 *
 */
void waitq_initialize(waitq_t *wq)
{
	memsetb(wq, sizeof(*wq), 0);
	irq_spinlock_initialize(&wq->lock, "wq.lock");
	list_initialize(&wq->sleepers);
}

/**
 * Initialize wait queue with an initial number of queued wakeups (or a wakeup debt if negative).rrrr
 */
void waitq_initialize_with_count(waitq_t *wq, int count)
{
	waitq_initialize(wq);
	wq->wakeup_balance = count;
}

/** Interrupt sleeping thread.
 *
 * This routine attempts to interrupt a thread from its sleep in
 * a waitqueue. If the thread is not found sleeping, no action
 * is taken.
 *
 * @param thread Thread to be interrupted.
 *
 */
void waitq_interrupt_sleep(thread_t *thread)
{
	assert(thread != NULL);
	thread->interrupted = true;
	thread_wakeup(thread);
}

#define PARAM_NON_BLOCKING(flags, usec) \
	(((flags) & SYNCH_FLAGS_NON_BLOCKING) && ((usec) == 0))

errno_t waitq_sleep(waitq_t *wq)
{
	return _waitq_sleep_timeout(wq, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);
}

errno_t waitq_sleep_timeout(waitq_t *wq, uint32_t usec)
{
	return _waitq_sleep_timeout(wq, usec, SYNCH_FLAGS_NON_BLOCKING);
}

/** Sleep until either wakeup, timeout or interruption occurs
 *
 * Sleepers are organised in a FIFO fashion in a structure called wait queue.
 *
 * Other functions as waitq_sleep() and all the *_timeout() functions are
 * implemented using this function.
 *
 * @param wq    Pointer to wait queue.
 * @param usec  Timeout in microseconds.
 * @param flags Specify mode of the sleep.
 *
 * The sleep can be interrupted only if the
 * SYNCH_FLAGS_INTERRUPTIBLE bit is specified in flags.
 *
 * If usec is greater than zero, regardless of the value of the
 * SYNCH_FLAGS_NON_BLOCKING bit in flags, the call will not return until either
 * timeout, interruption or wakeup comes.
 *
 * If usec is zero and the SYNCH_FLAGS_NON_BLOCKING bit is not set in flags,
 * the call will not return until wakeup or interruption comes.
 *
 * If usec is zero and the SYNCH_FLAGS_NON_BLOCKING bit is set in flags, the
 * call will immediately return, reporting either success or failure.
 *
 * @return ETIMEOUT, meaning that the sleep timed out, or a nonblocking call
 *                   returned unsuccessfully.
 * @return EINTR, meaning that somebody interrupted the sleeping thread.
 * @return EOK, meaning that none of the above conditions occured, and the
 *              thread was woken up successfuly by `waitq_wake_*()`.
 *
 */
errno_t _waitq_sleep_timeout(waitq_t *wq, uint32_t usec, unsigned int flags)
{
	assert((!PREEMPTION_DISABLED) || (PARAM_NON_BLOCKING(flags, usec)));
	return waitq_sleep_timeout_unsafe(wq, usec, flags, waitq_sleep_prepare(wq));
}

/** Prepare to sleep in a waitq.
 *
 * This function will return holding the lock of the wait queue
 * and interrupts disabled.
 *
 * @param wq Wait queue.
 *
 * @return Interrupt level as it existed on entry to this function.
 *
 */
wait_guard_t waitq_sleep_prepare(waitq_t *wq)
{
	ipl_t ipl = interrupts_disable();
	irq_spinlock_lock(&wq->lock, false);
	return (wait_guard_t) {
		.ipl = ipl,
	};
}

errno_t waitq_sleep_unsafe(waitq_t *wq, wait_guard_t guard)
{
	return waitq_sleep_timeout_unsafe(wq, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE, guard);
}

/** Internal implementation of waitq_sleep_timeout().
 *
 * This function implements logic of sleeping in a wait queue.
 * This call must be preceded by a call to waitq_sleep_prepare().
 *
 * @param wq    See waitq_sleep_timeout().
 * @param usec  See waitq_sleep_timeout().
 * @param flags See waitq_sleep_timeout().
 *
 * @param[out] blocked  See waitq_sleep_timeout().
 *
 * @return See waitq_sleep_timeout().
 *
 */
errno_t waitq_sleep_timeout_unsafe(waitq_t *wq, uint32_t usec, unsigned int flags, wait_guard_t guard)
{
	errno_t rc;

	/*
	 * If true, and this thread's sleep returns without a wakeup
	 * (timed out or interrupted), waitq ignores the next wakeup.
	 * This is necessary for futex to be able to handle those conditions.
	 */
	bool sleep_composable = (flags & SYNCH_FLAGS_FUTEX);
	bool interruptible = (flags & SYNCH_FLAGS_INTERRUPTIBLE);

	if (wq->closed) {
		rc = EOK;
		goto exit;
	}

	/* Checks whether to go to sleep at all */
	if (wq->wakeup_balance > 0) {
		wq->wakeup_balance--;

		rc = EOK;
		goto exit;
	}

	if (PARAM_NON_BLOCKING(flags, usec)) {
		/* Return immediately instead of going to sleep */
		rc = ETIMEOUT;
		goto exit;
	}

	atomic_store_explicit(&THREAD->sleep_queue, wq, memory_order_relaxed);

	/* This thread_t field is synchronized exclusively via
	 * waitq lock of the waitq currently listing it.
	 */
	list_append(&THREAD->wq_link, &wq->sleepers);

	// Needs to be run when interrupts are still disabled.
	deadline_t deadline = usec > 0 ? timeout_deadline_in_usec(usec) : 0;

	while (true) {
		/* reset has acquire semantic, to ensure that THREAD->interrupted
		 * written before a wakeup is seen after this
		 */
		thread_wait_reset();

		if (interruptible && THREAD->interrupted) {
			rc = EINTR;
			goto exit;
		}

		irq_spinlock_unlock(&wq->lock, false);

		bool timed_out = false;

		if (deadline) {
			timed_out = thread_wait_until(deadline);
		} else {
			thread_wait();
		}

		/*
		 * Although we don't need the lock necessarily after a successful
		 * wakeup, we have to wait for concurrently running waitq_wakeup() to exit.
		 * If we didn't always do this, we'd risk waitq_wakeup() that woke us up still
		 * running on another CPU even after this function returns, and that would be
		 * an issue if the waitq is allocated locally to wait for a one-of asynchronous event.
		 * We'd need more external synchronization in that case, and that would be a pain.
		 */
		irq_spinlock_lock(&wq->lock, false);

		if (!link_in_use(&THREAD->wq_link)) {
			// We were woken up by the desired event in addition to timeout/interruption. Return success.
			rc = EOK;
			goto exit;
		}

		if (timed_out) {
			rc = ETIMEOUT;
			goto exit;
		}

		// Interrupted.
	}

exit:
	list_remove(&THREAD->wq_link);

	if (rc != EOK && sleep_composable)
		wq->wakeup_balance--;

	atomic_store_explicit(&THREAD->sleep_queue, NULL, memory_order_relaxed);
	irq_spinlock_unlock(&wq->lock, false);
	interrupts_restore(guard.ipl);
	return rc;
}

bool waitq_try_down(waitq_t *wq)
{
	irq_spinlock_lock(&wq->lock, true);

	bool success = wq->closed || wq->wakeup_balance > 0;
	if (wq->wakeup_balance > 0)
		wq->wakeup_balance--;

	irq_spinlock_unlock(&wq->lock, true);

	return success;
}

static void _wake_one(waitq_t *wq)
{
	thread_t *thread = list_get_instance(list_first(&wq->sleepers), thread_t, wq_link);
	list_remove(&thread->wq_link);
	thread_wakeup(thread);
}

/**
 * Meant for implementing condvar signal.
 * Always wakes one thread if there are any sleeping,
 * has no effect if no threads are waiting for wakeup.
 */
void waitq_signal(waitq_t *wq)
{
	irq_spinlock_lock(&wq->lock, true);

	if (!list_empty(&wq->sleepers))
		_wake_one(wq);

	irq_spinlock_unlock(&wq->lock, true);
}

/**
 * Wakes up one thread sleeping on this waitq.
 * If there are no threads waiting, saves the wakeup so that the next sleep
 * returns immediately. If a previous failure in sleep created a wakeup debt
 * (see SYNCH_FLAGS_FUTEX) this debt is annulled and no thread is woken up.
 */
void waitq_wake_one(waitq_t *wq)
{
	irq_spinlock_lock(&wq->lock, true);

	if (wq-> closed)
		;
	else if (wq->wakeup_balance < 0 || list_empty(&wq->sleepers))
		wq->wakeup_balance++;
	else
		_wake_one(wq);

	irq_spinlock_unlock(&wq->lock, true);
}

static void _wake_all(waitq_t *wq)
{
	while (!list_empty(&wq->sleepers))
		_wake_one(wq);
}

/**
 * Wakes up all threads currently waiting on this waitq
 * and makes all future sleeps return instantly.
 */
void waitq_close(waitq_t *wq)
{
	irq_spinlock_lock(&wq->lock, true);
	wq->wakeup_balance = 0;
	wq->closed = true;
	_wake_all(wq);
	irq_spinlock_unlock(&wq->lock, true);
}

/**
 * Wakes up all threads currently waiting on this waitq
 */
void waitq_wake_all(waitq_t *wq)
{
	irq_spinlock_lock(&wq->lock, true);
	wq->wakeup_balance = 0;
	_wake_all(wq);
	irq_spinlock_unlock(&wq->lock, true);
}

/** @}
 */
