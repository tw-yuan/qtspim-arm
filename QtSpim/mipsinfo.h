/* SPIM S20 MIPS simulator.
   MIPS names and plain-language explanations used by the interface.

   Copyright (c) 2026, qtspim-arm contributors.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may
   be used to endorse or promote products derived from this software without
   specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MIPSINFO_H
#define MIPSINFO_H

#include <QString>
#include <QStringList>

#include <cstdint>

// Facts about the MIPS32 assembly language that SPIM accepts, for the
// highlighter, the register panel and the explanations of assembler errors.
// The instruction and directive names come from CPU/op.h, so they always
// match what the parser accepts.
namespace Mips {

enum class OpKind { None, Instruction, Pseudo, Directive };

// Lowercase names, e.g. "addu", "li", ".word".
OpKind opKind(const QString& name);
const QStringList& instructionNames();  // instructions and pseudo-instructions
const QStringList& directiveNames();

// True for register operands SPIM accepts: $0-$31, $zero, $at, $v0, ...,
// $f0-$f31.
bool isRegister(const QString& token);

// Up to |max| names from |candidates| that look like a misspelling of
// |word|, best first.
QStringList suggestions(const QString& word, const QStringList& candidates,
                        int max = 3);

// Conventional name of integer register |n| without the "$", e.g. "t2".
QString registerName(int n);

// What register |n| is used for, translated.  -1 = PC, -2 = HI, -3 = LO.
QString registerUsage(int n);

// SPIM system calls.
struct Syscall {
  int number;
  QString name;         // "read_int"
  QString title;        // translated, e.g. "read integer"
  QString description;  // translated, what it does with the registers
  bool readsInput;      // waits for the console
};
// Returns false when |number| is not a SPIM system call.
bool syscall(int number, Syscall* out);

// Translated name of MIPS exception code |code| (Cause bits 2-6).
QString exceptionName(int code);

// A message from the SPIM assembler, explained for a student.  |lineText| is
// the source line the message refers to.
struct Explanation {
  QString text;     // translated, one or two sentences
  int column = -1;  // where to underline in |lineText| (-1 = whole line)
  int length = 0;
};
Explanation explainParserMessage(const QString& message,
                                 const QString& lineText);
QString explainUndefinedSymbol(const QString& symbol);
QString explainRunError(const QString& message);

// The text of |line| without its comment.
QString stripComment(const QString& line);

// A 32-bit word in base 16 ("0x0000000a"), 10 (signed) or 2.
QString formatWord(uint32_t value, int base);
// Parses a value typed by the user: 0x... (hex), 0b... (binary), a signed
// decimal number, or digits in |base|.
bool parseWord(const QString& text, int base, uint32_t* value);

}  // namespace Mips

#endif  // MIPSINFO_H
