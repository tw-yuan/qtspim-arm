# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): program arguments.  Prints argc, argv
# and the number of environment entries main receives in $a0, $a1 and $a2.
# Note: spim 9.1.24 treats every further word on its command line that does
# not start with "-" as a new program file name, so after
# "-file argv.s one two three" main sees only argc = 1, argv[0] = "three".
# scripts/run-tests.sh runs spim with an empty environment, so envp is
# empty.
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
m_argc:	.asciiz "argc = "
m_argv:	.asciiz "argv["
m_mid:	.asciiz "] = \""
m_end:	.asciiz "\"\n"
m_null:	.asciiz "argv[argc] is NULL = "
m_envc:	.asciiz "environment entries = "
nl:	.asciiz "\n"

	.text
	.globl main
main:
	move $s0, $a0		# argc
	move $s1, $a1		# argv
	move $s2, $a2		# envp

	la $a0, m_argc
	li $v0, 4
	syscall
	move $a0, $s0
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall

	li $s3, 0
arg_loop:
	beq $s3, $s0, arg_done
	la $a0, m_argv
	li $v0, 4
	syscall
	move $a0, $s3
	li $v0, 1
	syscall
	la $a0, m_mid
	li $v0, 4
	syscall
	sll $t0, $s3, 2
	addu $t0, $s1, $t0
	lw $a0, 0($t0)
	li $v0, 4
	syscall
	la $a0, m_end
	li $v0, 4
	syscall
	addi $s3, $s3, 1
	b arg_loop
arg_done:

	la $a0, m_null
	li $v0, 4
	syscall
	sll $t0, $s0, 2
	addu $t0, $s1, $t0
	lw $t0, 0($t0)
	seq $a0, $t0, $zero
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall

	li $s3, 0		# count environment entries
env_loop:
	sll $t0, $s3, 2
	addu $t0, $s2, $t0
	lw $t0, 0($t0)
	beqz $t0, env_done
	addi $s3, $s3, 1
	b env_loop
env_done:
	la $a0, m_envc
	li $v0, 4
	syscall
	move $a0, $s3
	li $v0, 1
	syscall
	la $a0, nl
	li $v0, 4
	syscall
	jr $ra
