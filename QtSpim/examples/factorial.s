# factorial.s — compute 5! with a recursive function
#
# fact calls itself, so before the call it saves its return address ($ra)
# and its argument on the stack, and restores them afterwards.  Step
# through it and watch $sp in the registers and the Stack view of Memory.

	.data
msg:	.asciiz	"5! = "

	.text
	.globl	main
main:	addiu	$sp, $sp, -8	# save $ra: main calls a function too
	sw	$ra, 0($sp)

	li	$a0, 5		# $v0 = fact(5)
	jal	fact
	move	$t0, $v0

	li	$v0, 4		# print_string(msg)
	la	$a0, msg
	syscall
	move	$a0, $t0	# print_int(5!)
	li	$v0, 1
	syscall
	li	$v0, 11		# print_char('\n')
	li	$a0, 10
	syscall

	lw	$ra, 0($sp)	# restore $ra and return
	addiu	$sp, $sp, 8
	jr	$ra

# fact(n): n in $a0, returns n! in $v0
fact:	addiu	$sp, $sp, -8	# push $ra and n
	sw	$ra, 4($sp)
	sw	$a0, 0($sp)

	li	$v0, 1		# fact(0) = 1
	blez	$a0, done

	addi	$a0, $a0, -1	# $v0 = fact(n - 1)
	jal	fact
	lw	$a0, 0($sp)	# restore n
	mul	$v0, $v0, $a0	# $v0 = n * fact(n - 1)

done:	lw	$ra, 4($sp)	# pop $ra and n, and return
	addiu	$sp, $sp, 8
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
