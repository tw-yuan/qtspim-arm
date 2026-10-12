/* SPIM S20 MIPS simulator.
   Find and replace bar of the program editor.

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

#ifndef FINDBAR_H
#define FINDBAR_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QToolButton;
class QPushButton;

class FindBar : public QWidget {
  Q_OBJECT

 public:
  FindBar(QPlainTextEdit* editor, QWidget* parent = nullptr);

  void showFind();
  void showReplace();
  void findNext();
  void findPrevious();

 protected:
  void keyPressEvent(QKeyEvent* event) override;

 private:
  void find(bool backward);
  void replaceOne();
  void replaceAll();
  void updateCount();
  void close();

  QPlainTextEdit* editor_;
  QLineEdit* findEdit_;
  QLineEdit* replaceEdit_;
  QLabel* count_;
  QToolButton* caseButton_;
  QWidget* replaceRow_;
};

#endif  // FINDBAR_H
