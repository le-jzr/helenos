/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup kernel_generic_debug
 * @{
 */

/**
 * @file
 * @brief Kernel instrumentation functions.
 */

#include <debug/profile.h>

#include <stdalign.h>
#include <debug.h>
#include <symtab.h>
#include <errno.h>
#include <stdio.h>
#include <mm/slab.h>
#include <proc/thread.h>
#include <mem.h>
#include <stacktrace.h>

static slab_cache_t *data_cache = NULL;

#define PROFILE_TRACE_DEPTH 16

void debug_profile_init(void)
{
	data_cache = slab_cache_create("thread_profile_data_t",
	    sizeof(thread_profile_data_t), alignof(thread_profile_data_t),
	    NULL, NULL, 0);
}

void debug_profile_start(void)
{
	void *data = slab_alloc(data_cache, 0);
	if (data)
		memset(data, 0, sizeof(thread_profile_data_t));

	THREAD->profdata = data;
}

static void free_profile(thread_profile_data_t *p)
{
	while (p != NULL) {
		for (int i = 0; i < THREAD_PROFILE_DATA_LEN; i++)
			free_profile(p->child[i]);

		thread_profile_data_t *next = p->next;
		slab_free(data_cache, p);
		p = next;
	}
}

static void print_profile(thread_profile_data_t *p, uintptr_t total, char *prefix, size_t prefix_len, size_t prefix_cap)
{
	if (total == 0)
		total = 1;

	uintptr_t percent = (p->count * 100) / total;

	printf("%s%zu (%zu %%) 0x%zx\n", prefix, p->count, percent, p->address);

	prefix[prefix_len] = ' ';
	prefix[prefix_len + 1] = ' ';
	prefix[prefix_len + 2] = 0;

	while (p != NULL) {
		for (int i = 0; i < THREAD_PROFILE_DATA_LEN; i++) {
			if (p->child[i] == NULL)
				return;

			print_profile(p->child[i], p->count, prefix, prefix_len + 2, prefix_cap);
		}

		p = p->next;
	}

	prefix[prefix_len] = 0;
}

#define MAX_PREFIX_LEN (2 * PROFILE_TRACE_DEPTH + 1)

void debug_profile_stop(void)
{
	thread_profile_data_t *data = THREAD->profdata;
	THREAD->profdata = NULL;

	char prefix_buffer[MAX_PREFIX_LEN] = {};

	if (data)
		print_profile(data, data->count, prefix_buffer, 0, MAX_PREFIX_LEN);

	free_profile(data);
}

static thread_profile_data_t *descend(thread_profile_data_t *p, uintptr_t pc)
{
	while (true) {
		for (int i = 0; i < THREAD_PROFILE_DATA_LEN; i++) {
			if (p->child[i] == NULL) {
				// New child node.

				p->child[i] = slab_alloc(data_cache, FRAME_ATOMIC);
				if (p->child[i] == NULL) {
					LOG("can't allocate more memory for profile\n");
					return NULL;
				}
				memset(p->child[i], 0, sizeof(thread_profile_data_t));

				p->child[i]->address = pc;
				p->child[i]->count = 1;
				return p->child[i];
			}

			if (p->child[i]->address == pc) {
				// Existing child node.

				p->child[i]->count++;
				return p->child[i];
			}
		}

		// Move to the sibling node.

		if (p->next == NULL) {
			// New sibling node.

			p->next = slab_alloc(data_cache, FRAME_ATOMIC);
			if (p->next == NULL) {
				LOG("can't allocate more memory for profile\n");
				return NULL;
			}
			memset(p->next, 0, sizeof(thread_profile_data_t));
		}

		p = p->next;
	}
}

void debug_profile_gather(void)
{
	if (THREAD == NULL || THREAD->profdata == NULL)
		return;

	//printf("gather data\n");

	uintptr_t trace[PROFILE_TRACE_DEPTH];
	size_t trace_len = PROFILE_TRACE_DEPTH;

	stack_trace_gather_pc(trace, &trace_len);

	if (trace_len == PROFILE_TRACE_DEPTH) {
		//LOG("profile data point lost: stack too deep!\n");
		return;
	}

	thread_profile_data_t *data = THREAD->profdata;
	data->count++;

	for (; data && trace_len > 0; trace_len--) {
		data = descend(data, trace[trace_len - 1]);
	}
}

#ifdef CONFIG_TRACE

void __cyg_profile_func_enter(void *fn, void *call_site)
{
	const char *fn_sym = symtab_fmt_name_lookup((uintptr_t) fn);

	const char *call_site_sym;
	uintptr_t call_site_off;

	if (symtab_name_lookup((uintptr_t) call_site, &call_site_sym,
	    &call_site_off) == EOK)
		printf("%s()+%p->%s()\n", call_site_sym,
		    (void *) call_site_off, fn_sym);
	else
		printf("->%s()\n", fn_sym);
}

void __cyg_profile_func_exit(void *fn, void *call_site)
{
	const char *fn_sym = symtab_fmt_name_lookup((uintptr_t) fn);

	const char *call_site_sym;
	uintptr_t call_site_off;

	if (symtab_name_lookup((uintptr_t) call_site, &call_site_sym,
	    &call_site_off) == EOK)
		printf("%s()+%p<-%s()\n", call_site_sym,
		    (void *) call_site_off, fn_sym);
	else
		printf("<-%s()\n", fn_sym);
}

#endif /* CONFIG_TRACE */

/** @}
 */
