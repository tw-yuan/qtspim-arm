# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): load delay slots.  Run by the test
# manifest both with and without -delayed_loads.  With delayed loads the
# instruction right after a load still sees the old register value.
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

	.data
	.align 2
val:	.word 77
half:	.half -3
byte:	.byte 0x80
	.align 3
dbl:	.double 2.5
m_lw:	.asciiz "lw then use: "
m_lw2:	.asciiz "lw then use after one instruction: "
m_lh:	.asciiz "lh then use: "
m_lbu:	.asciiz "lbu then use: "
m_lwc1:	.asciiz "lwc1 then mfc1 (bits): "
m_two:	.asciiz "two loads in a row, use of first: "
m_nl:	.asciiz "\n"

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	li $t0, 5
	lw $t0, val
	move $s0, $t0		# load delay slot
	la $a0, m_lw
	jal print_s0
	nop

	li $t0, 5
	lw $t0, val
	nop
	move $s0, $t0
	la $a0, m_lw2
	jal print_s0
	nop

	li $t1, 6
	lh $t1, half
	move $s0, $t1
	la $a0, m_lh
	jal print_s0
	nop

	li $t2, 7
	lbu $t2, byte
	move $s0, $t2
	la $a0, m_lbu
	jal print_s0
	nop

	li $t3, 8
	mtc1 $t3, $f4
	lwc1 $f4, val
	mfc1 $s0, $f4
	la $a0, m_lwc1
	jal print_s0
	nop

	li $t4, 9
	li $t5, 10
	lw $t4, val
	lh $t5, half
	move $s0, $t4		# $t4 loaded two instructions ago
	la $a0, m_two
	jal print_s0
	nop

	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra
	nop

# print_s0: print the string at $a0 and the value of $s0.
print_s0:
	li $v0, 4
	syscall
	move $a0, $s0
	li $v0, 1
	syscall
	la $a0, m_nl
	li $v0, 4
	syscall
	jr $ra
	nop
