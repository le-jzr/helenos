/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

#ifndef ADT_ARRAY_H_
#define ADT_ARRAY_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <panic.h>

struct adt_array {
    void *b;
    size_t len;
    size_t cap;
};

#define adt_array(type) union { struct adt_array data; type *typed_b; }

#define adt_array_len(a) ((a)->data.len)

#define adt_array_at(a, idx) (*({ \
    auto __a = (a); \
    auto __idx = (idx); \
    assert(__idx >= 0); \
    assert(__idx < adt_array_len(__a)); \
    &(__a->typed_b[__idx]); \
}))

#define adt_array_foreach(a, var) \
    for (auto __a = (a); __a != NULL; __a = NULL) \
        for (auto var = &__a->typed_b[0]; var < &__a->typed_b[adt_array_len(__a)]; var++)

static inline size_t _adt_array_extend(struct adt_array *data, size_t sizeof_elem)
{
    if (data->len >= data->cap) {
        size_t new_cap = (data->cap == 0) ? 8 : data->cap << 1;
        if (new_cap > SIZE_MAX / sizeof_elem)
            panic("array size overflow");

        void *newb = realloc(data->b, new_cap * sizeof_elem);
        if (newb == NULL)
            panic("out of memory");

        data->b = newb;
        data->cap = new_cap;
    }

    return data->len++;
}

#define adt_array_push(a, val) \
    (typeof(a) _a = (a), _a->typed_b[_adt_array_extend(&_a->data, sizeof(_a->typed_b[0]))] = (val), (void) 0)

#define adt_array_pop(a) \
    ((a)->b[(assert((a)->len > 0), --((a)->len))])

#define adt_array_free(a) \
    ((a)->data.len = 0, (a)->data.cap = 0, free((a)->data.b))

#endif
