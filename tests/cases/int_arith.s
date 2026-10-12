# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): integer arithmetic, logical, shift,
# compare, bit-manipulation, branch and link instructions, including
# pseudo-instructions and edge cases (overflowing values, division by
# zero, 0x80000000 / -1, shift amounts >= 32).
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

	.data
m000:	.asciiz "addu 0x7fffffff + 1 = "
	.text
	li $t0, 0x7fffffff
	li $t1, 1
	addu $a1, $t0, $t1
	la $a0, m000
	jal show

	.data
m001:	.asciiz "add 5 + -7 = "
	.text
	li $t0, 5
	li $t1, -7
	add $a1, $t0, $t1
	la $a0, m001
	jal show

	.data
m002:	.asciiz "addi $0 + -32768 = "
	.text
	addi $a1, $zero, -32768
	la $a0, m002
	jal show

	.data
m003:	.asciiz "addiu -1 + 1 = "
	.text
	li $t0, -1
	addiu $a1, $t0, 1
	la $a0, m003
	jal show

	.data
m004:	.asciiz "addiu 0x7fffffff + 1 = "
	.text
	li $t0, 0x7fffffff
	addiu $a1, $t0, 1
	la $a0, m004
	jal show

	.data
m005:	.asciiz "sub 100 - 58 = "
	.text
	li $t0, 100
	li $t1, 58
	sub $a1, $t0, $t1
	la $a0, m005
	jal show

	.data
m006:	.asciiz "subu 0 - 1 = "
	.text
	li $t1, 1
	subu $a1, $zero, $t1
	la $a0, m006
	jal show

	.data
m007:	.asciiz "subu 0x80000000 - 1 = "
	.text
	li $t0, 0x80000000
	li $t1, 1
	subu $a1, $t0, $t1
	la $a0, m007
	jal show

	.data
m008:	.asciiz "neg 5 = "
	.text
	li $t0, 5
	neg $a1, $t0
	la $a0, m008
	jal show

	.data
m009:	.asciiz "negu 0x80000000 = "
	.text
	li $t0, 0x80000000
	negu $a1, $t0
	la $a0, m009
	jal show

	.data
m010:	.asciiz "abs -9 = "
	.text
	li $t0, -9
	abs $a1, $t0
	la $a0, m010
	jal show

	.data
m011:	.asciiz "abs 9 = "
	.text
	li $t0, 9
	abs $a1, $t0
	la $a0, m011
	jal show

	.data
m012:	.asciiz "not 0x0f0f0f0f = "
	.text
	li $t0, 0x0f0f0f0f
	not $a1, $t0
	la $a0, m012
	jal show

	.data
m013:	.asciiz "move -123 = "
	.text
	li $t0, -123
	move $a1, $t0
	la $a0, m013
	jal show

	.data
m014:	.asciiz "mult 0x12345678 * 0x9abcdef0 hi = "
	.text
	li $t0, 0x12345678
	li $t1, 0x9abcdef0
	mult $t0, $t1
	mfhi $a1
	la $a0, m014
	jal show

	.data
m015:	.asciiz "mult 0x12345678 * 0x9abcdef0 lo = "
	.text
	mflo $a1
	la $a0, m015
	jal show

	.data
m016:	.asciiz "multu 0x12345678 * 0x9abcdef0 hi = "
	.text
	li $t0, 0x12345678
	li $t1, 0x9abcdef0
	multu $t0, $t1
	mfhi $a1
	la $a0, m016
	jal show

	.data
m017:	.asciiz "multu 0x12345678 * 0x9abcdef0 lo = "
	.text
	mflo $a1
	la $a0, m017
	jal show

	.data
m018:	.asciiz "mult -3 * 7 hi = "
	.text
	li $t0, -3
	li $t1, 7
	mult $t0, $t1
	mfhi $a1
	la $a0, m018
	jal show

	.data
m019:	.asciiz "mult -3 * 7 lo = "
	.text
	mflo $a1
	la $a0, m019
	jal show

	.data
m020:	.asciiz "multu 0xffffffff * 0xffffffff hi = "
	.text
	li $t0, -1
	multu $t0, $t0
	mfhi $a1
	la $a0, m020
	jal show

	.data
m021:	.asciiz "multu 0xffffffff * 0xffffffff lo = "
	.text
	mflo $a1
	la $a0, m021
	jal show

	.data
m022:	.asciiz "mul 100000 * 30000 = "
	.text
	li $t0, 100000
	li $t1, 30000
	mul $a1, $t0, $t1
	la $a0, m022
	jal show

	.data
m023:	.asciiz "mul -46341 * 46341 = "
	.text
	li $t0, -46341
	li $t1, 46341
	mul $a1, $t0, $t1
	la $a0, m023
	jal show

	.data
m024:	.asciiz "mul imm 12 * -5 = "
	.text
	li $t0, 12
	mul $a1, $t0, -5
	la $a0, m024
	jal show

	.data
m025:	.asciiz "madd hi = "
	.text
	li $t0, 1
	mthi $t0
	li $t0, 0xfffffff0
	mtlo $t0
	li $t0, 0x10
	li $t1, 2
	madd $t0, $t1
	mfhi $a1
	la $a0, m025
	jal show

	.data
m026:	.asciiz "madd lo = "
	.text
	mflo $a1
	la $a0, m026
	jal show

	.data
m027:	.asciiz "maddu hi = "
	.text
	mthi $zero
	li $t0, -1
	mtlo $t0
	li $t0, -1
	li $t1, 2
	maddu $t0, $t1
	mfhi $a1
	la $a0, m027
	jal show

	.data
m028:	.asciiz "maddu lo = "
	.text
	mflo $a1
	la $a0, m028
	jal show

	.data
m029:	.asciiz "msub hi = "
	.text
	mthi $zero
	li $t0, 5
	mtlo $t0
	li $t0, 3
	li $t1, 4
	msub $t0, $t1
	mfhi $a1
	la $a0, m029
	jal show

	.data
m030:	.asciiz "msub lo = "
	.text
	mflo $a1
	la $a0, m030
	jal show

	.data
m031:	.asciiz "msubu hi = "
	.text
	mthi $zero
	mtlo $zero
	li $t0, 1
	li $t1, 1
	msubu $t0, $t1
	mfhi $a1
	la $a0, m031
	jal show

	.data
m032:	.asciiz "msubu lo = "
	.text
	mflo $a1
	la $a0, m032
	jal show

	.data
m033:	.asciiz "div 7 / 2 lo = "
	.text
	li $t0, 7
	li $t1, 2
	div $t0, $t1
	mflo $a1
	la $a0, m033
	jal show

	.data
m034:	.asciiz "div 7 / 2 hi = "
	.text
	mfhi $a1
	la $a0, m034
	jal show

	.data
m035:	.asciiz "div -7 / 2 lo = "
	.text
	li $t0, -7
	li $t1, 2
	div $t0, $t1
	mflo $a1
	la $a0, m035
	jal show

	.data
m036:	.asciiz "div -7 / 2 hi = "
	.text
	mfhi $a1
	la $a0, m036
	jal show

	.data
m037:	.asciiz "div 7 / -2 lo = "
	.text
	li $t0, 7
	li $t1, -2
	div $t0, $t1
	mflo $a1
	la $a0, m037
	jal show

	.data
m038:	.asciiz "div 7 / -2 hi = "
	.text
	mfhi $a1
	la $a0, m038
	jal show

	.data
m039:	.asciiz "div -7 / -2 lo = "
	.text
	li $t0, -7
	li $t1, -2
	div $t0, $t1
	mflo $a1
	la $a0, m039
	jal show

	.data
m040:	.asciiz "div -7 / -2 hi = "
	.text
	mfhi $a1
	la $a0, m040
	jal show

	.data
m041:	.asciiz "div 0 / 5 lo = "
	.text
	li $t0, 0
	li $t1, 5
	div $t0, $t1
	mflo $a1
	la $a0, m041
	jal show

	.data
m042:	.asciiz "div 0 / 5 hi = "
	.text
	mfhi $a1
	la $a0, m042
	jal show

	.data
m043:	.asciiz "divu 0xffffffff / 2 lo = "
	.text
	li $t0, -1
	li $t1, 2
	divu $t0, $t1
	mflo $a1
	la $a0, m043
	jal show

	.data
m044:	.asciiz "divu 0xffffffff / 2 hi = "
	.text
	mfhi $a1
	la $a0, m044
	jal show

	.data
m045:	.asciiz "divu 7 / 0xfffffffe lo = "
	.text
	li $t0, 7
	li $t1, -2
	divu $t0, $t1
	mflo $a1
	la $a0, m045
	jal show

	.data
m046:	.asciiz "divu 7 / 0xfffffffe hi = "
	.text
	mfhi $a1
	la $a0, m046
	jal show

	.data
m047:	.asciiz "div by zero leaves lo = "
	.text
	li $t0, 111
	mtlo $t0
	li $t0, 222
	mthi $t0
	li $t0, 5
	div $t0, $zero
	mflo $a1
	la $a0, m047
	jal show

	.data
m048:	.asciiz "div by zero leaves hi = "
	.text
	mfhi $a1
	la $a0, m048
	jal show

	.data
m049:	.asciiz "divu by zero leaves lo = "
	.text
	li $t0, 333
	mtlo $t0
	li $t0, 444
	mthi $t0
	li $t0, 5
	divu $t0, $zero
	mflo $a1
	la $a0, m049
	jal show

	.data
m050:	.asciiz "divu by zero leaves hi = "
	.text
	mfhi $a1
	la $a0, m050
	jal show

	.data
m051:	.asciiz "div 0x80000000 / -1 leaves lo = "
	.text
	li $t0, 555
	mtlo $t0
	li $t0, 666
	mthi $t0
	li $t0, 0x80000000
	li $t1, -1
	div $t0, $t1
	mflo $a1
	la $a0, m051
	jal show

	.data
m052:	.asciiz "div 0x80000000 / -1 leaves hi = "
	.text
	mfhi $a1
	la $a0, m052
	jal show

	.data
m053:	.asciiz "div pseudo 3-op 100 / 7 = "
	.text
	li $t0, 100
	li $t1, 7
	div $a1, $t0, $t1
	la $a0, m053
	jal show

	.data
m054:	.asciiz "div pseudo imm -100 / 7 = "
	.text
	li $t0, -100
	div $a1, $t0, 7
	la $a0, m054
	jal show

	.data
m055:	.asciiz "divu pseudo 3-op 0xfffffff0 / 16 = "
	.text
	li $t0, 0xfffffff0
	li $t1, 16
	divu $a1, $t0, $t1
	la $a0, m055
	jal show

	.data
m056:	.asciiz "rem -7 % 3 = "
	.text
	li $t0, -7
	li $t1, 3
	rem $a1, $t0, $t1
	la $a0, m056
	jal show

	.data
m057:	.asciiz "rem 7 % -3 = "
	.text
	li $t0, 7
	li $t1, -3
	rem $a1, $t0, $t1
	la $a0, m057
	jal show

	.data
m058:	.asciiz "remu 0xffffffff % 10 = "
	.text
	li $t0, -1
	li $t1, 10
	remu $a1, $t0, $t1
	la $a0, m058
	jal show

	.data
m059:	.asciiz "and 0xff00ff00 & 0x0ff00ff0 = "
	.text
	li $t0, 0xff00ff00
	li $t1, 0x0ff00ff0
	and $a1, $t0, $t1
	la $a0, m059
	jal show

	.data
m060:	.asciiz "or 0xff00ff00 | 0x0ff00ff0 = "
	.text
	li $t0, 0xff00ff00
	li $t1, 0x0ff00ff0
	or $a1, $t0, $t1
	la $a0, m060
	jal show

	.data
m061:	.asciiz "xor 0xff00ff00 ^ 0x0ff00ff0 = "
	.text
	li $t0, 0xff00ff00
	li $t1, 0x0ff00ff0
	xor $a1, $t0, $t1
	la $a0, m061
	jal show

	.data
m062:	.asciiz "nor 0xff00ff00 0x0ff00ff0 = "
	.text
	li $t0, 0xff00ff00
	li $t1, 0x0ff00ff0
	nor $a1, $t0, $t1
	la $a0, m062
	jal show

	.data
m063:	.asciiz "andi -1 & 0xffff = "
	.text
	li $t0, -1
	andi $a1, $t0, 0xffff
	la $a0, m063
	jal show

	.data
m064:	.asciiz "ori 0 | 0x8000 = "
	.text
	ori $a1, $zero, 0x8000
	la $a0, m064
	jal show

	.data
m065:	.asciiz "xori 0xffffffff ^ 0x00ff = "
	.text
	li $t0, -1
	xori $a1, $t0, 0x00ff
	la $a0, m065
	jal show

	.data
m066:	.asciiz "and imm pseudo -1 & 0x12345678 = "
	.text
	li $t0, -1
	and $a1, $t0, 0x12345678
	la $a0, m066
	jal show

	.data
m067:	.asciiz "lui 0xffff = "
	.text
	lui $a1, 0xffff
	la $a0, m067
	jal show

	.data
m068:	.asciiz "li 0x12345678 = "
	.text
	li $a1, 0x12345678
	la $a0, m068
	jal show

	.data
m069:	.asciiz "li -1 = "
	.text
	li $a1, -1
	la $a0, m069
	jal show

	.data
m070:	.asciiz "li 0x8000 = "
	.text
	li $a1, 0x8000
	la $a0, m070
	jal show

	.data
m071:	.asciiz "li -32769 = "
	.text
	li $a1, -32769
	la $a0, m071
	jal show

	.data
m072:	.asciiz "li 0xffff0000 = "
	.text
	li $a1, 0xffff0000
	la $a0, m072
	jal show

	.data
m073:	.asciiz "li 0x10000 = "
	.text
	li $a1, 0x10000
	la $a0, m073
	jal show

	.data
m074:	.asciiz "sll 1 << 31 = "
	.text
	li $t0, 1
	sll $a1, $t0, 31
	la $a0, m074
	jal show

	.data
m075:	.asciiz "sll 0x12345678 << 0 = "
	.text
	li $t0, 0x12345678
	sll $a1, $t0, 0
	la $a0, m075
	jal show

	.data
m076:	.asciiz "srl 0x80000000 >> 31 = "
	.text
	li $t0, 0x80000000
	srl $a1, $t0, 31
	la $a0, m076
	jal show

	.data
m077:	.asciiz "sra 0x80000000 >> 31 = "
	.text
	li $t0, 0x80000000
	sra $a1, $t0, 31
	la $a0, m077
	jal show

	.data
m078:	.asciiz "sra 0x7ffffff0 >> 4 = "
	.text
	li $t0, 0x7ffffff0
	sra $a1, $t0, 4
	la $a0, m078
	jal show

	.data
m079:	.asciiz "sllv 1 << 33 = "
	.text
	li $t0, 1
	li $t1, 33
	sllv $a1, $t0, $t1
	la $a0, m079
	jal show

	.data
m080:	.asciiz "srlv 0x80000000 >> 32 = "
	.text
	li $t0, 0x80000000
	li $t1, 32
	srlv $a1, $t0, $t1
	la $a0, m080
	jal show

	.data
m081:	.asciiz "srav 0x80000000 >> -1 = "
	.text
	li $t0, 0x80000000
	li $t1, -1
	srav $a1, $t0, $t1
	la $a0, m081
	jal show

	.data
m082:	.asciiz "srlv 0xf0000000 >> 36 = "
	.text
	li $t0, 0xf0000000
	li $t1, 36
	srlv $a1, $t0, $t1
	la $a0, m082
	jal show

	.data
m083:	.asciiz "rol 0x12345678 by 4 = "
	.text
	li $t0, 0x12345678
	rol $a1, $t0, 4
	la $a0, m083
	jal show

	.data
m084:	.asciiz "ror 0x12345678 by 4 = "
	.text
	li $t0, 0x12345678
	ror $a1, $t0, 4
	la $a0, m084
	jal show

	.data
m085:	.asciiz "rol reg 0x80000001 by 1 = "
	.text
	li $t0, 0x80000001
	li $t1, 1
	rol $a1, $t0, $t1
	la $a0, m085
	jal show

	.data
m086:	.asciiz "slt -1 < 1 = "
	.text
	li $t0, -1
	li $t1, 1
	slt $a1, $t0, $t1
	la $a0, m086
	jal show

	.data
m087:	.asciiz "sltu 0xffffffff < 1 = "
	.text
	li $t0, -1
	li $t1, 1
	sltu $a1, $t0, $t1
	la $a0, m087
	jal show

	.data
m088:	.asciiz "slti -5 < -4 = "
	.text
	li $t0, -5
	slti $a1, $t0, -4
	la $a0, m088
	jal show

	.data
m089:	.asciiz "sltiu 5 < 0xffffffff = "
	.text
	li $t0, 5
	sltiu $a1, $t0, -1
	la $a0, m089
	jal show

	.data
m090:	.asciiz "sltiu 0xffff8000 < -32768 = "
	.text
	li $t0, 0xffff8000
	sltiu $a1, $t0, -32768
	la $a0, m090
	jal show

	.data
m091:	.asciiz "seq 3 3 = "
	.text
	li $t0, 3
	li $t1, 3
	seq $a1, $t0, $t1
	la $a0, m091
	jal show

	.data
m092:	.asciiz "sne 3 4 = "
	.text
	li $t0, 3
	li $t1, 4
	sne $a1, $t0, $t1
	la $a0, m092
	jal show

	.data
m093:	.asciiz "sge -1 1 = "
	.text
	li $t0, -1
	li $t1, 1
	sge $a1, $t0, $t1
	la $a0, m093
	jal show

	.data
m094:	.asciiz "sgeu -1 1 = "
	.text
	li $t0, -1
	li $t1, 1
	sgeu $a1, $t0, $t1
	la $a0, m094
	jal show

	.data
m095:	.asciiz "sgt 2 -2 = "
	.text
	li $t0, 2
	li $t1, -2
	sgt $a1, $t0, $t1
	la $a0, m095
	jal show

	.data
m096:	.asciiz "sgtu 2 -2 = "
	.text
	li $t0, 2
	li $t1, -2
	sgtu $a1, $t0, $t1
	la $a0, m096
	jal show

	.data
m097:	.asciiz "sle 5 5 = "
	.text
	li $t0, 5
	li $t1, 5
	sle $a1, $t0, $t1
	la $a0, m097
	jal show

	.data
m098:	.asciiz "sleu 0x80000000 1 = "
	.text
	li $t0, 0x80000000
	li $t1, 1
	sleu $a1, $t0, $t1
	la $a0, m098
	jal show

	.data
m099:	.asciiz "seq imm 7 7 = "
	.text
	li $t0, 7
	seq $a1, $t0, 7
	la $a0, m099
	jal show

	.data
m100:	.asciiz "clz 0x00010000 = "
	.text
	li $t0, 0x00010000
	clz $a1, $t0
	la $a0, m100
	jal show

	.data
m101:	.asciiz "clz 0 = "
	.text
	clz $a1, $zero
	la $a0, m101
	jal show

	.data
m102:	.asciiz "clo 0xffff0000 = "
	.text
	li $t0, 0xffff0000
	clo $a1, $t0
	la $a0, m102
	jal show

	.data
m103:	.asciiz "clo 0 = "
	.text
	clo $a1, $zero
	la $a0, m103
	jal show

	.data
m104:	.asciiz "movn taken = "
	.text
	li $a1, 1
	li $t0, 99
	li $t1, 1
	movn $a1, $t0, $t1
	la $a0, m104
	jal show

	.data
m105:	.asciiz "movn not taken = "
	.text
	li $a1, 1
	li $t0, 99
	movn $a1, $t0, $zero
	la $a0, m105
	jal show

	.data
m106:	.asciiz "movz taken = "
	.text
	li $a1, 1
	li $t0, 77
	movz $a1, $t0, $zero
	la $a0, m106
	jal show

	.data
m107:	.asciiz "movz not taken = "
	.text
	li $a1, 1
	li $t0, 77
	li $t1, 1
	movz $a1, $t0, $t1
	la $a0, m107
	jal show

	.data
m108:	.asciiz "beq $zero, $zero = "
	.text
	li $a1, 1
	beq $zero, $zero, t108_taken
	li $a1, 0
t108_taken:
	la $a0, m108
	jal show

	.data
m109:	.asciiz "bne $zero, $zero = "
	.text
	li $a1, 1
	bne $zero, $zero, t109_taken
	li $a1, 0
t109_taken:
	la $a0, m109
	jal show

	.data
m110:	.asciiz "beqz 0 = "
	.text
	li $a1, 1
	beqz $zero, t110_taken
	li $a1, 0
t110_taken:
	la $a0, m110
	jal show

	.data
m111:	.asciiz "bnez -1 = "
	.text
	li $t0, -1
	li $a1, 1
	bnez $t0, t111_taken
	li $a1, 0
t111_taken:
	la $a0, m111
	jal show

	.data
m112:	.asciiz "bgez -1 = "
	.text
	li $t0, -1
	li $a1, 1
	bgez $t0, t112_taken
	li $a1, 0
t112_taken:
	la $a0, m112
	jal show

	.data
m113:	.asciiz "bgtz 0 = "
	.text
	li $a1, 1
	bgtz $zero, t113_taken
	li $a1, 0
t113_taken:
	la $a0, m113
	jal show

	.data
m114:	.asciiz "blez 0 = "
	.text
	li $a1, 1
	blez $zero, t114_taken
	li $a1, 0
t114_taken:
	la $a0, m114
	jal show

	.data
m115:	.asciiz "bltz 0x80000000 = "
	.text
	li $t0, 0x80000000
	li $a1, 1
	bltz $t0, t115_taken
	li $a1, 0
t115_taken:
	la $a0, m115
	jal show

	.data
m116:	.asciiz "blt -1 1 = "
	.text
	li $t0, -1
	li $t1, 1
	li $a1, 1
	blt $t0, $t1, t116_taken
	li $a1, 0
t116_taken:
	la $a0, m116
	jal show

	.data
m117:	.asciiz "bltu -1 1 = "
	.text
	li $t0, -1
	li $t1, 1
	li $a1, 1
	bltu $t0, $t1, t117_taken
	li $a1, 0
t117_taken:
	la $a0, m117
	jal show

	.data
m118:	.asciiz "bgt imm 5 4 = "
	.text
	li $t0, 5
	li $a1, 1
	bgt $t0, 4, t118_taken
	li $a1, 0
t118_taken:
	la $a0, m118
	jal show

	.data
m119:	.asciiz "bge imm 4 4 = "
	.text
	li $t0, 4
	li $a1, 1
	bge $t0, 4, t119_taken
	li $a1, 0
t119_taken:
	la $a0, m119
	jal show

	.data
m120:	.asciiz "ble imm 5 4 = "
	.text
	li $t0, 5
	li $a1, 1
	ble $t0, 4, t120_taken
	li $a1, 0
t120_taken:
	la $a0, m120
	jal show

	.data
m121:	.asciiz "bleu -1 1 = "
	.text
	li $t0, -1
	li $t1, 1
	li $a1, 1
	bleu $t0, $t1, t121_taken
	li $a1, 0
t121_taken:
	la $a0, m121
	jal show

	.data
m122:	.asciiz "bgeu imm 0x80000000 1 = "
	.text
	li $t0, 0x80000000
	li $a1, 1
	bgeu $t0, 1, t122_taken
	li $a1, 0
t122_taken:
	la $a0, m122
	jal show

	.data
m123:	.asciiz "bgtu imm 1 -1 = "
	.text
	li $t0, 1
	li $a1, 1
	bgtu $t0, -1, t123_taken
	li $a1, 0
t123_taken:
	la $a0, m123
	jal show

	.data
m124:	.asciiz "beq imm 0x12345678 = "
	.text
	li $t0, 0x12345678
	li $a1, 1
	beq $t0, 0x12345678, t124_taken
	li $a1, 0
t124_taken:
	la $a0, m124
	jal show

	.data
m125:	.asciiz "b always = "
	.text
	li $a1, 1
	b t125_taken
	li $a1, 0
t125_taken:
	la $a0, m125
	jal show

	.data
m126:	.asciiz "bgezal link offset = "
	.text
t126_here:
	bgezal $zero, t126_target
	nop
t126_target:
	la $t0, t126_here
	subu $a1, $ra, $t0
	la $a0, m126
	jal show

	.data
m127:	.asciiz "bltzal not taken still links = "
	.text
	li $ra, 0
t127_here:
	bltzal $zero, t127_target
t127_target:
	la $t0, t127_here
	subu $a1, $ra, $t0
	la $a0, m127
	jal show

	.data
m128:	.asciiz "jal link offset = "
	.text
t128_here:
	jal t128_target
	nop
t128_target:
	la $t0, t128_here
	subu $a1, $ra, $t0
	la $a0, m128
	jal show

	.data
m129:	.asciiz "jalr link offset = "
	.text
	la $t1, t129_target
t129_here:
	jalr $t1
	nop
t129_target:
	la $t0, t129_here
	subu $a1, $ra, $t0
	la $a0, m129
	jal show

	.data
m130:	.asciiz "jalr rd link offset = "
	.text
	la $t1, t130_target
t130_here:
	jalr $t2, $t1
	nop
t130_target:
	la $t0, t130_here
	subu $a1, $t2, $t0
	la $a0, m130
	jal show

	.data
m131:	.asciiz "bal link offset = "
	.text
t131_here:
	bal t131_target
	nop
t131_target:
	la $t0, t131_here
	subu $a1, $ra, $t0
	la $a0, m131
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
