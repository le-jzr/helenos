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

/**
 * Compute the modulo operation for two non-negative floating-point numbers
 * fx and fy, expressed in the form fx = x * 2^ex, where x is an integer.
 * y must have the most significant bit unset.
 * ex must be >= ey.
 */
static inline void _fmod_split(uint64_t x, int ex, uint64_t y, int ey,
    uint64_t *res, int *eres)
{
	assert(ex >= ey);

	/* Make y as small as possible. */
	int trailing = __builtin_ctzll(y);
	int shift = min(ex - ey, trailing);
	y >>= shift;
	ey += shift;

	while (ex > ey) {
		/* Make x as large as possible. */
		int leading = __builtin_clzll(x);
		int shift = min(ex - ey, leading);
		x <<= shift;
		ex -= shift;

		/* Equivalent to fx % (fy * n), where n is a power of 2. */
		x %= y;
	}

	assert(ex == ey);

	/* One extra check in case ex==ey on entry. */
	if (x >= y)
		x %= y;

	*res = x;
	*eres = ex;
}

/**
 * Split x into an unsigned integral value and a power of two multiplier.
 * Sign of input is ignored.
 */
static inline uint64_t _frexpi64(double x, int *e)
{
	const int exp_bias = DBL_MAX_EXP - 1;
	const int mant_bits = DBL_MANT_DIG - 1;
	const uint64_t mant_mask = (UINT64_C(1) << mant_bits) - 1;

	union {
		double f;
		uint64_t i;
	} u = { .f = fabs(x) };

	int raw_e = (u.i >> mant_bits);
	*e = raw_e - exp_bias - mant_bits;
	uint64_t mant = u.i & mant_mask;

	if (raw_e == 0) {
		/* Denormalized, adjust exponent by one. */
		*e += 1;
	} else {
		/* Normalized, add the hidden bit. */
		mant |= (UINT64_C(1) << mant_bits);
	}

	return mant;
}

/**
 * Return the value i * 2^e, truncating any extra bits if necessary.
 */
static inline double _ldexpi64(uint64_t i, int e)
{
	const int exp_bias = DBL_MAX_EXP - 1;
	const int mant_bits = DBL_MANT_DIG - 1;
	const int need_leading = 64 - DBL_MANT_DIG;

	int leading = __builtin_clzll(i);

	if (leading >= need_leading) {
		i <<= (leading - need_leading);
		e -= (leading - need_leading);
	} else {
		i >>= (need_leading - leading);
		e += (need_leading - leading);
	}

	int raw_exp = e + exp_bias + mant_bits;
	if (raw_exp <= 0) {
		/* Denormalized result. */
		if ((1 - raw_exp) >= 64)
			i = 0;
		else
			i >>= (1 - raw_exp);
		raw_exp = 0;
	} else {
		/* Normalized result, strip the hidden bit. */
		assert(i & (UINT64_C(1) << mant_bits));
		i &= ~(UINT64_C(1) << mant_bits);
	}

	union {
		double f;
		uint64_t i;
	} u;

	u.i = (((uint64_t) raw_exp) << mant_bits) | i;
	return u.f;
}

/* Exposed for testing only. */
void __testable_fmod_split(uint64_t x, int ex, uint64_t y, int ey,
    uint64_t *res, int *eres)
{
	_fmod_split(x, ex, y, ey, res, eres);
}

uint64_t __testable_frexpi64(double x, int *e)
{
	return _frexpi64(x, e);
}

double __testable_ldexpi64(uint64_t i, int e)
{
	return _ldexpi64(i, e);
}

/** Remainder function (64-bit floating point)
 *
 * Calculate the modulo of dividend by divisor.
 *
 * These functions shall return the value x - i * y,
 * for some integer i such that, if y is non-zero,
 * the result has the same sign as x and magnitude
 * less than the magnitude of y.
 *
 * @param dividend Dividend.
 * @param divisor  Divisor.
 *
 * @return Modulo.
 *
 */
double fmod(double x, double y)
{
	if (isnan(x) || isnan(y))
		return __builtin_nan("");

	if (isinf(x) || y == 0.0) {
		errno = EDOM;
		return __builtin_nan("");
	}

	if (islessequal(fabs(x), fabs(y)))
		return x;

	/* Reformat x as integer multiplied by power of two. */
	int ex;
	uint64_t ix = _frexpi64(x, &ex);

	/* Reformat y as integer multiplied by power of two. */
	int ey;
	uint64_t iy = _frexpi64(y, &ey);

	/* Do the modulo operation. */
	int er;
	uint64_t ir;
	_fmod_split(ix, ex, iy, ey, &ir, &er);

	/* Convert result back to the float format. */
	return copysign(_ldexpi64(ir, er), x);
}

/** @}
 */
