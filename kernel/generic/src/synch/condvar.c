/*
 * Copyright (c) 2001-2004 Jakub Jermar
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
 * @brief	Condition variables.
 */

#include <synch/condvar.h>
#include <synch/mutex.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch.h>

/** Initialize condition variable.
 *
 * @param cv		Condition variable.
 */
void condvar_initialize(condvar_t *cv)
{
	waitq_initialize(&cv->wq);
}

/** Signal the condition has become true to the first waiting thread by waking
 * it up.
 *
 * @param cv		Condition variable.
 */
void condvar_signal(condvar_t *cv)
{
	waitq_signal(&cv->wq);
}

/** Signal the condition has become true to all waiting threads by waking
 * them up.
 *
 * @param cv		Condition variable.
 */
void condvar_broadcast(condvar_t *cv)
{
	waitq_wake_all(&cv->wq);
}

/** Wait for the condition becoming true.
 *
 * @param cv		Condition variable.
 * @param mtx		Mutex.
 * @param usec		Timeout value in microseconds.
 *
 * @return		See comment for waitq_sleep_timeout().
 */
errno_t condvar_wait_timeout(condvar_t *cv, mutex_t *mtx, uint32_t usec)
{
	ipl_t ipl = waitq_sleep_prepare(&cv->wq);

	/* Unlock only after the waitq is locked so we don't miss a wakeup. */
	mutex_unlock(mtx);

	errno_t rc = waitq_sleep_timeout_unsafe(&cv->wq, usec, SYNCH_FLAGS_NON_BLOCKING, ipl);

	mutex_lock(mtx);
	return rc;
}

errno_t condvar_wait(condvar_t *cv, mutex_t *mtx)
{
	ipl_t ipl = waitq_sleep_prepare(&cv->wq);

	/* Unlock only after the waitq is locked so we don't miss a wakeup. */
	mutex_unlock(mtx);

	errno_t rc = waitq_sleep_unsafe(&cv->wq, ipl);

	mutex_lock(mtx);
	return rc;
}

/** @}
 */
