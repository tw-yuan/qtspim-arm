/* SPIM S20 MIPS simulator.
   The console: program output and keyboard input.

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

#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include <QString>
#include <QTimer>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;

class ConsolePanel : public QWidget {
  Q_OBJECT

 public:
  explicit ConsolePanel(QWidget* parent = nullptr);

  // Output of the program (syscalls).
  void appendOutput(const QString& text);
  // A message from the simulator, e.g. an exception without a handler.
  void appendMessage(const QString& text, bool error);
  void clear();
  void flush();

  // |running|: the program is running; |waiting|: a read syscall waits.
  // |prompt| explains what the program is waiting for.
  void setInputState(bool running, bool waiting, const QString& prompt,
                     bool singleCharacter);
  // Memory-mapped I/O: keys go to the program as they are typed and are not
  // echoed, as in the original QtSpim.
  void setRawInput(bool raw) { rawInput_ = raw; }

  QString text() const;
  QPlainTextEdit* view() const { return view_; }
  void setFontSize(int pointSize);
  void focusInput();

 signals:
  void inputSubmitted(const QString& text);

 private:
  void submit();
  void append(const QString& text, const QColor& color);

  QPlainTextEdit* view_;
  QWidget* inputBox_;
  QLabel* prompt_;
  QLineEdit* input_;
  QString pending_;
  QTimer flushTimer_;
  bool rawInput_ = false;
  bool singleCharacter_ = false;
};

#endif  // CONSOLEPANEL_H
