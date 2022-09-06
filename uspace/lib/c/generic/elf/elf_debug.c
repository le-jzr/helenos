#include "elf_debug.h"

#include <stdalign.h>
#include "../private/cc.h"
#include <stddef.h>
#include <align.h>
#include <libarch/config.h>
#include <stdio.h>
#include <io/kio.h>

#define DPRINTF(...) kio_printf(__VA_ARGS__)

static void print_val(const char *name, elf_dyn_t *dyn)
{
	DPRINTF("%s(%"PRIdPTR")\n", name, (uintptr_t) dyn->d_un.d_val);
}

static void print_ptr(const char *name, elf_dyn_t *dyn)
{
	DPRINTF("%s(0x%"PRIxPTR")\n", name, (uintptr_t) dyn->d_un.d_ptr);
}

#define NUL_ENTRY(name) case name: DPRINTF(#name "\n"); break
#define VAL_ENTRY(name) case name: print_val(#name, dyn); break
#define PTR_ENTRY(name) case name: print_ptr(#name, dyn); break

INTERNAL void elf_debug_print_dyn(elf_dyn_t *dyn, const char *strtab)
{
	switch (dyn->d_tag) {
	NUL_ENTRY(DT_NULL);
	case DT_NEEDED:
		if (strtab)
			DPRINTF("DT_NEEDED(%s)\n", strtab + dyn->d_un.d_val);
		else
			print_val("DT_NEEDED", dyn);
		break;
	VAL_ENTRY(DT_PLTRELSZ);
	PTR_ENTRY(DT_PLTGOT);
	PTR_ENTRY(DT_HASH);
	PTR_ENTRY(DT_STRTAB);
	PTR_ENTRY(DT_SYMTAB);
	PTR_ENTRY(DT_RELA);
	VAL_ENTRY(DT_RELASZ);
	VAL_ENTRY(DT_RELAENT);
	VAL_ENTRY(DT_STRSZ);
	VAL_ENTRY(DT_SYMENT);
	PTR_ENTRY(DT_INIT);
	PTR_ENTRY(DT_FINI);
	VAL_ENTRY(DT_SONAME);
	VAL_ENTRY(DT_RPATH);
	NUL_ENTRY(DT_SYMBOLIC);
	PTR_ENTRY(DT_REL);
	VAL_ENTRY(DT_RELSZ);
	VAL_ENTRY(DT_RELENT);
	VAL_ENTRY(DT_PLTREL);
	PTR_ENTRY(DT_DEBUG);
	NUL_ENTRY(DT_TEXTREL);
	PTR_ENTRY(DT_JMPREL);
	NUL_ENTRY(DT_BIND_NOW);
	PTR_ENTRY(DT_INIT_ARRAY);
	PTR_ENTRY(DT_FINI_ARRAY);
	VAL_ENTRY(DT_INIT_ARRAYSZ);
	VAL_ENTRY(DT_FINI_ARRAYSZ);
	VAL_ENTRY(DT_RUNPATH);
	VAL_ENTRY(DT_FLAGS);
	PTR_ENTRY(DT_PREINIT_ARRAY);
	VAL_ENTRY(DT_PREINIT_ARRAYSZ);
	VAL_ENTRY(DT_RELACOUNT);
	default:
		DPRINTF("unknown dyn tag %zd\n", dyn->d_tag);
		break;
	}
}

INTERNAL void elf_debug_print_segment_type(uint32_t type)
{
	DPRINTF("    p_type: ");

	switch (type) {
	case PT_LOAD:
		DPRINTF("PT_LOAD");
		break;
	case PT_NULL:
		DPRINTF("PT_NULL");
		break;
	case PT_PHDR:
		DPRINTF("PT_PHDR");
		break;
	case PT_NOTE:
		DPRINTF("PT_NOTE");
		break;
	case PT_INTERP:
		DPRINTF("PT_INTERP");
		break;
	case PT_DYNAMIC:
		DPRINTF("PT_DYNAMIC");
		break;
	case PT_TLS:
		DPRINTF("PT_TLS");
		break;
	case PT_SHLIB:
		DPRINTF("PT_SHLIB");
		break;
	case PT_GNU_EH_FRAME:
		DPRINTF("PT_GNU_EH_FRAME");
		break;
	case PT_GNU_STACK:
		DPRINTF("PT_GNU_STACK");
		break;
	case PT_GNU_RELRO:
		DPRINTF("PT_GNU_RELRO");
		break;
	default:
		DPRINTF("0x%x", type);
		break;
	}

	DPRINTF("\n");
}

INTERNAL void elf_debug_print_flags(uint32_t flags)
{
	DPRINTF("    p_flags:");
	if (flags & PF_R)
		DPRINTF(" PF_R");
	if (flags & PF_W)
		DPRINTF(" PF_W");
	if (flags & PF_X)
		DPRINTF(" PF_X");
	flags &= ~(PF_R | PF_W | PF_X);
	if (flags)
		DPRINTF(" 0x%x", flags);
	DPRINTF("\n");
}

INTERNAL void elf_debug_print_segment(int i, const elf_segment_header_t *phdr)
{
	DPRINTF("Segment %d {\n", i);
	elf_debug_print_segment_type(phdr->p_type);
	elf_debug_print_flags(phdr->p_flags);
	DPRINTF("    p_offset: 0x%llx (%llu)\n", (unsigned long long) phdr->p_offset, (unsigned long long) phdr->p_offset);
	DPRINTF("    p_vaddr: 0x%llx (%llu)\n", (unsigned long long) phdr->p_vaddr, (unsigned long long) phdr->p_vaddr);
	DPRINTF("    p_paddr: 0x%llx (%llu)\n", (unsigned long long) phdr->p_paddr, (unsigned long long) phdr->p_paddr);
	DPRINTF("    p_filesz: 0x%llx (%llu)\n", (unsigned long long) phdr->p_filesz, (unsigned long long) phdr->p_filesz);
	DPRINTF("    p_memsz: 0x%llx (%llu)\n", (unsigned long long) phdr->p_memsz, (unsigned long long) phdr->p_memsz);
	DPRINTF("    p_align: 0x%llx (%llu)\n", (unsigned long long) phdr->p_align, (unsigned long long) phdr->p_align);
	DPRINTF("}\n");
}

INTERNAL errno_t elf_validate_phdr(int i, const elf_segment_header_t *phdr, uint64_t elf_size)
{
	if ((phdr->p_flags & ~(PF_X | PF_R | PF_W)) != 0) {
		DPRINTF("Unknown flags in segment header.\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	uint64_t offset = phdr->p_offset;
	uint64_t filesz = phdr->p_filesz;
	uint64_t page_limit = UINTPTR_MAX - PAGE_SIZE + 1;

	if (elf_size < offset || elf_size < filesz) {
		DPRINTF("Truncated ELF file, file size = 0x%llx (%llu).\n", (unsigned long long) elf_size, (unsigned long long) elf_size);
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	// Check that ALIGN_UP(offset + filesz, PAGE_SIZE) doesn't overflow.
	if (offset > page_limit || filesz > page_limit - offset) {
		DPRINTF("Declared segment file size too large.\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	// Check that file data is in bounds, even after aligning segment boundaries to multiples of PAGE_SIZE.
	if (elf_size < ALIGN_UP(offset + filesz, PAGE_SIZE)) {
		DPRINTF("Truncated ELF file, file size = 0x%llx (%llu).\n", (unsigned long long) elf_size, (unsigned long long) elf_size);
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	uint64_t vaddr = phdr->p_vaddr;
	uint64_t memsz = phdr->p_memsz;
	uint64_t max = UINTPTR_MAX;

	if (memsz > 0) {
		if (memsz > max || vaddr > max || (max - (memsz - 1)) < vaddr) {
			DPRINTF("vaddr + memsz is outside legal memory range.\n");
			elf_debug_print_segment(i, phdr);
			return EINVAL;
		}

		if (vaddr < PAGE_SIZE && vaddr + memsz > UINTPTR_MAX - PAGE_SIZE + 1) {
			// After alignment, segment spans the entire address space,
			// so its real size overflows uintptr_t.
			DPRINTF("Segment spans entire address space.\n");
			elf_debug_print_segment(i, phdr);
			return EINVAL;
		}
	}

	if (phdr->p_memsz < phdr->p_filesz) {
		DPRINTF("memsz < filesz\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	if (!(phdr->p_flags & PF_R) && phdr->p_filesz != 0) {
		DPRINTF("Nonzero p_filesz in a segment with no read permission.\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	if ((phdr->p_type == PT_LOAD) && (phdr->p_filesz != 0) && !(phdr->p_flags & PF_W) && (phdr->p_filesz != phdr->p_memsz) && (phdr->p_offset + phdr->p_filesz) % PAGE_SIZE != 0) {
		// Technically could be supported, but it's more likely a linking bug than an intended feature.
		DPRINTF("File data does not end on a page boundary (would need zeroing out of page end) in a non-writable segment.\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	size_t align = PAGE_SIZE;
	if (align < phdr->p_align)
		align = phdr->p_align;

	// Check that alignment is a power of two.
	if (__builtin_popcount(align) != 1) {
		DPRINTF("non power-of-2 alignment\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	if (phdr->p_vaddr % align != phdr->p_offset % align) {
		DPRINTF("vaddr is misaligned with offset\n");
		elf_debug_print_segment(i, phdr);
		return EINVAL;
	}

	return EOK;
}

INTERNAL void elf_debug_print_header(const elf_header_t *header)
{
	DPRINTF("TODO: print ELF header\n");
}

INTERNAL errno_t elf_validate_header(const elf_header_t *header, uint64_t elf_size)
{
	if (elf_size < sizeof(*header)) {
		DPRINTF("Truncated ELF header.\n");
		return EINVAL;
	}

	/* Identify ELF */
	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3) {
		DPRINTF("Invalid magic numbers in ELF file header.\n");
		elf_debug_print_header(header);
		return EINVAL;
	}

	/* Identify ELF compatibility */
	if (header->e_ident[EI_DATA] != ELF_DATA_ENCODING ||
	    header->e_machine != ELF_MACHINE ||
	    header->e_ident[EI_VERSION] != EV_CURRENT ||
	    header->e_version != EV_CURRENT ||
	    header->e_ident[EI_CLASS] != ELF_CLASS) {
		DPRINTF("Incompatible data/version/class.\n");
		elf_debug_print_header(header);
		return EINVAL;
	}

	if (header->e_phentsize != sizeof(elf_segment_header_t)) {
		DPRINTF("e_phentsize: %u != %zu\n", header->e_phentsize,
		    sizeof(elf_segment_header_t));
		elf_debug_print_header(header);
		return EINVAL;
	}

	/* Check if the object type is supported. */
	if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
		DPRINTF("Object type %d is not supported\n", header->e_type);
		elf_debug_print_header(header);
		return EINVAL;
	}

	if (header->e_phoff == 0) {
		DPRINTF("Program header table is not present!\n");
		elf_debug_print_header(header);
		return EINVAL;
	}

	// Check that all of program header table is inside the file.
	if (header->e_phoff >= elf_size) {
		DPRINTF("Truncated ELF file, file size = 0x%"PRIx64" (%"PRIu64")\n", elf_size, elf_size);
		elf_debug_print_header(header);
		return EINVAL;
	}

	if ((elf_size - header->e_phoff) / header->e_phentsize < header->e_phnum) {
		DPRINTF("Truncated ELF file, file size = 0x%"PRIx64" (%"PRIu64")\n", elf_size, elf_size);
		elf_debug_print_header(header);
		return EINVAL;
	}

	// Check alignment.
	if (header->e_phoff % alignof(elf_segment_header_t) != 0) {
		DPRINTF("Program header table has invalid alignment.");
		elf_debug_print_header(header);
		return EINVAL;
	}

	return EOK;
}
