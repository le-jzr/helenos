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

/** @addtogroup libmath
 * @{
 */
/** @file
 */

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <math.h>
#include <stdint.h>

#include "../test/testables.h"

#define M_TAU (2 * M_PI)

/**
 * Calculates x modulo Tau.
 *
 * For trigonometric functions to be accurate on large arguments, we first need
 * to accurately compute the remainder of division by Tau.
 * Unfortunately, fmod() cannot be used for this. fmod() is exact if the
 * divisor is exact, but even the tiniest input imprecision is amplified to the
 * magnitude of the first argument. Since Tau is irrational, fmod(x, Tau) would
 * give garbage for x significantly larger than Tau.
 *
 * Instead, we use a particular property of the modulo operation, which allows
 * us to use precomputed values of the modulo for large powers of two, i.e.:
 *
 *         (n * 2^k) % Tau = (n  * (2^k % Tau)) % Tau
 *
 * As long as n is an integer.
 *
 * In this formula, n is relatively small and (2^y mod Tau) is determined using
 * precomputed tables, limiting the error to something bearable.
 *
 * @param x  Argument of the modulo.
 * @return   A value in the interval (-Tau, Tau), which is the remainder of x / Tau.
 */
double __fmod_tau(double x)
{
	if (isnan(x))
		return __builtin_nan();

	if (isinf(x)) {
		errno = EDOM;
		return __builtin_nan();
	}

	int e;
	double y = frexp(x, &e);

	if (e <= 2)
		return x;

	return x * __fmod_pow2_tau(e);
}

double __fmod_int_tau(uint64_t i)
{
}

double __fmod_pow2_tau(int e)
{
	assert(e > 2);
}

/** @}
 */
