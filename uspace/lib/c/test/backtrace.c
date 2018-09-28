/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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
/**
 * @file
 * @brief Test backtrace printout
 */

#include <pcut/pcut.h>
#include <stacktrace.h>
#include <backtrace.h>

PCUT_INIT;

PCUT_TEST_SUITE(backtrace);

PCUT_TEST(stacktrace_kio_print)
{
	kio_printf("Testing stacktrace_kio_print():\n");
	stacktrace_kio_print();
}

PCUT_TEST(stacktrace_print)
{
	printf("Testing stacktrace_print():\n");
	stacktrace_print();
}

static void _error_callback) (void *data, const char *msg, int rc)
{
	if (errnum < 0) {
		fprintf(stderr,
		    "libbacktrace error: %s (no debuginfo)\n", msg);
	} else {
		fprintf(stderr,
		    "libbacktrace error: %s (%s)\n", msg, str_error_name(rc));
	}
}

PCUT_TEST(libbacktrace)
{
	printf("Testing libbacktrace:\n");

	struct backtrace_state *s = backtrace_create_state(__pcb.exepath, 0,
	    _error_callback, NULL);

	backtrace_print(s, 0, stdout);
}

PCUT_EXPORT(backtrace);
