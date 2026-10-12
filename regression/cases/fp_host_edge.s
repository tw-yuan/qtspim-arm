# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): more host-dependent floating-point
# cases (see fp_host_dependent.s): other invalid operations, NaN
# propagation, and float-to-integer conversions at the exact limits of
# the int32 range.
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
e_zero:	.float 0.0
e_mzero:	.float -0.0
e_one:	.float 1.0
e_f2p31m:	.float 2147483520.0
e_f2p31:	.float 2147483648.0
e_fm2p31:	.float -2147483648.0
e_fmhalf:	.float -0.5
	.align 3
ed_zero:	.double 0.0
ed_mzero:	.double -0.0
ed_a:	.double 2147483647.9
ed_b:	.double 2147483648.0
ed_c:	.double -2147483648.0
ed_d:	.double -2147483648.9
ed_e:	.double -2147483649.0
ed_f:	.double -2147483648.5
ed_g:	.double 2147483646.5
ed_h:	.double 2147483647.4
ed_i:	.double -2147483648.6
ed_j:	.double 2147483647.5
ed_k:	.double 1.0e300

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	.data
m000:	.asciiz "add.s inf + -inf = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	li $t0, 0xff800000
	mtc1 $t0, $f2
	add.s $f12, $f0, $f2
	la $a0, m000
	jal show_bits_s

	.data
m001:	.asciiz "add.d inf + -inf = "
	.text
	li $t0, 0x7ff00000
	mtc1 $t0, $f1
	li $t0, 0
	mtc1 $t0, $f0
	li $t0, 0xfff00000
	mtc1 $t0, $f3
	li $t0, 0
	mtc1 $t0, $f2
	add.d $f12, $f0, $f2
	la $a0, m001
	jal show_bits_d

	.data
m002:	.asciiz "sub.d inf - inf = "
	.text
	li $t0, 0x7ff00000
	mtc1 $t0, $f1
	li $t0, 0
	mtc1 $t0, $f0
	sub.d $f12, $f0, $f0
	la $a0, m002
	jal show_bits_d

	.data
m003:	.asciiz "mul.d 0 * inf = "
	.text
	l.d $f0, ed_zero
	li $t0, 0x7ff00000
	mtc1 $t0, $f3
	li $t0, 0
	mtc1 $t0, $f2
	mul.d $f12, $f0, $f2
	la $a0, m003
	jal show_bits_d

	.data
m004:	.asciiz "mul.s inf * -0 = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	l.s $f2, e_mzero
	mul.s $f12, $f0, $f2
	la $a0, m004
	jal show_bits_s

	.data
m005:	.asciiz "div.s inf / inf = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	div.s $f12, $f0, $f0
	la $a0, m005
	jal show_bits_s

	.data
m006:	.asciiz "div.d inf / inf = "
	.text
	li $t0, 0x7ff00000
	mtc1 $t0, $f1
	li $t0, 0
	mtc1 $t0, $f0
	div.d $f12, $f0, $f0
	la $a0, m006
	jal show_bits_d

	.data
m007:	.asciiz "div.d -0 / 0 = "
	.text
	l.d $f0, ed_mzero
	l.d $f2, ed_zero
	div.d $f12, $f0, $f2
	la $a0, m007
	jal show_bits_d

	.data
m008:	.asciiz "sqrt.s -0 = "
	.text
	l.s $f0, e_mzero
	sqrt.s $f12, $f0
	la $a0, m008
	jal show_bits_s

	.data
m009:	.asciiz "sqrt.d -inf = "
	.text
	li $t0, 0xfff00000
	mtc1 $t0, $f1
	li $t0, 0
	mtc1 $t0, $f0
	sqrt.d $f12, $f0
	la $a0, m009
	jal show_bits_d

	.data
m010:	.asciiz "sqrt.s qNaN(0x7fc00123) = "
	.text
	li $t0, 0x7fc00123
	mtc1 $t0, $f0
	sqrt.s $f12, $f0
	la $a0, m010
	jal show_bits_s

	.data
m011:	.asciiz "div.s 1 / qNaN(0xffc00055) = "
	.text
	l.s $f0, e_one
	li $t0, 0xffc00055
	mtc1 $t0, $f2
	div.s $f12, $f0, $f2
	la $a0, m011
	jal show_bits_s

	.data
m012:	.asciiz "mul.d qNaN(0x7ff80000 00000007) squared = "
	.text
	li $t0, 0x7ff80000
	mtc1 $t0, $f1
	li $t0, 7
	mtc1 $t0, $f0
	mul.d $f12, $f0, $f0
	la $a0, m012
	jal show_bits_d

	.data
m013:	.asciiz "cvt.w.d 2147483647.9 = "
	.text
	l.d $f0, ed_a
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m013
	jal show

	.data
m014:	.asciiz "cvt.w.d 2147483648.0 = "
	.text
	l.d $f0, ed_b
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m014
	jal show

	.data
m015:	.asciiz "cvt.w.d -2147483648.0 = "
	.text
	l.d $f0, ed_c
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m015
	jal show

	.data
m016:	.asciiz "cvt.w.d -2147483648.9 = "
	.text
	l.d $f0, ed_d
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m016
	jal show

	.data
m017:	.asciiz "cvt.w.d -2147483649.0 = "
	.text
	l.d $f0, ed_e
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m017
	jal show

	.data
m018:	.asciiz "cvt.w.d 1e300 = "
	.text
	l.d $f0, ed_k
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m018
	jal show

	.data
m019:	.asciiz "cvt.w.d -0 = "
	.text
	l.d $f0, ed_mzero
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m019
	jal show

	.data
m020:	.asciiz "cvt.w.s denormal = "
	.text
	li $t0, 0x00000001
	mtc1 $t0, $f0
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m020
	jal show

	.data
m021:	.asciiz "cvt.w.s 2147483520.0 = "
	.text
	l.s $f0, e_f2p31m
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m021
	jal show

	.data
m022:	.asciiz "cvt.w.s 2147483648.0 = "
	.text
	l.s $f0, e_f2p31
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m022
	jal show

	.data
m023:	.asciiz "cvt.w.s -2147483648.0 = "
	.text
	l.s $f0, e_fm2p31
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m023
	jal show

	.data
m024:	.asciiz "trunc.w.d -2147483648.5 = "
	.text
	l.d $f0, ed_f
	trunc.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m024
	jal show

	.data
m025:	.asciiz "round.w.d 2147483646.5 = "
	.text
	l.d $f0, ed_g
	round.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m025
	jal show

	.data
m026:	.asciiz "round.w.d 2147483647.4 = "
	.text
	l.d $f0, ed_h
	round.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m026
	jal show

	.data
m027:	.asciiz "round.w.d -2147483648.6 = "
	.text
	l.d $f0, ed_i
	round.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m027
	jal show

	.data
m028:	.asciiz "floor.w.d -2147483648.5 = "
	.text
	l.d $f0, ed_f
	floor.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m028
	jal show

	.data
m029:	.asciiz "ceil.w.d 2147483647.5 = "
	.text
	l.d $f0, ed_j
	ceil.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m029
	jal show

	.data
m030:	.asciiz "ceil.w.s -0.5 = "
	.text
	l.s $f0, e_fmhalf
	ceil.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m030
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

	.data
fp_sep:	.asciiz "  bits "
fp_nl:	.asciiz "\n"

	.text
# show_s: print the string at $a0, then $f12 with print_float, then the raw
# bits of $f12.  Clobbers $t8, $t9, $v0, $a0, $a1.
show_s:
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	li $v0, 4
	syscall
	li $v0, 2
	syscall
	la $a0, fp_sep
	li $v0, 4
	syscall
	mfc1 $a0, $f12
	jal print_hex
	la $a0, fp_nl
	li $v0, 4
	syscall
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra

# show_d: print the string at $a0, then $f12/$f13 with print_double, then the
# raw bits (high word, low word).  Clobbers $t8, $t9, $v0, $a0, $a1.
show_d:
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	li $v0, 4
	syscall
	li $v0, 3
	syscall
	la $a0, fp_sep
	li $v0, 4
	syscall
	mfc1 $a0, $f13
	jal print_hex
	li $a0, ' '
	li $v0, 11
	syscall
	mfc1 $a0, $f12
	jal print_hex
	la $a0, fp_nl
	li $v0, 4
	syscall
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra

# show_bits_s: print the string at $a0, then only the raw bits of $f12.
show_bits_s:
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	li $v0, 4
	syscall
	mfc1 $a0, $f12
	jal print_hex
	la $a0, fp_nl
	li $v0, 4
	syscall
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra

# show_bits_d: print the string at $a0, then only the raw bits of $f12/$f13.
show_bits_d:
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	li $v0, 4
	syscall
	mfc1 $a0, $f13
	jal print_hex
	li $a0, ' '
	li $v0, 11
	syscall
	mfc1 $a0, $f12
	jal print_hex
	la $a0, fp_nl
	li $v0, 4
	syscall
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra
