#
# Copyright (c) 2005 Martin Decky
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
#include <arch/context_struct.h>
#include <arch/asm/regname.h>

.text

FUNCTION_BEGIN(context_swap)
	stw sp, CONTEXT_OFFSET_SP(r3)
	stw r2, CONTEXT_OFFSET_R2(r3)
	stw r13, CONTEXT_OFFSET_R13(r3)
	stw r14, CONTEXT_OFFSET_R14(r3)
	stw r15, CONTEXT_OFFSET_R15(r3)
	stw r16, CONTEXT_OFFSET_R16(r3)
	stw r17, CONTEXT_OFFSET_R17(r3)
	stw r18, CONTEXT_OFFSET_R18(r3)
	stw r19, CONTEXT_OFFSET_R19(r3)
	stw r20, CONTEXT_OFFSET_R20(r3)
	stw r21, CONTEXT_OFFSET_R21(r3)
	stw r22, CONTEXT_OFFSET_R22(r3)
	stw r23, CONTEXT_OFFSET_R23(r3)
	stw r24, CONTEXT_OFFSET_R24(r3)
	stw r25, CONTEXT_OFFSET_R25(r3)
	stw r26, CONTEXT_OFFSET_R26(r3)
	stw r27, CONTEXT_OFFSET_R27(r3)
	stw r28, CONTEXT_OFFSET_R28(r3)
	stw r29, CONTEXT_OFFSET_R29(r3)
	stw r30, CONTEXT_OFFSET_R30(r3)
	stw r31, CONTEXT_OFFSET_R31(r3)

	mflr r5
	stw r5, CONTEXT_OFFSET_PC(r3)

	mfcr r5
	stw r5, CONTEXT_OFFSET_CR(r3)

	lwz sp, CONTEXT_OFFSET_SP(r4)
	lwz r2, CONTEXT_OFFSET_R2(r4)
	lwz r13, CONTEXT_OFFSET_R13(r4)
	lwz r14, CONTEXT_OFFSET_R14(r4)
	lwz r15, CONTEXT_OFFSET_R15(r4)
	lwz r16, CONTEXT_OFFSET_R16(r4)
	lwz r17, CONTEXT_OFFSET_R17(r4)
	lwz r18, CONTEXT_OFFSET_R18(r4)
	lwz r19, CONTEXT_OFFSET_R19(r4)
	lwz r20, CONTEXT_OFFSET_R20(r4)
	lwz r21, CONTEXT_OFFSET_R21(r4)
	lwz r22, CONTEXT_OFFSET_R22(r4)
	lwz r23, CONTEXT_OFFSET_R23(r4)
	lwz r24, CONTEXT_OFFSET_R24(r4)
	lwz r25, CONTEXT_OFFSET_R25(r4)
	lwz r26, CONTEXT_OFFSET_R26(r4)
	lwz r27, CONTEXT_OFFSET_R27(r4)
	lwz r28, CONTEXT_OFFSET_R28(r4)
	lwz r29, CONTEXT_OFFSET_R29(r4)
	lwz r30, CONTEXT_OFFSET_R30(r4)
	lwz r31, CONTEXT_OFFSET_R31(r4)

	lwz r5, CONTEXT_OFFSET_CR(r4)
	mtcr r5

	lwz r5, CONTEXT_OFFSET_PC(r4)
	mtlr r5

	blr
FUNCTION_END(context_swap)

FUNCTION_BEGIN(context_replace_arch)
	li r2, 0
	mtcr r2
	addi sp, r4, 0
	mtlr r3
	blr
FUNCTION_END(context_replace_arch)
