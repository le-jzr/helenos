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

/** Move memory block without overlapping.
 *
 * Copy cnt bytes from src address to dst address. The source
 * and destination memory areas cannot overlap.
 *
 * @param dst Destination address to copy to.
 * @param src Source address to copy from.
 * @param cnt Number of bytes to copy.
 *
 * @return Destination address.
 *
 */
void *memcpy(void *dst, const void *src, size_t cnt)
{
	uint8_t *dp = (uint8_t *) dst;
	const uint8_t *sp = (uint8_t *) src;

	while (cnt-- != 0)
		*dp++ = *sp++;

	return dst;
}

#if 0

// TODO: The "optimized" version is actually much slower in practice.
//       Find out why.

struct along {
	unsigned long n;
} __attribute__((packed));

static void *unaligned_memcpy(void *dst, const void *src, size_t n)
{
	size_t i, j;
	struct along *adst = dst;
	const struct along *asrc = src;

	for (i = 0; i < n / sizeof(unsigned long); i++)
		adst[i].n = asrc[i].n;

	for (j = 0; j < n % sizeof(unsigned long); j++)
		((unsigned char *) (((unsigned long *) dst) + i))[j] =
		    ((unsigned char *) (((unsigned long *) src) + i))[j];

	return (char *) dst;
}

/** Copy memory block. */
void *memcpy(void *dst, const void *src, size_t n)
{
	size_t i;
	size_t mod, fill;
	size_t word_size;
	size_t n_words;

	const unsigned long *srcw;
	unsigned long *dstw;
	const uint8_t *srcb;
	uint8_t *dstb;

	word_size = sizeof(unsigned long);

	/*
	 * Are source and destination addresses congruent modulo word_size?
	 * If not, use unaligned_memcpy().
	 */

	if (((uintptr_t) dst & (word_size - 1)) !=
	    ((uintptr_t) src & (word_size - 1)))
		return unaligned_memcpy(dst, src, n);

	/*
	 * mod is the address modulo word size. fill is the length of the
	 * initial buffer segment before the first word boundary.
	 * If the buffer is very short, use unaligned_memcpy(), too.
	 */

	mod = (uintptr_t) dst & (word_size - 1);
	fill = word_size - mod;
	if (fill > n)
		fill = n;

	/* Copy the initial segment. */

	srcb = src;
	dstb = dst;

	i = fill;
	while (i-- != 0)
		*dstb++ = *srcb++;

	/* Compute remaining length. */

	n -= fill;
	if (n == 0)
		return dst;

	/* Pointers to aligned segment. */

	dstw = (unsigned long *) dstb;
	srcw = (const unsigned long *) srcb;

	n_words = n / word_size;	/* Number of whole words to copy. */
	n -= n_words * word_size;	/* Remaining bytes at the end. */

	/* "Fast" copy. */
	i = n_words;
	while (i-- != 0)
		*dstw++ = *srcw++;

	/*
	 * Copy the rest.
	 */

	srcb = (const uint8_t *) srcw;
	dstb = (uint8_t *) dstw;

	i = n;
	while (i-- != 0)
		*dstb++ = *srcb++;

	return dst;
}

#endif

/** @}
 */

