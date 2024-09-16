#ifndef ELF_DEBUG_H_
#define ELF_DEBUG_H_

#include <types/common.h>
#include <abi/elf.h>
#include <errno.h>
#include <stdint.h>

void elf_debug_print_dyn(elf_dyn_t *dyn, const char *strtab);
void elf_debug_print_segment_type(uint32_t type);
void elf_debug_print_flags(uint32_t flags);
void elf_debug_print_segment(int i, const elf_segment_header_t *phdr);
errno_t elf_validate_phdr(int i, const elf_segment_header_t *phdr, uint64_t elf_size);
void elf_debug_print_header(const elf_header_t *header);
errno_t elf_validate_header(const elf_header_t *header, uint64_t elf_size);

#endif
