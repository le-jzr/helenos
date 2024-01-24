/*
 * Copyright (c) 2023 Jiří Zárevúcky
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

#ifndef KERN_KOBJ_H_
#define KERN_KOBJ_H_

#include <lib/refcount.h>
#include <adt/hash_table.h>
#include <lib/ra.h>
#include <synch/mutex.h>

typedef intptr_t kobj_handle_t;

typedef struct {
	void (*destroy)(void *);
} kobj_class_t;

typedef struct {
	atomic_refcount_t refcount;
	const kobj_class_t *type;
} kobj_t;

#define KOBJ_INITIALIZER(tp) { \
	.refcount = REFCOUNT_INITIALIZER(), \
	.type = (tp), \
}

static inline void kobj_initialize(kobj_t *kobj, const kobj_class_t *type)
{
	*kobj = (kobj_t) KOBJ_INITIALIZER(type);
}

kobj_t *kobj_ref(kobj_t *kobj);
kobj_t *kobj_try_ref(kobj_t *kobj);
void kobj_put(kobj_t *kobj);

/**
 * "Proxy" object for kobj_t references.
 * When a proxy is created, one actually creates a pair of objects, an inner
 * and outer proxy object. Reference to the inner proxy object can transparently
 * be used like the original reference, the difference being in the ability to
 * invalidate the inner object, making any further operations return errors.
 */
typedef struct kobj_proxy kobj_proxy_t;

kobj_proxy_t *kobj_proxy_create(kobj_t *wrapped);
kobj_t *kobj_proxy_get_inner(kobj_proxy_t *proxy);
void kobj_proxy_invalidate(kobj_proxy_t *proxy);

typedef struct {
	mutex_t lock;
	hash_table_t refs;
	ra_arena_t *handles;
} kobj_table_t;

errno_t kobj_table_initialize(kobj_table_t *table);
void kobj_table_destroy(kobj_table_t *table);
void *kobj_table_lookup(kobj_table_t *table, kobj_handle_t handle, const kobj_class_t *type);
void *kobj_table_shallow_lookup(kobj_table_t *table, kobj_handle_t handle);
kobj_handle_t kobj_table_insert(kobj_table_t *table, void *kobj);
kobj_t *kobj_table_remove(kobj_table_t *table, kobj_handle_t handle);

sys_errno_t sys_kobj_put(sysarg_t handle);

#endif
