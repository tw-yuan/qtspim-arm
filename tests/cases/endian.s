# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): byte order.  SPIM simulates a
# little-endian MIPS: byte, halfword and word loads and stores, lwl/lwr/
# swl/swr, unaligned pseudo-instructions, data directives and the word
# order of doubles in memory and in floating-point register pairs.
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
w1:	.word 0x11223344
w2:	.word 0x8899aabb
bytes:	.byte 1, 2, 3, 4
halves:	.half 0x1234, 0x5678
chars:	.ascii "ABCD"
negs:	.byte -1
	.half -2
	.align 3
dbl:	.double 1.0
scratch:	.word 0, 0, 0, 0

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	.data
m000:	.asciiz "lb w1+0 = "
	.text
	la $t0, w1
	lb $a1, 0($t0)
	la $a0, m000
	jal show

	.data
m001:	.asciiz "lb w1+1 = "
	.text
	la $t0, w1
	lb $a1, 1($t0)
	la $a0, m001
	jal show

	.data
m002:	.asciiz "lb w1+2 = "
	.text
	la $t0, w1
	lb $a1, 2($t0)
	la $a0, m002
	jal show

	.data
m003:	.asciiz "lb w1+3 = "
	.text
	la $t0, w1
	lb $a1, 3($t0)
	la $a0, m003
	jal show

	.data
m004:	.asciiz "lbu w2+0 = "
	.text
	la $t0, w2
	lbu $a1, 0($t0)
	la $a0, m004
	jal show

	.data
m005:	.asciiz "lbu w2+1 = "
	.text
	la $t0, w2
	lbu $a1, 1($t0)
	la $a0, m005
	jal show

	.data
m006:	.asciiz "lbu w2+2 = "
	.text
	la $t0, w2
	lbu $a1, 2($t0)
	la $a0, m006
	jal show

	.data
m007:	.asciiz "lbu w2+3 = "
	.text
	la $t0, w2
	lbu $a1, 3($t0)
	la $a0, m007
	jal show

	.data
m008:	.asciiz "lh w1+0 = "
	.text
	la $t0, w1
	lh $a1, 0($t0)
	la $a0, m008
	jal show

	.data
m009:	.asciiz "lh w1+2 = "
	.text
	la $t0, w1
	lh $a1, 2($t0)
	la $a0, m009
	jal show

	.data
m010:	.asciiz "lhu w2+0 = "
	.text
	la $t0, w2
	lhu $a1, 0($t0)
	la $a0, m010
	jal show

	.data
m011:	.asciiz "lhu w2+2 = "
	.text
	la $t0, w2
	lhu $a1, 2($t0)
	la $a0, m011
	jal show

	.data
m012:	.asciiz "lw bytes 1,2,3,4 = "
	.text
	la $t0, bytes
	lw $a1, 0($t0)
	la $a0, m012
	jal show

	.data
m013:	.asciiz "lw halves 0x1234,0x5678 = "
	.text
	la $t0, halves
	lw $a1, 0($t0)
	la $a0, m013
	jal show

	.data
m014:	.asciiz "lw .ascii \"ABCD\" = "
	.text
	la $t0, chars
	lw $a1, 0($t0)
	la $a0, m014
	jal show

	.data
m015:	.asciiz "sb 0xaa at scratch+1 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaa
	sb $t1, 1($t0)
	lw $a1, 0($t0)
	la $a0, m015
	jal show

	.data
m016:	.asciiz "sh 0xbeef at scratch+2 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xbeef
	sh $t1, 2($t0)
	lw $a1, 0($t0)
	la $a0, m016
	jal show

	.data
m017:	.asciiz "lwl from w1+0 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwl $a1, 0($t0)
	la $a0, m017
	jal show

	.data
m018:	.asciiz "lwl from w1+1 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwl $a1, 1($t0)
	la $a0, m018
	jal show

	.data
m019:	.asciiz "lwl from w1+2 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwl $a1, 2($t0)
	la $a0, m019
	jal show

	.data
m020:	.asciiz "lwl from w1+3 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwl $a1, 3($t0)
	la $a0, m020
	jal show

	.data
m021:	.asciiz "lwr from w1+0 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwr $a1, 0($t0)
	la $a0, m021
	jal show

	.data
m022:	.asciiz "lwr from w1+1 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwr $a1, 1($t0)
	la $a0, m022
	jal show

	.data
m023:	.asciiz "lwr from w1+2 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwr $a1, 2($t0)
	la $a0, m023
	jal show

	.data
m024:	.asciiz "lwr from w1+3 into 0xaabbccdd = "
	.text
	la $t0, w1
	li $a1, 0xaabbccdd
	lwr $a1, 3($t0)
	la $a0, m024
	jal show

	.data
m025:	.asciiz "swl 0xaabbccdd to scratch+0 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swl $t1, 0($t0)
	lw $a1, 0($t0)
	la $a0, m025
	jal show

	.data
m026:	.asciiz "swl 0xaabbccdd to scratch+1 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swl $t1, 1($t0)
	lw $a1, 0($t0)
	la $a0, m026
	jal show

	.data
m027:	.asciiz "swl 0xaabbccdd to scratch+2 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swl $t1, 2($t0)
	lw $a1, 0($t0)
	la $a0, m027
	jal show

	.data
m028:	.asciiz "swl 0xaabbccdd to scratch+3 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swl $t1, 3($t0)
	lw $a1, 0($t0)
	la $a0, m028
	jal show

	.data
m029:	.asciiz "swr 0xaabbccdd to scratch+0 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swr $t1, 0($t0)
	lw $a1, 0($t0)
	la $a0, m029
	jal show

	.data
m030:	.asciiz "swr 0xaabbccdd to scratch+1 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swr $t1, 1($t0)
	lw $a1, 0($t0)
	la $a0, m030
	jal show

	.data
m031:	.asciiz "swr 0xaabbccdd to scratch+2 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swr $t1, 2($t0)
	lw $a1, 0($t0)
	la $a0, m031
	jal show

	.data
m032:	.asciiz "swr 0xaabbccdd to scratch+3 = "
	.text
	la $t0, scratch
	li $t1, 0x11223344
	sw $t1, 0($t0)
	li $t1, 0xaabbccdd
	swr $t1, 3($t0)
	lw $a1, 0($t0)
	la $a0, m032
	jal show

	.data
m033:	.asciiz "ulw from w1+1 = "
	.text
	la $t0, w1
	ulw $a1, 1($t0)
	la $a0, m033
	jal show

	.data
m034:	.asciiz "ulh from w1+1 = "
	.text
	la $t0, w1
	ulh $a1, 1($t0)
	la $a0, m034
	jal show

	.data
m035:	.asciiz "ulhu from w2+1 = "
	.text
	la $t0, w2
	ulhu $a1, 1($t0)
	la $a0, m035
	jal show

	.data
m036:	.asciiz "usw 0xcafef00d to scratch+1, low word = "
	.text
	la $t0, scratch
	sw $zero, 0($t0)
	sw $zero, 4($t0)
	li $t1, 0xcafef00d
	usw $t1, 1($t0)
	lw $a1, 0($t0)
	la $a0, m036
	jal show

	.data
m037:	.asciiz "usw 0xcafef00d to scratch+1, high word = "
	.text
	la $t0, scratch
	lw $a1, 4($t0)
	la $a0, m037
	jal show

	.data
m038:	.asciiz "ush 0xbeef to scratch+1 = "
	.text
	la $t0, scratch
	sw $zero, 0($t0)
	li $t1, 0xbeef
	ush $t1, 1($t0)
	lw $a1, 0($t0)
	la $a0, m038
	jal show

	.data
m039:	.asciiz "double 1.0 low word = "
	.text
	la $t0, dbl
	lw $a1, 0($t0)
	la $a0, m039
	jal show

	.data
m040:	.asciiz "double 1.0 high word = "
	.text
	la $t0, dbl
	lw $a1, 4($t0)
	la $a0, m040
	jal show

	.data
m041:	.asciiz "ldc1/sdc1 copy: low word = "
	.text
	la $t0, dbl
	ldc1 $f0, 0($t0)
	la $t0, scratch
	sdc1 $f0, 0($t0)
	lw $a1, 0($t0)
	la $a0, m041
	jal show

	.data
m042:	.asciiz "ldc1/sdc1 copy: high word = "
	.text
	la $t0, scratch
	lw $a1, 4($t0)
	la $a0, m042
	jal show

	.data
m043:	.asciiz "mfc1 $f1 of ldc1 1.0 = "
	.text
	la $t0, dbl
	ldc1 $f0, 0($t0)
	mfc1 $a1, $f1
	la $a0, m043
	jal show

	.data
m044:	.asciiz "lb .byte -1 = "
	.text
	la $t0, negs
	lb $a1, 0($t0)
	la $a0, m044
	jal show

	.data
m045:	.asciiz "lbu .byte -1 = "
	.text
	la $t0, negs
	lbu $a1, 0($t0)
	la $a0, m045
	jal show

	.data
m046:	.asciiz "lh .half -2 = "
	.text
	la $t0, negs
	lh $a1, 2($t0)
	la $a0, m046
	jal show

	.data
m047:	.asciiz "lhu .half -2 = "
	.text
	la $t0, negs
	lhu $a1, 2($t0)
	la $a0, m047
	jal show

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
