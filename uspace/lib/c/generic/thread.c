/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <fibril.h>
#include <fibril_synch.h>
#include <libc.h>
#include <libarch/faddr.h>
#include <abi/proc/uarg.h>
#include <stack.h>
#include <str.h>
#include <async.h>
#include <as.h>
#include <rcu.h>
#include "private/thread.h"

typedef sysarg_t thread_id_t;

#define SYS_THREAD_SIZE        PAGE_SIZE
#define SYS_THREAD_STACK_SIZE  (SYS_THREAD_SIZE - offsetof(struct sys_thread, stack))

/** Dynamic data for a running system thread instance.
 *  The stack portion is only used during launch and exit.
 */
typedef struct sys_thread {
	uspace_arg_t uarg;
	fibril_t *fibril;
	thread_id_t id;
	uint8_t stack[];
} sys_thread_t;

#if JOIN_IS_IMPLEMENTED
static _Atomic sys_thread_t *last_exitted = NULL;
#endif

#ifdef CONFIG_SEPARATE_THREAD_POOLS
static FIBRIL_SEMAPHORE_INITIALIZE(light_exit_semaphore, 0);
static FIBRIL_SEMAPHORE_INITIALIZE(heavy_exit_semaphore, 0);
#else
static FIBRIL_SEMAPHORE_INITIALIZE(thread_exit_semaphore, 0);
#endif

static errno_t sys_thread_create(uspace_arg_t *uarg, const char *name,
    thread_id_t *out_tid)
{
	return __SYSCALL4(SYS_THREAD_CREATE, (sysarg_t) uarg,
	    (sysarg_t) name, (sysarg_t) str_size(name), (sysarg_t) out_tid);
}

static _Noreturn void sys_thread_exit(void)
{
	__SYSCALL1(SYS_THREAD_EXIT, 0);
	__builtin_unreachable();
}

static thread_id_t sys_thread_get_id(void)
{
	thread_id_t thread_id;
	(void) __SYSCALL1(SYS_THREAD_GET_ID, (sysarg_t) &thread_id);
	return thread_id;
}

/** Main thread function.
 *
 * This function is called from __thread_entry().
 */
void __thread_main(uspace_arg_t *uarg)
{
	sys_thread_t *t = (sys_thread_t *) uarg;
	assert(t);
	assert(t->fibril);

	fibril_setup(t->fibril);

#ifdef FUTEX_UPGRADABLE
	rcu_register_fibril();
	futex_upgrade_all_and_wait();
#endif

	/* Sleep the fibril until it's time to exit. */
#ifdef CONFIG_SEPARATE_THREAD_POOLS
	if (t->fibril->is_heavy)
		fibril_semaphore_down(&heavy_exit_semaphore);
	else
		fibril_semaphore_down(&light_exit_semaphore);
#else
	fibril_semaphore_down(&thread_exit_semaphore);
#endif

	t->id = sys_thread_get_id();

	/*
	 * The running thread can't deallocate its own stack,
	 * but we can deallocate the previous exitted thread (if thread join is
	 * implemented).
	 * This way, only one thread stack is stuck waiting to be deallocated.
	 */
	// FIXME: join is not implemented
#if JOIN_IS_IMPLEMENTED
	// XXX: must be acquire-release for correct ordering of the id field
	sys_thread_t *last =
	    __atomic_exchange_n(&last_exitted, t, __ATOMIC_ACQ_REL);
	sys_thread_join(last->id);
	as_area_destroy(last);
#endif

#ifdef FUTEX_UPGRADABLE
	rcu_deregister_fibril();
#endif

	fibril_teardown(t->fibril, false);
	sys_thread_exit();
}

/**
 * Add one new anonymous thread to the fibril thread pool.
 *
 * Non-libc code should never call this function directly.
 * Instead, use `fibril_set_thread_count()`.
 */
errno_t thread_add(bool heavy)
{
	sys_thread_t *t = as_area_create(AS_AREA_ANY, SYS_THREAD_SIZE,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE |
	    AS_AREA_GUARD | AS_AREA_LATE_RESERVE,
	    AS_AREA_UNPAGED);
	if (t == AS_MAP_FAILED)
		return ENOMEM;

	/* Allocate memory for the thread fibril data. */
	t->fibril = fibril_alloc();
	if (!t->fibril) {
		as_area_destroy(t);
		return ENOMEM;
	}
	t->fibril->is_heavy = heavy;

	/* Make heap thread safe. */
	malloc_enable_multithreaded();

	t->uarg.uspace_entry = (void *) FADDR(__thread_entry);
	t->uarg.uspace_stack = &t->stack[0];
	t->uarg.uspace_stack_size = SYS_THREAD_STACK_SIZE;
	t->uarg.uspace_thread_function = NULL;
	t->uarg.uspace_thread_arg = NULL;
	t->uarg.uspace_uarg = &t->uarg;

	errno_t rc = sys_thread_create(&t->uarg, "", NULL);
	if (rc != EOK) {
		fibril_free(t->fibril);
		as_area_destroy(t);
	}
	return rc;
}

/**
 * Remove one thread from the fibril thread pool.
 *
 * This function will never terminate the main thread, i.e. the thread that
 * first entered `main()`, but additional calls to `thread_remove()` may
 * cause the same numberl of future threads created by `thread_add()` to exit
 * immediately.
 *
 * Non-libc code should never call this function directly.
 * Instead, use `fibril_set_thread_count()`.
 */
void thread_remove(bool heavy)
{
#ifdef CONFIG_SEPARATE_THREAD_POOLS
	if (heavy)
		fibril_semaphore_up(&heavy_exit_semaphore);
	else
		fibril_semaphore_up(&light_exit_semaphore);
#else
	fibril_semaphore_up(&thread_exit_semaphore);
#endif
}

/**
 * Block thread executing the current fibril unconditionally
 * for specified number of microseconds.
 */
void fibril_thread_usleep(useconds_t usec)
{
	(void) __SYSCALL1(SYS_THREAD_USLEEP, usec);
}

/**
 * Block thread executing the current fibril unconditionally
 * for specified number of seconds.
 */
void fibril_thread_sleep(unsigned int sec)
{
	/*
	 * Sleep in 1000 second steps to support
	 * full argument range
	 */

	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;

		fibril_thread_usleep(period * 1000000);
		sec -= period;
	}
}

/** @}
 */
