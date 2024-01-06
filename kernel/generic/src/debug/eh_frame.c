/*
 * Copyright (c) 2024 Jiří Zárevúcky
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

#include <debug/eh_frame.h>

#include <debug/sections.h>
#include <debug/names.h>
#include <stdio.h>

#include "util.h"

static void print_cfa_code(const uint8_t *data, const uint8_t *data_end, int64_t data_align_factor)
{

	while (data < data_end) {
		uint8_t inst = read_byte(&data, data_end);
		switch (inst & 0xc0) {
		case DW_CFA_advance_loc:
			unsigned delta = inst & 0x3f;
			printf("DW_CFA_advance_loc(%d)\n", delta);
			// emit
			// loc += delta * code_align_factor
			break;
		case DW_CFA_offset:
			uint8_t reg = inst & 0x3f;
			int64_t factored_offset = read_uleb128(&data, data_end);
			//printf("DW_CFA_offset(%d, %"PRIu64")\n", reg, factored_offset);
			printf("r%d = CFA[%"PRId64"]\n", reg, factored_offset * data_align_factor);
			break;
		case DW_CFA_restore:
			reg = inst & 0x3f;
			printf("DW_CFA_restore(r%d)\n", reg);
			break;
		case 0:
			switch (inst) {
			case DW_CFA_def_cfa:
				uint64_t reg = read_uleb128(&data, data_end);
				uint64_t offset = read_uleb128(&data, data_end);
				printf("CFA = r%"PRIu64" + %" PRIu64 "\n", reg, offset);
				break;
			case DW_CFA_def_cfa_sf:
				reg = read_uleb128(&data, data_end);
				int64_t sf_offset = read_sleb128(&data, data_end);
				printf("CFA = r%"PRIu64" + %" PRId64 "\n", reg, sf_offset * data_align_factor);
				break;
			case DW_CFA_def_cfa_offset:
				offset = read_uleb128(&data, data_end);
				printf("CFA = old reg + %" PRIu64 "\n", offset);
				break;
			case DW_CFA_def_cfa_register:
				reg = read_uleb128(&data, data_end);
				printf("CFA = r%" PRIu64 " + old offset\n", reg);
				break;
			case DW_CFA_set_loc:
				// FIXME: should be modified by augmentation data from CIE
				const uint8_t *addr = data + (int) read_uint32(&data, data_end);
				printf("DW_CFA_set_loc(%p)\n", addr);
				break;
			case DW_CFA_advance_loc1:
				delta = read_byte(&data, data_end);
				printf("DW_CFA_advance_loc1(%d)\n", delta);
				break;
			case DW_CFA_advance_loc2:
				delta = read_uint16(&data, data_end);
				printf("DW_CFA_advance_loc2(%d)\n", delta);
				break;
			case DW_CFA_advance_loc4:
				delta = read_uint32(&data, data_end);
				printf("DW_CFA_advance_loc4(%d)\n", delta);
				break;
			case DW_CFA_nop:
				printf("DW_CFA_nop()\n");
				break;
			case DW_CFA_remember_state:
				printf("DW_CFA_remember_state()\n");
				break;
			case DW_CFA_restore_state:
				printf("DW_CFA_restore_state()\n");
				break;
			case DW_CFA_restore_extended:
				reg = read_uleb128(&data, data_end);
				printf("DW_CFA_restore(r%" PRIu64 ")\n", reg);
				break;
			default:
				printf("Unexpected CFA instruction %s (0x%02x).\n", dw_cfa_name(inst), inst);
				return;
			}
		}
	}
}

struct cie {
	const uint8_t *start;
	const uint8_t *end;

	uint8_t version;
	const char *aug;
	uint64_t code_align_factor;
	int64_t data_align_factor;
	int ret_addr_reg;
	uint8_t code_enc;
	const uint8_t *inst_start;
};

static void print_cie(struct cie *cie)
{
	printf("CIE: %p .. %p (%zu bytes)\n", cie->start, cie->end, cie->end - cie->start);
	printf("version: %d\n", cie->version);
	printf("augmentation string: \"%s\"\n", cie->aug);
	printf("code_align_factor: %"PRIu64"\n", cie->code_align_factor);
	printf("data_align_factor: %"PRId64"\n", cie->data_align_factor);
	printf("ret_addr_reg: %d\n", cie->ret_addr_reg);
	print_cfa_code(cie->inst_start, cie->end, cie->data_align_factor);
}

static bool read_cie(struct cie *out, const uint8_t *data, const uint8_t *end)
{
	unsigned width;
	out->start = data;
	const uint64_t entry_len = read_initial_length(&data, end, &width);
	out->end = data + entry_len;

	if (out->end > end)
		return false;

	end = out->end;

	if (entry_len == 0)
		return false;

	uint32_t cie_ptr = read_uint32(&data, end);
	if (cie_ptr != 0)
		return false;

	out->version = read_byte(&data, end);

	/* Only version 1 supported. */
	if (out->version != 1)
		return false;

	const char *aug = read_string(&data, end);
	out->aug = aug;
	out->code_align_factor = read_uleb128(&data, end);
	out->data_align_factor = read_sleb128(&data, end);
	out->ret_addr_reg = read_uleb128(&data, end);

	// Augmentation section
	out->code_enc = DW_EH_PE_omit;

	if (aug[0] == 'z') {
		uint64_t augmentation_section_len = read_uleb128(&data, end);
		const uint8_t *aug_end = data + augmentation_section_len;

		aug++;

		if (aug[0] == 'R') {
			out->code_enc = read_byte(&data, aug_end);
			}

		aug++;

		/* Not supporting any C++ exception stuff in kernel. */
		assert(aug[0] == 0);

		data = aug_end;
	}

	if (out->code_enc != (DW_EH_PE_pcrel | DW_EH_PE_sdata4)) {
		printf("code_enc: %s | %s\n", dw_eh_pe_name(out->code_enc & 0xf0), dw_eh_pe_name(out->code_enc & 0x0f));
	}

	assert(out->code_enc == (DW_EH_PE_pcrel | DW_EH_PE_sdata4));

	out->inst_start = data;
	return true;
}

void eh_frame_parse(void)
{
	const uint8_t *data = &eh_frame_start;
	const uint8_t *end = &eh_frame_end;

	printf("eh_frame: %p\n", data);
	printf("eh_frame_size: %zu\n", end - data);

	while (data < end) {
		unsigned width;
		const uint8_t *entry_start = data;
		const uint64_t entry_len = read_initial_length(&data, end, &width);
		const uint8_t *entry_end = data + entry_len;

		if (entry_len == 0)
			break;

		const uint8_t *cie_ptr_ptr = data;

		uint32_t cie_ptr = read_uint32(&data, entry_end);
		printf("CIE_PTR = %u\n", cie_ptr);

		struct cie cie;

		if (cie_ptr == 0) {
			if (read_cie(&cie, entry_start, entry_end))
				print_cie(&cie);
			data = entry_end;
		} else {
			if (!read_cie(&cie, cie_ptr_ptr - cie_ptr, end)) {
				data = entry_end;
				continue;
			}

			// TODO: honor pointer encoding
			const void *textptr = data;
			int32_t init_loc = read_uint32(&data, entry_end);
			int32_t range = read_uint32(&data, entry_end);

			printf("FDE: %p (%"PRIu64" bytes) (%p .. %p)\n", data, entry_len, textptr + init_loc, textptr + init_loc + range);

			// TODO: aug data
			if (cie.aug[0] != 0){
				size_t aug_len = read_uleb128(&data, entry_end);
				data += aug_len;
			}

			print_cfa_code(data, entry_end, cie.data_align_factor);

			// TODO
			data = entry_end;
		}
	}
}
