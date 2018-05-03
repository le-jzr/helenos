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

#include <str.h>

/** Fill block of memory.
 *
 * Fill cnt bytes at dst address with the value val.
 *
 * @param dst Destination address to fill.
 * @param cnt Number of bytes to fill.
 * @param val Value to fill.
 *
 */
void memsetb(void *dst, size_t cnt, uint8_t val)
{
	memset(dst, val, cnt);
}

/** Fill block of memory.
 *
 * Fill cnt words at dst address with the value val. The filling
 * is done word-by-word.
 *
 * @param dst Destination address to fill.
 * @param cnt Number of words to fill.
 * @param val Value to fill.
 *
 */
void memsetw(void *dst, size_t cnt, uint16_t val)
{
	size_t i;
	uint16_t *ptr = (uint16_t *) dst;

	for (i = 0; i < cnt; i++)
		ptr[i] = val;
}

/** Fill block of memory.
 *
 * Fill cnt bytes at dst address with the value val.
 *
 * @param dst Destination address to fill.
 * @param val Value to fill.
 * @param cnt Number of bytes to fill.
 *
 * @return Destination address.
 *
 */
void *memset(void *dst, int val, size_t cnt)
{
	uint8_t *dp = (uint8_t *) dst;

	while (cnt-- != 0)
		*dp++ = val;

	return dst;
}

#if 0

/** Fill memory block with a constant value. */
void *memset(void *dest, int b, size_t n)
{
	char *pb;
	unsigned long *pw;
	size_t word_size;
	size_t n_words;

	unsigned long pattern;
	size_t i;
	size_t fill;

	/* Fill initial segment. */
	word_size = sizeof(unsigned long);
	fill = word_size - ((uintptr_t) dest & (word_size - 1));
	if (fill > n)
		fill = n;

	pb = dest;

	i = fill;
	while (i-- != 0)
		*pb++ = b;

	/* Compute remaining size. */
	n -= fill;
	if (n == 0)
		return dest;

	n_words = n / word_size;
	n = n % word_size;
	pw = (unsigned long *) pb;

	/* Create word-sized pattern for aligned segment. */
	pattern = 0;
	i = word_size;
	while (i-- != 0)
		pattern = (pattern << 8) | (uint8_t) b;

	/* Fill aligned segment. */
	i = n_words;
	while (i-- != 0)
		*pw++ = pattern;

	pb = (char *) pw;

	/* Fill final segment. */
	i = n;
	while (i-- != 0)
		*pb++ = b;

	return dest;
}

#endif

/** @}
 */

