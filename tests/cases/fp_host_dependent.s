# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): floating-point results that SPIM takes
# directly from the host FPU and C runtime, so they may differ between
# x86_64 and arm64 hosts: NaN bit patterns produced by invalid operations
# and float-to-integer conversions of out-of-range values, infinities and
# NaNs.  NaN values are only shown as raw bits, never printed with
# print_float / print_double, because the C library formats NaN signs
# differently on different platforms.
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
h_zero:	.float 0.0
h_one:	.float 1.0
h_mone:	.float -1.0
h_3e9:	.float 3.0e9
h_m3e9:	.float -3.0e9
h_2p2e9:	.float 2.2e9
	.align 3
hd_zero:	.double 0.0
hd_one:	.double 1.0
hd_mone:	.double -1.0
hd_1e10:	.double 1.0e10
hd_m1e10:	.double -1.0e10
hd_big:	.double 2147483647.5

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	.data
m000:	.asciiz "div.s 0 / 0 = "
	.text
	l.s $f0, h_zero
	div.s $f12, $f0, $f0
	la $a0, m000
	jal show_bits_s

	.data
m001:	.asciiz "sub.s inf - inf = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	sub.s $f12, $f0, $f0
	la $a0, m001
	jal show_bits_s

	.data
m002:	.asciiz "mul.s 0 * inf = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	l.s $f2, h_zero
	mul.s $f12, $f0, $f2
	la $a0, m002
	jal show_bits_s

	.data
m003:	.asciiz "sqrt.s -1 = "
	.text
	l.s $f0, h_mone
	sqrt.s $f12, $f0
	la $a0, m003
	jal show_bits_s

	.data
m004:	.asciiz "div.d 0 / 0 = "
	.text
	l.d $f0, hd_zero
	div.d $f12, $f0, $f0
	la $a0, m004
	jal show_bits_d

	.data
m005:	.asciiz "sqrt.d -1 = "
	.text
	l.d $f0, hd_mone
	sqrt.d $f12, $f0
	la $a0, m005
	jal show_bits_d

	.data
m006:	.asciiz "cvt.s.d of 0/0 = "
	.text
	l.d $f0, hd_zero
	div.d $f2, $f0, $f0
	cvt.s.d $f12, $f2
	la $a0, m006
	jal show_bits_s

	.data
m007:	.asciiz "add.s qNaN(0x7fc00001) + 1 = "
	.text
	li $t0, 0x7fc00001
	mtc1 $t0, $f0
	l.s $f2, h_one
	add.s $f12, $f0, $f2
	la $a0, m007
	jal show_bits_s

	.data
m008:	.asciiz "add.s sNaN(0x7f800001) + 1 = "
	.text
	li $t0, 0x7f800001
	mtc1 $t0, $f0
	l.s $f2, h_one
	add.s $f12, $f0, $f2
	la $a0, m008
	jal show_bits_s

	.data
m009:	.asciiz "neg.s qNaN = "
	.text
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	neg.s $f12, $f0
	la $a0, m009
	jal show_bits_s

	.data
m010:	.asciiz "abs.s -qNaN = "
	.text
	li $t0, 0xffc00000
	mtc1 $t0, $f0
	abs.s $f12, $f0
	la $a0, m010
	jal show_bits_s

	.data
m011:	.asciiz "mov.s sNaN = "
	.text
	li $t0, 0x7f800001
	mtc1 $t0, $f0
	mov.s $f12, $f0
	la $a0, m011
	jal show_bits_s

	.data
m012:	.asciiz "cvt.d.s qNaN(0x7fc00001) = "
	.text
	li $t0, 0x7fc00001
	mtc1 $t0, $f0
	cvt.d.s $f12, $f0
	la $a0, m012
	jal show_bits_d

	.data
m013:	.asciiz "cvt.w.s 3e9 = "
	.text
	l.s $f0, h_3e9
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m013
	jal show

	.data
m014:	.asciiz "cvt.w.s -3e9 = "
	.text
	l.s $f0, h_m3e9
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m014
	jal show

	.data
m015:	.asciiz "cvt.w.s 2.2e9 = "
	.text
	l.s $f0, h_2p2e9
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m015
	jal show

	.data
m016:	.asciiz "cvt.w.s inf = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m016
	jal show

	.data
m017:	.asciiz "cvt.w.s -inf = "
	.text
	li $t0, 0xff800000
	mtc1 $t0, $f0
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m017
	jal show

	.data
m018:	.asciiz "cvt.w.s NaN = "
	.text
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m018
	jal show

	.data
m019:	.asciiz "cvt.w.d 1e10 = "
	.text
	l.d $f0, hd_1e10
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m019
	jal show

	.data
m020:	.asciiz "cvt.w.d -1e10 = "
	.text
	l.d $f0, hd_m1e10
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m020
	jal show

	.data
m021:	.asciiz "cvt.w.d NaN = "
	.text
	li $t0, 0x7ff80000
	mtc1 $t0, $f1
	mtc1 $zero, $f0
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m021
	jal show

	.data
m022:	.asciiz "trunc.w.s 3e9 = "
	.text
	l.s $f0, h_3e9
	trunc.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m022
	jal show

	.data
m023:	.asciiz "trunc.w.d 1e10 = "
	.text
	l.d $f0, hd_1e10
	trunc.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m023
	jal show

	.data
m024:	.asciiz "trunc.w.d NaN = "
	.text
	li $t0, 0x7ff80000
	mtc1 $t0, $f1
	mtc1 $zero, $f0
	trunc.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m024
	jal show

	.data
m025:	.asciiz "round.w.s 3e9 = "
	.text
	l.s $f0, h_3e9
	round.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m025
	jal show

	.data
m026:	.asciiz "round.w.d 2147483647.5 = "
	.text
	l.d $f0, hd_big
	round.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m026
	jal show

	.data
m027:	.asciiz "round.w.s NaN = "
	.text
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	round.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m027
	jal show

	.data
m028:	.asciiz "floor.w.d 1e10 = "
	.text
	l.d $f0, hd_1e10
	floor.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m028
	jal show

	.data
m029:	.asciiz "floor.w.s -inf = "
	.text
	li $t0, 0xff800000
	mtc1 $t0, $f0
	floor.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m029
	jal show

	.data
m030:	.asciiz "ceil.w.d -1e10 = "
	.text
	l.d $f0, hd_m1e10
	ceil.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m030
	jal show

	.data
m031:	.asciiz "ceil.w.s inf = "
	.text
	li $t0, 0x7f800000
	mtc1 $t0, $f0
	ceil.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m031
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
