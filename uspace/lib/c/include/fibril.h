/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#ifndef LIBC_FIBRIL_H_
#define LIBC_FIBRIL_H_

#include <context.h>
#include <types/common.h>
#include <adt/list.h>
#include <libarch/tls.h>
#include <time.h>

#define FIBRIL_WRITER	1

typedef void (*fibril_idle_func_t)(suseconds_t);
typedef void (*fibril_poke_func_t)(void);

struct fibril;

typedef struct {
	struct fibril *fibril;
} fibril_event_t;

typedef struct {
	struct fibril *owned_by;
} fibril_owner_info_t;

typedef void (*fibril_tmr_func_t)(void *);

typedef struct {
	/** Timer list link. */
	link_t link;

	/** Expiration time. */
	struct timeval expires;

	/** Function to call. */
	fibril_tmr_func_t fn;
	void *arg;

	/** Event to trigger when the function finishes. */
	fibril_event_t finished;
} fibril_tmr_t;

typedef struct fibril {
	link_t link;
	link_t all_link;
	context_t ctx;
	void *stack;
	void *arg;
	errno_t (*func)(void *);
	tcb_t *tcb;

	errno_t retval;
	int flags;

	fibril_owner_info_t *waits_for;

	bool running;
} fibril_t;

// TODO: Left for backwards compatibility only.
typedef fibril_t *fid_t;

#define FIBRIL_EVENT_INIT ((fibril_event_t) {0})
#define FIBRIL_TMR_INIT ((fibril_tmr_t) {0})

/** Fibril-local variable specifier */
#define fibril_local __thread

#define FIBRIL_DFLT_STK_SIZE	0

extern fibril_t *fibril_create_generic(errno_t (*)(void *), void *, size_t);
extern void fibril_destroy(fibril_t *);
extern int fibril_yield(void);
extern fibril_t *fibril_self(void);
extern void fibril_add_ready(fid_t);

// FIXME: compatibility
static inline fid_t fibril_get_id(void)
{
	return fibril_self();
}


static inline fibril_t *fibril_create(errno_t (*func)(void *), void *arg)
{
	return fibril_create_generic(func, arg, FIBRIL_DFLT_STK_SIZE);
}

extern void fibril_wait_for(fibril_event_t *);
extern errno_t fibril_wait_timeout(fibril_event_t *, struct timeval *);
extern void fibril_notify(fibril_event_t *);
extern void fibril_tmr_arm(fibril_tmr_t *, suseconds_t, fibril_tmr_func_t, void *);
extern bool fibril_tmr_disarm(fibril_tmr_t *);

extern void fibril_global_lock(void);
extern void fibril_global_unlock(void);
extern void fibril_global_assert_is_locked(void);

extern void fibril_wait_for_locked(fibril_event_t *);
extern errno_t fibril_wait_timeout_locked(fibril_event_t *, struct timeval *);
extern void fibril_notify_locked(fibril_event_t *);
extern void fibril_tmr_arm_locked(fibril_tmr_t *, struct timeval *, fibril_tmr_func_t, void *);
extern bool fibril_tmr_disarm_locked(fibril_tmr_t *);

extern void fibril_set_idle_func(fibril_idle_func_t, fibril_poke_func_t);

#endif

/** @}
 */
