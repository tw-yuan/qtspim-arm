# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): branch delay slots.  Run by the test
# manifest both with and without -delayed_branches.  With delayed branches
# the instruction after a branch or jump (the delay slot) is executed, and
# links point past the delay slot; branch-likely instructions annul the
# delay slot when the branch is not taken.
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
m_b:	.asciiz "b: delay slot executed = "
m_beq:	.asciiz "beq taken: delay slot executed = "
m_bne:	.asciiz "bne not taken: delay slot executed = "
m_beql:	.asciiz "beql taken: delay slot executed = "
m_bnel:	.asciiz "bnel not taken: delay slot executed = "
m_j:	.asciiz "j: delay slot executed = "
m_jr:	.asciiz "jr: delay slot executed = "
m_jal:	.asciiz "jal: $ra - address of jal = "
m_jalr:	.asciiz "jalr: $ra - address of jalr = "
m_bal:	.asciiz "bgezal: $ra - address of bgezal = "
m_slot:	.asciiz "jal: delay slot executed = "
m_bc1t:	.asciiz "bc1t taken: delay slot executed = "
m_nl:	.asciiz "\n"

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	li $s0, 0
	b l1
	addi $s0, $s0, 1	# delay slot
	addi $s0, $s0, 100	# never executed
l1:	la $a0, m_b
	jal print_s0
	nop

	li $s0, 0
	beq $zero, $zero, l2
	addi $s0, $s0, 1
	addi $s0, $s0, 100
l2:	la $a0, m_beq
	jal print_s0
	nop

	li $s0, 0
	bne $zero, $zero, l3
	addi $s0, $s0, 1	# delay slot (also the fall-through path)
l3:	la $a0, m_bne
	jal print_s0
	nop

	li $s0, 0
	beql $zero, $zero, l4
	addi $s0, $s0, 1
	addi $s0, $s0, 100
l4:	la $a0, m_beql
	jal print_s0
	nop

	li $s0, 0
	bnel $zero, $zero, l5
	addi $s0, $s0, 1	# annulled when not taken (with delayed branches)
l5:	la $a0, m_bnel
	jal print_s0
	nop

	li $s0, 0
	j l6
	addi $s0, $s0, 1
	addi $s0, $s0, 100
l6:	la $a0, m_j
	jal print_s0
	nop

	li $s0, 0
	la $t0, l7
	jr $t0
	addi $s0, $s0, 1
	addi $s0, $s0, 100
l7:	la $a0, m_jr
	jal print_s0
	nop

	li $s0, 0
here_jal:
	jal l8
	addi $s0, $s0, 1
	addi $s0, $s0, 100
l8:	la $t0, here_jal
	subu $s1, $ra, $t0
	la $a0, m_slot
	jal print_s0
	nop
	move $s0, $s1
	la $a0, m_jal
	jal print_s0
	nop

	la $t1, l9
here_jalr:
	jalr $t1
	nop
	nop
l9:	la $t0, here_jalr
	subu $s0, $ra, $t0
	la $a0, m_jalr
	jal print_s0
	nop

here_bal:
	bgezal $zero, l10
	nop
	nop
l10:	la $t0, here_bal
	subu $s0, $ra, $t0
	la $a0, m_bal
	jal print_s0
	nop

	li $s0, 0
	c.eq.s $f0, $f0
	bc1t l11
	addi $s0, $s0, 1
	addi $s0, $s0, 100
l11:	la $a0, m_bc1t
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
