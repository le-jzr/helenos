/*
 * Copyright (c) 2005 Ondrej Palkovsky
 * Copyright (c) 2018 Jiří Zárevúcky
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

#include <symtab.h>
#include <byteorder.h>
#include <str.h>
#include <print.h>
#include <typedefs.h>
#include <errno.h>
#include <console/prompt.h>

#include <abi/elf.h>

/** @addtogroup kernel_generic_debug
 * @{
 */

/**
 * @file
 * @brief Kernel symbol resolver.
 */

static const elf_symbol_t *_symtab = NULL;
static size_t _symtab_len = 0;
static const char *_strtab = NULL;

void symtab_init(const elf_section_header_t *shtab, size_t shtab_len)
{
	for (size_t i = 0; i < shtab_len; i++) {
		if (shtab[i].sh_type != SHT_SYMTAB)
			continue;

		uintptr_t symtabaddr = shtab[i].sh_addr;
		uintptr_t strtabidx = shtab[i].sh_link;

		if (strtabidx >= shtab_len) {
			printf("Strtab section index out of bounds.\n");
			return;
		}

		uintptr_t strtabaddr = shtab[strtabidx].sh_addr;

		/* Check whether symtab and strtab are loaded. */
		if (symtabaddr == 0 || strtabaddr == 0) {
			printf("Symtab section present but not loaded.\n");
			return;
		}

		/* Check whether symtab entries have expected size. */
		if (shtab[i].sh_entsize != sizeof(elf_symbol_t)) {
			printf("Symtab has wrong entry size.\n");
			return;
		}

		/* sh_addr has the physical location of the loaded section. */

		_symtab = (const elf_symbol_t *) PA2KA(symtabaddr);
		_symtab_len = shtab[i].sh_size / shtab[i].sh_entsize;
		_strtab = (const char *) PA2KA(strtabaddr);
		printf("symtab: %p (%zu bytes)\n", _symtab, _symtab_len);
		printf("strtab: %p\n", _strtab);
	}
}

/** Get name of a symbol that seems most likely to correspond to address.
 *
 * @param addr   Address.
 * @param name   Place to store pointer to the symbol name.
 * @param offset Place to store offset from the symbol address.
 *
 * @return Zero on success or an error code, ENOENT if not found,
 *         ENOTSUP if symbol table not available.
 *
 */
errno_t symtab_name_lookup(uintptr_t addr, const char **name, uintptr_t *offset)
{
	*name = NULL;
	*offset = 0;

	if (!_symtab)
		return ENOTSUP;

	/*
	 * This is just a rarely used debugging feature, so we don't need to
	 * do anything fast and smart. Just iterate through the whole symbol
	 * table. It's not so large that it would pose an issue.
	 */

	const elf_symbol_t *best = NULL;

	// TODO: We can discriminate between data and function symbols.

	for (size_t i = 0; i < _symtab_len; i++) {
		switch (ELF_ST_TYPE(_symtab[i].st_info)) {
		case STT_NOTYPE:
		case STT_OBJECT:
		case STT_FUNC:
			break;
		default:
			/* Anything else is not relevant. */
			continue;
		}

		uintptr_t val = _symtab[i].st_value;
		size_t sz = _symtab[i].st_size;

		if (val > addr)
			continue;

		//printf("symbol %p, %zu, %zu\n", (void *)val, sz, _symtab[i].st_name);

		if (addr < val + sz) {
			/* We are within defined bounds, this is the one. */
			best = &_symtab[i];
			break;
		}

		if (sz == 0 && (!best || best->st_value < val)) {
			/* Object has unknown size. Keep the closest. */
			best = &_symtab[i];
		}
	}
	if (!best)
		return ENOENT;

	printf ("\n SYMBOL FINISHED, %u\n", best->st_name);
	*name = _strtab + best->st_name;
	*offset = addr - best->st_value;

	printf ("\n SYMBOL FINISHED, %s, %zu\n\n", *name, *offset);
	return EOK;
}

/** Lookup symbol by address and format for display.
 *
 * Returns name of closest corresponding symbol,
 * "unknown" if none exists and "N/A" if no symbol
 * information is available.
 *
 * @param addr Address.
 * @param name Place to store pointer to the symbol name.
 *
 * @return Pointer to a human-readable string.
 *
 */
const char *symtab_fmt_name_lookup(uintptr_t addr)
{
	const char *name;
	errno_t rc = symtab_name_lookup(addr, &name, NULL);

	switch (rc) {
	case EOK:
		return name;
	case ENOENT:
		return "unknown";
	default:
		return "N/A";
	}
}

#ifdef CONFIG_SYMTAB

/** Find symbols that match the parameter forward and print them.
 *
 * @param name     Search string
 * @param startpos Starting position, changes to found position
 *
 * @return Pointer to the part of string that should be completed or NULL.
 *
 */
static const char *symtab_search_one(const char *name, size_t *startpos)
{
	size_t namelen = str_length(name);

	size_t pos;
	for (pos = *startpos; symbol_table[pos].address_le; pos++) {
		const char *curname = symbol_table[pos].symbol_name;

		/* Find a ':' in curname */
		const char *colon = str_chr(curname, ':');
		if (colon == NULL)
			continue;

		if (str_length(curname) < namelen)
			continue;

		if (str_lcmp(name, curname, namelen) == 0) {
			*startpos = pos;
			return (curname + str_lsize(curname, namelen));
		}
	}

	return NULL;
}

#endif

/** Return address that corresponds to the entry.
 *
 * Search symbol table, and if there is one match, return it
 *
 * @param name Name of the symbol
 * @param addr Place to store symbol address
 *
 * @return Zero on success, ENOENT - not found, EOVERFLOW - duplicate
 *         symbol, ENOTSUP - no symbol information available.
 *
 */
errno_t symtab_addr_lookup(const char *name, uintptr_t *addr)
{
#ifdef CONFIG_SYMTAB
	size_t found = 0;
	size_t pos = 0;
	const char *hint;

	while ((hint = symtab_search_one(name, &pos))) {
		if (str_length(hint) == 0) {
			*addr = uint64_t_le2host(symbol_table[pos].address_le);
			found++;
		}
		pos++;
	}

	if (found > 1)
		return EOVERFLOW;

	if (found < 1)
		return ENOENT;

	return EOK;

#else
	return ENOTSUP;
#endif
}

/** Find symbols that match parameter and print them */
void symtab_print_search(const char *name)
{
#ifdef CONFIG_SYMTAB
	size_t pos = 0;
	while (symtab_search_one(name, &pos)) {
		uintptr_t addr = uint64_t_le2host(symbol_table[pos].address_le);
		char *realname = symbol_table[pos].symbol_name;
		printf("%p: %s\n", (void *) addr, realname);
		pos++;
	}

#else
	printf("No symbol information available.\n");
#endif
}

/** Symtab completion enum, see kernel/generic/include/kconsole.h */
const char *symtab_hints_enum(const char *input, const char **help,
    void **ctx)
{
#ifdef CONFIG_SYMTAB
	size_t len = str_length(input);
	struct symtab_entry **entry = (struct symtab_entry **)ctx;

	if (*entry == NULL)
		*entry = symbol_table;

	for (; (*entry)->address_le; (*entry)++) {
		const char *curname = (*entry)->symbol_name;

		/* Find a ':' in curname */
		const char *colon = str_chr(curname, ':');
		if (colon == NULL)
			continue;

		if (str_length(curname) < len)
			continue;

		if (str_lcmp(input, curname, len) == 0) {
			(*entry)++;
			if (help)
				*help = NULL;
			return (curname + str_lsize(curname, len));
		}
	}

	return NULL;

#else
	return NULL;
#endif
}

/** @}
 */
