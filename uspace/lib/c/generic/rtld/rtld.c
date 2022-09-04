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

/** @addtogroup rtld
 * @brief
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <rtld/module.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <stdlib.h>
#include <str.h>

rtld_t *runtime_env;

/** Initialize and process a dynamically linked executable.
 *
 * @param p_info Program info
 * @return EOK on success or non-zero error code
 */
errno_t rtld_prog_process(elf_finfo_t *p_info, rtld_t **rre)
{
	rtld_t *env;
	module_t *prog;

	DPRINTF("Load dynamically linked program.\n");

	/* Allocate new RTLD environment to pass to the loaded program */
	env = calloc(1, sizeof(rtld_t));
	if (env == NULL)
		return ENOMEM;

	prog = calloc(1, sizeof(module_t));
	if (prog == NULL) {
		free(env);
		return ENOMEM;
	}

	/*
	 * First we need to process dynamic sections of the executable
	 * program and insert it into the module graph.
	 */

	DPRINTF("Parse program .dynamic section at %p\n", p_info->dynamic);
	dynamic_parse(p_info->dynamic, 0, &prog->dyn);
	prog->bias = 0;
	prog->dyn.soname = "[program]";
	prog->rtld = env;
	prog->exec = true;
	prog->local = false;

	prog->tdata = p_info->tls.tdata;
	prog->tdata_size = p_info->tls.tdata_size;
	prog->tbss_size = p_info->tls.tbss_size;
	prog->tls_align = p_info->tls.tls_align;

	DPRINTF("prog tdata at %p size %zu, tbss size %zu\n",
	    prog->tdata, prog->tdata_size, prog->tbss_size);

	/* Initialize list of loaded modules */
	list_initialize(&env->modules);
	list_initialize(&env->imodules);
	list_append(&prog->modules_link, &env->modules);

	/*
	 * Now we can continue with loading all other modules.
	 */

	DPRINTF("Load all program dependencies\n");
	errno_t rc = module_load_deps(prog);
	if (rc != EOK) {
		return rc;
	}

	/* Compute static TLS size */
	modules_process_tls(env);

	/*
	 * Now relocate/link all modules together.
	 */

	/* Process relocations in all modules */
	DPRINTF("Relocate all modules\n");
	modules_process_relocs(env, prog);

	*rre = env;
	return EOK;
}

/** Create TLS (Thread Local Storage) data structures.
 *
 * @return Pointer to TCB.
 */
tcb_t *rtld_tls_make(rtld_t *rtld)
{
	void *data = memalign(rtld->tls_align, rtld->tls_size);
	memcpy(data, rtld->tls_template, rtld->tls_size);
	tcb_t *tcb = data + rtld->tls_tp_offset;
#ifdef CONFIG_TLS_VARIANT_2
	tcb->self = tcb;
#endif
	return tcb;
}

/** Get address of thread-local variable.
 *
 * @param rtld RTLD instance
 * @param tcb TCB of the thread whose instance to return
 * @param mod_id Module ID
 * @param offset Offset within TLS block of the module
 *
 * @return Address of thread-local variable
 */
void *rtld_tls_get_addr(rtld_t *rtld, tcb_t *tcb, unsigned long mod_id,
    unsigned long offset)
{
	assert(mod_id != 0 && (mod_id & 1) == 0);
	// TODO

	const unsigned long sign_bit = (unsigned long)LONG_MAX + 1;

	// static tpoffset encoded in mod_id. bypass dtv
	unsigned long modoff = mod_id >> 1;
	// correct sign bit.
	modoff |= mod_id & sign_bit;

	return (void *)tcb + (long)modoff + offset;
}

/** @}
 */
