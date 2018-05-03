/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief Memory string operations.
 *
 * This file provides architecture independent functions to manipulate blocks
 * of memory. These functions are optimized as much as generic functions of
 * this type can be.
 */

#include <c.h>
#include <str.h>

/** Move memory block with possible overlapping.
 *
 * Copy cnt bytes from src address to dst address. The source
 * and destination memory areas may overlap.
 *
 * @param dst Destination address to copy to.
 * @param src Source address to copy from.
 * @param cnt Number of bytes to copy.
 *
 * @return Destination address.
 *
 */
void *memmove(void *dst, const void *src, size_t cnt)
{
	/* Nothing to do? */
	if (src == dst)
		return dst;

	/* Non-overlapping? */
	if ((dst >= src + cnt) || (src >= dst + cnt))
		return memcpy(dst, src, cnt);

	uint8_t *dp;
	const uint8_t *sp;

	/* Which direction? */
	if (src > dst) {
		/* Forwards. */
		dp = dst;
		sp = src;

		while (cnt-- != 0)
			*dp++ = *sp++;
	} else {
		/* Backwards. */
		dp = dst + (cnt - 1);
		sp = src + (cnt - 1);

		while (cnt-- != 0)
			*dp-- = *sp--;
	}

	return dst;
}

/** Compare two memory areas.
 *
 * @param s1  Pointer to the first area to compare.
 * @param s2  Pointer to the second area to compare.
 * @param len Size of the areas in bytes.
 *
 * @return Zero if areas have the same contents. If they differ,
 *	   the sign of the result is the same as the sign of the
 *	   difference of the first pair of different bytes.
 *
 */
int memcmp(const void *s1, const void *s2, size_t len)
{
	uint8_t *u1 = (uint8_t *) s1;
	uint8_t *u2 = (uint8_t *) s2;
	size_t i;

	for (i = 0; i < len; i++) {
		if (*u1 != *u2)
			return (int)(*u1) - (int)(*u2);
		++u1;
		++u2;
	}

	return 0;
}

/** @}
 */
