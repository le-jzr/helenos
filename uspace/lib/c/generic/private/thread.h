/*
 * Copyright (c) 2011 Martin Decky
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

#ifndef LIBC_PRIVATE_THREAD_H_
#define LIBC_PRIVATE_THREAD_H_

#include <fibril.h>
#include <fibril_private.h>
#include <abi/proc/uarg.h>

extern void __thread_entry(void);
extern void __thread_main(uspace_arg_t *);
extern errno_t thread_add(void);
extern void thread_remove(void);

typedef enum {
	FIBRIL_PREEMPT,
	FIBRIL_TO_MANAGER,
	FIBRIL_FROM_MANAGER,
	FIBRIL_FROM_DEAD
} fibril_switch_type_t;

extern fibril_t *fibril_alloc(void);
extern void fibril_free(fibril_t *);
extern void fibril_setup(fibril_t *);
extern void fibril_teardown(fibril_t *f, bool locked);
extern int fibril_switch(fibril_switch_type_t stype);

extern void fibril_add_manager(fid_t fid);
extern void fibril_remove_manager(void);

#endif

/** @}
 */
