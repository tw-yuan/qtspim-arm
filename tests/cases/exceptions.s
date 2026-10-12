# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): exceptions handled by the default
# exception handler (CPU/exceptions.s): arithmetic overflow, address errors,
# bad data address, break, traps, division-by-zero check of the div
# pseudo-instruction and signaling floating-point compares.  After each
# exception the handler skips the faulting instruction; the test prints the
# destination register to show it was not modified.
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
word:	.word 0x11223344, 0x55667788
m_add:	.asciiz "after add overflow, $t2 = "
m_addi:	.asciiz "after addi overflow, $t2 = "
m_sub:	.asciiz "after sub overflow, $t2 = "
m_lw:	.asciiz "after unaligned lw, $t2 = "
m_lh:	.asciiz "after unaligned lh, $t2 = "
m_sw:	.asciiz "after unaligned sw, word = "
m_sh:	.asciiz "after unaligned sh, word = "
m_null:	.asciiz "after lw from address 0, $t2 = "
m_high:	.asciiz "after lw from 0x7ffffff0 (above stack), $t2 = "
m_kseg:	.asciiz "after lw from kernel data in user code, $t2 = "
m_break:	.asciiz "after break, $t2 = "
m_teq:	.asciiz "after teq, $t2 = "
m_tne:	.asciiz "after tne (not taken), $t2 = "
m_tgei:	.asciiz "after tgei, $t2 = "
m_tltiu:	.asciiz "after tltiu, $t2 = "
m_div:	.asciiz "after div pseudo-op by zero, $t2 = "
m_rem:	.asciiz "after rem pseudo-op by zero, $t2 = "
m_clt:	.asciiz "after c.lt.s with NaN, $t2 = "
m_cun:	.asciiz "after c.un.s with NaN (quiet compare), $t2 = "
m_done:	.asciiz "done\n"

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	li $t2, 1
	li $t0, 0x7fffffff
	li $t1, 1
	add $t2, $t0, $t1
	move $a1, $t2
	la $a0, m_add
	jal show

	li $t2, 2
	li $t0, 0x7fff0000
	addi $t2, $t0, 0x7fff
	addi $t2, $t2, 0x7fff	# 0x7fff7fff + 0x7fff = 0x7ffffffe: no overflow
	addi $t2, $t2, 2	# overflow
	move $a1, $t2
	la $a0, m_addi
	jal show

	li $t2, 3
	li $t0, 0x80000000
	li $t1, 1
	sub $t2, $t0, $t1
	move $a1, $t2
	la $a0, m_sub
	jal show

	li $t2, 4
	la $t0, word
	lw $t2, 1($t0)
	move $a1, $t2
	la $a0, m_lw
	jal show

	li $t2, 5
	la $t0, word
	lh $t2, 3($t0)
	move $a1, $t2
	la $a0, m_lh
	jal show

	la $t0, word
	li $t1, -1
	sw $t1, 2($t0)
	lw $a1, 0($t0)
	la $a0, m_sw
	jal show

	la $t0, word
	li $t1, -1
	sh $t1, 1($t0)
	lw $a1, 0($t0)
	la $a0, m_sh
	jal show

	li $t2, 6
	lw $t2, 0($zero)
	move $a1, $t2
	la $a0, m_null
	jal show

	li $t2, 7
	li $t0, 0x7ffffff0
	lw $t2, 0($t0)
	move $a1, $t2
	la $a0, m_high
	jal show

	li $t2, 8
	li $t0, 0x90000000
	lw $t2, 0($t0)
	move $a1, $t2
	la $a0, m_kseg
	jal show

	li $t2, 9
	break 0
	move $a1, $t2
	la $a0, m_break
	jal show

	li $t2, 10
	teq $zero, $zero
	move $a1, $t2
	la $a0, m_teq
	jal show

	li $t2, 11
	tne $zero, $zero
	move $a1, $t2
	la $a0, m_tne
	jal show

	li $t2, 12
	li $t0, 100
	tgei $t0, 100
	move $a1, $t2
	la $a0, m_tgei
	jal show

	li $t2, 13
	li $t0, 1
	tltiu $t0, -1		# immediate is sign-extended, compared unsigned
	move $a1, $t2
	la $a0, m_tltiu
	jal show

	li $t2, 14
	li $t0, 7
	div $t2, $t0, $zero
	move $a1, $t2
	la $a0, m_div
	jal show

	li $t2, 15
	li $t0, 7
	rem $t2, $t0, $zero
	move $a1, $t2
	la $a0, m_rem
	jal show

	li $t2, 16
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	mtc1 $zero, $f2
	c.lt.s $f0, $f2		# signaling compare with a NaN
	move $a1, $t2
	la $a0, m_clt
	jal show

	li $t2, 17
	c.un.s $f0, $f2		# quiet compare: no exception
	move $a1, $t2
	la $a0, m_cun
	jal show

	la $a0, m_done
	li $v0, 4
	syscall

	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra

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
