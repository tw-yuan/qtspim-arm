# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): input system calls read_int (5),
# read_float (6), read_double (7), read_string (8) and read_char (12).
# Input comes from syscall_read.in.  Each read consumes one line (or as
# much of it as fits in the buffer, like fgets).
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
nl:	.asciiz "\n"
s_int:	.asciiz "read_int: "
s_float:	.asciiz "read_float: "
s_double:	.asciiz "read_double: "
s_string:	.asciiz "read_string: ["
s_close:	.asciiz "]\n"
s_char:	.asciiz "read_char: "
	.align 2
buf:	.space 64
small:	.space 8

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	# read_int x 6: "42", "-17", "   12abc", "4294967297", "", "+7"
	li $s0, 6
ri_loop:
	li $v0, 5
	syscall
	move $s1, $v0
	la $a0, s_int
	li $v0, 4
	syscall
	move $a0, $s1
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addi $s0, $s0, -1
	bnez $s0, ri_loop

	# read_float x 3: "3.5", "-0.125", "1e-3"
	li $s0, 3
rf_loop:
	li $v0, 6
	syscall
	la $a0, s_float
	li $v0, 4
	syscall
	mov.s $f12, $f0
	li $v0, 2
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addi $s0, $s0, -1
	bnez $s0, rf_loop

	# read_double x 2: "2.718281828459045", "1e308"
	li $s0, 2
rd_loop:
	li $v0, 7
	syscall
	la $a0, s_double
	li $v0, 4
	syscall
	mov.d $f12, $f0
	li $v0, 3
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addi $s0, $s0, -1
	bnez $s0, rd_loop

	# read_string into an 8-byte buffer: "abcdefghijk" -> 7 chars + NUL,
	# the rest of the line is left for the next read.
	la $a0, small
	li $a1, 8
	li $v0, 8
	syscall
	la $a0, s_string
	li $v0, 4
	syscall
	la $a0, small
	syscall
	la $a0, s_close
	syscall

	# read_string into a 64-byte buffer: rest of the line, "hijk\n"
	la $a0, buf
	li $a1, 64
	li $v0, 8
	syscall
	la $a0, s_string
	li $v0, 4
	syscall
	la $a0, buf
	syscall
	la $a0, s_close
	syscall

	# read_string: "with spaces and\ttab"
	la $a0, buf
	li $a1, 64
	li $v0, 8
	syscall
	la $a0, s_string
	li $v0, 4
	syscall
	la $a0, buf
	syscall
	la $a0, s_close
	syscall

	# read_char x 3: 'Z', 'y', '\n'
	li $s0, 3
rc_loop:
	li $v0, 12
	syscall
	move $s1, $v0
	la $a0, s_char
	li $v0, 4
	syscall
	move $a0, $s1
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addi $s0, $s0, -1
	bnez $s0, rc_loop

	# At end of input: read_int returns 0, read_string gets an empty string,
	# read_char returns '\n'.
	li $v0, 5
	syscall
	move $s1, $v0
	la $a0, s_int
	li $v0, 4
	syscall
	move $a0, $s1
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall

	li $t0, 0x58585858	# "XXXX" so we can see whether it is overwritten
	sw $t0, buf
	sw $zero, buf+4
	la $a0, buf
	li $a1, 64
	li $v0, 8
	syscall
	la $a0, s_string
	li $v0, 4
	syscall
	la $a0, buf
	syscall
	la $a0, s_close
	syscall

	li $v0, 12
	syscall
	move $s1, $v0
	la $a0, s_char
	li $v0, 4
	syscall
	move $a0, $s1
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall

	lw $ra, 0($sp)
	addi $sp, $sp, 4
	jr $ra
