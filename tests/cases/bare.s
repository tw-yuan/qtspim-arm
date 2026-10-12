# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): a program for the bare machine (spim
# -bare -noexception).  It uses only real MIPS instructions, numeric and
# symbolic register names, and fills every branch and load delay slot,
# since the bare machine has delayed branches and delayed loads.
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
msg:	.asciiz "bare machine: sum 1..10 = "
nl:	.asciiz "\n"
	.align 2
val:	.word 1234

	.text
	.globl __start
__start:
	lui $4, 0x1000		# msg: the bare machine data segment starts at 0x10000000
	ori $2, $0, 4
	syscall

	ori $t0, $zero, 10	# sum = 1 + 2 + ... + 10
	ori $t1, $zero, 0
loop:
	addu $t1, $t1, $t0
	addiu $t0, $t0, -1
	bne $t0, $zero, loop
	sll $0, $0, 0		# branch delay slot (nop is a pseudo-op)

	addu $a0, $t1, $zero
	ori $v0, $zero, 1
	syscall

	lui $a0, 0x1000
	ori $a0, $a0, 27	# nl
	ori $v0, $zero, 4
	syscall

	lui $t2, 0x1000
	lw $t3, 32($t2)		# val
	addu $a0, $t3, $zero	# load delay slot: still the old $t3 (0)
	ori $v0, $zero, 1
	syscall
	lui $a0, 0x1000
	ori $a0, $a0, 27
	ori $v0, $zero, 4
	syscall
	addu $a0, $t3, $zero	# now 1234
	ori $v0, $zero, 1
	syscall
	lui $a0, 0x1000
	ori $a0, $a0, 27
	ori $v0, $zero, 4
	syscall

	ori $v0, $zero, 10
	syscall
