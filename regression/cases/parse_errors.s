# SPIM S20 MIPS simulator.
# Supplementary test (qtspim-arm): assembler diagnostics.  Records the
# format of parser warnings and errors (file, line number, source line and
# caret), which the planned editor will parse to locate errors.  SPIM stops
# reading a file at the first syntax error, so this file contains warnings
# and non-fatal errors first and a single syntax error at the end.  Note
# that for some errors SPIM reports the line after the offending one,
# because the scanner has already read the end of the line.
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
msg:	.asciiz "parse_errors: program ran\n"

	.text
	.globl main
main:
	addi $t0, $t0, 0x12345		# immediate out of range
	ori $t0, $t0, -1		# immediate out of range
	sll $t0, $t0, 40		# shift amount out of range
	rotr $t0, $t0, 4		# MIPS32 Rev 2: not implemented
	lw $t0, undefined_label		# undefined symbol
	j also_undefined
dup:	nop
dup:	nop				# label defined twice
	.globl main
	la $a0, msg
	li $v0, 4
	syscall
	jr $ra
	add $t0, $t0			# syntax error: missing operand
	nop
