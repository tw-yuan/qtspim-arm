# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): file system calls open (13), read (14),
# write (15) and close (16).  Only read-only access is used (O_RDONLY is 0
# on every host), and the paths are relative to the repository root, where
# scripts/run-tests.sh runs spim.
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
path:	.asciiz "tests/cases/data/file_syscall.txt"
missing:	.asciiz "tests/cases/data/does-not-exist.txt"
msg_write:	.asciiz "written with the write syscall\n"
msg_write_end:
m_open:	.asciiz "open = "
m_read:	.asciiz "read = "
m_read2:	.asciiz "second read = "
m_eof:	.asciiz "read at EOF = "
m_close:	.asciiz "close = "
m_missing:	.asciiz "open missing file = "
m_write:	.asciiz "write = "
m_badfd:	.asciiz "read from closed fd = "
m_text:	.asciiz "text: ["
m_end:	.asciiz "]\n"
	.align 2
buf:	.space 128

	.text
	.globl main
main:
	addi $sp, $sp, -4
	sw $ra, 0($sp)

	la $a0, path		# open(path, O_RDONLY, 0)
	li $a1, 0
	li $a2, 0
	li $v0, 13
	syscall
	move $s0, $v0
	slt $a1, $v0, $zero	# print only whether it failed: fd numbers
	xori $a1, $a1, 1	# depend on the host
	la $a0, m_open
	jal show

	move $a0, $s0		# read(fd, buf, 20)
	la $a1, buf
	li $a2, 20
	li $v0, 14
	syscall
	move $a1, $v0
	la $a0, m_read
	jal show

	move $a0, $s0		# read(fd, buf + 20, 100)
	la $a1, buf+20
	li $a2, 100
	li $v0, 14
	syscall
	move $s1, $v0
	move $a1, $v0
	la $a0, m_read2
	jal show

	la $t0, buf		# NUL-terminate and print the contents
	addiu $t0, $t0, 20
	addu $t0, $t0, $s1
	sb $zero, 0($t0)
	la $a0, m_text
	li $v0, 4
	syscall
	la $a0, buf
	syscall
	la $a0, m_end
	syscall

	move $a0, $s0		# read at EOF
	la $a1, buf
	li $a2, 10
	li $v0, 14
	syscall
	move $a1, $v0
	la $a0, m_eof
	jal show

	move $a0, $s0		# close(fd)
	li $v0, 16
	syscall
	move $a1, $v0
	la $a0, m_close
	jal show

	move $a0, $s0		# read from the closed descriptor
	la $a1, buf
	li $a2, 10
	li $v0, 14
	syscall
	move $a1, $v0
	la $a0, m_badfd
	jal show

	la $a0, missing		# open a file that does not exist
	li $a1, 0
	li $a2, 0
	li $v0, 13
	syscall
	move $a1, $v0
	la $a0, m_missing
	jal show

	li $a0, 1		# write(1, msg, len)
	la $a1, msg_write
	la $a2, msg_write_end
	subu $a2, $a2, $a1
	addiu $a2, $a2, -1	# without the NUL
	li $v0, 15
	syscall
	move $a1, $v0
	la $a0, m_write
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
