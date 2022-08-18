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
 * @brief Mutexes.
 */

#include <assert.h>
#include <errno.h>
#include <synch/mutex.h>
#include <synch/semaphore.h>
#include <arch.h>
#include <stacktrace.h>
#include <cpu.h>
#include <proc/thread.h>

/** Initialize mutex.
 *
 * @param mtx   Mutex.
 * @param type  Type of the mutex.
 */
void mutex_initialize(mutex_t *mtx, mutex_type_t type)
{
	mtx->type = type;
	mtx->owner = NULL;
	mtx->nesting = 0;
	semaphore_initialize(&mtx->sem, 1);
}

/** Find out whether the mutex is currently locked.
 *
 * @param mtx  Mutex.
 *
 * @return  True if the mutex is locked, false otherwise.
 */
bool mutex_locked(mutex_t *mtx)
{
	bool success = semaphore_try_down(&mtx->sem);
	if (success) {
		semaphore_up(&mtx->sem);
	}
	return !success;
}

#define MUTEX_DEADLOCK_THRESHOLD	100000000

bool mutex_try_lock(mutex_t *mtx)
{
	if (mtx->type == MUTEX_RECURSIVE && mtx->owner == THREAD) {
		assert(THREAD);
		mtx->nesting++;
		return EOK;
	}

	bool success = semaphore_try_down(&mtx->sem);
	if (success) {
		mtx->owner = THREAD;
		mtx->nesting = 1;
	}
	return success;
}

static void mutex_lock_active(mutex_t *mtx)
{
	assert((mtx->type == MUTEX_ACTIVE) || !THREAD);

	unsigned int cnt = 0;
	bool deadlock_reported = false;
	bool success = false;

	do {
		if (cnt++ > MUTEX_DEADLOCK_THRESHOLD) {
			printf("cpu%u: looping on active mutex %p\n",
				CPU->id, mtx);
			stack_trace();
			cnt = 0;
			deadlock_reported = true;
		}
		success = semaphore_try_down(&mtx->sem);
	} while (!success);

	if (deadlock_reported)
		printf("cpu%u: not deadlocked\n", CPU->id);
}

void mutex_lock(mutex_t *mtx)
{
	if (mtx->type == MUTEX_RECURSIVE && mtx->owner == THREAD) {
		assert(THREAD);
		mtx->nesting++;
		return;
	}

	if (mtx->type == MUTEX_ACTIVE || !THREAD) {
		mutex_lock_active(mtx);
		return;
	}

	semaphore_down(&mtx->sem);
	mtx->owner = THREAD;
	mtx->nesting = 1;
}

/** Acquire mutex with timeout.
 *
 * @param mtx    Mutex.
 * @param usec   Timeout in microseconds.
 *
 * @return See comment for waitq_sleep_timeout().
 *
 */
errno_t mutex_lock_timeout(mutex_t *mtx, uint32_t usec)
{
	if (usec == 0)
		return mutex_try_lock(mtx) ? EOK : ETIMEOUT;

	assert(mtx->type != MUTEX_ACTIVE);
	assert(THREAD);

	if (mtx->type == MUTEX_RECURSIVE && mtx->owner == THREAD) {
		mtx->nesting++;
		return EOK;
	}

	errno_t rc = semaphore_down_timeout(&mtx->sem, usec);
	if (rc == EOK) {
		mtx->owner = THREAD;
		mtx->nesting = 1;
	}
	return rc;
}

/** Release mutex.
 *
 * @param mtx  Mutex.
 */
void mutex_unlock(mutex_t *mtx)
{
	if (mtx->type == MUTEX_RECURSIVE) {
		assert(mtx->owner == THREAD);
		if (--mtx->nesting > 0)
			return;
		mtx->owner = NULL;
	}
	semaphore_up(&mtx->sem);
}

/** @}
 */
