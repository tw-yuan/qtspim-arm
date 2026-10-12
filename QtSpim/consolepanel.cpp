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

#include "consolepanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStyle>
#include <QTextCursor>
#include <QVBoxLayout>

#include "theme.h"

ConsolePanel::ConsolePanel(QWidget* parent) : QWidget(parent) {
  view_ = new QPlainTextEdit(this);
  view_->setObjectName(QStringLiteral("consoleView"));
  view_->setReadOnly(true);
  view_->setUndoRedoEnabled(false);
  view_->setFrameShape(QFrame::NoFrame);
  view_->setMaximumBlockCount(50000);
  view_->setFont(Theme::monoFont(13));
  view_->setPlaceholderText(
      tr("No output yet. Text printed with syscalls (print_int, "
         "print_string, …) appears here."));
  view_->setAccessibleName(tr("Console output"));
  view_->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                 Qt::TextSelectableByKeyboard);

  inputBox_ = new QWidget(this);
  inputBox_->setObjectName(QStringLiteral("consoleInput"));
  inputBox_->setAttribute(Qt::WA_StyledBackground);
  prompt_ = new QLabel(inputBox_);
  prompt_->setObjectName(QStringLiteral("inputPrompt"));
  prompt_->setWordWrap(true);
  input_ = new QLineEdit(inputBox_);
  input_->setObjectName(QStringLiteral("consoleLine"));
  input_->setFont(Theme::monoFont(13));
  input_->setAccessibleName(tr("Program input"));
  input_->addAction(
      Theme::icon(QStringLiteral("corner-down-left"), Theme::kText3, 16),
      QLineEdit::TrailingPosition);

  auto* inputLayout = new QVBoxLayout(inputBox_);
  inputLayout->setContentsMargins(12, 8, 12, 10);
  inputLayout->setSpacing(6);
  inputLayout->addWidget(prompt_);
  inputLayout->addWidget(input_);
  inputBox_->hide();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(view_, 1);
  layout->addWidget(inputBox_);

  flushTimer_.setSingleShot(true);
  flushTimer_.setInterval(30);
  connect(&flushTimer_, &QTimer::timeout, this, &ConsolePanel::flush);
  connect(input_, &QLineEdit::returnPressed, this, &ConsolePanel::submit);
  connect(input_, &QLineEdit::textEdited, this, [this](const QString& text) {
    // Keys go to the program at once for memory-mapped I/O and read_char.
    if ((rawInput_ || singleCharacter_) && !text.isEmpty()) {
      input_->clear();
      if (!rawInput_) append(text, Theme::kAccentText);
      emit inputSubmitted(text);
    }
  });
}

void ConsolePanel::appendOutput(const QString& text) {
  pending_ += text;
  if (!flushTimer_.isActive()) flushTimer_.start();
}

void ConsolePanel::appendMessage(const QString& text, bool error) {
  flush();
  append(text, error ? Theme::kError : Theme::kText3);
}

void ConsolePanel::flush() {
  flushTimer_.stop();
  if (pending_.isEmpty()) return;
  const QString text = pending_;
  pending_.clear();
  append(text, Theme::kText);
}

void ConsolePanel::append(const QString& text, const QColor& color) {
  QScrollBar* bar = view_->verticalScrollBar();
  const bool atEnd = bar->value() >= bar->maximum() - 4;
  QTextCursor c(view_->document());
  c.movePosition(QTextCursor::End);
  QTextCharFormat f;
  f.setForeground(color);
  c.insertText(text, f);
  if (atEnd) bar->setValue(bar->maximum());
}

void ConsolePanel::clear() {
  flushTimer_.stop();
  pending_.clear();
  view_->clear();
  input_->clear();
}

void ConsolePanel::setInputState(bool running, bool waiting,
                                 const QString& prompt, bool singleCharacter) {
  flush();
  singleCharacter_ = singleCharacter;
  // While the program runs, keys can be typed ahead; the hint and the focus
  // ring appear when a read syscall waits.
  inputBox_->setVisible(running || waiting);
  inputBox_->setProperty("waiting", waiting);
  prompt_->setVisible(waiting && !prompt.isEmpty());
  prompt_->setText(prompt);
  input_->setPlaceholderText(waiting ? (singleCharacter
                                            ? tr("Type a character")
                                            : tr("Type here and press Return"))
                                     : tr("Input for the program"));
  inputBox_->style()->unpolish(inputBox_);
  inputBox_->style()->polish(inputBox_);
  input_->style()->unpolish(input_);
  input_->style()->polish(input_);
}

void ConsolePanel::focusInput() {
  if (inputBox_->isVisible()) input_->setFocus();
}

void ConsolePanel::submit() {
  if (rawInput_) {
    emit inputSubmitted(QStringLiteral("\n"));
    return;
  }
  const QString line = input_->text() + QLatin1Char('\n');
  input_->clear();
  flush();
  // Echo what was typed, as the original console did.
  append(line, Theme::kAccentText);
  emit inputSubmitted(line);
}

QString ConsolePanel::text() const {
  const_cast<ConsolePanel*>(this)->flush();
  return view_->toPlainText();
}

void ConsolePanel::setFontSize(int pointSize) {
  view_->setFont(Theme::monoFont(pointSize));
  input_->setFont(Theme::monoFont(pointSize));
}
