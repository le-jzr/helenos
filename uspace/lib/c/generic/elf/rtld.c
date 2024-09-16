


/**
 * In-process dynamic linker implementation.
 *
 * The parent task does most of the heavy lifting for us before starting us,
 * so all we have left to do is processing runtime relocations.
 *
 * Functions in this file should only call protected functions until all program
 * relocations are fully processed. This includes functions called recursively.
 *
 *
 * That specifically means no dynamic allocations or debug output are allowed
 * until the linking is all done. Until then, the dynamic linking code keeps
 * silent about most issues it encounters, and only reports them after that,
 * with the assumption any issues with dynamic linking information did not
 * affect the functions of libc so bad that output becomes impossible.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <types/common.h>
#include <abi/elf.h>

#include "../private/loader.h"
#include "../private/rtld_arch.h"
#include <loader/pcb.h>
#include <io/kio.h>
#include <stdio.h>

static char early_print_buffer[1024];

#define DPRINTF(...) ({ \
	size_t n = __snprintf(early_print_buffer, sizeof(early_print_buffer), __VA_ARGS__); \
	if (n > sizeof(early_print_buffer))  \
		n = sizeof(early_print_buffer);  \
	if (early_print_buffer[n-1] == '\n') \
		n--;                             \
	__kio_write(early_print_buffer, n);  \
})

#define PANIC(...) (DPRINTF(__VA_ARGS__), *(volatile int*)NULL = *(volatile int*)NULL)

#define DEBUG_HASH 1

static int errors = 0;
#define ERRORF(...) (++errors, (void)DPRINTF(__VA_ARGS__))
#define DTRACE(...) DPRINTF(__VA_ARGS__)
//#define DULTRATRACE(...) DTRACE(__VA_ARGS__)
#define DULTRATRACE(...) ((void) 0)

struct lildyn {
	const uint32_t *hash;
	const elf_symbol_t *symtab;
	const char *strtab;
};

static unsigned long elf_hash(const char *s)
{
	const unsigned char *p = (const unsigned char *)s;

	// Straight out of the spec.

    unsigned long h = 0, high;
    while (*p) {
        h = (h << 4) + *p++;
        if ((high = h & 0xF0000000))
            h ^= high >> 24;
        h &= ~high;
    }
    return h;
}

// TODO: deduplicate with code in elf2.c
static const elf_symbol_t *hash_lookup_symbol(const struct lildyn *dyn,
		const char *symbol_name, unsigned long symbol_hash)
{
	DULTRATRACE("Looking for symbol \"%s\"\n", symbol_name);

	const uint32_t *hash = dyn->hash;
	const elf_symbol_t *symtab = dyn->symtab;
	const char *strtab = dyn->strtab;

	size_t hash_len = hash[0] + hash[1] + 2;
	size_t symtab_len = hash[1];

	uint32_t nbuckets = hash[0];
	uint32_t bucket = symbol_hash % nbuckets;

	if (bucket + 2 >= hash_len)
		return NULL;

	uint32_t sym_idx = hash[bucket + 2];

	while (sym_idx != STN_UNDEF && sym_idx < symtab_len) {
		const elf_symbol_t *sym = &symtab[sym_idx];

		const char *sym_name = strtab + sym->st_name;
		DULTRATRACE("Found symbol \"%s\"\n", sym_name);

		if (DEBUG_HASH && elf_hash(sym_name) % nbuckets != bucket)
			DPRINTF("Symbol \"%s\" in unexpected bucket.\n", sym_name);

		if (strcmp(symbol_name, sym_name) == 0)
			return sym;

		uint32_t chain_idx = nbuckets + 2 + sym_idx;

		if (chain_idx >= hash_len)
			return NULL;

		sym_idx = hash[chain_idx];
	}

	return NULL;
}

static const elf_dyn_t *get_dynamic(const elf_rtld_info_t *info)
{
	if (!info)
		return NULL;

	const elf_header_t *header = (void *) info->header;
	const elf_segment_header_t *phdr = (void *) info->phdr;
	int phdr_len = header->e_phnum;

	for (int i = 0; i < phdr_len; i++) {
		if (phdr[i].p_type == PT_DYNAMIC)
			return (void *) info->bias + phdr[i].p_vaddr;
	}

	return NULL;
}

static struct lildyn get_lildyn(const elf_dyn_t *dyn, uintptr_t bias)
{
	struct lildyn lildyn = {0};

	if (!dyn)
		return lildyn;

	for (int i = 0; dyn[i].d_tag != DT_NULL; i++) {
		void *ptr = (void *) (bias + dyn[i].d_un.d_ptr);

		switch (dyn[i].d_tag) {
		case DT_HASH:
			DTRACE("DT_HASH = %p\n", ptr);
			lildyn.hash = ptr;
			break;
		case DT_STRTAB:
			DTRACE("DT_STRTAB = %p\n", ptr);
			lildyn.strtab = ptr;
			break;
		case DT_SYMTAB:
			DTRACE("DT_SYMTAB = %p\n", ptr);
			lildyn.symtab = ptr;
			break;
		}
	}

	return lildyn;
}

static int lookup_symbol(int elf_list_len, const struct lildyn lildyn[],
		const char *symbol, unsigned long symbol_hash, int first,
		const elf_symbol_t **out_sym)
{
	for (int i = first; i < elf_list_len; i++) {
		DTRACE("Looking up symbol %s in module %d.\n", symbol, i);

		const elf_symbol_t *s = hash_lookup_symbol(&lildyn[i], symbol, symbol_hash);

		if (s && s->st_shndx != SHN_UNDEF && ELF_ST_BIND(s->st_info) != STB_LOCAL) {
			*out_sym = s;
			return i;
		}
	}

	return -1;
}

static inline uintptr_t read_width(uintptr_t vaddr, int width)
{
	switch (width) {
	case 8:
		return *(uint8_t *)vaddr;
	case 16:
		return *(uint16_t *)vaddr;
	case 32:
		return *(uint32_t *)vaddr;
	case 64:
		return *(uint64_t *)vaddr;
	default:
		return 0;
	}
}

static inline void write_width(uintptr_t vaddr, int width, uintptr_t value)
{
	switch (width) {
	case 8:
		*(uint8_t *)vaddr = (uint8_t) value;
		break;
	case 16:
		*(uint16_t *)vaddr = (uint16_t) value;
		break;
	case 32:
		*(uint32_t *)vaddr = (uint32_t) value;
		break;
	case 64:
		*(uint64_t *)vaddr = value;
		break;
	default:
		break;
	}
}

static elf_rel_desc_t get_reloc_desc(uintptr_t r_info)
{
	uintptr_t reloc_type = ELF_R_TYPE(r_info);
	elf_rel_desc_t desc = (reloc_type < arch_rel_len) ? arch_rel_list[reloc_type] : (elf_rel_desc_t) { .width = 0, .type = 0 };

	if (desc.type == 0 && desc.width == 0)
		PANIC("unknown relocation type %zu\n", reloc_type);

	return desc;
}

static void relocate_one(const elf_rtld_info_t *elf_list[], int elf_list_len, int elf_id,
		const struct lildyn lildyn[], uintptr_t vaddr, uintptr_t r_info, uintptr_t addend)
{
	elf_rel_desc_t d = get_reloc_desc(r_info);
	if (d.type == 0)
		return;

	size_t sym_idx = ELF_R_SYM(r_info);

	int sym_elf_id = elf_id;
	const elf_symbol_t *sym = &lildyn[elf_id].symtab[sym_idx];
	const char *name = lildyn[elf_id].strtab + lildyn[elf_id].symtab[sym_idx].st_name;

	DULTRATRACE("Relocation type 0%o;%d for symbol \"%s\", addend = 0x%zx.\n", d.type, d.width, name, addend);

	// If the symbol has default visibility, it means it can be overridden in other
	// objects even if it's defined locally. All STV_DEFAULT entries for the symbol
	// in the program must be relocated to the same address, so we have to find it.

	if (sym_idx != STN_UNDEF && ELF_ST_VISIBILITY(sym->st_other) == STV_DEFAULT) {
		unsigned long hash = elf_hash(name);

		DULTRATRACE("Global resolution for symbol \"%s\"\n", name);

		// If this is a copy relocation, search shared libraries for the original definition.
		// REL_COPY can only be present in the main executable.
		int first = d.type == REL_COPY ? 1 : 0;

		sym_elf_id = lookup_symbol(elf_list_len, lildyn, name, hash, first, &sym);

		if (sym_elf_id < 0) {
			ERRORF("ELF %d, sym %zu: undefined symbol %s\n", elf_id, sym_idx, name);
			return;
		}
	}

	if (d.type == REL_COPY) {
		void *dst = (void *) vaddr;
		void *src = (void *) (elf_list[sym_elf_id]->bias + sym->st_value);
		size_t size = sym->st_size;

		// A copy relocation is a special kind of relocation created when a data object located
		// in a shared library is accessed by a non-pie executable. The executable cannot access
		// the data object in shared library without text relocations, so instead, the data object
		// is duplicated in the executable's data section and a copy relocation is emitted to make
		// dynamic linker copy the initialization data to the new location.
		// The resolution of accesses to the object in the original library then proceeds as if
		// the object was just overridden in the executable.
		//
		// All that is to say... don't put non-static global variables in a library. It's messy.
		//
		DTRACE("Copy relocation for \"%s\" from %p to %p (%zu bytes)\n", name, src, dst, size);
		memcpy(dst, src, size);
		return;
	}

	uintptr_t value = 0;

	if (d.type & REL_ADDEND)
		value += addend;

	if (d.type & REL_BASE)
		value += elf_list[sym_elf_id]->bias;

	if (d.type & REL_PLACE)
		value -= vaddr;

	if (d.type & REL_SYMVAL)
		value += elf_list[sym_elf_id]->bias + sym->st_value;

	if (d.type & REL_SYMSZ)
		value += sym->st_size;

	// DTPMOD is defined to start at 1, which must be the index of the main executable.
	if (d.type & REL_DTPMOD)
		value += sym_elf_id + 1;

	if (d.type & REL_DTPOFF)
		value += sym->st_value;

	write_width(vaddr, d.width, value);
}

typedef void (*elf_initfini_fn_t)(void);

struct bigdyn {
	const char *strtab;
	size_t strtab_len;
	size_t pltrelsz;
	const void *pltgot;
	const elf_rela_t *rela;
	size_t rela_len;
	const elf_rel_t *rel;
	size_t rel_len;
	const elf_rela_t *plt_rela;
	size_t plt_rela_len;
	const elf_rel_t *plt_rel;
	size_t plt_rel_len;
	elf_initfini_fn_t init;
	elf_initfini_fn_t fini;
	const char *soname;
	const char *rpath;
	const char *runpath;

	elf_initfini_fn_t *init_array;
	size_t init_array_len;

	elf_initfini_fn_t *fini_array;
	size_t fini_array_len;

	elf_initfini_fn_t *preinit_array;
	size_t preinit_array_len;

	bool origin;
	bool symbolic;
	bool textrel;
	bool bind_now;
	bool static_tls;
};

static struct bigdyn get_bigdyn(const elf_dyn_t *dyn, uintptr_t bias)
{
	struct bigdyn bigdyn = {0};

	if (!dyn)
		return bigdyn;

	size_t rpath_offset = 0;
	size_t soname_offset = 0;
	size_t runpath_offset = 0;

	size_t pltrelsz = 0;
	void *pltrel = NULL;
	uintptr_t pltrel_type = 0;

	for (int i = 0; dyn[i].d_tag != DT_NULL; i++) {
		uintptr_t val = dyn[i].d_un.d_val;
		void *ptr = (void *) (bias + dyn[i].d_un.d_ptr);

		switch (dyn[i].d_tag) {
		case DT_PLTRELSZ:
			DTRACE("DT_PLTRELSZ = 0x%zx\n", val);
			pltrelsz = val;
			break;
		case DT_PLTGOT:
			DTRACE("DT_PLTGOT = %p\n", ptr);
			bigdyn.pltgot = ptr;
			break;
		case DT_RELA:
			DTRACE("DT_RELA = %p\n", ptr);
			bigdyn.rela = ptr;
			break;
		case DT_RELASZ:
			DTRACE("DT_RELASZ = 0x%zx\n", val);
			bigdyn.rela_len = val / sizeof(elf_rela_t);
			break;
		case DT_STRTAB:
			DTRACE("DT_STRTAB = %p\n", ptr);
			bigdyn.strtab = ptr;
			break;
		case DT_STRSZ:
			DTRACE("DT_STRSZ = 0x%zx\n", val);
			bigdyn.strtab_len = val;
			break;
		case DT_INIT:
			DTRACE("DT_INIT = %p\n", ptr);
			bigdyn.init = ptr;
			break;
		case DT_FINI:
			DTRACE("DT_FINI = %p\n", ptr);
			bigdyn.fini = ptr;
			break;
		case DT_SONAME:
			soname_offset = val;
			break;
		case DT_RPATH:
			rpath_offset = val;
			break;
		case DT_SYMBOLIC:
			DTRACE("DT_SYMBOLIC\n");
			bigdyn.symbolic = true;
			break;
		case DT_REL:
			DTRACE("DT_REL = %p\n", ptr);
			bigdyn.rel = ptr;
			break;
		case DT_RELSZ:
			DTRACE("DT_RELSZ = 0x%zx\n", val);
			bigdyn.rel_len = val / sizeof(elf_rel_t);
			break;
		case DT_PLTREL:
			pltrel_type = val;
			break;
		case DT_TEXTREL:
			DTRACE("DT_TEXTREL\n");
			bigdyn.textrel = true;
			break;
		case DT_JMPREL:
			DTRACE("DT_JMPREL = %p\n", ptr);
			pltrel = ptr;
			break;
		case DT_BIND_NOW:
			DTRACE("DT_BIND_NOW\n");
			bigdyn.bind_now = true;
			break;
		case DT_INIT_ARRAY:
			DTRACE("DT_INIT_ARRAY = %p\n", ptr);
			bigdyn.init_array = ptr;
			break;
		case DT_INIT_ARRAYSZ:
			DTRACE("DT_INIT_ARRAYSZ = 0x%zx\n", val);
			bigdyn.init_array_len = val / sizeof(elf_initfini_fn_t);
			break;
		case DT_FINI_ARRAY:
			DTRACE("DT_FINI_ARRAY = %p\n", ptr);
			bigdyn.fini_array = ptr;
			break;
		case DT_FINI_ARRAYSZ:
			DTRACE("DT_FINI_ARRAYSZ = 0x%zx\n", val);
			bigdyn.fini_array_len = val / sizeof(elf_initfini_fn_t);
			break;
		case DT_PREINIT_ARRAY:
			DTRACE("DT_PREINIT_ARRAY = %p\n", ptr);
			bigdyn.preinit_array = ptr;
			break;
		case DT_PREINIT_ARRAYSZ:
			DTRACE("DT_PREINIT_ARRAYSZ = 0x%zx\n", val);
			bigdyn.preinit_array_len = val / sizeof(elf_initfini_fn_t);
			break;
		case DT_RUNPATH:
			runpath_offset = val;
			break;
		case DT_FLAGS:
			if (val & DF_ORIGIN) {
				DTRACE("DF_ORIGIN\n");
				bigdyn.origin = true;
			}
			if (val & DF_SYMBOLIC) {
				DTRACE("DF_SYMBOLIC\n");
				bigdyn.symbolic = true;
			}
			if (val & DF_TEXTREL) {
				DTRACE("DF_TEXTREL\n");
				bigdyn.textrel = true;
			}
			if (val & DF_BIND_NOW) {
				DTRACE("DF_BIND_NOW\n");
				bigdyn.bind_now = true;
			}
			if (val & DF_STATIC_TLS) {
				DTRACE("DF_STATIC_TLS\n");
				bigdyn.static_tls = true;
			}
			break;
		}
	}

	if (pltrel_type == DT_REL) {
		DTRACE("DT_PLTREL = DT_REL\n");
		bigdyn.plt_rel = pltrel;
		bigdyn.plt_rel_len = pltrelsz / sizeof(elf_rel_t);
	}

	if (pltrel_type == DT_RELA) {
		DTRACE("DT_PLTREL = DT_RELA\n");
		bigdyn.plt_rela = pltrel;
		bigdyn.plt_rela_len = pltrelsz / sizeof(elf_rela_t);
	}

	bigdyn.soname = bigdyn.strtab + soname_offset;
	DTRACE("DT_SONAME = \"%s\"\n", bigdyn.soname);
	bigdyn.rpath = bigdyn.strtab + rpath_offset;
	DTRACE("DT_RPATH = \"%s\"\n", bigdyn.rpath);
	bigdyn.runpath = bigdyn.strtab + runpath_offset;
	DTRACE("DT_RUNPATH = \"%s\"\n", bigdyn.runpath);
	return bigdyn;
}

static uintptr_t read_reloc_place(uintptr_t bias, uintptr_t r_info, uintptr_t r_offset)
{
	elf_rel_desc_t desc = get_reloc_desc(r_info);
	return (desc.width == 0) ? 0 : read_width(bias + r_offset, desc.width);
}

static void process_rel(
		const elf_rtld_info_t *elf_list[], int elf_list_len,
		int elf_id, const struct lildyn lildyn[], const elf_rel_t *rel, size_t rel_len)
{
	uintptr_t bias = elf_list[elf_id]->bias;

	for (uintptr_t i = 0; i < rel_len; i++) {
		uintptr_t vaddr = bias + rel[i].r_offset;
		uintptr_t addend = read_reloc_place(bias, rel[i].r_info, rel[i].r_offset);
		relocate_one(elf_list, elf_list_len, elf_id, lildyn,
				vaddr, rel[i].r_info, addend);
	}
}

static void process_rela(
		const elf_rtld_info_t *elf_list[], int elf_list_len,
		int elf_id, const struct lildyn lildyn[], const elf_rela_t *rela, size_t rela_len)
{
	uintptr_t bias = elf_list[elf_id]->bias;

	for (uintptr_t i = 0; i < rela_len; i++) {
		relocate_one(elf_list, elf_list_len, elf_id, lildyn,
				bias + rela[i].r_offset, rela[i].r_info, rela[i].r_addend);
	}
}

static void relocate(const elf_rtld_info_t *elf_list[], int elf_list_len,
		const struct lildyn lildyn[])
{
	for (int elf_id = 0; elf_id < elf_list_len; elf_id++) {
		uintptr_t bias = elf_list[elf_id]->bias;
		const elf_dyn_t *dyn = get_dynamic(elf_list[elf_id]);
		struct bigdyn bigdyn = get_bigdyn(dyn, bias);

		DTRACE("Processing relocations with implicit addend.\n");
		process_rel(elf_list, elf_list_len, elf_id, lildyn, bigdyn.rel, bigdyn.rel_len);

		DTRACE("Processing relocations with explicit addend.\n");
		process_rela(elf_list, elf_list_len, elf_id, lildyn, bigdyn.rela, bigdyn.rela_len);

		DTRACE("Processing PLT relocations with implicit addend.\n");
		process_rel(elf_list, elf_list_len, elf_id, lildyn, bigdyn.plt_rel, bigdyn.plt_rel_len);

		DTRACE("Processing PLT relocations with explicit addend.\n");
		process_rela(elf_list, elf_list_len, elf_id, lildyn, bigdyn.plt_rela, bigdyn.plt_rela_len);
	}
}

void RELOCATOR_NAME(pcb_t *pcb);

void RELOCATOR_NAME(pcb_t *pcb)
{
	// Prepare a few pointers for symbol resolution on stack, since we don't have
	// access to libc facilities yet. At three pointers per entry, we have space
	// for at least a few thousand initially loaded libraries. That should
	// probably be enough for as long as this code exists, and if not, just increase
	// the initial stack. Anyone loading that much code can spare a few more kB.

	const elf_rtld_info_t **res_order = pcb->resolution_order;
	struct lildyn lildyn[pcb->module_count];

	DPRINTF("Loading %zu modules.\n", pcb->module_count);

	for (size_t i = 0; i < pcb->module_count; i++) {
		uintptr_t bias = res_order[i]->bias;
		DPRINTF("Preparing module %zu.\n", i);
		DPRINTF("bias = 0x%zx\n", bias);
		const elf_dyn_t *dyn = get_dynamic(res_order[i]);
		lildyn[i] = get_lildyn(dyn, bias);
	}

	DPRINTF("Modules prepared. Relocating.\n");
	relocate(res_order, pcb->module_count, lildyn);

	// Now that relocations are done, we have to allocate TLS for the initial thread.

	// TODO

	// Once TLS is allocated and the thread pointer set, run initialization functions
	// in the correct order.

	while (true) {}
}
