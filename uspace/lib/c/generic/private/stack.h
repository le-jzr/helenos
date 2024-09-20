/*
 * Copyright (c) 2024 Jiří Zárevúcky
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

#include <errno.h>
#include <stdio.h>

static inline errno_t _stack_pre_push(void **array, size_t *array_len,
    size_t *stack_len, const void *val, size_t elem_size)
{
	if (*stack_len >= *array_len) {
		size_t new_array_len = (*array_len > 0) ? (*array_len << 1) : 4;
		if (*stack_len >= new_array_len)
			return ENOMEM;

		void *new_array = reallocarray(*array, new_array_len, elem_size);
		if (!new_array)
			return ENOMEM;

		*array = new_array;
		*array_len = new_array_len;
	}

	return EOK;
}

#define DEFINE_STACK_TYPE(name, elem_type) \
	typedef struct { \
		elem_type *array; \
		size_t array_len; \
		size_t stack_len; \
	} name##_t; \
	\
	static __attribute__((warn_unused_result)) errno_t name##_push(name##_t *stack, elem_type val) { \
		errno_t rc = _stack_pre_push((void **) &stack->array, \
			&stack->array_len, &stack->stack_len, &val, sizeof(elem_type)); \
		if (rc != EOK) return rc; \
		stack->array[stack->stack_len] = val; \
		stack->stack_len++; \
		return EOK; \
	} \
	\
	static elem_type name##_pop(name##_t *stack) { \
		assert(stack->stack_len > 0); \
		stack->stack_len--; \
		return stack->array[stack->stack_len]; \
	} \
	\
	static inline bool name##_empty(name##_t *stack) { \
		return stack->stack_len == 0; \
	} \
	\
	static void name##_destroy(name##_t *stack, void (*destroy_fn)(elem_type)) { \
		while (destroy_fn && !name##_empty(stack)) \
			destroy_fn(name##_pop(stack)); \
		\
		if (stack->array) free(stack->array); \
		*stack = (name##_t) { }; \
	}
