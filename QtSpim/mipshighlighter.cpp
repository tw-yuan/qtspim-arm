/* SPIM S20 MIPS simulator.
   Syntax highlighting for MIPS assembly.

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

#include "mipshighlighter.h"

#include <QRegularExpression>

#include "mipsinfo.h"
#include "theme.h"

MipsHighlighter::MipsHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document) {
  opcode_.setForeground(Theme::kSynOpcode);
  opcode_.setFontWeight(QFont::DemiBold);
  directive_.setForeground(Theme::kSynDirective);
  register_.setForeground(Theme::kSynRegister);
  number_.setForeground(Theme::kSynNumber);
  string_.setForeground(Theme::kSynString);
  label_.setForeground(Theme::kSynLabel);
  label_.setFontWeight(QFont::DemiBold);
  comment_.setForeground(Theme::kSynComment);
  comment_.setFontItalic(true);
}

void MipsHighlighter::highlightBlock(const QString& text) {
  const QString code = Mips::stripComment(text);
  if (code.size() < text.size()) {
    setFormat(code.size(), text.size() - code.size(), comment_);
  }

  static const QRegularExpression token(QStringLiteral(
      "(\\s+)"                        // 1 white space
      "|(\"(?:[^\"\\\\]|\\\\.)*\"?)"  // 2 string
      "|('(?:[^'\\\\]|\\\\.)*'?)"     // 3 character
      "|(\\.[A-Za-z_][\\w.]*)"        // 4 directive
      "|(\\$\\w*)"                    // 5 register
      "|(-?0[xX][0-9a-fA-F]+|-?\\d+(?:\\.\\d+)?(?:[eE][-+]?\\d+)?)"  // 6
      "|([A-Za-z_$][\\w.$]*\\s*:)"  // 7 label definition
      "|([A-Za-z_][\\w.]*)"         // 8 word
      "|(.)"));                     // 9 anything else
  bool seenOperation = false;
  auto it = token.globalMatch(code);
  while (it.hasNext()) {
    const auto m = it.next();
    const int start = m.capturedStart(0);
    const int length = m.capturedLength(0);
    if (m.capturedLength(2) > 0 || m.capturedLength(3) > 0) {
      setFormat(start, length, string_);
    } else if (m.capturedLength(4) > 0) {
      setFormat(start, length, directive_);
      seenOperation = true;
    } else if (m.capturedLength(5) > 0) {
      setFormat(start, length, register_);
    } else if (m.capturedLength(6) > 0) {
      setFormat(start, length, number_);
    } else if (m.capturedLength(7) > 0 && !seenOperation) {
      setFormat(start, length, label_);
    } else if (m.capturedLength(8) > 0) {
      if (!seenOperation) {
        seenOperation = true;
        const Mips::OpKind kind = Mips::opKind(m.captured(8).toLower());
        if (kind == Mips::OpKind::Instruction || kind == Mips::OpKind::Pseudo) {
          setFormat(start, length, opcode_);
        }
      }
    }
  }
}
