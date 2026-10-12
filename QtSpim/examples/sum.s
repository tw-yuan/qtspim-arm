# sum.s — add up the numbers in an array
#
# A loop walks through the array one word (4 bytes) at a time.  Set a
# breakpoint on the line with "loop:" and press Continue to watch $s0 grow.

	.data
array:	.word	10, 20, 30, 40
msg:	.asciiz	"sum = "

	.text
	.globl	main
main:	la	$t0, array	# $t0 = address of the array
	li	$t1, 4		# $t1 = elements left
	li	$s0, 0		# $s0 = running sum
loop:	lw	$t2, 0($t0)	# load the next element
	addu	$s0, $s0, $t2	# add it to the sum
	addiu	$t0, $t0, 4	# point to the next word
	addi	$t1, $t1, -1	# one fewer left
	bnez	$t1, loop	# repeat until none left

	li	$v0, 4		# print_string(msg)
	la	$a0, msg
	syscall
	move	$a0, $s0	# print_int(sum)
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
