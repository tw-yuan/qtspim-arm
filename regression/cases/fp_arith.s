# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): floating-point arithmetic, conversions,
# comparisons, conditional moves, loads and stores.  Values are printed
# with print_float / print_double and as raw bits.  Only operations whose
# result does not depend on the host FPU are tested here; host-dependent
# cases (NaN encodings, out-of-range conversions) are in
# fp_host_dependent.s.
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
fs_one:	.float 1.0
fs_mone:	.float -1.0
fs_zero:	.float 0.0
fs_1p5:	.float 1.5
fs_2p25:	.float 2.25
fs_0p1:	.float 0.1
fs_0p3:	.float 0.3
fs_3:	.float 3.0
fs_2:	.float 2.0
fs_1e20:	.float 1.0e20
fs_2p9:	.float 2.9
fs_m2p9:	.float -2.9
fs_2p5:	.float 2.5
fs_m2p5:	.float -2.5
fs_1p5b:	.float 1.5
fs_m1p7:	.float -1.7
fs_max:	.float 3.4028235e38
fs_tiny:	.float 1.0e-10
fs_16m:	.float 16777216.0
fs_pi:	.float 3.14159265
	.align 3
fd_one:	.double 1.0
fd_zero:	.double 0.0
fd_0p1:	.double 0.1
fd_0p2:	.double 0.2
fd_3:	.double 3.0
fd_2:	.double 2.0
fd_1e300:	.double 1.0e300
fd_1em300:	.double 1.0e-300
fd_m1e9:	.double -1.0e9
fd_2p7:	.double 2.7
fd_2p1:	.double 2.1
fd_m2p5:	.double -2.5
fd_almost_half:	.double 0.49999999999999994
fd_pi:	.double 3.141592653589793
fd_mzero:	.double -0.0
fd_store:	.double 0.0
fs_store:	.float 0.0

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	.data
m000:	.asciiz "add.s 1.5 + 2.25 = "
	.text
	l.s $f0, fs_1p5
	l.s $f2, fs_2p25
	add.s $f12, $f0, $f2
	la $a0, m000
	jal show_s

	.data
m001:	.asciiz "sub.s 0.1 - 0.3 = "
	.text
	l.s $f0, fs_0p1
	l.s $f2, fs_0p3
	sub.s $f12, $f0, $f2
	la $a0, m001
	jal show_s

	.data
m002:	.asciiz "mul.s 1.5 * 2.25 = "
	.text
	l.s $f0, fs_1p5
	l.s $f2, fs_2p25
	mul.s $f12, $f0, $f2
	la $a0, m002
	jal show_s

	.data
m003:	.asciiz "mul.s 1e20 * 1e20 = "
	.text
	l.s $f0, fs_1e20
	l.s $f2, fs_1e20
	mul.s $f12, $f0, $f2
	la $a0, m003
	jal show_s

	.data
m004:	.asciiz "div.s 1 / 3 = "
	.text
	l.s $f0, fs_one
	l.s $f2, fs_3
	div.s $f12, $f0, $f2
	la $a0, m004
	jal show_s

	.data
m005:	.asciiz "div.s 1 / 0 = "
	.text
	l.s $f0, fs_one
	l.s $f2, fs_zero
	div.s $f12, $f0, $f2
	la $a0, m005
	jal show_s

	.data
m006:	.asciiz "div.s -1 / 0 = "
	.text
	l.s $f0, fs_mone
	l.s $f2, fs_zero
	div.s $f12, $f0, $f2
	la $a0, m006
	jal show_s

	.data
m007:	.asciiz "sqrt.s 2 = "
	.text
	l.s $f0, fs_2
	sqrt.s $f12, $f0
	la $a0, m007
	jal show_s

	.data
m008:	.asciiz "abs.s -2.5 = "
	.text
	l.s $f0, fs_m2p5
	abs.s $f12, $f0
	la $a0, m008
	jal show_s

	.data
m009:	.asciiz "neg.s 0 = "
	.text
	l.s $f0, fs_zero
	neg.s $f12, $f0
	la $a0, m009
	jal show_s

	.data
m010:	.asciiz "neg.s 1.5 = "
	.text
	l.s $f0, fs_1p5
	neg.s $f12, $f0
	la $a0, m010
	jal show_s

	.data
m011:	.asciiz "mov.s pi = "
	.text
	l.s $f0, fs_pi
	mov.s $f12, $f0
	la $a0, m011
	jal show_s

	.data
m012:	.asciiz "add.s 16777216 + 1 = "
	.text
	l.s $f0, fs_16m
	l.s $f2, fs_one
	add.s $f12, $f0, $f2
	la $a0, m012
	jal show_s

	.data
m013:	.asciiz "float max = "
	.text
	l.s $f12, fs_max
	la $a0, m013
	jal show_s

	.data
m014:	.asciiz "float 1e-10 = "
	.text
	l.s $f12, fs_tiny
	la $a0, m014
	jal show_s

	.data
m015:	.asciiz "li.s 0.1 = "
	.text
	li.s $f12, 0.1
	la $a0, m015
	jal show_s

	.data
m016:	.asciiz "mtc1 0x00000001 (denormal) = "
	.text
	li $t0, 1
	mtc1 $t0, $f12
	la $a0, m016
	jal show_s

	.data
m017:	.asciiz "mtc1 0x80000000 (-0) = "
	.text
	li $t0, 0x80000000
	mtc1 $t0, $f12
	la $a0, m017
	jal show_s

	.data
m018:	.asciiz "add.d 0.1 + 0.2 = "
	.text
	l.d $f0, fd_0p1
	l.d $f2, fd_0p2
	add.d $f12, $f0, $f2
	la $a0, m018
	jal show_d

	.data
m019:	.asciiz "sub.d 0.1 - 0.2 = "
	.text
	l.d $f0, fd_0p1
	l.d $f2, fd_0p2
	sub.d $f12, $f0, $f2
	la $a0, m019
	jal show_d

	.data
m020:	.asciiz "mul.d 0.1 * 3 = "
	.text
	l.d $f0, fd_0p1
	l.d $f2, fd_3
	mul.d $f12, $f0, $f2
	la $a0, m020
	jal show_d

	.data
m021:	.asciiz "mul.d 1e300 * 1e300 = "
	.text
	l.d $f0, fd_1e300
	l.d $f2, fd_1e300
	mul.d $f12, $f0, $f2
	la $a0, m021
	jal show_d

	.data
m022:	.asciiz "mul.d 1e-300 * 1e-300 = "
	.text
	l.d $f0, fd_1em300
	l.d $f2, fd_1em300
	mul.d $f12, $f0, $f2
	la $a0, m022
	jal show_d

	.data
m023:	.asciiz "div.d 1 / 3 = "
	.text
	l.d $f0, fd_one
	l.d $f2, fd_3
	div.d $f12, $f0, $f2
	la $a0, m023
	jal show_d

	.data
m024:	.asciiz "div.d 1 / 0 = "
	.text
	l.d $f0, fd_one
	l.d $f2, fd_zero
	div.d $f12, $f0, $f2
	la $a0, m024
	jal show_d

	.data
m025:	.asciiz "sqrt.d 2 = "
	.text
	l.d $f0, fd_2
	sqrt.d $f12, $f0
	la $a0, m025
	jal show_d

	.data
m026:	.asciiz "abs.d -2.5 = "
	.text
	l.d $f0, fd_m2p5
	abs.d $f12, $f0
	la $a0, m026
	jal show_d

	.data
m027:	.asciiz "neg.d 0 = "
	.text
	l.d $f0, fd_zero
	neg.d $f12, $f0
	la $a0, m027
	jal show_d

	.data
m028:	.asciiz "mov.d pi = "
	.text
	l.d $f0, fd_pi
	mov.d $f12, $f0
	la $a0, m028
	jal show_d

	.data
m029:	.asciiz "double -0 = "
	.text
	l.d $f12, fd_mzero
	la $a0, m029
	jal show_d

	.data
m030:	.asciiz "double 1e-300 = "
	.text
	l.d $f12, fd_1em300
	la $a0, m030
	jal show_d

	.data
m031:	.asciiz "li.d 2.5 = "
	.text
	li.d $f12, 2.5
	la $a0, m031
	jal show_d

	.data
m032:	.asciiz "cvt.d.s 0.1f = "
	.text
	l.s $f0, fs_0p1
	cvt.d.s $f12, $f0
	la $a0, m032
	jal show_d

	.data
m033:	.asciiz "cvt.s.d 0.1 = "
	.text
	l.d $f0, fd_0p1
	cvt.s.d $f12, $f0
	la $a0, m033
	jal show_s

	.data
m034:	.asciiz "cvt.s.d 1e300 = "
	.text
	l.d $f0, fd_1e300
	cvt.s.d $f12, $f0
	la $a0, m034
	jal show_s

	.data
m035:	.asciiz "cvt.s.d 1e-300 = "
	.text
	l.d $f0, fd_1em300
	cvt.s.d $f12, $f0
	la $a0, m035
	jal show_s

	.data
m036:	.asciiz "cvt.s.w -7 = "
	.text
	li $t0, -7
	mtc1 $t0, $f0
	cvt.s.w $f12, $f0
	la $a0, m036
	jal show_s

	.data
m037:	.asciiz "cvt.s.w 16777217 = "
	.text
	li $t0, 16777217
	mtc1 $t0, $f0
	cvt.s.w $f12, $f0
	la $a0, m037
	jal show_s

	.data
m038:	.asciiz "cvt.d.w 0x7fffffff = "
	.text
	li $t0, 0x7fffffff
	mtc1 $t0, $f0
	cvt.d.w $f12, $f0
	la $a0, m038
	jal show_d

	.data
m039:	.asciiz "cvt.d.w 0x80000000 = "
	.text
	li $t0, 0x80000000
	mtc1 $t0, $f0
	cvt.d.w $f12, $f0
	la $a0, m039
	jal show_d

	.data
m040:	.asciiz "cvt.w.s 2.9 = "
	.text
	l.s $f0, fs_2p9
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m040
	jal show

	.data
m041:	.asciiz "cvt.w.s -2.9 = "
	.text
	l.s $f0, fs_m2p9
	cvt.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m041
	jal show

	.data
m042:	.asciiz "cvt.w.d -1e9 = "
	.text
	l.d $f0, fd_m1e9
	cvt.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m042
	jal show

	.data
m043:	.asciiz "trunc.w.s -2.5 = "
	.text
	l.s $f0, fs_m2p5
	trunc.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m043
	jal show

	.data
m044:	.asciiz "trunc.w.d 2.7 = "
	.text
	l.d $f0, fd_2p7
	trunc.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m044
	jal show

	.data
m045:	.asciiz "round.w.s 2.5 = "
	.text
	l.s $f0, fs_2p5
	round.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m045
	jal show

	.data
m046:	.asciiz "round.w.s -2.5 = "
	.text
	l.s $f0, fs_m2p5
	round.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m046
	jal show

	.data
m047:	.asciiz "round.w.s 1.5 = "
	.text
	l.s $f0, fs_1p5b
	round.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m047
	jal show

	.data
m048:	.asciiz "round.w.s -1.7 = "
	.text
	l.s $f0, fs_m1p7
	round.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m048
	jal show

	.data
m049:	.asciiz "round.w.d 0.49999999999999994 = "
	.text
	l.d $f0, fd_almost_half
	round.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m049
	jal show

	.data
m050:	.asciiz "floor.w.s -2.5 = "
	.text
	l.s $f0, fs_m2p5
	floor.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m050
	jal show

	.data
m051:	.asciiz "floor.w.d 2.7 = "
	.text
	l.d $f0, fd_2p7
	floor.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m051
	jal show

	.data
m052:	.asciiz "ceil.w.s -2.5 = "
	.text
	l.s $f0, fs_m2p5
	ceil.w.s $f4, $f0
	mfc1 $a1, $f4
	la $a0, m052
	jal show

	.data
m053:	.asciiz "ceil.w.d 2.1 = "
	.text
	l.d $f0, fd_2p1
	ceil.w.d $f4, $f0
	mfc1 $a1, $f4
	la $a0, m053
	jal show

	.data
m054:	.asciiz "c.eq.s 1 1 bc1t = "
	.text
	l.s $f0, fs_one
	l.s $f2, fs_one
	c.eq.s $f0, $f2
	li $a1, 1
	bc1t t054_taken
	li $a1, 0
t054_taken:
	la $a0, m054
	jal show

	.data
m055:	.asciiz "c.eq.s 1 2 bc1t = "
	.text
	l.s $f0, fs_one
	l.s $f2, fs_2
	c.eq.s $f0, $f2
	li $a1, 1
	bc1t t055_taken
	li $a1, 0
t055_taken:
	la $a0, m055
	jal show

	.data
m056:	.asciiz "c.lt.s 2 1 bc1f = "
	.text
	l.s $f0, fs_2
	l.s $f2, fs_one
	c.lt.s $f0, $f2
	li $a1, 1
	bc1f t056_taken
	li $a1, 0
t056_taken:
	la $a0, m056
	jal show

	.data
m057:	.asciiz "c.le.s -0 0 bc1t = "
	.text
	li $t0, 0x80000000
	mtc1 $t0, $f0
	mtc1 $zero, $f2
	c.le.s $f0, $f2
	li $a1, 1
	bc1t t057_taken
	li $a1, 0
t057_taken:
	la $a0, m057
	jal show

	.data
m058:	.asciiz "c.eq.d 0.1+0.2 0.3 bc1t = "
	.text
	l.d $f0, fd_0p1
	l.d $f2, fd_0p2
	add.d $f4, $f0, $f2
	li.d $f6, 0.3
	c.eq.d $f4, $f6
	li $a1, 1
	bc1t t058_taken
	li $a1, 0
t058_taken:
	la $a0, m058
	jal show

	.data
m059:	.asciiz "c.lt.d 0.1 0.2 bc1t = "
	.text
	l.d $f0, fd_0p1
	l.d $f2, fd_0p2
	c.lt.d $f0, $f2
	li $a1, 1
	bc1t t059_taken
	li $a1, 0
t059_taken:
	la $a0, m059
	jal show

	.data
m060:	.asciiz "c.le.d 0.2 0.1 bc1t = "
	.text
	l.d $f0, fd_0p2
	l.d $f2, fd_0p1
	c.le.d $f0, $f2
	li $a1, 1
	bc1t t060_taken
	li $a1, 0
t060_taken:
	la $a0, m060
	jal show

	.data
m061:	.asciiz "c.lt.s cc 3 bc1t 3 = "
	.text
	l.s $f0, fs_one
	l.s $f2, fs_2
	c.lt.s 3, $f0, $f2
	li $a1, 1
	bc1t 3, t061_taken
	li $a1, 0
t061_taken:
	la $a0, m061
	jal show

	.data
m062:	.asciiz "c.lt.s cc 3 then bc1t cc 0 = "
	.text
	c.eq.s $f0, $f0
	c.lt.s 3, $f2, $f0
	li $a1, 1
	bc1t t062_taken
	li $a1, 0
t062_taken:
	la $a0, m062
	jal show

	.data
m063:	.asciiz "c.un.s NaN 1 bc1t = "
	.text
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	l.s $f2, fs_one
	c.un.s $f0, $f2
	li $a1, 1
	bc1t t063_taken
	li $a1, 0
t063_taken:
	la $a0, m063
	jal show

	.data
m064:	.asciiz "c.eq.s NaN NaN bc1t = "
	.text
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	c.eq.s $f0, $f0
	li $a1, 1
	bc1t t064_taken
	li $a1, 0
t064_taken:
	la $a0, m064
	jal show

	.data
m065:	.asciiz "c.ueq.s NaN 1 bc1t = "
	.text
	li $t0, 0x7fc00000
	mtc1 $t0, $f0
	l.s $f2, fs_one
	c.ueq.s $f0, $f2
	li $a1, 1
	bc1t t065_taken
	li $a1, 0
t065_taken:
	la $a0, m065
	jal show

	.data
m066:	.asciiz "c.olt.d NaN 1 bc1t = "
	.text
	li $t0, 0x7ff80000
	mtc1 $t0, $f1
	mtc1 $zero, $f0
	l.d $f2, fd_one
	c.olt.d $f0, $f2
	li $a1, 1
	bc1t t066_taken
	li $a1, 0
t066_taken:
	la $a0, m066
	jal show

	.data
m067:	.asciiz "c.f.s 1 1 bc1t = "
	.text
	l.s $f0, fs_one
	l.s $f2, fs_one
	c.f.s $f0, $f2
	li $a1, 1
	bc1t t067_taken
	li $a1, 0
t067_taken:
	la $a0, m067
	jal show

	.data
m068:	.asciiz "movt taken = "
	.text
	c.eq.s $f0, $f0
	li $a1, 1
	li $t0, 42
	li $t1, 0x3f800000
	mtc1 $t1, $f0
	c.eq.s $f0, $f0
	movt $a1, $t0, 0
	la $a0, m068
	jal show

	.data
m069:	.asciiz "movf not taken = "
	.text
	li $t1, 0x3f800000
	mtc1 $t1, $f0
	c.eq.s $f0, $f0
	li $a1, 1
	li $t0, 42
	movf $a1, $t0, 0
	la $a0, m069
	jal show

	.data
m070:	.asciiz "movt.s taken = "
	.text
	l.s $f12, fs_one
	l.s $f0, fs_pi
	c.eq.s $f0, $f0
	movt.s $f12, $f0, 0
	la $a0, m070
	jal show_s

	.data
m071:	.asciiz "movf.d not taken = "
	.text
	l.d $f12, fd_one
	l.d $f0, fd_pi
	c.eq.d $f0, $f0
	movf.d $f12, $f0, 0
	la $a0, m071
	jal show_d

	.data
m072:	.asciiz "movn.s taken = "
	.text
	l.s $f12, fs_one
	l.s $f0, fs_pi
	li $t0, 1
	movn.s $f12, $f0, $t0
	la $a0, m072
	jal show_s

	.data
m073:	.asciiz "movz.d taken = "
	.text
	l.d $f12, fd_one
	l.d $f0, fd_pi
	movz.d $f12, $f0, $zero
	la $a0, m073
	jal show_d

	.data
m074:	.asciiz "s.s/l.s round trip = "
	.text
	l.s $f0, fs_pi
	s.s $f0, fs_store
	l.s $f12, fs_store
	la $a0, m074
	jal show_s

	.data
m075:	.asciiz "s.d/l.d round trip = "
	.text
	l.d $f0, fd_pi
	s.d $f0, fd_store
	l.d $f12, fd_store
	la $a0, m075
	jal show_d

	.data
m076:	.asciiz "swc1/lwc1 round trip = "
	.text
	la $t0, fs_store
	l.s $f0, fs_2p5
	swc1 $f0, 0($t0)
	lwc1 $f12, 0($t0)
	la $a0, m076
	jal show_s

	.data
m077:	.asciiz "sdc1/ldc1 round trip = "
	.text
	la $t0, fd_store
	l.d $f0, fd_m2p5
	sdc1 $f0, 0($t0)
	ldc1 $f12, 0($t0)
	la $a0, m077
	jal show_d

	.data
m078:	.asciiz "fd_store low word = "
	.text
	lw $a1, fd_store
	la $a0, m078
	jal show

	.data
m079:	.asciiz "fd_store high word = "
	.text
	la $t0, fd_store
	lw $a1, 4($t0)
	la $a0, m079
	jal show

	.data
m080:	.asciiz "mfc1 1.0 = "
	.text
	l.s $f0, fs_one
	mfc1 $a1, $f0
	la $a0, m080
	jal show

	.data
m081:	.asciiz "cfc1 $31 (FCSR) = "
	.text
	cfc1 $a1, $31
	la $a0, m081
	jal show

	.data
m082:	.asciiz "cfc1 $0 (FIR) = "
	.text
	cfc1 $a1, $0
	la $a0, m082
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
