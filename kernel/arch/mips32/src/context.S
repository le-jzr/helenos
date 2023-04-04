#
# Copyright (c) 2003-2004 Jakub Jermar
# Copyright (c) 2023 Jiří Zárevúcky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#include <abi/asmtool.h>

.text

.set noat
.set noreorder
.set nomacro

FUNCTION_BEGIN(context_swap_arch)
	addiu $sp, $sp, -48

	sw $ra,  0($sp)
	sw $s0,  4($sp)
	sw $s1,  8($sp)
	sw $s2, 12($sp)
	sw $s3, 16($sp)
	sw $s4, 20($sp)
	sw $s5, 24($sp)
	sw $s6, 28($sp)
	sw $s7, 32($sp)
	sw $s8, 36($sp)
	sw $gp, 40($sp)
	sw $k1, 44($sp)

	sw $sp, 0($a0)
	lw $sp, 0($a1)

	lw $ra,  0($sp)
	lw $s0,  4($sp)
	lw $s1,  8($sp)
	lw $s2, 12($sp)
	lw $s3, 16($sp)
	lw $s4, 20($sp)
	lw $s5, 24($sp)
	lw $s6, 28($sp)
	lw $s7, 32($sp)
	lw $s8, 36($sp)
	lw $gp, 40($sp)
	lw $k1, 44($sp)

	j $ra
	addiu $sp, $sp, 48
FUNCTION_END(context_swap_arch)

FUNCTION_BEGIN(context_replace_arch)
	li $fp, 0
	li $k1, 0
	li $ra, 0

	addiu $sp, $a1, 0

	j $a0
	nop
FUNCTION_END(context_replace_arch)
