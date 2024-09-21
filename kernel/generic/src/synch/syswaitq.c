/*
 * Copyright (c) 2018 Jakub Jermar
 * Copyright (c) 2024 Jiří Zárevúcky
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
 * @brief Wrapper for using wait queue as a kobject.
 */

#include <synch/syswaitq.h>
#include <synch/waitq.h>
#include <mm/slab.h>
#include <proc/task.h>
#include <syscall/copy.h>

typedef struct {
	kobj_t kobj;
	waitq_t waitq;
} syswaitq_t;

static SLAB_CACHE(syswaitq_cache, syswaitq_t, 1, NULL, NULL, 0);

static void syswaitq_destroy(void *arg)
{
	syswaitq_t *wq = arg;
	assert(list_empty(&wq->waitq.sleepers));
	slab_free(&syswaitq_cache, wq);
}

static kobj_class_t syswaitq_class = {
	.destroy = syswaitq_destroy,
};

/** Create a waitq for the current task
 *
 * @param[out] whandle  Userspace address of the destination buffer that will
 *                      receive the allocated waitq capability.
 *
 * @return              Error code.
 */
sys_errno_t sys_waitq_create(uspace_ptr_cap_waitq_handle_t whandle)
{
	syswaitq_t *wq = slab_alloc(&syswaitq_cache, FRAME_ATOMIC);
	if (!wq)
		return (sys_errno_t) ENOMEM;

	kobj_initialize(&wq->kobj, &syswaitq_class);
	waitq_initialize(&wq->waitq);

	kobj_handle_t handle = kobj_table_insert(&TASK->kobj_table, &wq->kobj);
	if (!handle) {
		kobj_put(&wq->kobj);
		return (sys_errno_t) ENOMEM;
	}

	errno_t rc = copy_to_uspace(whandle, &handle, sizeof(handle));
	if (rc != EOK)
		kobj_put(kobj_table_remove(&TASK->kobj_table, handle));

	return (sys_errno_t) rc;
}

/** Destroy a waitq
 *
 * @param whandle  Waitq capability handle of the waitq to be destroyed.
 *
 * @return         Error code.
 */
sys_errno_t sys_waitq_destroy(cap_waitq_handle_t whandle)
{
	// TODO: This syscall is wholly unnecessary, there only needs to be one
	//       syscall to destroy any handle.
	//       Typechecking the destroyed reference is not kernel's obligation.
	kobj_put(kobj_table_remove(&TASK->kobj_table, cap_handle_raw(whandle)));
	return EOK;
}

/** Sleep in the waitq
 *
 * @param whandle  Waitq capability handle of the waitq in which to sleep.
 * @param timeout  Timeout in microseconds.
 * @param flags    Flags from SYNCH_FLAGS_* family. SYNCH_FLAGS_INTERRUPTIBLE is
 *                 always implied.
 *
 * @return         Error code.
 */
sys_errno_t sys_waitq_sleep(cap_waitq_handle_t whandle, uint32_t timeout,
    unsigned int flags)
{
	syswaitq_t *wq = kobj_table_lookup(&TASK->kobj_table,
	    cap_handle_raw(whandle), &syswaitq_class);

	if (!wq)
		return (sys_errno_t) ENOENT;

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif

	errno_t rc = _waitq_sleep_timeout(&wq->waitq, timeout,
	    SYNCH_FLAGS_INTERRUPTIBLE | flags);

#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif

	kobj_put(&wq->kobj);

	return (sys_errno_t) rc;
}

/** Wakeup a thread sleeping in the waitq
 *
 * @param whandle  Waitq capability handle of the waitq to invoke wakeup on.
 *
 * @return         Error code.
 */
sys_errno_t sys_waitq_wakeup(cap_waitq_handle_t whandle)
{
	syswaitq_t *wq = kobj_table_lookup(&TASK->kobj_table,
	    cap_handle_raw(whandle), &syswaitq_class);

	if (!wq)
		return (sys_errno_t) ENOENT;

	waitq_wake_one(&wq->waitq);

	kobj_put(&wq->kobj);
	return (sys_errno_t) EOK;
}

/** @}
 */
