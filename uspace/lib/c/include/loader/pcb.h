/*
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

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief Program Control Block interface.
 */

#ifndef _LIBC_PCB_H_
#define _LIBC_PCB_H_

#include <tls.h>

typedef void (*entry_point_t)(void);

struct pcb;
typedef void (*reloc_entry_point_t)(struct pcb *);

struct pcb_inbox_entry {
	char *name;
	int file;
};

/** Program Control Block.
 *
 * Holds pointers to data passed from the program loader to the program
 * and/or to the dynamic linker. This includes the program entry point,
 * arguments, environment variables etc.
 *
 */
typedef struct pcb {
	/** Program entry point. */
	entry_point_t entry;

	/** Current working directory. */
	char *cwd;

	/** Number of command-line arguments. */
	int argc;
	/** Command-line arguments. */
	char **argv;

	/** List of inbox files. */
	struct pcb_inbox_entry *inbox;
	int inbox_entries;

	/*
	 * ELF-specific data.
	 */

	/** Pointer to ELF dynamic section of the program. */
	void *dynamic;
	/** Pointer to dynamic linker state structure (rtld_t). */
	void *rtld_runtime;

	/** Thread local storage for the main thread. */
	tcb_t *tcb;

	/** Entry point of the in-process dynamic relocator. */
	reloc_entry_point_t reloc_entry;

	void *tls_template;
	void *initialization_order;
	void *resolution_order;
	size_t module_count;

	/**
	 * The bottom of position-independed modules loaded by the parent task.
	 * Most data referenced in this structure (and the structure itself) is
	 * stored between initial_stack_limit and vaddr_limit.
	 * If it's no longer needed, the whole chunk of memory can be freed/recycled.
	 */
	uintptr_t vaddr_limit;
	/** The upper limit of initial stack. */
	uintptr_t initial_stack_limit;
	/** The lowest address of initial stack. */
	uintptr_t initial_stack_base;
} pcb_t;

/**
 * A pointer to the program control block. Having received the PCB pointer,
 * the C library startup code stores it here for later use.
 */
extern pcb_t *__pcb;

#endif

/**
 * @}
 */
