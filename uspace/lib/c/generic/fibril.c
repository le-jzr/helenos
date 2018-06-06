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

#include <fibril.h>

#include <assert.h>
#include <stdlib.h>
#include <mem.h>

#include <adt/list.h>
#include <stack.h>
#include <tls.h>
#include <as.h>
#include <context.h>
#include <futex.h>

#ifdef FUTEX_UPGRADABLE
#include <rcu.h>
#endif

#include "private/fibril.h"

typedef enum {
	FIBRIL_FROM_DEAD,
	FIBRIL_FROM_BLOCKED,
	FIBRIL_PREEMPT,
} fibril_switch_type_t;

/* This futex serializes access to global data. */
static futex_t fibril_futex = FUTEX_INITIALIZER;
// FIXME: This should be relaxed atomic in order to not violate C11 memory model.
static fibril_t *fibril_futex_owner;
static int idling_threads = 0;

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(fibril_list);
static LIST_INITIALIZE(zombie_list);
static LIST_INITIALIZE(timer_list);

static void default_idle_func(suseconds_t timeout)
{
	(void) timeout;
}

static void default_poke_func(void)
{
}

static fibril_idle_func_t fibril_idle_func = default_idle_func;
static fibril_poke_func_t fibril_poke_func = default_poke_func;

void fibril_global_lock(void)
{
	assert(fibril_futex_owner != fibril_self());
	futex_lock(&fibril_futex);
	assert(fibril_futex_owner == NULL);
	fibril_futex_owner = fibril_self();
}

void fibril_global_unlock(void)
{
	assert(fibril_futex_owner == fibril_self());
	fibril_futex_owner = NULL;

	/* If there are zombies queued, steal the list before unlock. */
	LIST_INITIALIZE(private_zombie_list);
	list_concat(&private_zombie_list, &zombie_list);

	/* Determine whether there is a thread in need of a good poking. */
	bool poke = (idling_threads > 0) && !list_empty(&ready_list);

	futex_unlock(&fibril_futex);

	/* Cleanup the zombies after unlock. all_link is already cleared. */
	fibril_t *z;
	while ((z = list_pop(&private_zombie_list, fibril_t, link))) {
		if (z->stack)
			as_area_destroy(z->stack);
		tls_free(z->tcb);
		free(z);
	}

	/*
	 * Poke an idler.
	 * We do this after unlock and one at a time because
	 * (a) we don't want to execute the user function under lock, and
	 * (b) the pokees are immediately locking the fibril_futex,
	 *     so this order avoids the woken up thread going immediately
	 *     back to sleep. The poked thread will acquire the futex
	 *     uncontested (in most cases), and will keep the ball rolling
	 *     on unlock.
	 */
	if (poke)
		fibril_poke_func();
}

static void fibril_global_lock_hand_to(fibril_t *fibril)
{
	assert(fibril_futex_owner == fibril_self());
	fibril_futex_owner = fibril;
}

void fibril_global_assert_is_locked(void)
{
	assert(fibril_futex_owner == fibril_self());
}

static void fibril_zombify(fibril_t *fibril)
{
	fibril_global_assert_is_locked();
	list_remove(&fibril->all_link);
	list_append(&fibril->link, &zombie_list);
}

static int fibril_switch(fibril_switch_type_t stype);

/* Only used as marker for triggered events. */
static fibril_t _fibril_event_triggered;
#define _FIBRIL_EVENT_TRIGGERED (&_fibril_event_triggered)

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

	/*
	 * Fibril begins its life switched from another fibril.
	 * The fibril for `main()` is created by other means.
	 */
	fibril_global_unlock();

	fibril_t *fibril = fibril_self();

	/* Call the implementing function. */
	fibril->retval = fibril->func(fibril->arg);

	fibril_global_lock();
	fibril_switch(FIBRIL_FROM_DEAD);
	/* Not reached */
}

static fibril_t *fibril_alloc(void)
{
	fibril_t *fibril = malloc(sizeof(fibril_t));
	if (!fibril)
		return NULL;

	tcb_t *tcb = tls_make();
	if (!tcb) {
		free(fibril);
		return NULL;
	}

	memset(fibril, 0, sizeof(*fibril));
	fibril->tcb = tcb;
	tcb->fibril_data = fibril;
	return fibril;
}

/** Setup fibril information into TCB structure
 *
 */
fibril_t *fibril_setup(void)
{
	fibril_t *fibril = fibril_alloc();
	if (!fibril)
		return NULL;

	__tcb_set(fibril->tcb);

	fibril_global_lock();
	list_append(&fibril->all_link, &fibril_list);
	fibril_global_unlock();
	return fibril;
}

void fibril_teardown(fibril_t *fibril)
{
	fibril_global_lock();
	fibril_zombify(fibril);
	fibril_global_unlock();
}

/** Fire all timeouts that expired. */
static suseconds_t handle_expired_timeouts(void)
{
	if (list_empty(&timer_list))
		return -1;

	struct timeval tv;
	getuptime(&tv);

	while (!list_empty(&timer_list)) {
		link_t *cur = list_first(&timer_list);
		fibril_tmr_t *tmr = list_get_instance(cur, fibril_tmr_t, link);

		if (tv_gt(&tmr->expires, &tv))
			return tv_sub_diff(&tmr->expires, &tv);

		list_remove(&tmr->link);

		tmr->fn(tmr->arg);

		fibril_notify_locked(&tmr->finished);
	}

	return -1;
}

static void run_idle_func(suseconds_t timeout)
{
	idling_threads++;
	fibril_global_unlock();
	fibril_idle_func(timeout);
	fibril_global_lock();
	idling_threads--;
}

/** Switch from the current fibril.
 *
 * `fibril_futex` must be held on entry, and it is still held on exit.
 *
 * @param stype Switch type.
 *              One of FIBRIL_PREEMPT, FIBRIL_FROM_BLOCKED, FIBRIL_FROM_DEAD.
 *              The parameter describes the circumstances of the switch.
 *
 * @return 0 if there is no ready fibril,
 * @return 1 otherwise.
 *
 */
static int fibril_switch(fibril_switch_type_t stype)
{
	fibril_global_assert_is_locked();
	// TODO: assert that the running thread doesn't hold any futexes other than `fibril_futex`.

	fibril_t *srcf = __tcb_get()->fibril_data;
	fibril_t *dstf = NULL;

	/* Run expired timers. */
	suseconds_t next_timeout = handle_expired_timeouts();

	if (stype == FIBRIL_PREEMPT) {
		// XXX: If two or more fibrils are yielding to each other,
		//      we need to make sure the idle func still gets called,
		//      so that IPC messages can arrive.
		//      For simplicity,
		//      we always call the idle func when yielding,
		//      whether or not there are ready fibrils.

		/* Run the idle function once without blocking. */
		run_idle_func(0);

		if (list_empty(&ready_list))
			/* No ready fibril. */
			return 0;
	} else {
		/*
		 * If no fibril is ready, and we have to block,
		 * loop the idle function.
		 */
		while (list_empty(&ready_list)) {
			run_idle_func(next_timeout);
			next_timeout = handle_expired_timeouts();
		}
	}

	assert(!list_empty(&ready_list));

	/* Get the next ready fibril. */
	dstf = list_get_instance(list_first(&ready_list), fibril_t, link);
	list_remove(&dstf->link);

	/* Put the current fibril into the correct list. */
	switch (stype) {
	case FIBRIL_PREEMPT:
		list_append(&srcf->link, &ready_list);
		break;
	case FIBRIL_FROM_DEAD:
		fibril_zombify(srcf);
#ifdef FUTEX_UPGRADABLE
		rcu_deregister_fibril();
#endif
		break;
	case FIBRIL_FROM_BLOCKED:
		/* Nothing. */
		break;
	}

	fibril_global_lock_hand_to(dstf);

	/* Swap to the next fibril. */
	context_swap(&srcf->ctx, &dstf->ctx);

	/* Restored by another fibril! */

	fibril_global_assert_is_locked();
	return 1;
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
fibril_t *fibril_create_generic(errno_t (*func)(void *), void *arg, size_t stksz)
{
	size_t stack_size = (stksz == FIBRIL_DFLT_STK_SIZE) ?
	    stack_size_get() : stksz;
	void *stack = as_area_create(AS_AREA_ANY, stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (stack == AS_MAP_FAILED)
		return NULL;

	fibril_t *fibril = fibril_alloc();
	if (!fibril) {
		as_area_destroy(stack);
		return NULL;
	}
	fibril->stack = stack;
	fibril->func = func;
	fibril->arg = arg;

	context_create_t sctx = {
		.fn = fibril_main,
		.stack_base = fibril->stack,
		.stack_size = stack_size,
		.tls = fibril->tcb,
	};

	context_create(&fibril->ctx, &sctx);

	fibril_global_lock();
	list_append(&fibril->all_link, &fibril_list);
	fibril_global_unlock();
	return fibril;
}

/** Delete a fibril that is not running.
 *
 * Free resources of a fibril that has been created with fibril_create()
 * but never readied using fibril_add_ready().
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 */
void fibril_destroy(fibril_t *fibril)
{
	fibril_global_lock();
	assert(!fibril->running);
	fibril_zombify(fibril);
	fibril_global_unlock();
}

void fibril_wait_for_locked(fibril_event_t *event)
{
	assert(event->fibril == NULL || event->fibril == _FIBRIL_EVENT_TRIGGERED);

	if (event->fibril == NULL) {
		event->fibril = fibril_self();
		fibril_switch(FIBRIL_FROM_BLOCKED);
	}

	*event = FIBRIL_EVENT_INIT;
}

void fibril_wait_for(fibril_event_t *event)
{
	fibril_global_lock();
	fibril_wait_for_locked(event);
	fibril_global_unlock();
}

static void event_timeout_expired(void *arg)
{
	fibril_event_t *event = arg;
	assert(event->fibril != NULL);

	if (event->fibril == _FIBRIL_EVENT_TRIGGERED)
		return;

	list_append(&event->fibril->link, &ready_list);
	event->fibril = NULL;
}

/**
 * Same as `fibril_wait_for()`, except with a timeout.
 *
 * Compared to using regular `fibril_wait_for()` with a `fibril_tmr_t` timer,
 * this function avoids the possible situation where a notification is lost
 * due to a race with the timeout.
 */
errno_t fibril_wait_timeout_locked(fibril_event_t *event, struct timeval *expires)
{
	assert(event->fibril == NULL || event->fibril == _FIBRIL_EVENT_TRIGGERED);

	if (event->fibril == _FIBRIL_EVENT_TRIGGERED) {
		*event = FIBRIL_EVENT_INIT;
		return EOK;
	}
	event->fibril = fibril_self();

	fibril_tmr_t timer = FIBRIL_TMR_INIT;
	fibril_tmr_arm_locked(&timer, expires, event_timeout_expired, event);
	fibril_switch(FIBRIL_FROM_BLOCKED);
	fibril_tmr_disarm_locked(&timer);

	if (event->fibril == _FIBRIL_EVENT_TRIGGERED) {
		*event = FIBRIL_EVENT_INIT;
		return EOK;
	}

	assert(event->fibril == NULL);
	return ETIMEOUT;
}

errno_t fibril_wait_timeout(fibril_event_t *event, struct timeval *expires)
{
	fibril_global_lock();
	errno_t rc = fibril_wait_timeout_locked(event, expires);
	fibril_global_unlock();
	return rc;
}

void fibril_notify_locked(fibril_event_t *event)
{
	if (event->fibril == NULL) {
		event->fibril = _FIBRIL_EVENT_TRIGGERED;
		return;
	}

	if (event->fibril == _FIBRIL_EVENT_TRIGGERED)
		return;

	list_append(&event->fibril->link, &ready_list);
	event->fibril = _FIBRIL_EVENT_TRIGGERED;
}

void fibril_notify(fibril_event_t *event)
{
	fibril_global_lock();
	fibril_notify_locked(event);
	fibril_global_unlock();
}

/** Start a fibril that has not been running yet.
 */
void fibril_add_ready(fibril_t *fibril)
{
	fibril_global_lock();

	assert(!fibril->running);
	fibril->running = true;

	list_append(&fibril->link, &ready_list);
	fibril_global_unlock();
}

int fibril_yield(void)
{
	fibril_global_lock();
	int ret = fibril_switch(FIBRIL_PREEMPT);
	fibril_global_unlock();
	return ret;
}

/** @return the currently running fibril. */
fibril_t *fibril_self(void)
{
	return __tcb_get()->fibril_data;
}

/**
 * Global fibril lock must be held when calling this function.
 *
 * Arms a timer, causing function `fn` to be called with argument `arg` once
 * current time exceeds the value of `expires`.
 *
 * The function is called in the context of the first fibril that sleeps, exits,
 * or preempts after the timer expires, and is called with the global fibril
 * lock held. This is a low level primitive and should only be used for
 * implementing higher-level mechanisms.
 *
 * The caller must use fibril_tmr_disarm to clean up the timer, unless
 * the timer is guaranteed to fire before the `timer` object is deallocated.
 * If the timer has already fired by the time disarm is called, the disarm
 * has no effect.
 */
void fibril_tmr_arm_locked(fibril_tmr_t *timer, struct timeval *expires,
    fibril_tmr_func_t fn, void *arg)
{
	fibril_global_assert_is_locked();

	*timer = (fibril_tmr_t) {
		.fn = fn,
		.arg = arg,
		.finished = FIBRIL_EVENT_INIT,
		.expires = *expires,
	};

	link_t *iter = timer_list.head.next;
	while (iter != &timer_list.head) {
		fibril_tmr_t *cur = list_get_instance(iter, fibril_tmr_t, link);
		if (tv_gteq(&cur->expires, &timer->expires))
			break;

		iter = iter->next;
	}

	list_insert_before(&timer->link, iter);
}

void fibril_tmr_arm(fibril_tmr_t *timer, suseconds_t duration, fibril_tmr_func_t fn, void *arg)
{
	getuptime(&timer->expires);
	tv_add_diff(&timer->expires, duration);

	fibril_global_lock();
	fibril_tmr_arm_locked(timer, &timer->expires, fn, arg);
	fibril_global_unlock();
}

bool fibril_tmr_disarm_locked(fibril_tmr_t *timer)
{
	bool fired = !link_in_use(&timer->link);
	if (fired)
		fibril_wait_for_locked(&timer->finished);
	else
		list_remove(&timer->link);
	return fired;
}

/**
 * Disarm a timer previously armed via `fibril_tmr_arm()`.
 * If the timer expired already, this function waits until the callback
 * has finished executing and returns true.
 * Otherwise, the callback is guaranteed to not have been executed and
 * the function returns false.
 *
 * Only one fibril may execute this function at any given time for a given
 * timer. Multiple fibrils attempting to use the same timer concurrently
 * may result in process abort.
 *
 * @return whether the timer was fired before being disarmed.
 */
bool fibril_tmr_disarm(fibril_tmr_t *timer)
{
	fibril_global_lock();
	bool fired = fibril_tmr_disarm_locked(timer);
	fibril_global_unlock();
	return fired;
}

// FIXME: Better documentation.
/**
 * Sets an idle func and the poke func.
 * Both are executed while holding fibril global lock.
 */
void fibril_set_idle_func(fibril_idle_func_t idle_fn, fibril_poke_func_t poke_fn)
{
	fibril_idle_func = idle_fn;
	fibril_poke_func = poke_fn;
}

/** @}
 */
