/*
 * Copyright (c) 2023 Jiří Zárevúcky
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

#ifndef DWARFS_CONSTANTS_H_
#define DWARFS_CONSTANTS_H_

enum {
	DW_SECT_INFO = 1,        // .debug_info.dwo
	DW_SECT_ABBREV = 3,      // .debug_abbrev.dwo
	DW_SECT_LINE = 4,        // .debug_line.dwo
	DW_SECT_LOCLISTS = 5,    // .debug_loclists.dwo
	DW_SECT_STR_OFFSETS = 6, // .debug_str_offsets.dwo
	DW_SECT_MACRO = 7,       // .debug_macro.dwo
	DW_SECT_RNGLISTS = 8,    // .debug_rnglists.dwo

	DW_SECT_MAX,
};

enum {
	DW_ACCESS_public = 0x01,
	DW_ACCESS_protected = 0x02,
	DW_ACCESS_private = 0x03,

	DW_ACCESS_MAX,
};

enum {
	DW_AT_sibling = 0x01,
	DW_AT_location = 0x02,
	DW_AT_name = 0x03,
	DW_AT_reserved_0x04 = 0x04,
	DW_AT_fund_type = 0x05,
	DW_AT_mod_fund_type = 0x06,
	DW_AT_user_def_type = 0x07,
	DW_AT_mod_u_d_type = 0x08,
	DW_AT_ordering = 0x09,
	DW_AT_subscr_data = 0x0a,
	DW_AT_byte_size = 0x0b,
	DW_AT_bit_offset = 0x0c,
	DW_AT_bit_size = 0x0d,
	DW_AT_reserved_0x0e = 0x0e,
	DW_AT_unused8 = 0x0f,
	DW_AT_stmt_list = 0x10,
	DW_AT_low_pc = 0x11,
	DW_AT_high_pc = 0x12,
	DW_AT_language = 0x13,
	DW_AT_member = 0x14,
	DW_AT_discr = 0x15,
	DW_AT_discr_value = 0x16,
	DW_AT_visibility = 0x17,
	DW_AT_import = 0x18,
	DW_AT_string_length = 0x19,
	DW_AT_common_reference = 0x1a,
	DW_AT_comp_dir = 0x1b,
	DW_AT_const_value = 0x1c,
	DW_AT_containing_type = 0x1d,
	DW_AT_default_value = 0x1e,
	DW_AT_reserved_0x1f = 0x1f,
	DW_AT_inline = 0x20,
	DW_AT_is_optional = 0x21,
	DW_AT_lower_bound = 0x22,
	DW_AT_reserved_0x23 = 0x23,
	DW_AT_reserved_0x24 = 0x24,
	DW_AT_producer = 0x25,
	DW_AT_reserved_0x26 = 0x26,
	DW_AT_prototyped = 0x27,
	DW_AT_reserved_0x28 = 0x28,
	DW_AT_reserved_0x29 = 0x29,
	DW_AT_return_addr = 0x2a,
	DW_AT_reserved_0x2b = 0x2b,
	DW_AT_start_scope = 0x2c,
	DW_AT_reserved_0x2d = 0x2d,
	DW_AT_bit_stride = 0x2e,
	DW_AT_upper_bound = 0x2f,
	DW_AT_virtual = 0x30,
	DW_AT_abstract_origin = 0x31,
	DW_AT_accessibility = 0x32,
	DW_AT_address_class = 0x33,
	DW_AT_artificial = 0x34,
	DW_AT_base_types = 0x35,
	DW_AT_calling_convention = 0x36,
	DW_AT_count = 0x37,
	DW_AT_data_member_location = 0x38,
	DW_AT_decl_column = 0x39,
	DW_AT_decl_file = 0x3a,
	DW_AT_decl_line = 0x3b,
	DW_AT_declaration = 0x3c,
	DW_AT_discr_list = 0x3d,
	DW_AT_encoding = 0x3e,
	DW_AT_external = 0x3f,
	DW_AT_frame_base = 0x40,
	DW_AT_friend = 0x41,
	DW_AT_identifier_case = 0x42,
	DW_AT_macro_info = 0x43,
	DW_AT_namelist_item = 0x44,
	DW_AT_priority = 0x45,
	DW_AT_segment = 0x46,
	DW_AT_specification = 0x47,
	DW_AT_static_link = 0x48,
	DW_AT_type = 0x49,
	DW_AT_use_location = 0x4a,
	DW_AT_variable_parameter = 0x4b,
	DW_AT_virtuality = 0x4c,
	DW_AT_vtable_elem_location = 0x4d,
	DW_AT_allocated = 0x4e,
	DW_AT_associated = 0x4f,
	DW_AT_data_location = 0x50,
	DW_AT_byte_stride = 0x51,
	DW_AT_entry_pc = 0x52,
	DW_AT_use_UTF8 = 0x53,
	DW_AT_extension = 0x54,
	DW_AT_ranges = 0x55,
	DW_AT_trampoline = 0x56,
	DW_AT_call_column = 0x57,
	DW_AT_call_file = 0x58,
	DW_AT_call_line = 0x59,
	DW_AT_description = 0x5a,
	DW_AT_binary_scale = 0x5b,
	DW_AT_decimal_scale = 0x5c,
	DW_AT_small = 0x5d,
	DW_AT_decimal_sign = 0x5e,
	DW_AT_digit_count = 0x5f,
	DW_AT_picture_string = 0x60,
	DW_AT_mutable = 0x61,
	DW_AT_threads_scaled = 0x62,
	DW_AT_explicit = 0x63,
	DW_AT_object_pointer = 0x64,
	DW_AT_endianity = 0x65,
	DW_AT_elemental = 0x66,
	DW_AT_pure = 0x67,
	DW_AT_recursive = 0x68,
	DW_AT_signature = 0x69,
	DW_AT_main_subprogram = 0x6a,
	DW_AT_data_bit_offset = 0x6b,
	DW_AT_const_expr = 0x6c,
	DW_AT_enum_class = 0x6d,
	DW_AT_linkage_name = 0x6e,
	DW_AT_string_length_bit_size = 0x6f,
	DW_AT_string_length_byte_size = 0x70,
	DW_AT_rank = 0x71,
	DW_AT_str_offsets_base = 0x72,
	DW_AT_addr_base = 0x73,
	DW_AT_rnglists_base = 0x74,
	DW_AT_dwo_id = 0x75,
	DW_AT_dwo_name = 0x76,
	DW_AT_reference = 0x77,
	DW_AT_rvalue_reference = 0x78,
	DW_AT_macros = 0x79,
	DW_AT_call_all_calls = 0x7a,
	DW_AT_call_all_source_calls = 0x7b,
	DW_AT_call_all_tail_calls = 0x7c,
	DW_AT_call_return_pc = 0x7d,
	DW_AT_call_value = 0x7e,
	DW_AT_call_origin = 0x7f,
	DW_AT_call_parameter = 0x80,
	DW_AT_call_pc = 0x81,
	DW_AT_call_tail_call = 0x82,
	DW_AT_call_target = 0x83,
	DW_AT_call_target_clobbered = 0x84,
	DW_AT_call_data_location = 0x85,
	DW_AT_call_data_value = 0x86,
	DW_AT_noreturn = 0x87,
	DW_AT_alignment = 0x88,
	DW_AT_export_symbols = 0x89,
	DW_AT_deleted = 0x8a,
	DW_AT_defaulted = 0x8b,
	DW_AT_loclists_base = 0x8c,

	DW_AT_MAX,

	DW_AT_lo_user = 0x2000,
	DW_AT_hi_user = 0x3fff,
};

enum {
	DW_ATE_address = 0x01,
	DW_ATE_boolean = 0x02,
	DW_ATE_complex_float = 0x03,
	DW_ATE_float = 0x04,
	DW_ATE_signed = 0x05,
	DW_ATE_signed_char = 0x06,
	DW_ATE_unsigned = 0x07,
	DW_ATE_unsigned_char = 0x08,
	DW_ATE_imaginary_float = 0x09,
	DW_ATE_packed_decimal = 0x0a,
	DW_ATE_numeric_string = 0x0b,
	DW_ATE_edited = 0x0c,
	DW_ATE_signed_fixed = 0x0d,
	DW_ATE_unsigned_fixed = 0x0e,
	DW_ATE_decimal_float = 0x0f,
	DW_ATE_UTF = 0x10,
	DW_ATE_UCS = 0x11,
	DW_ATE_ASCII = 0x12,

	DW_ATE_MAX,

	DW_ATE_lo_user = 0x80,
	DW_ATE_hi_user = 0xff,
};

enum {
	DW_CC_normal = 0x01,
	DW_CC_program = 0x02,
	DW_CC_nocall = 0x03,
	DW_CC_pass_by_reference = 0x04,
	DW_CC_pass_by_value = 0x05,

	DW_CC_MAX,

	DW_CC_lo_user = 0x40,
	DW_CC_hi_user = 0xff,
};

enum {
	DW_UT_compile = 1,
	DW_UT_type = 2,
	DW_UT_partial = 3,
	DW_UT_skeleton = 4,
	DW_UT_split_compile = 5,
	DW_UT_split_type = 6,

	DW_UT_MAX,

	DW_UT_lo_user = 0x80,
	DW_UT_hi_user = 0xff,
};

enum {
	DW_TAG_array_type = 0x01,
	DW_TAG_class_type = 0x02,
	DW_TAG_entry_point = 0x03,
	DW_TAG_enumeration_type = 0x04,
	DW_TAG_formal_parameter = 0x05,
	DW_TAG_global_subroutine = 0x06,
	DW_TAG_global_variable = 0x07,
	DW_TAG_imported_declaration = 0x08,
	DW_TAG_reserved_0x09 = 0x09,
	DW_TAG_label = 0x0a,
	DW_TAG_lexical_block = 0x0b,
	DW_TAG_local_variable = 0x0c,
	DW_TAG_member = 0x0d,
	DW_TAG_reserved_0x0e = 0x0e,
	DW_TAG_pointer_type = 0x0f,
	DW_TAG_reference_type = 0x10,
	DW_TAG_compile_unit = 0x11,
	DW_TAG_string_type = 0x12,
	DW_TAG_structure_type = 0x13,
	DW_TAG_subroutine = 0x14,
	DW_TAG_subroutine_type = 0x15,
	DW_TAG_typedef = 0x16,
	DW_TAG_union_type = 0x17,
	DW_TAG_unspecified_parameters = 0x18,
	DW_TAG_variant = 0x19,
	DW_TAG_common_block = 0x1a,
	DW_TAG_common_inclusion = 0x1b,
	DW_TAG_inheritance = 0x1c,
	DW_TAG_inlined_subroutine = 0x1d,
	DW_TAG_module = 0x1e,
	DW_TAG_ptr_to_member_type = 0x1f,
	DW_TAG_set_type = 0x20,
	DW_TAG_subrange_type = 0x21,
	DW_TAG_with_stmt = 0x22,
	DW_TAG_access_declaration = 0x23,
	DW_TAG_base_type = 0x24,
	DW_TAG_catch_block = 0x25,
	DW_TAG_const_type = 0x26,
	DW_TAG_constant = 0x27,
	DW_TAG_enumerator = 0x28,
	DW_TAG_file_type = 0x29,
	DW_TAG_friend = 0x2a,
	DW_TAG_namelist = 0x2b,
	DW_TAG_namelist_item = 0x2c,
	DW_TAG_packed_type = 0x2d,
	DW_TAG_subprogram = 0x2e,
	DW_TAG_template_type_parameter = 0x2f,
	DW_TAG_template_value_parameter = 0x30,
	DW_TAG_thrown_type = 0x31,
	DW_TAG_try_block = 0x32,
	DW_TAG_variant_part = 0x33,
	DW_TAG_variable = 0x34,
	DW_TAG_volatile_type = 0x35,
	DW_TAG_dwarf_procedure = 0x36,
	DW_TAG_restrict_type = 0x37,
	DW_TAG_interface_type = 0x38,
	DW_TAG_namespace = 0x39,
	DW_TAG_imported_module = 0x3a,
	DW_TAG_unspecified_type = 0x3b,
	DW_TAG_partial_unit = 0x3c,
	DW_TAG_imported_unit = 0x3d,
	DW_TAG_mutable_type = 0x3e,
	DW_TAG_condition = 0x3f,
	DW_TAG_shared_type = 0x40,
	DW_TAG_type_unit = 0x41,
	DW_TAG_rvalue_reference_type = 0x42,
	DW_TAG_template_alias = 0x43,
	DW_TAG_coarray_type = 0x44,
	DW_TAG_generic_subrange = 0x45,
	DW_TAG_dynamic_type = 0x46,
	DW_TAG_atomic_type = 0x47,
	DW_TAG_call_site = 0x48,
	DW_TAG_call_site_parameter = 0x49,
	DW_TAG_skeleton_unit = 0x4a,
	DW_TAG_immutable_type = 0x4b,

	DW_TAG_MAX,

	DW_TAG_lo_user = 0x4080,
	DW_TAG_hi_user = 0xffff,
};

enum {
	DW_FORM_addr = 0x01,
	DW_FORM_reserved_0x02 = 0x02,
	DW_FORM_block2 = 0x03,
	DW_FORM_block4 = 0x04,
	DW_FORM_data2 = 0x05,
	DW_FORM_data4 = 0x06,
	DW_FORM_data8 = 0x07,
	DW_FORM_string = 0x08,
	DW_FORM_block = 0x09,
	DW_FORM_block1 = 0x0a,
	DW_FORM_data1 = 0x0b,
	DW_FORM_flag = 0x0c,
	DW_FORM_sdata = 0x0d,
	DW_FORM_strp = 0x0e,
	DW_FORM_udata = 0x0f,
	DW_FORM_ref_addr = 0x10,
	DW_FORM_ref1 = 0x11,
	DW_FORM_ref2 = 0x12,
	DW_FORM_ref4 = 0x13,
	DW_FORM_ref8 = 0x14,
	DW_FORM_ref_udata = 0x15,
	DW_FORM_indirect = 0x16,
	DW_FORM_sec_offset = 0x17,
	DW_FORM_exprloc = 0x18,
	DW_FORM_flag_present = 0x19,
	DW_FORM_strx = 0x1a,
	DW_FORM_addrx = 0x1b,
	DW_FORM_ref_sup4 = 0x1c,
	DW_FORM_strp_sup = 0x1d,
	DW_FORM_data16 = 0x1e,
	DW_FORM_line_strp = 0x1f,
	DW_FORM_ref_sig8 = 0x20,
	DW_FORM_implicit_const = 0x21,
	DW_FORM_loclistx = 0x22,
	DW_FORM_rnglistx = 0x23,
	DW_FORM_ref_sup8 = 0x24,
	DW_FORM_strx1 = 0x25,
	DW_FORM_strx2 = 0x26,
	DW_FORM_strx3 = 0x27,
	DW_FORM_strx4 = 0x28,
	DW_FORM_addrx1 = 0x29,
	DW_FORM_addrx2 = 0x2a,
	DW_FORM_addrx3 = 0x2b,
	DW_FORM_addrx4 = 0x2c,

	DW_FORM_MAX,
};

enum {
	DW_OP_reg = 0x01,
	DW_OP_basereg = 0x02,
	DW_OP_addr = 0x03,
	DW_OP_const = 0x04,
	DW_OP_deref2 = 0x05,
	DW_OP_deref = 0x06,
	DW_OP_add = 0x07,
	DW_OP_const1u = 0x08,
	DW_OP_const1s = 0x09,
	DW_OP_const2u = 0x0a,
	DW_OP_const2s = 0x0b,
	DW_OP_const4u = 0x0c,
	DW_OP_const4s = 0x0d,
	DW_OP_const8u = 0x0e,
	DW_OP_const8s = 0x0f,
	DW_OP_constu = 0x10,
	DW_OP_consts = 0x11,
	DW_OP_dup = 0x12,
	DW_OP_drop = 0x13,
	DW_OP_over = 0x14,
	DW_OP_pick = 0x15,
	DW_OP_swap = 0x16,
	DW_OP_rot = 0x17,
	DW_OP_xderef = 0x18,
	DW_OP_abs = 0x19,
	DW_OP_and = 0x1a,
	DW_OP_div = 0x1b,
	DW_OP_minus = 0x1c,
	DW_OP_mod = 0x1d,
	DW_OP_mul = 0x1e,
	DW_OP_neg = 0x1f,
	DW_OP_not = 0x20,
	DW_OP_or = 0x21,
	DW_OP_plus = 0x22,
	DW_OP_plus_uconst = 0x23,
	DW_OP_shl = 0x24,
	DW_OP_shr = 0x25,
	DW_OP_shra = 0x26,
	DW_OP_xor = 0x27,
	DW_OP_bra = 0x28,
	DW_OP_eq = 0x29,
	DW_OP_ge = 0x2a,
	DW_OP_gt = 0x2b,
	DW_OP_le = 0x2c,
	DW_OP_lt = 0x2d,
	DW_OP_ne = 0x2e,
	DW_OP_skip = 0x2f,
	DW_OP_lit0 = 0x30,
	DW_OP_lit1 = 0x31,
	DW_OP_lit2 = 0x32,
	DW_OP_lit3 = 0x33,
	DW_OP_lit4 = 0x34,
	DW_OP_lit5 = 0x35,
	DW_OP_lit6 = 0x36,
	DW_OP_lit7 = 0x37,
	DW_OP_lit8 = 0x38,
	DW_OP_lit9 = 0x39,
	DW_OP_lit10 = 0x3a,
	DW_OP_lit11 = 0x3b,
	DW_OP_lit12 = 0x3c,
	DW_OP_lit13 = 0x3d,
	DW_OP_lit14 = 0x3e,
	DW_OP_lit15 = 0x3f,
	DW_OP_lit16 = 0x40,
	DW_OP_lit17 = 0x41,
	DW_OP_lit18 = 0x42,
	DW_OP_lit19 = 0x43,
	DW_OP_lit20 = 0x44,
	DW_OP_lit21 = 0x45,
	DW_OP_lit22 = 0x46,
	DW_OP_lit23 = 0x47,
	DW_OP_lit24 = 0x48,
	DW_OP_lit25 = 0x49,
	DW_OP_lit26 = 0x4a,
	DW_OP_lit27 = 0x4b,
	DW_OP_lit28 = 0x4c,
	DW_OP_lit29 = 0x4d,
	DW_OP_lit30 = 0x4e,
	DW_OP_lit31 = 0x4f,
	DW_OP_reg0 = 0x50,
	DW_OP_reg1 = 0x51,
	DW_OP_reg2 = 0x52,
	DW_OP_reg3 = 0x53,
	DW_OP_reg4 = 0x54,
	DW_OP_reg5 = 0x55,
	DW_OP_reg6 = 0x56,
	DW_OP_reg7 = 0x57,
	DW_OP_reg8 = 0x58,
	DW_OP_reg9 = 0x59,
	DW_OP_reg10 = 0x5a,
	DW_OP_reg11 = 0x5b,
	DW_OP_reg12 = 0x5c,
	DW_OP_reg13 = 0x5d,
	DW_OP_reg14 = 0x5e,
	DW_OP_reg15 = 0x5f,
	DW_OP_reg16 = 0x60,
	DW_OP_reg17 = 0x61,
	DW_OP_reg18 = 0x62,
	DW_OP_reg19 = 0x63,
	DW_OP_reg20 = 0x64,
	DW_OP_reg21 = 0x65,
	DW_OP_reg22 = 0x66,
	DW_OP_reg23 = 0x67,
	DW_OP_reg24 = 0x68,
	DW_OP_reg25 = 0x69,
	DW_OP_reg26 = 0x6a,
	DW_OP_reg27 = 0x6b,
	DW_OP_reg28 = 0x6c,
	DW_OP_reg29 = 0x6d,
	DW_OP_reg30 = 0x6e,
	DW_OP_reg31 = 0x6f,
	DW_OP_breg0 = 0x70,
	DW_OP_breg1 = 0x71,
	DW_OP_breg2 = 0x72,
	DW_OP_breg3 = 0x73,
	DW_OP_breg4 = 0x74,
	DW_OP_breg5 = 0x75,
	DW_OP_breg6 = 0x76,
	DW_OP_breg7 = 0x77,
	DW_OP_breg8 = 0x78,
	DW_OP_breg9 = 0x79,
	DW_OP_breg10 = 0x7a,
	DW_OP_breg11 = 0x7b,
	DW_OP_breg12 = 0x7c,
	DW_OP_breg13 = 0x7d,
	DW_OP_breg14 = 0x7e,
	DW_OP_breg15 = 0x7f,
	DW_OP_breg16 = 0x80,
	DW_OP_breg17 = 0x81,
	DW_OP_breg18 = 0x82,
	DW_OP_breg19 = 0x83,
	DW_OP_breg20 = 0x84,
	DW_OP_breg21 = 0x85,
	DW_OP_breg22 = 0x86,
	DW_OP_breg23 = 0x87,
	DW_OP_breg24 = 0x88,
	DW_OP_breg25 = 0x89,
	DW_OP_breg26 = 0x8a,
	DW_OP_breg27 = 0x8b,
	DW_OP_breg28 = 0x8c,
	DW_OP_breg29 = 0x8d,
	DW_OP_breg30 = 0x8e,
	DW_OP_breg31 = 0x8f,
	DW_OP_regx = 0x90,
	DW_OP_fbreg = 0x91,
	DW_OP_bregx = 0x92,
	DW_OP_piece = 0x93,
	DW_OP_deref_size = 0x94,
	DW_OP_xderef_size = 0x95,
	DW_OP_nop = 0x96,
	DW_OP_push_object_address = 0x97,
	DW_OP_call2 = 0x98,
	DW_OP_call4 = 0x99,
	DW_OP_call_ref = 0x9a,
	DW_OP_form_tls_address = 0x9b,
	DW_OP_call_frame_cfa = 0x9c,
	DW_OP_bit_piece = 0x9d,
	DW_OP_implicit_value = 0x9e,
	DW_OP_stack_value = 0x9f,
	DW_OP_implicit_pointer = 0xa0,
	DW_OP_addrx = 0xa1,
	DW_OP_constx = 0xa2,
	DW_OP_entry_value = 0xa3,
	DW_OP_const_type = 0xa4,
	DW_OP_regval_type = 0xa5,
	DW_OP_deref_type = 0xa6,
	DW_OP_xderef_type = 0xa7,
	DW_OP_convert = 0xa8,
	DW_OP_reinterpret = 0xa9,

	DW_OP_MAX,

	DW_OP_lo_user = 0xe0,
	DW_OP_hi_user = 0xff,
};

enum {
	DW_LLE_end_of_list = 0x00,
	DW_LLE_base_addressx = 0x01,
	DW_LLE_startx_endx = 0x02,
	DW_LLE_startx_length = 0x03,
	DW_LLE_offset_pair = 0x04,
	DW_LLE_default_location = 0x05,
	DW_LLE_base_address = 0x06,
	DW_LLE_start_end = 0x07,
	DW_LLE_start_length = 0x08,

	DW_LLE_MAX,
};

enum {
	DW_DS_unsigned = 0x01,
	DW_DS_leading_overpunch = 0x02,
	DW_DS_trailing_overpunch = 0x03,
	DW_DS_leading_separate = 0x04,
	DW_DS_trailing_separate = 0x05,

	DW_DS_MAX,
};

enum {
	DW_END_default = 0x00,
	DW_END_big = 0x01,
	DW_END_little = 0x02,

	DW_END_MAX,

	DW_END_lo_user = 0x40,
	DW_END_hi_user = 0xff,
};

enum {
	DW_VIS_local = 0x01,
	DW_VIS_exported = 0x02,
	DW_VIS_qualified = 0x03,

	DW_VIS_MAX,
};

enum {
	DW_VIRTUALITY_none = 0x00,
	DW_VIRTUALITY_virtual = 0x01,
	DW_VIRTUALITY_pure_virtual = 0x02,

	DW_VIRTUALITY_MAX,
};

enum {
	DW_LANG_C89 = 0x01,
	DW_LANG_C = 0x02,
	DW_LANG_Ada83 = 0x03,
	DW_LANG_C_plus_plus = 0x04,
	DW_LANG_Cobol74 = 0x05,
	DW_LANG_Cobol85 = 0x06,
	DW_LANG_Fortran77 = 0x07,
	DW_LANG_Fortran90 = 0x08,
	DW_LANG_Pascal83 = 0x09,
	DW_LANG_Modula2 = 0x0a,
	DW_LANG_Java = 0x0b,
	DW_LANG_C99 = 0x0c,
	DW_LANG_Ada95 = 0x0d,
	DW_LANG_Fortran95 = 0x0e,
	DW_LANG_PLI = 0x0f,
	DW_LANG_ObjC = 0x10,
	DW_LANG_ObjC_plus_plus = 0x11,
	DW_LANG_UPC = 0x12,
	DW_LANG_D = 0x13,
	DW_LANG_Python = 0x14,
	DW_LANG_OpenCL = 0x15,
	DW_LANG_Go = 0x16,
	DW_LANG_Modula3 = 0x17,
	DW_LANG_Haskell = 0x18,
	DW_LANG_C_plus_plus_03 = 0x19,
	DW_LANG_C_plus_plus_11 = 0x1a,
	DW_LANG_OCaml = 0x1b,
	DW_LANG_Rust = 0x1c,
	DW_LANG_C11 = 0x1d,
	DW_LANG_Swift = 0x1e,
	DW_LANG_Julia = 0x1f,
	DW_LANG_Dylan = 0x20,
	DW_LANG_C_plus_plus_14 = 0x21,
	DW_LANG_Fortran03 = 0x22,
	DW_LANG_Fortran08 = 0x23,
	DW_LANG_RenderScript = 0x24,
	DW_LANG_BLISS = 0x25,

	DW_LANG_MAX,

	DW_LANG_lo_user = 0x8000,
	DW_LANG_hi_user = 0xffff,
};

enum {
	DW_ADDR_none = 0,
};

enum {
	DW_ID_case_sensitive = 0x00,
	DW_ID_up_case = 0x01,
	DW_ID_down_case = 0x02,
	DW_ID_case_insensitive = 0x03,

	DW_ID_MAX,
};

enum {
	DW_LNS_copy = 0x01,
	DW_LNS_advance_pc = 0x02,
	DW_LNS_advance_line = 0x03,
	DW_LNS_set_file = 0x04,
	DW_LNS_set_column = 0x05,
	DW_LNS_negate_stmt = 0x06,
	DW_LNS_set_basic_block = 0x07,
	DW_LNS_const_add_pc = 0x08,
	DW_LNS_fixed_advance_pc = 0x09,
	DW_LNS_set_prologue_end = 0x0a,
	DW_LNS_set_epilogue_begin = 0x0b,
	DW_LNS_set_isa = 0x0c,

	DW_LNS_MAX,
};

enum {
	DW_LNE_end_sequence = 0x01,
	DW_LNE_set_address = 0x02,
	DW_LNE_define_file = 0x03,
	DW_LNE_set_discriminator = 0x04,

	DW_LNE_MAX,

	DW_LNE_lo_user = 0x80,
	DW_LNE_hi_user = 0xff,
};

enum {
	DW_LNCT_path = 0x1,
	DW_LNCT_directory_index = 0x2,
	DW_LNCT_timestamp = 0x3,
	DW_LNCT_size = 0x4,
	DW_LNCT_MD5 = 0x5,

	DW_LNCT_MAX,

	DW_LNCT_lo_user = 0x2000,
	DW_LNCT_hi_user = 0x3fff,
};

enum {
	DW_INL_not_inlined = 0x00,
	DW_INL_inlined = 0x01,
	DW_INL_declared_not_inlined = 0x02,
	DW_INL_declared_inlined = 0x03,

	DW_INL_MAX,
};

enum {
	DW_ORD_row_major = 0x00,
	DW_ORD_col_major = 0x01,

	DW_ORD_MAX,
};

enum {
	DW_DSC_label = 0x00,
	DW_DSC_range = 0x01,

	DW_DSC_MAX,
};

enum {
	DW_IDX_compile_unit = 1,
	DW_IDX_type_unit = 2,
	DW_IDX_die_offset = 3,
	DW_IDX_parent = 4,
	DW_IDX_type_hash = 5,

	DW_IDX_MAX,

	DW_IDX_lo_user = 0x2000,
	DW_IDX_hi_user = 0x3fff,
};

enum {
	DW_DEFAULTED_no = 0x00,
	DW_DEFAULTED_in_class = 0x01,
	DW_DEFAULTED_out_of_class = 0x02,

	DW_DEFAULTED_MAX,
};

enum {
	DW_MACRO_define = 0x01,
	DW_MACRO_undef = 0x02,
	DW_MACRO_start_file = 0x03,
	DW_MACRO_end_file = 0x04,
	DW_MACRO_define_strp = 0x05,
	DW_MACRO_undef_strp = 0x06,
	DW_MACRO_import = 0x07,
	DW_MACRO_define_sup = 0x08,
	DW_MACRO_undef_sup = 0x09,
	DW_MACRO_import_sup = 0x0a,
	DW_MACRO_define_strx = 0x0b,
	DW_MACRO_undef_strx = 0x0c,

	DW_MACRO_MAX,

	DW_MACRO_lo_user = 0xe0,
	DW_MACRO_hi_user = 0xff,
};

enum {
	DW_CFA_nop = 0,
	DW_CFA_set_loc = 0x01,
	DW_CFA_advance_loc1 = 0x02,
	DW_CFA_advance_loc2 = 0x03,
	DW_CFA_advance_loc4 = 0x04,
	DW_CFA_offset_extended = 0x05,
	DW_CFA_restore_extended = 0x06,
	DW_CFA_undefined = 0x07,
	DW_CFA_same_value = 0x08,
	DW_CFA_register = 0x09,
	DW_CFA_remember_state = 0x0a,
	DW_CFA_restore_state = 0x0b,
	DW_CFA_def_cfa = 0x0c,
	DW_CFA_def_cfa_register = 0x0d,
	DW_CFA_def_cfa_offset = 0x0e,
	DW_CFA_def_cfa_expression = 0x0f,
	DW_CFA_expression = 0x10,
	DW_CFA_offset_extended_sf = 0x11,
	DW_CFA_def_cfa_sf = 0x12,
	DW_CFA_def_cfa_offset_sf = 0x13,
	DW_CFA_val_offset = 0x14,
	DW_CFA_val_offset_sf = 0x15,
	DW_CFA_val_expression = 0x16,

	DW_CFA_MAX,

	DW_CFA_lo_user = 0x1c,
	DW_CFA_GNU_args_size = 0x2e,
	DW_CFA_GNU_negative_offset_extended = 0x2f,
	DW_CFA_hi_user = 0x3f,

	/* These hold extra data in the 6 low-order bits. */
	DW_CFA_advance_loc = 0x40,
	DW_CFA_offset = 0x80,
	DW_CFA_restore = 0xc0,
};

enum {
	DW_RLE_end_of_list = 0x00,
	DW_RLE_base_addressx = 0x01,
	DW_RLE_startx_endx = 0x02,
	DW_RLE_startx_length = 0x03,
	DW_RLE_offset_pair = 0x04,
	DW_RLE_base_address = 0x05,
	DW_RLE_start_end = 0x06,
	DW_RLE_start_length = 0x07,

	DW_RLE_MAX,
};

/* Definitions from Linux Standard Base Core - Generic 5.0 */
enum {
	DW_EH_PE_absptr   = 0x00,  // The Value is a literal pointer whose size is determined by the architecture.
	DW_EH_PE_uleb128  = 0x01,  // Unsigned value is encoded using the Little Endian Base 128 (LEB128) as defined by DWARF Debugging Information Format, Version 4.
	DW_EH_PE_udata2   = 0x02,  // A 2 bytes unsigned value.
	DW_EH_PE_udata4	  = 0x03,  // A 4 bytes unsigned value.
	DW_EH_PE_udata8	  = 0x04,  // An 8 bytes unsigned value.
	DW_EH_PE_sleb128  = 0x09,  // Signed value is encoded using the Little Endian Base 128 (LEB128) as defined by DWARF Debugging Information Format, Version 4.
	DW_EH_PE_sdata2   = 0x0a,  // A 2 bytes signed value.
	DW_EH_PE_sdata4   = 0x0b,  // A 4 bytes signed value.
	DW_EH_PE_sdata8   = 0x0c,  // An 8 bytes signed value.
	DW_EH_PE_omit_low = 0x0f,  // No value is present.

	DW_EH_PE_pcrel   = 0x10,  // Value is relative to the current program counter.
	DW_EH_PE_textrel = 0x20,  // Value is relative to the beginning of the .text section.
	DW_EH_PE_datarel = 0x30,  // Value is relative to the beginning of the .got or .eh_frame_hdr section.
	DW_EH_PE_funcrel = 0x40,  // Value is relative to the beginning of the function.
	DW_EH_PE_aligned = 0x50,  // Value is aligned to an address unit sized boundary.
	DW_EH_PE_omit_high = 0xf0, // No value is present.

	DW_EH_PE_omit    = 0xff,
};

#endif /* DWARFS_CONSTANTS_H_ */
