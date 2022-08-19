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
/** @file
 */

#ifndef KERN_FAIR_SPIN_MUTEX_H_
#define KERN_FAIR_SPIN_MUTEX_H_

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef CONFIG_SMP

typedef struct fair_spin_mutex {
	atomic_uintptr_t owner;
	const char *name;
	ipl_t ipl;
	// With this size, the lock is guaranteed to work for
	// up to 2^16 - 2 concurrently locking processors/harts.
	// If more processors attempt to enter the critical
	// section at the same time, it will trigger a panic.
	atomic_uint_fast32_t ticketgate;
} fair_spin_mutex_t;

#define FAIR_SPIN_MUTEX_INITIALIZER(n) ((fair_spin_mutex_t) { \
	.ticketgate = ATOMIC_VAR_INIT(0), \
	.owner = ATOMIC_VAR_INIT(0), \
	.ipl = 0, \
	.name = n, \
})

#else

typedef struct fair_spin_mutex {
	bool locked;
	ipl_t ipl;
	const char *name;
} fair_spin_mutex_t;

#define FAIR_SPIN_MUTEX_INITIALIZER(n) (fair_spin_mutex_t) { .locked = false, .ipl = 0, .name = n }

#endif


#define fair_spin_mutex_assert_owned(mutex) assert(fair_spin_mutex_probably_owned__(mutex))
#define fair_spin_mutex_assert_not_owned(mutex) assert(fair_spin_mutex_probably_not_owned__(mutex))

extern void fair_spin_mutex_lock(fair_spin_mutex_t *);
extern void fair_spin_mutex_unlock(fair_spin_mutex_t *);
extern bool fair_spin_mutex_try_lock(fair_spin_mutex_t *);

extern bool fair_spin_mutex_probably_owned__(fair_spin_mutex_t *);
extern bool fair_spin_mutex_probably_not_owned__(fair_spin_mutex_t *);

#endif

/** @}
 */
