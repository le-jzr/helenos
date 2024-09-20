/*
 * elf2.h
 *
 *  Created on: Sep 17, 2024
 *      Author: jzr
 */

#ifndef USPACE_LIB_C_GENERIC_ELF_ELF2_H_
#define USPACE_LIB_C_GENERIC_ELF_ELF2_H_

#include <elf/elf.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// In this structure, we keep a copy of the file header,
// program headers, and the dynamic section, so that we don't
// need to keep all the files filling up our virtual
// address space. We also copy them onto child's stack
// next to argv if they aren't already part of any DT_LOAD
// segment.
//
typedef struct elf_info {
	int fd;
	size_t file_size;
	elf_header_t header;
	size_t dyn_len;
	elf_dyn_t *dyn;
	size_t dyn_strtab_len;
	char *dyn_strtab;

	uintptr_t bias;
	const char *name;

	size_t phdr_len;
	elf_segment_header_t phdr[];
} elf_head_t;

void elf_compute_bias(uintptr_t *vaddr_limit, elf_head_t *infos[], size_t infos_len);
void elf_info_free(void *arg);

errno_t elf_read_modules(const char *root_name, int root_fd, elf_head_t ***init_order, elf_head_t ***res_order, size_t *nmodules);
errno_t elf_load_modules(elf_head_t *modules[], size_t modules_len);

#endif /* USPACE_LIB_C_GENERIC_ELF_ELF2_H_ */
