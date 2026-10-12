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

#include "codeeditor.h"

#include <QAbstractTextDocumentLayout>
#include <QContextMenuEvent>
#include <QHelpEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextLayout>
#include <QToolTip>

#include <algorithm>

#include "mipshighlighter.h"
#include "mipsinfo.h"
#include "theme.h"

namespace {

constexpr int kMarkColumn = 20;  // breakpoints, arrow and error icons

class BlockData : public QTextBlockUserData {
 public:
  bool breakpoint = false;
};

bool hasBreakpoint(const QTextBlock& block) {
  auto* data = static_cast<BlockData*>(block.userData());
  return data != nullptr && data->breakpoint;
}

void setBlockBreakpoint(QTextBlock block, bool on) {
  auto* data = static_cast<BlockData*>(block.userData());
  if (data == nullptr) {
    if (!on) return;
    data = new BlockData;
    block.setUserData(data);
  }
  data->breakpoint = on;
}

class Gutter : public QWidget {
 public:
  explicit Gutter(CodeEditor* editor) : QWidget(editor), editor_(editor) {
    setCursor(Qt::PointingHandCursor);
    setToolTip(CodeEditor::tr("Click to set or remove a breakpoint"));
  }
  QSize sizeHint() const override { return {editor_->gutterWidth(), 0}; }

 protected:
  void paintEvent(QPaintEvent* event) override { editor_->paintGutter(event); }
  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton) {
      editor_->gutterClicked(qRound(event->position().y()));
    }
  }

 private:
  CodeEditor* editor_;
};

}  // namespace

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent),
      gutter_(new Gutter(this)),
      highlighter_(new MipsHighlighter(document())) {
  setFrameShape(QFrame::NoFrame);
  setLineWrapMode(QPlainTextEdit::NoWrap);
  setCenterOnScroll(false);
  setEditorFont(13);
  setTabWidth(8);
  setAccessibleName(tr("Program editor"));

  QPalette p = palette();
  p.setColor(QPalette::Base, Theme::kWindow);
  p.setColor(QPalette::Text, Theme::kText);
  p.setColor(QPalette::Highlight, QColor("#CFE0F7"));
  p.setColor(QPalette::HighlightedText, Theme::kText);
  setPalette(p);

  connect(this, &CodeEditor::blockCountChanged, this,
          &CodeEditor::updateGutterWidth);
  connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateGutter);
  connect(this, &CodeEditor::cursorPositionChanged, this,
          &CodeEditor::updateSelections);
  connect(document(), &QTextDocument::contentsChange, this,
          [this](int, int removed, int added) {
            if (removed == 0 && added == 0) return;
            // Marks and the execution line refer to the assembled text.
            if (!marks_.isEmpty()) clearMarks();
          });
  connect(this, &CodeEditor::blockCountChanged, this,
          [this] { emit breakpointsChanged(); });
  updateGutterWidth();
  updateSelections();
}

void CodeEditor::setEditorFont(int pointSize) {
  const int tabWidth =
      qRound(tabStopDistance() /
             std::max(1.0, QFontMetricsF(font()).horizontalAdvance(' ')));
  setFont(Theme::monoFont(pointSize));
  if (tabWidth > 0) setTabWidth(tabWidth);
  updateGutterWidth();
}

void CodeEditor::setTabWidth(int spaces) {
  setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' ')) *
                     spaces);
}

// --- Breakpoints ---

QSet<int> CodeEditor::breakpointLines() const {
  QSet<int> lines;
  for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
    if (hasBreakpoint(b)) lines.insert(b.blockNumber() + 1);
  }
  return lines;
}

void CodeEditor::setBreakpointLines(const QSet<int>& lines) {
  for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
    setBlockBreakpoint(b, lines.contains(b.blockNumber() + 1));
  }
  gutter_->update();
  emit breakpointsChanged();
}

void CodeEditor::toggleBreakpoint(int line) {
  QTextBlock b = document()->findBlockByNumber(line - 1);
  if (!b.isValid()) return;
  setBlockBreakpoint(b, !hasBreakpoint(b));
  gutter_->update();
  emit breakpointsChanged();
}

void CodeEditor::clearBreakpoints() { setBreakpointLines({}); }

void CodeEditor::setCodeLines(const QSet<int>& lines) {
  codeLines_ = lines;
  gutter_->update();
}

// --- Execution line and marks ---

void CodeEditor::setExecutionLine(int line, bool waiting) {
  executionLine_ = line;
  waiting_ = waiting;
  updateSelections();
  gutter_->update();
  revealLine(line);
}

void CodeEditor::revealLine(int line) {
  if (line <= 0) return;
  const QTextBlock block = document()->findBlockByNumber(line - 1);
  if (!block.isValid()) return;
  const QRectF r = blockBoundingGeometry(block).translated(contentOffset());
  const QRect view = viewport()->rect();
  if (r.top() < view.top() || r.bottom() > view.bottom()) {
    const int visibleLines =
        std::max(1, view.height() / std::max(1, fontMetrics().height()));
    verticalScrollBar()->setValue(
        std::max(0, block.firstLineNumber() - visibleLines / 3));
  }
}

void CodeEditor::setMarks(const QList<EditorMark>& marks) {
  marks_ = marks;
  updateSelections();
  gutter_->update();
  viewport()->update();
}

void CodeEditor::clearMarks() {
  marks_.clear();
  updateSelections();
  gutter_->update();
  viewport()->update();
}

void CodeEditor::updateSelections() {
  QList<QTextEdit::ExtraSelection> selections;

  const int cursorLine = textCursor().blockNumber() + 1;
  if (!isReadOnly() && cursorLine != executionLine_) {
    QTextEdit::ExtraSelection s;
    s.format.setBackground(QColor("#F6F6F8"));
    s.format.setProperty(QTextFormat::FullWidthSelection, true);
    s.cursor = textCursor();
    s.cursor.clearSelection();
    selections << s;
  }

  QSet<int> markedLines;
  for (const EditorMark& m : marks_) {
    const QTextBlock block = document()->findBlockByNumber(m.line - 1);
    if (!block.isValid()) continue;
    if (!markedLines.contains(m.line) && !m.warning) {
      QTextEdit::ExtraSelection line;
      line.format.setBackground(Theme::kErrorTint);
      line.format.setProperty(QTextFormat::FullWidthSelection, true);
      line.cursor = QTextCursor(block);
      selections << line;
      markedLines.insert(m.line);
    }
    QTextEdit::ExtraSelection under;
    under.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    under.format.setUnderlineColor(m.warning ? Theme::kPaused
                                             : Theme::kBreakpoint);
    QTextCursor c(block);
    const QString text = block.text();
    int start = m.column;
    int length = m.length;
    if (start < 0 || start >= text.size()) {
      // The code part of the line, without indentation and comment.
      const QString code = Mips::stripComment(text);
      start = 0;
      while (start < code.size() && code[start].isSpace()) ++start;
      int end = code.size();
      while (end > start && code[end - 1].isSpace()) --end;
      length = end - start;
    }
    if (length <= 0) continue;
    c.setPosition(block.position() + start);
    c.setPosition(block.position() + std::min<int>(start + length, text.size()),
                  QTextCursor::KeepAnchor);
    under.cursor = c;
    selections << under;
  }

  if (executionLine_ > 0) {
    const QTextBlock block = document()->findBlockByNumber(executionLine_ - 1);
    if (block.isValid()) {
      QTextEdit::ExtraSelection s;
      s.format.setBackground(waiting_ ? Theme::kWaitingLine
                                      : Theme::kCurrentLine);
      s.format.setProperty(QTextFormat::FullWidthSelection, true);
      s.cursor = QTextCursor(block);
      selections << s;
    }
  }
  setExtraSelections(selections);
}

// --- Gutter ---

int CodeEditor::gutterWidth() const {
  int digits = 2;
  for (int max = std::max(1, blockCount()); max >= 100; max /= 10) ++digits;
  return kMarkColumn +
         fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 14;
}

void CodeEditor::updateGutterWidth() {
  setViewportMargins(gutterWidth(), 0, 0, 0);
}

void CodeEditor::updateGutter(const QRect& rect, int dy) {
  if (dy != 0) {
    gutter_->scroll(0, dy);
  } else {
    gutter_->update(0, rect.y(), gutter_->width(), rect.height());
  }
  if (rect.contains(viewport()->rect())) updateGutterWidth();
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
  QPlainTextEdit::resizeEvent(event);
  const QRect cr = contentsRect();
  gutter_->setGeometry(QRect(cr.left(), cr.top(), gutterWidth(), cr.height()));
}

void CodeEditor::paintGutter(QPaintEvent* event) {
  QPainter p(gutter_);
  p.fillRect(event->rect(), Theme::kWindow);
  p.setRenderHint(QPainter::Antialiasing);

  QHash<int, bool> markLines;  // line -> warning only
  for (const EditorMark& m : marks_) {
    markLines[m.line] = markLines.value(m.line, true) && m.warning;
  }

  QFont numberFont = font();
  QFont boldFont = font();
  boldFont.setBold(true);
  const int lineHeight = fontMetrics().height();
  const QColor accent = Theme::accent();

  QTextBlock block = firstVisibleBlock();
  int number = block.blockNumber() + 1;
  qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
  qreal bottom = top + blockBoundingRect(block).height();
  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      const bool executing = number == executionLine_;
      const bool marked = markLines.contains(number);
      const bool warningOnly = markLines.value(number, false);
      QColor numberColor = Theme::kTextDisabled;
      if (executing) {
        numberColor = Theme::kText;
      } else if (marked && !warningOnly) {
        numberColor = Theme::kError;
      }
      p.setFont(executing ? boldFont : numberFont);
      p.setPen(numberColor);
      p.drawText(QRectF(kMarkColumn, top, gutter_->width() - kMarkColumn - 8,
                        lineHeight),
                 Qt::AlignRight | Qt::AlignVCenter, QString::number(number));

      const QPointF center(kMarkColumn / 2.0 + 2, top + lineHeight / 2.0);
      if (hasBreakpoint(block)) {
        const bool active = codeLines_.isEmpty() || codeLines_.contains(number);
        p.setPen(QPen(Theme::kBreakpoint, 1.5));
        p.setBrush(active ? QBrush(Theme::kBreakpoint) : QBrush(Qt::NoBrush));
        p.drawEllipse(center, 5, 5);
      }
      if (executing) {
        QPainterPath arrow;
        arrow.moveTo(center + QPointF(-3.5, -5));
        arrow.lineTo(center + QPointF(5, 0));
        arrow.lineTo(center + QPointF(-3.5, 5));
        arrow.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(hasBreakpoint(block) ? QColor(Qt::white) : accent);
        if (hasBreakpoint(block)) {
          p.save();
          p.translate(center);
          p.scale(0.6, 0.6);
          p.translate(-center);
          p.drawPath(arrow);
          p.restore();
        } else {
          p.drawPath(arrow);
        }
      } else if (marked && !hasBreakpoint(block)) {
        const QIcon icon =
            warningOnly
                ? Theme::icon(QStringLiteral("triangle-alert"), Theme::kPaused,
                              14)
                : Theme::icon(QStringLiteral("circle-x"), Theme::kError, 14);
        icon.paint(&p, QRect(int(center.x()) - 7, int(center.y()) - 7, 14, 14));
      }
    }
    block = block.next();
    top = bottom;
    bottom = top + blockBoundingRect(block).height();
    ++number;
  }
}

QTextBlock CodeEditor::blockAtY(int y) const {
  QTextBlock block = firstVisibleBlock();
  qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
  while (block.isValid()) {
    const qreal bottom = top + blockBoundingRect(block).height();
    if (y >= top && y < bottom) return block;
    block = block.next();
    top = bottom;
  }
  return QTextBlock();
}

void CodeEditor::gutterClicked(int y) {
  const QTextBlock block = blockAtY(y);
  if (block.isValid()) toggleBreakpoint(block.blockNumber() + 1);
}

// --- Inline explanations of marks ---

void CodeEditor::paintEvent(QPaintEvent* event) {
  QPlainTextEdit::paintEvent(event);
  if (marks_.isEmpty()) return;

  QPainter p(viewport());
  p.setRenderHint(QPainter::Antialiasing);
  QFont f = Theme::uiFont(-1);
  p.setFont(f);
  const QFontMetrics fm(f);
  QSet<int> done;
  for (const EditorMark& m : marks_) {
    if (done.contains(m.line) || m.message.isEmpty()) continue;
    done.insert(m.line);
    const QTextBlock block = document()->findBlockByNumber(m.line - 1);
    if (!block.isValid() || !block.isVisible()) continue;
    const QRectF geometry =
        blockBoundingGeometry(block).translated(contentOffset());
    if (geometry.bottom() < 0 || geometry.top() > viewport()->height()) {
      continue;
    }
    const QTextLayout* layout = block.layout();
    if (layout == nullptr || layout->lineCount() == 0) continue;
    const QTextLine last = layout->lineAt(layout->lineCount() - 1);
    const qreal x = geometry.left() + last.naturalTextWidth() + 28;
    const qreal available = viewport()->width() - x - 8;
    if (available < 80) continue;
    const QString text =
        fm.elidedText(m.message, Qt::ElideRight, int(available) - 16);
    const QRectF box(x, geometry.top() + last.y() + 1,
                     fm.horizontalAdvance(text) + 16, last.height() - 2);
    p.setPen(Qt::NoPen);
    p.setBrush(m.warning ? QColor("#FFF6E0") : QColor("#FBE3E0"));
    p.drawRoundedRect(box, Theme::kRadiusTag, Theme::kRadiusTag);
    p.setPen(m.warning ? QColor("#7A4B00") : Theme::kErrorText);
    p.drawText(box, Qt::AlignCenter, text);
  }
}

bool CodeEditor::event(QEvent* event) {
  if (event->type() == QEvent::ToolTip) {
    auto* help = static_cast<QHelpEvent*>(event);
    const QPoint pos = viewport()->mapFrom(this, help->pos());
    const int line = cursorForPosition(pos).blockNumber() + 1;
    QStringList texts;
    for (const EditorMark& m : marks_) {
      if (m.line == line && !m.message.isEmpty()) texts << m.message;
    }
    if (!texts.isEmpty()) {
      QToolTip::showText(help->globalPos(), texts.join(QLatin1Char('\n')),
                         this);
      return true;
    }
    QToolTip::hideText();
    event->ignore();
    return true;
  }
  return QPlainTextEdit::event(event);
}

// --- Editing ---

void CodeEditor::keyPressEvent(QKeyEvent* event) {
  if (isReadOnly()) {
    QPlainTextEdit::keyPressEvent(event);
    return;
  }
  if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
      !(event->modifiers() & Qt::ShiftModifier)) {
    // Keep the indentation of the line; indent after a line with only a
    // label.
    const QString line = textCursor().block().text();
    int n = 0;
    while (n < line.size() && (line[n] == ' ' || line[n] == '\t')) ++n;
    QString indent = line.left(n);
    const QString code = Mips::stripComment(line).trimmed();
    static const QRegularExpression labelOnly(
        QStringLiteral("^[A-Za-z_.$][A-Za-z0-9_.$]*:$"));
    if (indent.isEmpty() && labelOnly.match(code).hasMatch()) {
      indent = QStringLiteral("\t");
    }
    textCursor().insertText(QStringLiteral("\n") + indent);
    ensureCursorVisible();
    return;
  }
  if (event->key() == Qt::Key_Tab && textCursor().hasSelection() &&
      textCursor().selectedText().contains(QChar::ParagraphSeparator)) {
    indentSelection(true);
    return;
  }
  if (event->key() == Qt::Key_Backtab) {
    indentSelection(false);
    return;
  }
  QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::indentSelection(bool indent) {
  QTextCursor c = textCursor();
  const int start = c.selectionStart();
  const int end = c.selectionEnd();
  QTextBlock first = document()->findBlock(start);
  QTextBlock last = document()->findBlock(end > start ? end - 1 : end);
  c.beginEditBlock();
  for (QTextBlock b = first; b.isValid(); b = b.next()) {
    QTextCursor lc(b);
    if (indent) {
      if (!b.text().isEmpty()) lc.insertText(QStringLiteral("\t"));
    } else {
      const QString t = b.text();
      if (t.startsWith(QLatin1Char('\t'))) {
        lc.deleteChar();
      } else {
        int n = 0;
        while (n < t.size() && n < 8 && t[n] == ' ') ++n;
        for (int i = 0; i < n; ++i) lc.deleteChar();
      }
    }
    if (b == last) break;
  }
  c.endEditBlock();
}

void CodeEditor::toggleComment() {
  if (isReadOnly()) return;
  QTextCursor c = textCursor();
  const int start = c.selectionStart();
  const int end = c.selectionEnd();
  QTextBlock first = document()->findBlock(start);
  QTextBlock last = document()->findBlock(end > start ? end - 1 : end);
  bool allCommented = true;
  for (QTextBlock b = first; b.isValid(); b = b.next()) {
    const QString t = b.text().trimmed();
    if (!t.isEmpty() && !t.startsWith(QLatin1Char('#'))) allCommented = false;
    if (b == last) break;
  }
  c.beginEditBlock();
  for (QTextBlock b = first; b.isValid(); b = b.next()) {
    const QString t = b.text();
    if (allCommented) {
      const int hash = t.indexOf(QLatin1Char('#'));
      if (hash >= 0) {
        QTextCursor lc(b);
        lc.setPosition(b.position() + hash);
        lc.setPosition(b.position() + hash +
                           (t.mid(hash + 1, 1) == QLatin1String(" ") ? 2 : 1),
                       QTextCursor::KeepAnchor);
        lc.removeSelectedText();
      }
    } else if (!t.trimmed().isEmpty()) {
      QTextCursor(b).insertText(QStringLiteral("# "));
    }
    if (b == last) break;
  }
  c.endEditBlock();
}

void CodeEditor::goToLine(int line) {
  const QTextBlock block = document()->findBlockByNumber(line - 1);
  if (!block.isValid()) return;
  QTextCursor c(block);
  setTextCursor(c);
  centerCursor();
  setFocus();
}

int CodeEditor::currentLineNumber() const {
  return textCursor().blockNumber() + 1;
}

void CodeEditor::contextMenuEvent(QContextMenuEvent* event) {
  QMenu* menu = createStandardContextMenu(event->pos());
  const int line = cursorForPosition(event->pos()).blockNumber() + 1;
  menu->addSeparator();
  QAction* bp = menu->addAction(tr("Toggle Breakpoint"));
  connect(bp, &QAction::triggered, this,
          [this, line] { toggleBreakpoint(line); });
  QAction* comment = menu->addAction(tr("Toggle Comment"));
  comment->setEnabled(!isReadOnly());
  connect(comment, &QAction::triggered, this, &CodeEditor::toggleComment);
  menu->exec(event->globalPos());
  delete menu;
}

bool CodeEditor::canInsertFromMimeData(const QMimeData* source) const {
  return source->hasUrls() || QPlainTextEdit::canInsertFromMimeData(source);
}

void CodeEditor::insertFromMimeData(const QMimeData* source) {
  if (source->hasUrls()) {
    QStringList files;
    for (const QUrl& url : source->urls()) {
      if (url.isLocalFile()) files << url.toLocalFile();
    }
    if (!files.isEmpty()) {
      emit filesDropped(files);
      return;
    }
  }
  QPlainTextEdit::insertFromMimeData(source);
}
