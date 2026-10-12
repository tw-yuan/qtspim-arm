# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): output system calls print_int (1),
# print_float (2), print_double (3), print_string (4) and print_char (11).
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
s_empty:	.asciiz ""
s_hello:	.asciiz "Hello, SPIM!\n"
s_esc:	.asciiz "tab[\t] quote[\"] backslash[\\] end\n"
s_ascii:	.ascii "ascii-no-nul "
s_after:	.asciiz "follows-ascii\n"
s_ints:	.asciiz "print_int:\n"
s_floats:	.asciiz "print_float:\n"
s_doubles:	.asciiz "print_double:\n"
s_chars:	.asciiz "print_char: "
s_strings:	.asciiz "print_string:\n"
ints:	.word 0, 1, -1, 42, -42, 2147483647, -2147483648, 123456789, 0x80000001
ints_end:
floats:	.float 0.0, -0.0, 1.0, -2.5, 0.1, 3.14159265, 1.0e10, 1.0e-10
	.float 3.4028235e38, 1.17549435e-38, 123456.789, 0.5
floats_end:		# NB: the .align below also moves this label, so the
			# loop prints one extra word (current SPIM behavior).
	.align 3
doubles:	.double 0.0, -0.0, 1.0, -2.5, 0.1, 3.141592653589793, 1.0e100
	.double 1.0e-100, 1.7976931348623157e308, 2.2250738585072014e-308
	.double 123456789.123456789, 0.5
doubles_end:
chars:	.byte 'A', 'z', '0', '~', ' ', '!'
chars_end:

	.text
	.globl main
main:
	# print_string
	la $a0, s_strings
	li $v0, 4
	syscall
	la $a0, s_hello
	syscall
	la $a0, s_empty
	syscall
	la $a0, s_esc
	syscall
	la $a0, s_ascii		# .ascii is not terminated: runs into s_after
	syscall

	# print_int
	la $a0, s_ints
	li $v0, 4
	syscall
	la $s0, ints
	la $s1, ints_end
int_loop:
	lw $a0, 0($s0)
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addiu $s0, $s0, 4
	bne $s0, $s1, int_loop

	# print_float
	la $a0, s_floats
	li $v0, 4
	syscall
	la $s0, floats
	la $s1, floats_end
float_loop:
	l.s $f12, 0($s0)
	li $v0, 2
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addiu $s0, $s0, 4
	bne $s0, $s1, float_loop

	# print_double
	la $a0, s_doubles
	li $v0, 4
	syscall
	la $s0, doubles
	la $s1, doubles_end
double_loop:
	l.d $f12, 0($s0)
	li $v0, 3
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	addiu $s0, $s0, 8
	bne $s0, $s1, double_loop

	# print_char
	la $a0, s_chars
	li $v0, 4
	syscall
	la $s0, chars
	la $s1, chars_end
char_loop:
	lb $a0, 0($s0)
	li $v0, 11
	syscall
	addiu $s0, $s0, 1
	bne $s0, $s1, char_loop
	li $a0, 0x141		# only the low byte is printed
	li $v0, 11
	syscall
	li $a0, '\n'
	syscall

	# print_float / print_double of infinities
	li $t0, 0x7f800000
	mtc1 $t0, $f12
	li $v0, 2
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	li $t0, 0xfff00000
	mtc1 $t0, $f13
	mtc1 $zero, $f12
	li $v0, 3
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	jr $ra
