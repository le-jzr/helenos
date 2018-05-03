/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

#include <str_ext.h>
#include <mm/slab.h>

/** @addtogroup generic
 * @{
 */

/** Duplicate string with size limit.
 *
 * Allocate a new string and copy up to @max_size bytes from the source
 * string into it. The duplicate string is allocated via sleeping
 * malloc(), thus this function can sleep in no memory conditions.
 * No more than @max_size + 1 bytes is allocated, but if the size
 * occupied by the source string is smaller than @max_size + 1,
 * less is allocated.
 *
 * The allocation cannot fail and the return value is always
 * a valid pointer. The duplicate string is always a well-formed
 * null-terminated UTF-8 string, but it can differ from the source
 * string on the byte level.
 *
 * @param src Source string.
 * @param n   Maximum number of bytes to duplicate.
 *
 * @return Duplicate string.
 *
 */
char *str_ndup_blocking(const char *src, size_t n)
{
	size_t size = str_size(src);
	if (size > n)
		size = n;

	char *dest = nfmalloc(size + 1);
	assert(dest);

	str_ncpy(dest, size + 1, src, size);
	return dest;
}

/** @}
 */
