/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2007 Jakub Jermar
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
#include <abi/mm/as.h>
#include <as.h>
#include <stdio.h>
#include <libarch/barrier.h>
#include <context.h>
#include <futex.h>
#include <assert.h>
#include <async.h>

#include <io/kio.h>

#include "private/thread.h"

#ifdef FUTEX_UPGRADABLE
#include <rcu.h>
#endif

/**
 * This futex serializes access to ready_list,
 * manager_list and fibril_list.
 */
static futex_t fibril_futex = FUTEX_INITIALIZER;

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(manager_list);
static LIST_INITIALIZE(fibril_list);

#ifdef CONFIG_SEPARATE_THREAD_POOLS
static futex_t heavy_ready_list_sem = FUTEX_INITIALIZE(0);
static LIST_INITIALIZE(heavy_ready_list);
static const bool separate_pools = true;
#else
static const bool separate_pools = false;
#endif

// TODO: Currently, we default to 1 thread because historically, some servers
//       assumed that all fibrils run on one thread. This should be fixed and
//       the thread count should eventually be set according to the environment
//       (i.e. #cpus and/or environment variables).

/*
 * Number of threads reserved for light fibrils, not including the main thread.
 */
static int thread_count_light = 0;

/*
 * Number of heavy fibrils running.
 * Each heavy fibril reserves an extra thread.
 * Stays zero when separate pools are enabled.
 */
static int thread_count_heavy = 0;

/*
 * Number of threads currently executing, not including the main thread.
 * Doesn't include heavy pool when separate pools are enabled.
 */
static int thread_count_real = 0;

/** Function that spans the whole life-cycle of a fibril.
 *
 * Each fibril begins execution in this function. Then the function implementing
 * the fibril logic is called.  After its return, the return value is saved.
 * The fibril then switches to another fibril, which cleans up after it.
 *
 */
static void fibril_main(void)
{
#ifdef FUTEX_UPGRADABLE
	rcu_register_fibril();
#endif

	/* fibril_futex and async_futex are locked when a fibril is started. */
	futex_unlock(&fibril_futex);
	futex_up(&async_futex);

	fibril_t *fibril = __tcb_get()->fibril_data;

	/* Call the implementing function. */
	fibril->retval = fibril->func(fibril->arg);

	futex_down(&async_futex);
	fibril_switch(FIBRIL_FROM_DEAD);
	/* Not reached */
}

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
 * Free fibril data that hasn't been set up yet.
 *
 * If `fibril_setup()` was already called, use `fibril_teardown()`.
 */
void fibril_free(fibril_t *fibril)
{
	tls_free(fibril->tcb);
	free(fibril);
}

/** Setup fibril information. */
void fibril_setup(fibril_t *fibril)
{
	__tcb_set(fibril->tcb);

	futex_lock(&fibril_futex);
	list_append(&fibril->all_link, &fibril_list);
	futex_unlock(&fibril_futex);
}

void fibril_teardown(fibril_t *fibril, bool locked)
{
	if (!locked)
		futex_lock(&fibril_futex);
	list_remove(&fibril->all_link);
	if (!locked)
		futex_unlock(&fibril_futex);

	fibril_free(fibril);
}

#ifdef CONFIG_SEPARATE_THREAD_POOLS
static int fibril_switch_heavy(fibril_switch_type_t stype)
{
	fibril_t *srcf = __tcb_get()->fibril_data;

	// Manager fibrils run in light thread pool.
	assert(stype != FIBRIL_FROM_MANAGER);

	/* Preemption has no meaning for heavy fibril. */
	if (stype == FIBRIL_PREEMPT)
		return 0;

	/* async_futex is locked, but we don't need it. */
	assert((atomic_signed_t) async_futex.val.count <= 0);
	futex_up(&async_futex);

	/* Wait until a fibril is available. */
	futex_down(&heavy_ready_list_sem);

	futex_lock(&fibril_futex);
	assert(!list_empty(&heavy_ready_list));

	fibril_t *dstf = list_get_instance(list_first(&heavy_ready_list),
	    fibril_t, link);
	list_remove(&dstf->link);

	if (stype == FIBRIL_FROM_DEAD) {
		dstf->clean_after_me = srcf;
		list_remove(&srcf->all_link);
	}

#ifdef FUTEX_UPGRADABLE
	if (stype == FIBRIL_FROM_DEAD)
		rcu_deregister_fibril();
#endif

	futex_give_to(&fibril_futex, dstf);

	/* Swap to the next fibril. */
	context_swap(&srcf->ctx, &dstf->ctx);

	/* Restored by another fibril! */

	/* Must be after context_swap()! */
	futex_unlock(&fibril_futex);

	if (srcf->clean_after_me) {
		/*
		 * Cleanup after the dead fibril from which we
		 * restored context here.
		 */
		fibril_t *f = srcf->clean_after_me;
		srcf->clean_after_me = NULL;

		assert(f->stack);
		as_area_destroy(f->stack);
		fibril_teardown(f, true);
		thread_remove(true);
	}

	return 1;
}
#endif

/** Switch from the current fibril.
 *
 * The async_futex must be held when entering this function,
 * and is still held on return.
 *
 * @param stype Switch type. One of FIBRIL_PREEMPT, FIBRIL_TO_MANAGER,
 *              FIBRIL_FROM_MANAGER, FIBRIL_FROM_DEAD. The parameter
 *              describes the circumstances of the switch.
 *
 * @return 0 if there is no ready fibril,
 * @return 1 otherwise.
 *
 */
int fibril_switch(fibril_switch_type_t stype)
{
	/* Make sure the async_futex is held. */
	assert((atomic_signed_t) async_futex.val.count <= 0);

	fibril_t *srcf = __tcb_get()->fibril_data;
	fibril_t *dstf = NULL;

#ifdef CONFIG_SEPARATE_THREAD_POOLS
	if (srcf->is_heavy)
		return fibril_switch_heavy(stype);
#endif

	futex_lock(&fibril_futex);

	/*
	 * There is always at least enough threads to run each of the heavy
	 * fibrils, plus the implicit main thread.
	 */
	assert(thread_count_real >= thread_count_heavy);

	/* Choose a new fibril to run */
	if (list_empty(&ready_list)) {
		if (stype == FIBRIL_PREEMPT || stype == FIBRIL_FROM_MANAGER) {
			// FIXME: This means that as long as there is a fibril
			// that only yields, IPC messages are never retrieved.
			futex_unlock(&fibril_futex);
			return 0;
		}

		/* If we are going to manager and none exists, create it */
		while (list_empty(&manager_list)) {
			futex_unlock(&fibril_futex);
			async_create_manager();
			futex_lock(&fibril_futex);
		}

		dstf = list_get_instance(list_first(&manager_list),
		    fibril_t, link);
	} else {
		dstf = list_get_instance(list_first(&ready_list), fibril_t,
		    link);
	}

	list_remove(&dstf->link);
	if (stype == FIBRIL_FROM_DEAD)
		dstf->clean_after_me = srcf;

	/* Put the current fibril into the correct run list */
	switch (stype) {
	case FIBRIL_PREEMPT:
		list_append(&srcf->link, &ready_list);
		break;
	case FIBRIL_FROM_MANAGER:
		list_append(&srcf->link, &manager_list);
		break;
	case FIBRIL_FROM_DEAD:
		if (srcf->is_heavy)
			thread_count_heavy--;
		list_remove(&srcf->all_link);
		/* Not adding to any list. */
		break;
	case FIBRIL_FROM_BLOCKED:
		// Nothing.
		break;
	}

	/* Check if we need to exit a thread. */
	if (thread_count_heavy + thread_count_light + 4 < thread_count_real / 2) {
		/* We keep up to twice the number of currently required threads,
		 * + 4, to avoid thrashing when heavy fibrils are continually
		 * allocated and deallocated.
		 */

		assert(thread_count_real > 0);

		// FIXME: We can't signal the semaphore with async_futex locked.
		if (stype == FIBRIL_FROM_MANAGER || stype == FIBRIL_PREEMPT) {
			thread_count_real--;
			dstf->stop_thread = true;
		}
	}

#ifdef FUTEX_UPGRADABLE
	if (stype == FIBRIL_FROM_DEAD) {
		rcu_deregister_fibril();
	}
#endif

	futex_give_to(&fibril_futex, dstf);

	/* Swap to the next fibril. */
	context_swap(&srcf->ctx, &dstf->ctx);

	/* Restored by another fibril! */

	/* Must be after context_swap()! */
	futex_unlock(&fibril_futex);

	/* thread_remove() is internally semaphore up, which locks async_futex
	 * and potentially calls fibril_add_ready(), so neither fibril_futex,
	 * nor async_futex may be locked during the call.
	 */
	if (srcf->stop_thread) {
		srcf->stop_thread = false;
		thread_remove(false);
	}

	if (srcf->clean_after_me) {
		/*
		 * Cleanup after the dead fibril from which we
		 * restored context here.
		 */
		fibril_t *f = srcf->clean_after_me;
		srcf->clean_after_me = NULL;

		assert(f->stack);
		as_area_destroy(f->stack);
		fibril_teardown(f, true);
	}

	return 1;
}

/**
 * Turns a fibril that has not been started yet into a "heavy" fibril.
 * A heavy fibril can stall the running thread for arbitrary periods of
 * time (e.g. due to long computation or thread-blocking system calls)
 * without consequences.
 *
 * Implementation note: This is achieved by spawning a new thread when
 * this function is called, and destroying it after the fibril exits.
 * However, the thread is not pinned to the fibril that caused its creation.
 * Heavy fibrils cannot starve light fibrils or other heavy fibrils, but
 * it is possible for misbehaving light fibrils to starve heavy fibrils.
 */
errno_t fibril_make_heavy(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;

	futex_lock(&fibril_futex);
	assert(!fibril->is_running);

	if (fibril->is_heavy) {
		futex_unlock(&fibril_futex);
		return EOK;
	}

	if (separate_pools) {
		/* Always spawn a new thread. */
		futex_unlock(&fibril_futex);
		errno_t rc = thread_add(true);
		if (rc != EOK)
			return rc;
		futex_lock(&fibril_futex);
	} else {
		/* Check whether we need to spawn an additional thread. */
		if (thread_count_real < thread_count_heavy + 1) {
			futex_unlock(&fibril_futex);

			errno_t rc = thread_add(true);
			if (rc != EOK)
				return rc;

			futex_lock(&fibril_futex);
			thread_count_real++;
		}

		thread_count_heavy++;
	}

	fibril->is_heavy = true;
	futex_unlock(&fibril_futex);
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
	fibril_t *fibril = fibril_alloc();
	if (fibril == NULL)
		return 0;

	size_t stack_size = (stksz == FIBRIL_DFLT_STK_SIZE) ?
	    stack_size_get() : stksz;
	fibril->stack = as_area_create(AS_AREA_ANY, stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (fibril->stack == (void *) -1) {
		fibril_free(fibril);
		return 0;
	}

	fibril->func = func;
	fibril->arg = arg;

	context_create_t sctx = {
		.fn = fibril_main,
		.stack_base = fibril->stack,
		.stack_size = stack_size,
		.tls = fibril->tcb,
	};

	context_create(&fibril->ctx, &sctx);
	return (fid_t) fibril;
}

/** Delete a fibril that has never run.
 *
 * Free resources of a fibril that has been created with fibril_create()
 * but never readied using fibril_add_ready().
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 */
void fibril_destroy(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;

	if (fibril->is_heavy) {
		if (separate_pools) {
			thread_remove(true);
		} else {
			futex_lock(&fibril_futex);
			thread_count_heavy--;
			futex_unlock(&fibril_futex);
		}
	}

	as_area_destroy(fibril->stack);
	fibril_free(fibril);
}

/** Add a fibril to the ready list.
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 *
 */
void fibril_add_ready(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;

	futex_lock(&fibril_futex);
	if (!fibril->is_running) {
		fibril->is_running = true;
		list_append(&fibril->all_link, &fibril_list);
	}

#ifdef CONFIG_SEPARATE_THREAD_POOLS
	if (fibril->is_heavy) {
		list_append(&fibril->link, &heavy_ready_list);
		futex_unlock(&fibril_futex);
		futex_up(&heavy_ready_list_sem);
		return;
	}
#endif
	list_append(&fibril->link, &ready_list);

	/* Check whether we should spawn an additional thread. */
	if (thread_count_real < thread_count_heavy + thread_count_light) {
		futex_unlock(&fibril_futex);
		errno_t rc = thread_add(false);
		futex_lock(&fibril_futex);

		if (rc == EOK)
			thread_count_real++;
	}

	futex_unlock(&fibril_futex);
}

/** Add a fibril to the manager list.
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 *
 */
void fibril_add_manager(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;

	futex_lock(&fibril_futex);
	list_append(&fibril->link, &manager_list);
	futex_unlock(&fibril_futex);
}

/** Remove one manager from the manager list. */
void fibril_remove_manager(void)
{
	futex_lock(&fibril_futex);
	if (!list_empty(&manager_list))
		list_remove(list_first(&manager_list));
	futex_unlock(&fibril_futex);
}

/** Return fibril id of the currently running fibril.
 *
 * @return fibril ID of the currently running fibril.
 *
 */
fid_t fibril_get_id(void)
{
	return (fid_t) __tcb_get()->fibril_data;
}

int fibril_yield(void)
{
	futex_down(&async_futex);
	int ret = fibril_switch(FIBRIL_PREEMPT);
	futex_up(&async_futex);
	return ret;

/**
 * Set the number of threads in the fibril thread pool reserved for running
 * light fibrils. The total number of threads will become at least
 * `count + # of heavy fibrils`.
 *
 * The default count set at the program start depends on implementation,
 * execution environment (available hardware), and user settings. Under normal
 * circumstances, a program should never call this function explicitly.
 */
void fibril_set_thread_count(int count)
{
	assert(count > 0);

	futex_lock(&fibril_futex);
	/* -1 because the variables don't include the main thread that is
	 * always available until the program exits.
	 */
	thread_count_light = count - 1;
	futex_unlock(&fibril_futex);
}

/**
 * Same as `fibril_set_thread_count()`, except that it additionally forces
 * all thread to be created immediately instead of as needed.
 *
 * Used for some tests. Shouldn't be used by a normal program.
 */
errno_t fibril_force_thread_count(int count)
{
	assert(count > 0);

	futex_lock(&fibril_futex);
	thread_count_light = count - 1;

	while (thread_count_real < thread_count_heavy + thread_count_light) {
		futex_unlock(&fibril_futex);
		errno_t rc = thread_add(false);
		if (rc != EOK)
			return rc;
		futex_lock(&fibril_futex);
		thread_count_real++;
	}

	futex_unlock(&fibril_futex);
	return EOK;
}

fid_t fibril_create(errno_t (*func)(void *), void *arg)
{
	return fibril_create_generic(func, arg, FIBRIL_DFLT_STK_SIZE);
}

fid_t fibril_run_heavy(errno_t (*func)(void *), void *arg)
{
	fid_t f = fibril_create(func, arg);
	if (!f)
		return 0;

	if (fibril_make_heavy(f) != EOK) {
		fibril_destroy(f);
		return 0;
	}

	fibril_add_ready(f);
	return f;
}

/** @}
 */
