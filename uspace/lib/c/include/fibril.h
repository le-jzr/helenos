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

// FIXME: Not necessary, fix includes downstream.
#include <adt/list.h>

#include <types/common.h>
#include <time.h>

typedef struct fibril fibril_t;

typedef struct {
	fibril_t *owned_by;
} fibril_owner_info_t;


typedef sysarg_t fid_t;

/** Fibril-local variable specifier */
#define fibril_local __thread

#define FIBRIL_DFLT_STK_SIZE	0

extern fid_t fibril_create_generic(errno_t (*)(void *), void *, size_t);
extern fid_t fibril_create(errno_t (*)(void *), void *);
extern fid_t fibril_run_heavy(errno_t (*)(void *), void *);
extern errno_t fibril_make_heavy(fid_t);
extern void fibril_destroy(fid_t);
extern void fibril_add_ready(fid_t);
extern fid_t fibril_get_id(void);
extern int fibril_yield(void);

extern void fibril_set_thread_count(int);
extern errno_t fibril_force_thread_count(int);
extern void fibril_thread_usleep(useconds_t);
extern void fibril_thread_sleep(unsigned int);

#endif

/** @}
 */
