# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): sbrk (9).  Prints the addresses returned
# by successive sbrk calls (the heap layout), checks that the allocated
# memory is usable, and ends with a negative sbrk, which is a run-time
# error.
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

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	li $a0, 0		# sbrk(0): current break
	li $v0, 9
	syscall
	move $a1, $v0
	la $a0, m_zero
	jal show

	li $a0, 16		# sbrk(16)
	li $v0, 9
	syscall
	move $s0, $v0
	move $a1, $v0
	la $a0, m_16
	jal show

	li $a0, 5		# sbrk(5): odd size
	li $v0, 9
	syscall
	move $a1, $v0
	la $a0, m_5
	jal show

	li $a0, 0		# break after the odd size
	li $v0, 9
	syscall
	move $a1, $v0
	la $a0, m_zero
	jal show

	li $a0, 0x4000		# sbrk(16 KB): grows the data segment
	li $v0, 9
	syscall
	move $s1, $v0
	move $a1, $v0
	la $a0, m_16kb
	jal show

	li $a0, 0
	li $v0, 9
	syscall
	move $a1, $v0
	la $a0, m_zero
	jal show

	# Use the memory
	li $t0, 0x12345678
	sw $t0, 0($s0)
	sw $t0, 12($s0)
	lw $a1, 12($s0)
	la $a0, m_rw16
	jal show

	li $t0, 0x0badcafe
	li $t2, 0x3ffc
	addu $t2, $s1, $t2	# last word of the 16 KB block
	sw $t0, 0($t2)
	lw $a1, 0($t2)
	la $a0, m_rw16kb
	jal show

	li $a0, -16		# sbrk with a negative size: run-time error
	li $v0, 9
	syscall
	move $a1, $v0
	la $a0, m_neg
	jal show

	la $a0, m_after		# not reached
	li $v0, 4
	syscall

	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra

	.data
m_zero:	.asciiz "sbrk(0) = "
m_16:	.asciiz "sbrk(16) = "
m_5:	.asciiz "sbrk(5) = "
m_16kb:	.asciiz "sbrk(0x4000) = "
m_rw16:	.asciiz "read back from 16-byte block = "
m_rw16kb:	.asciiz "read back from end of 16 KB block = "
m_neg:	.asciiz "sbrk(-16) = "
m_after:	.asciiz "after sbrk(-16) (must not be printed)\n"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

	.data
hex_digits:	.asciiz "0123456789abcdef"

	.text
# print_hex: print $a0 as 0x followed by 8 hex digits.
# Clobbers $t8, $t9, $v0, $a0.
print_hex:
	move $t8, $a0
	li $v0, 11
	li $a0, '0'
	syscall
	li $a0, 'x'
	syscall
	li $t9, 8
ph_loop:
	srl $a0, $t8, 28
	sll $t8, $t8, 4
	lb $a0, hex_digits($a0)
	syscall
	addi $t9, $t9, -1
	bnez $t9, ph_loop
	jr $ra

# show: print the string at $a0, then $a1 in decimal and in hex, then a
# newline.  Clobbers $t8, $t9, $v0, $a0.
show:
	addi $sp, $sp, -8
	sw $ra, 0($sp)
	sw $a1, 4($sp)
	li $v0, 4
	syscall
	lw $a0, 4($sp)
	li $v0, 1
	syscall
	li $a0, ' '
	li $v0, 11
	syscall
	lw $a0, 4($sp)
	jal print_hex
	li $a0, 10
	li $v0, 11
	syscall
	lw $ra, 0($sp)
	addi $sp, $sp, 8
	jr $ra
