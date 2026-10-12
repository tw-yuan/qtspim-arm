# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): memory layout.  Prints the addresses
# SPIM assigns to labels in the data, kernel data, text and kernel text
# segments (including automatic alignment, .align, .space and .data with
# an address) and the initial register values main is called with.
# Note: the stack contents (and so $sp) depend on the program arguments
# and on the environment, which scripts/run-tests.sh clears with env -i.
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
d_byte:	.byte 1
d_half:	.half 2			# aligned to 2 automatically
d_byte2:	.byte 3
d_word:	.word 4			# aligned to 4 automatically
d_byte3:	.byte 5
d_float:	.float 6.0
d_byte4:	.byte 7
d_double:	.double 8.0		# aligned to 8 automatically
d_ascii:	.ascii "abc"
d_asciiz:	.asciiz "abc"
d_space:	.space 5
d_after_space:	.byte 9
	.align 4
d_align4:	.byte 10
	.align 0		# turns off automatic alignment
d_byte5:	.byte 11
d_unaligned_word:	.word 12
	.data 0x10012000
d_at:	.word 13
	.kdata
k_word:	.word 14
	.ktext
k_code:	nop
	.text
t_code:	nop

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	move $s7, $sp		# save the values main was entered with
	addi $s7, $s7, 4
	move $s4, $a0
	move $s5, $a1
	move $s6, $a2
	move $s3, $ra

	.data
m000:	.asciiz "d_byte = "
	.text
	la $a1, d_byte
	la $a0, m000
	jal show

	.data
m001:	.asciiz "d_half = "
	.text
	la $a1, d_half
	la $a0, m001
	jal show

	.data
m002:	.asciiz "d_byte2 = "
	.text
	la $a1, d_byte2
	la $a0, m002
	jal show

	.data
m003:	.asciiz "d_word = "
	.text
	la $a1, d_word
	la $a0, m003
	jal show

	.data
m004:	.asciiz "d_byte3 = "
	.text
	la $a1, d_byte3
	la $a0, m004
	jal show

	.data
m005:	.asciiz "d_float = "
	.text
	la $a1, d_float
	la $a0, m005
	jal show

	.data
m006:	.asciiz "d_byte4 = "
	.text
	la $a1, d_byte4
	la $a0, m006
	jal show

	.data
m007:	.asciiz "d_double = "
	.text
	la $a1, d_double
	la $a0, m007
	jal show

	.data
m008:	.asciiz "d_ascii = "
	.text
	la $a1, d_ascii
	la $a0, m008
	jal show

	.data
m009:	.asciiz "d_asciiz = "
	.text
	la $a1, d_asciiz
	la $a0, m009
	jal show

	.data
m010:	.asciiz "d_space = "
	.text
	la $a1, d_space
	la $a0, m010
	jal show

	.data
m011:	.asciiz "d_after_space = "
	.text
	la $a1, d_after_space
	la $a0, m011
	jal show

	.data
m012:	.asciiz "d_align4 = "
	.text
	la $a1, d_align4
	la $a0, m012
	jal show

	.data
m013:	.asciiz "d_byte5 = "
	.text
	la $a1, d_byte5
	la $a0, m013
	jal show

	.data
m014:	.asciiz "d_unaligned_word = "
	.text
	la $a1, d_unaligned_word
	la $a0, m014
	jal show

	.data
m015:	.asciiz "d_at (.data 0x10012000) = "
	.text
	la $a1, d_at
	la $a0, m015
	jal show

	.data
m016:	.asciiz "k_word (.kdata) = "
	.text
	la $a1, k_word
	la $a0, m016
	jal show

	.data
m017:	.asciiz "k_code (.ktext) = "
	.text
	la $a1, k_code
	la $a0, m017
	jal show

	.data
m018:	.asciiz "t_code (.text) = "
	.text
	la $a1, t_code
	la $a0, m018
	jal show

	.data
m019:	.asciiz "main = "
	.text
	la $a1, main
	la $a0, m019
	jal show

	.data
m020:	.asciiz "__start = "
	.text
	la $a1, __start
	la $a0, m020
	jal show

	.data
m021:	.asciiz "initial $gp = "
	.text
	move $a1, $gp
	la $a0, m021
	jal show

	.data
m022:	.asciiz "initial $sp (in main) = "
	.text
	move $a1, $s7
	la $a0, m022
	jal show

	.data
m023:	.asciiz "argc = "
	.text
	move $a1, $s4
	la $a0, m023
	jal show

	.data
m024:	.asciiz "$a1 (argv) - $sp = "
	.text
	subu $a1, $s5, $s7
	la $a0, m024
	jal show

	.data
m025:	.asciiz "$a2 (envp) - $sp = "
	.text
	subu $a1, $s6, $s7
	la $a0, m025
	jal show

	.data
m026:	.asciiz "initial $fp = "
	.text
	move $a1, $fp
	la $a0, m026
	jal show

	.data
m027:	.asciiz "initial $ra - __start = "
	.text
	la $t0, __start
	subu $a1, $s3, $t0
	la $a0, m027
	jal show

	.data
m028:	.asciiz "initial status register = "
	.text
	mfc0 $a1, $12
	la $a0, m028
	jal show

	.data
m029:	.asciiz "value at d_word = "
	.text
	lw $a1, d_word
	la $a0, m029
	jal show

	.data
m030:	.asciiz "value at d_double high word = "
	.text
	la $t0, d_double
	lw $a1, 4($t0)
	la $a0, m030
	jal show

	.data
m031:	.asciiz "bytes at d_ascii = "
	.text
	la $t0, d_ascii
	lw $a1, 0($t0)
	la $a0, m031
	jal show

	.data
m032:	.asciiz "bytes at d_asciiz = "
	.text
	la $t0, d_asciiz
	ulw $a1, 0($t0)
	la $a0, m032
	jal show

	.data
m033:	.asciiz "value at d_unaligned_word = "
	.text
	la $t0, d_unaligned_word
	ulw $a1, 0($t0)
	la $a0, m033
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
