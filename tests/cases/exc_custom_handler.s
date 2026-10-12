# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): a minimal user-supplied exception handler,
# loaded with spim -exception_file in place of CPU/exceptions.s.  It prints
# the exception code and the EPC, skips the faulting instruction and
# returns.  It also provides the __start startup code.
#
# Copyright (c) 2026, qtspim-arm contributors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# Neither the name of the James R. Larus nor the names of its contributors may be
# used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

	.kdata
kh_msg:	.asciiz "custom handler: ExcCode="
kh_epc:	.asciiz " EPC="
kh_nl:	.asciiz "\n"
kh_hex:	.asciiz "0123456789abcdef"
	.align 2
kh_save_v0:	.word 0
kh_save_a0:	.word 0
kh_save_t0:	.word 0
kh_save_t1:	.word 0

	.ktext 0x80000180
	.set noat
	move $k1, $at
	.set at
	sw $v0, kh_save_v0
	sw $a0, kh_save_a0
	sw $t0, kh_save_t0
	sw $t1, kh_save_t1

	li $v0, 4
	la $a0, kh_msg
	syscall
	mfc0 $k0, $13		# Cause
	srl $a0, $k0, 2
	andi $a0, $a0, 0x1f
	li $v0, 1
	syscall
	li $v0, 4
	la $a0, kh_epc
	syscall
	mfc0 $t0, $14		# EPC, printed as 8 hex digits
	li $t1, 8
	li $v0, 11
kh_loop:
	srl $a0, $t0, 28
	sll $t0, $t0, 4
	lb $a0, kh_hex($a0)
	syscall
	addi $t1, $t1, -1
	bnez $t1, kh_loop
	li $v0, 4
	la $a0, kh_nl
	syscall

	mfc0 $k0, $14		# skip the faulting instruction
	addiu $k0, $k0, 4
	mtc0 $k0, $14
	lw $v0, kh_save_v0
	lw $a0, kh_save_a0
	lw $t0, kh_save_t0
	lw $t1, kh_save_t1
	.set noat
	move $at, $k1
	.set at
	mtc0 $zero, $13
	mfc0 $k0, $12
	ori $k0, 0x1
	mtc0 $k0, $12
	eret

	.text
	.globl __start
__start:
	lw $a0, 0($sp)		# argc
	addiu $a1, $sp, 4	# argv
	addiu $a2, $a1, 4	# envp
	sll $v0, $a0, 2
	addu $a2, $a2, $v0
	jal main
	nop
	li $v0, 10
	syscall
