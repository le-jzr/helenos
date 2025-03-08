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

#include <fibril.h>
#include <fibril_synch.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <threads.h>
#include "../private/fibril.h"

#undef thrd_equal

enum {
    _once_initial = 0,
    _once_finished = 1,
};

static FIBRIL_MUTEX_INITIALIZE(_once_mutex);

void call_once(once_flag *flag, void (*func)(void))
{
    int state = atomic_load_explicit(&flag->__flag, memory_order_acquire);

    /* Fast exit. */
    if (state == _once_finished)
        return;

    /*
     * All concurrent initialization attempts are serialized.
     * This should be a rare occurrence, so we don't mind serializing
     * unrelated calls too much.
     */
    fibril_mutex_lock(&_once_mutex);

    /* Repeat the test once protected by the mutex. Doesn't need a barrier. */
    state = atomic_load_explicit(&flag->__flag, memory_order_relaxed);
    if (state != _once_finished) {
        func();
        atomic_store_explicit(&flag->__flag, _once_finished, memory_order_release);
    }

    fibril_mutex_unlock(&_once_mutex);
}

/* Used just for comparisons, marks _tss_key entries that are unallocated. */
static void _tss_dtor_unallocated(void *arg)
{
}

struct _tss_key {
    tss_dtor_t dtor;
    size_t gen;
};

struct _tss {
    void *val;
    size_t gen;
};

static thread_local size_t _tss_gen;
static thread_local size_t _tss_len;
static thread_local struct _tss *_tss;
static thread_local bool _tss_in_destructor;

static atomic_size_t _tss_key_gen;

static FIBRIL_MUTEX_INITIALIZE(_tss_key_mutex);
static size_t _tss_key_len;
static size_t _tss_key_next;
static struct _tss_key *_tss_key;

static inline bool _tss_changed(void)
{
    /*
     * We don't need any synchronization here since a correctly
     * working program will always synchronize between
     * tss_create()/tss_destroy() and tss_get()/tss_set() to the same key.
     */

    return
        _tss_gen != atomic_load_explicit(&_tss_key_gen, memory_order_relaxed);
}

static void _tss_update(void)
{
    /*
     * The set of keys changed.
     * We need to make sure there aren't any stale recycled keys.
     */

    if (!_tss_in_destructor)
        fibril_mutex_lock(&_tss_key_mutex);

    assert(_tss_len <= _tss_key_len);

    /* Clear out stale keys. */
    for (size_t i = 0; i < _tss_len; i++) {
        if (_tss[i].gen != _tss_key[i].gen) {
            _tss[i].gen = _tss_key[i].gen;
            _tss[i].val = NULL;
        }
    }

    /* Resize the array if needed. */
    if (_tss_len < _tss_key_len) {
        struct _tss *new_tss =
            reallocarray(_tss, _tss_key_len, sizeof(struct _tss));

        if (new_tss) {
            _tss = new_tss;

            for (size_t i = _tss_len; i < _tss_key_len; i++)
                _tss[i] = (struct _tss) { .gen = _tss_key[i].gen };

            _tss_len = _tss_key_len;
        }
    }

    /* Don't update the generation counter if we still need to resize later. */
    if (_tss_len == _tss_key_len)
        _tss_gen = atomic_load_explicit(&_tss_key_gen, memory_order_relaxed);

    if (!_tss_in_destructor)
        fibril_mutex_unlock(&_tss_key_mutex);
}

void __tss_on_thread_exit()
{
    struct _tss *tss = _tss;

    if (!tss)
        return;

    fibril_mutex_lock(&_tss_key_mutex);
    _tss_in_destructor = true;

    if (_tss_changed())
        _tss_update();

    for (size_t repeats = 0; repeats < TSS_DTOR_ITERATIONS; repeats++) {
        for (size_t i = 0; i < _tss_len; i++) {
            if (_tss[i].val && _tss_key[i].dtor) {
                void *val = _tss[i].val;
                _tss[i].val = NULL;
                _tss_key[i].dtor(val);
            }
        }

        bool clean = true;

        for (size_t i = 0; i < _tss_len; i++) {
            if (_tss[i].val) {
                clean = false;
                break;
            }
        }

        if (clean)
            break;
    }

    fibril_mutex_unlock(&_tss_key_mutex);
}

static inline void _tss_key_gen_inc(void)
{
    assert(fibril_mutex_is_locked(&_tss_key_mutex));
    /* Just some handwaving to make incrementing this variable kosher. */
    int gen = atomic_load_explicit(&_tss_key_gen, memory_order_relaxed);
    atomic_store_explicit(&_tss_key_gen, gen + 1, memory_order_relaxed);
}

static int _tss_create_specific(tss_t *key, tss_dtor_t dtor, size_t i)
{
    assert(fibril_mutex_is_locked(&_tss_key_mutex));
    assert(_tss_key[i].dtor == _tss_dtor_unallocated);

    _tss_key[i].dtor = dtor;
    _tss_key[i].gen++;
    key->__handle = i;
    _tss_key_gen_inc();

    for (; i < _tss_len; i++) {
        if (_tss_key[i].dtor == _tss_dtor_unallocated)
            break;
    }

    _tss_key_next = i;
    return thrd_success;
}

static int _tss_create_locked(tss_t *key, tss_dtor_t dtor)
{
    assert(fibril_mutex_is_locked(&_tss_key_mutex));

    if (_tss_key_next < _tss_key_len)
        return _tss_create_specific(key, dtor, _tss_key_next);

    for (size_t i = 0; i < _tss_key_len; i++) {
        if (_tss_key[i].dtor == _tss_dtor_unallocated)
            return _tss_create_specific(key, dtor, i);
    }

    /* Current array is full, expand it. */
    size_t new_len = (_tss_key_len + 1) * 2;
    struct _tss_key *new_keys =
        reallocarray(_tss_key, new_len, sizeof(struct _tss_key));

    if (!new_keys)
        return thrd_error;

    _tss_key = new_keys;
    _tss_key_len = new_len;

    for (size_t i = _tss_key_next; i < _tss_key_len; i++)
        _tss_key[i] = (struct _tss_key) { .dtor = _tss_dtor_unallocated };

    return _tss_create_specific(key, dtor, _tss_key_next);
}

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    assert(!_tss_in_destructor);

    fibril_mutex_lock(&_tss_key_mutex);
    int rc = _tss_create_locked(key, dtor);
    fibril_mutex_unlock(&_tss_key_mutex);
    return rc;
}

void tss_delete(tss_t key)
{
    assert(!_tss_in_destructor);

    fibril_mutex_lock(&_tss_key_mutex);

    _tss_key[key.__handle].dtor = _tss_dtor_unallocated;
    _tss_key[key.__handle].gen++;
    _tss_key_gen_inc();

    if (_tss_key_next >= _tss_key_len)
        _tss_key_next = key.__handle;

    fibril_mutex_unlock(&_tss_key_mutex);
}

void *tss_get(tss_t key)
{
    if (_tss_len <= key.__handle)
        return NULL;

    if (_tss_changed())
        _tss_update();

    return _tss[key.__handle].val;
}

int tss_set(tss_t key, void *val)
{
    if (_tss_changed())
        _tss_update();

    if (_tss_len <= key.__handle && val != NULL) {
        /* Presumably failed reallocating in _tss_update(). */
        return thrd_error;
    }

    _tss[key.__handle].val = val;
    return thrd_success;
}
