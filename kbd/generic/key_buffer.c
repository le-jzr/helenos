/*
 * Copyright (C) 2006 Josef Cejka
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

#include <key_buffer.h>
#include <fifo.h>

#define KBD_BUFFER_SIZE 128

FIFO_INITIALIZE_STATIC(buffer, char, KBD_BUFFER_SIZE);
fifo_count_t buffer_items;

void key_buffer_free(void)
{
	buffer_items = 0;
	buffer.head = buffer.tail = 0;
}

void key_buffer_init(void)
{
	key_buffer_free();
}

/** 
 * @return empty buffer space
 */
int key_buffer_available(void)
{
	return KBD_BUFFER_SIZE - buffer_items;
}

void key_buffer_push(char key)
{
	/* TODO: somebody may wait for key */
	if (buffer_items < KBD_BUFFER_SIZE) {
		fifo_push(buffer, key);
		buffer_items++;
	}
}

/** Pop character from buffer.
 * @param c
 * @return zero on empty buffer, nonzero else
 */
int key_buffer_pop(char *c)
{
	if (buffer_items > 0) {
		buffer_items--;
		*c = fifo_pop(buffer);
		return 1;
	}
	return 0;
}


