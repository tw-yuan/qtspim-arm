/* SPIM S20 MIPS simulator.
   The program editor.

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

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QList>
#include <QPlainTextEdit>
#include <QSet>
#include <QString>

class MipsHighlighter;

// A mark the editor shows on a line: an assembler error or warning.
struct EditorMark {
  int line = 0;     // 1-based
  int column = -1;  // -1 = the whole line
  int length = 0;
  QString message;  // short explanation, shown after the line
  bool warning = false;
};

// QPlainTextEdit with line numbers, breakpoints in the gutter, the current
// line of execution, error marks, auto-indentation and comment toggling.
class CodeEditor : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit CodeEditor(QWidget* parent = nullptr);

  void setEditorFont(int pointSize);
  void setTabWidth(int spaces);

  // Breakpoints, by 1-based line.
  QSet<int> breakpointLines() const;
  void setBreakpointLines(const QSet<int>& lines);
  void toggleBreakpoint(int line);
  void clearBreakpoints();
  // Lines that have machine code (breakpoints elsewhere are drawn hollow).
  // An empty set means unknown (not assembled).
  void setCodeLines(const QSet<int>& lines);

  // The line being executed (0 = none); |waiting| = waiting for input.
  void setExecutionLine(int line, bool waiting);
  int executionLine() const { return executionLine_; }

  void setMarks(const QList<EditorMark>& marks);
  void clearMarks();

  void goToLine(int line);
  // Scrolls so that |line| is visible, without moving the cursor.
  void revealLine(int line);
  int currentLineNumber() const;
  void toggleComment();

  // Called by the gutter.
  int gutterWidth() const;
  void paintGutter(QPaintEvent* event);
  void gutterClicked(int y);

 signals:
  void breakpointsChanged();
  // Files dropped on the editor (they are opened, not inserted as text).
  void filesDropped(const QStringList& files);

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  bool event(QEvent* event) override;
  bool canInsertFromMimeData(const QMimeData* source) const override;
  void insertFromMimeData(const QMimeData* source) override;

 private:
  void updateGutterWidth();
  void updateGutter(const QRect& rect, int dy);
  void updateSelections();
  void indentSelection(bool indent);
  QTextBlock blockAtY(int y) const;

  QWidget* gutter_;
  MipsHighlighter* highlighter_;
  int executionLine_ = 0;
  bool waiting_ = false;
  QList<EditorMark> marks_;
  QSet<int> codeLines_;
};

#endif  // CODEEDITOR_H
