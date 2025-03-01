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

#include <assert.h>
#include <io/kio.h>
#include <panic.h>
#include <stacktrace.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

__thread int _nested_panics = 0;

void panic(const char *fmt, ...)
{
    va_list vl;
    va_start(vl);
    kio_vprintf(fmt, vl);
    va_end(vl);

    stacktrace_kio_print();

	if (_nested_panics > 0)
	   abort();

	_nested_panics++;

	va_start(vl);
	vprintf(fmt, vl);
	va_end(vl);

	stacktrace_print();

	abort();
}

void __helenos_assert_quick_abort(const char *cond, const char *file, unsigned int line)
{
	/* Sometimes we know in advance that regular printf() would likely fail. */
	_nested_panics++;

	panic("Assertion failed (%s) in task %" PRIu64 ", file \"%s\", line %u.\n",
	    cond, task_get_id(), file, line);
}

void __helenos_assert_abort(const char *cond, const char *file, unsigned int line)
{
   	panic("Assertion failed (%s) in task %" PRIu64 ", file \"%s\", line %u.\n",
	    cond, task_get_id(), file, line);
}

/** @}
 */
