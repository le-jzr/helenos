/*
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
 * @brief Fair spin mutex.
 */

#include <synch/fairspinmutex.h>

#include <cpu.h>
#include <debug.h>
#include <stacktrace.h>

// Gate and ticket are in a single atomic variable to allow non-blocking lock attempts.
#define GATE_OFFSET 0
#define GATE(ticketgate) ((uint16_t) ((ticketgate) & 0xffffu))
#define TICKET_OFFSET 16
#define TICKET(ticketgate) ((uint16_t) (((ticketgate) >> TICKET_OFFSET) & 0xffffu))
#define TICKET_INC 0x10000u
#define TICKET_MASK 0xffff0000u

static inline uint16_t cpus_waiting(uint_fast32_t ticketgate)
{
	return TICKET(ticketgate) - GATE(ticketgate);
}

// Give it a bit of safety margin to make sure we detect the overflow condition
// on as many cpus as possible.
#define MAX_WAITING (UINT16_MAX / 2)

void fair_spin_mutex_lock(fair_spin_mutex_t *mutex)
{
	fair_spin_mutex_assert_not_owned(mutex);

	ipl_t ipl = interrupts_disable();

	// Acquire a ticket. Ticket is in the top half, so we don't care if the increment overflows.
	uint_fast32_t ticketgate = atomic_fetch_add_explicit(&mutex->ticketgate, TICKET_INC, memory_order_relaxed);
	uint16_t ticket = TICKET(ticketgate);

	if (cpus_waiting(ticketgate) > MAX_WAITING) {
		// There is more than MAX_WAITING processors currently waiting to enter.
		// Since that's halfway to exhausting the maximum numerical range,
		// do the "safe" thing and die loudly.
		// This is not an assert because this doesn't happen due to a code
		// bug, but rather due to HelenOS being run on a system with unexpectedly
		// large number of cores.
		panic("Too many processors locking a fair mutex at the same time");
	}

#ifdef CONFIG_DEBUG_SPINLOCK
	size_t i = 0;
	bool deadlock_reported = false;
#endif

	while (ticket != GATE(ticketgate)) {
		spin_loop_body();

#ifdef CONFIG_DEBUG_SPINLOCK
		/*
		 * We need to be careful about particular locks
		 * which are directly used to report deadlocks
		 * via printf() (and recursively other functions).
		 * This conserns especially printf_lock and the
		 * framebuffer lock.
		 *
		 * Any lock whose name is prefixed by "*" will be
		 * ignored by this deadlock detection routine
		 * as this might cause an infinite recursion.
		 * We trust our code that there is no possible deadlock
		 * caused by these locks (except when an exception
		 * is triggered for instance by printf()).
		 *
		 * We encountered false positives caused by very
		 * slow framebuffer interaction (especially when
		 * run in a simulator) that caused problems with both
		 * printf_lock and the framebuffer lock.
		 */
		if (mutex->name[0] != '*' && i++ > DEADLOCK_THRESHOLD) {
			printf("cpu%u: looping on spinlock %p:%s, caller=%p ticket=%d (%s)\n",
			    CPU->id, mutex, mutex->name, (void *) CALLER, GATE(ticketgate),
			    symtab_fmt_name_lookup(CALLER));
			stack_trace();
			i = 0;
			deadlock_reported = true;
		}
#endif

		ticketgate = atomic_load_explicit(&mutex->ticketgate, memory_order_acquire);
	}

#ifdef CONFIG_DEBUG_SPINLOCK
	CPU->mutex_locks++;

	if (deadlock_reported)
		printf("cpu%u: not deadlocked\n", CPU->id);

	if (mutex->name[0] == '!') {
		printf("cpu%u: acquired spinlock %p:%s, caller=%p ticket=%d (%s)\n",
		    CPU->id, mutex, mutex->name, (void *) CALLER, ticket,
		    symtab_fmt_name_lookup(CALLER));
		stack_trace();
	}
#endif

	// The mutex is now ours.
	mutex->ipl = ipl;
	atomic_store_explicit(&mutex->owner, (uintptr_t) CPU, memory_order_relaxed);
}

void fair_spin_mutex_unlock(fair_spin_mutex_t *mutex)
{
	fair_spin_mutex_assert_owned(mutex);

	atomic_store_explicit(&mutex->owner, 0, memory_order_relaxed);
	ipl_t ipl = mutex->ipl;

	uint_fast32_t ticketgate = atomic_load_explicit(&mutex->ticketgate, memory_order_relaxed);
	if (GATE(ticketgate) < UINT16_MAX) {
		// Easy case, we can just increment.
		(void) atomic_fetch_add_explicit(&mutex->ticketgate, 1, memory_order_release);
	} else {
		// GATE is its maximum value, we need to reset it to zero atomically.
		// This is guaranteed to succeed in finite time, since there's only so many
		// cpus that can increment the ticket while this cpu holds the lock.

		do {
			assert(GATE(ticketgate) == UINT16_MAX);
		} while (!atomic_compare_exchange_weak_explicit(
		    &mutex->ticketgate, &ticketgate, ticketgate & TICKET_MASK,
		    memory_order_release, memory_order_relaxed));
	}

	interrupts_restore(ipl);

#ifdef CONFIG_DEBUG_SPINLOCK
	CPU->mutex_locks--;

	if (mutex->name[0] == '!') {
		printf("cpu%u: released spinlock %p:%s, ticket=%d\n",
		    CPU->id, mutex, mutex->name, GATE(ticketgate));
		stack_trace();
	}
#endif
}

bool fair_spin_mutex_try_lock(fair_spin_mutex_t *mutex)
{
	fair_spin_mutex_assert_not_owned(mutex);

	ipl_t ipl = interrupts_disable();

	// Check if we can gain entry without waiting.
	uint_fast32_t ticketgate = atomic_load_explicit(&mutex->ticketgate, memory_order_relaxed);

	if (GATE(ticketgate) != TICKET(ticketgate) ||
	    !atomic_compare_exchange_strong_explicit(&mutex->ticketgate, &ticketgate,
	    ticketgate + TICKET_INC, memory_order_acquire, memory_order_relaxed)) {
		interrupts_restore(ipl);
		return false;
	}

	// The mutex is now ours.

#ifdef CONFIG_DEBUG_SPINLOCK
	CPU->mutex_locks++;

	if (mutex->name[0] == '!') {
		printf("cpu%u: acquired spinlock %p:%s, caller=%p ticket=%d (%s)\n",
		    CPU->id, mutex, mutex->name, (void *) CALLER, GATE(ticketgate),
		    symtab_fmt_name_lookup(CALLER));
		stack_trace();
	}
#endif

	mutex->ipl = ipl;
	atomic_store_explicit(&mutex->owner, (uintptr_t) CPU, memory_order_relaxed);
	return true;
}

bool fair_spin_mutex_probably_owned__(fair_spin_mutex_t *mutex)
{
	assert(CPU != NULL);
	return atomic_load_explicit(&mutex->owner, memory_order_relaxed) == (uintptr_t) CPU;
}

bool fair_spin_mutex_probably_not_owned__(fair_spin_mutex_t *mutex)
{
	assert(CPU != NULL);
	return atomic_load_explicit(&mutex->owner, memory_order_relaxed) != (uintptr_t) CPU;
}

/** @}
 */
