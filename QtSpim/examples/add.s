# add.s — read two numbers and print their sum
#
# read_int (syscall 5) waits for you to type a number in the console and
# press Return; the number is stored in $v0.

	.data
ask:	.asciiz	"Enter a number: "
res:	.asciiz	"Sum: "

	.text
	.globl	main
main:	li	$v0, 4		# print_string(ask)
	la	$a0, ask
	syscall
	li	$v0, 5		# read_int: result in $v0
	syscall
	move	$t0, $v0	# keep the first number

	li	$v0, 4		# ask again
	la	$a0, ask
	syscall
	li	$v0, 5
	syscall
	add	$t1, $t0, $v0	# $t1 = sum

	li	$v0, 4		# print_string(res)
	la	$a0, res
	syscall
	move	$a0, $t1	# print_int(sum)
	li	$v0, 1
	syscall
	li	$v0, 11		# print_char('\n')
	li	$a0, 10
	syscall
	jr	$ra

# ----------------------------------------------------------------------------
# An example program for QtSpim ARM.
#
# Copyright (c) 2026, qtspim-arm contributors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of the James R. Larus nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
