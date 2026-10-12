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

#include "findbar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>

#include "theme.h"

namespace {

QToolButton* iconButton(const QString& icon, const QString& tip,
                        QWidget* parent) {
  auto* b = new QToolButton(parent);
  b->setIcon(Theme::icon(icon, Theme::kText2, 16));
  b->setIconSize(QSize(16, 16));
  b->setToolTip(tip);
  b->setAccessibleName(tip);
  b->setAutoRaise(true);
  return b;
}

}  // namespace

FindBar::FindBar(QPlainTextEdit* editor, QWidget* parent)
    : QWidget(parent), editor_(editor) {
  setObjectName(QStringLiteral("findBar"));
  setAttribute(Qt::WA_StyledBackground);

  findEdit_ = new QLineEdit(this);
  findEdit_->setPlaceholderText(tr("Find"));
  findEdit_->setClearButtonEnabled(true);
  findEdit_->setAccessibleName(tr("Find"));
  findEdit_->addAction(Theme::icon(QStringLiteral("search"), Theme::kText3, 16),
                       QLineEdit::LeadingPosition);
  count_ = new QLabel(this);
  count_->setObjectName(QStringLiteral("hint"));
  count_->setMinimumWidth(56);
  caseButton_ =
      iconButton(QStringLiteral("case-sensitive"), tr("Match case"), this);
  caseButton_->setCheckable(true);
  QToolButton* previous =
      iconButton(QStringLiteral("chevron-up"), tr("Previous match"), this);
  QToolButton* next =
      iconButton(QStringLiteral("chevron-down"), tr("Next match"), this);
  QToolButton* closeButton = iconButton(QStringLiteral("x"), tr("Close"), this);

  replaceEdit_ = new QLineEdit(this);
  replaceEdit_->setPlaceholderText(tr("Replace with"));
  replaceEdit_->setAccessibleName(tr("Replace with"));
  auto* replaceButton = new QPushButton(tr("Replace"), this);
  auto* replaceAllButton = new QPushButton(tr("Replace All"), this);

  auto* findRow = new QHBoxLayout;
  findRow->setContentsMargins(0, 0, 0, 0);
  findRow->setSpacing(4);
  findRow->addWidget(findEdit_, 1);
  findRow->addWidget(count_);
  findRow->addWidget(caseButton_);
  findRow->addWidget(previous);
  findRow->addWidget(next);
  findRow->addWidget(closeButton);

  replaceRow_ = new QWidget(this);
  auto* replaceLayout = new QHBoxLayout(replaceRow_);
  replaceLayout->setContentsMargins(0, 0, 0, 0);
  replaceLayout->setSpacing(4);
  replaceLayout->addWidget(replaceEdit_, 1);
  replaceLayout->addWidget(replaceButton);
  replaceLayout->addWidget(replaceAllButton);
  replaceLayout->addSpacing(32);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 6, 8, 6);
  layout->setSpacing(4);
  layout->addLayout(findRow);
  layout->addWidget(replaceRow_);

  connect(findEdit_, &QLineEdit::textChanged, this, [this] {
    // Search again from the start of the current match.
    QTextCursor c = editor_->textCursor();
    c.setPosition(c.selectionStart());
    editor_->setTextCursor(c);
    find(false);
  });
  connect(findEdit_, &QLineEdit::returnPressed, this, &FindBar::findNext);
  connect(replaceEdit_, &QLineEdit::returnPressed, this, &FindBar::replaceOne);
  connect(caseButton_, &QToolButton::toggled, this, &FindBar::updateCount);
  connect(previous, &QToolButton::clicked, this, &FindBar::findPrevious);
  connect(next, &QToolButton::clicked, this, &FindBar::findNext);
  connect(closeButton, &QToolButton::clicked, this, &FindBar::close);
  connect(replaceButton, &QPushButton::clicked, this, &FindBar::replaceOne);
  connect(replaceAllButton, &QPushButton::clicked, this, &FindBar::replaceAll);
  hide();
}

void FindBar::showFind() {
  replaceRow_->hide();
  show();
  const QString selected = editor_->textCursor().selectedText();
  if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator)) {
    findEdit_->setText(selected);
  }
  findEdit_->setFocus();
  findEdit_->selectAll();
  updateCount();
}

void FindBar::showReplace() {
  showFind();
  replaceRow_->setVisible(!editor_->isReadOnly());
}

void FindBar::findNext() { find(false); }

void FindBar::findPrevious() { find(true); }

void FindBar::find(bool backward) {
  const QString text = findEdit_->text();
  if (text.isEmpty()) {
    updateCount();
    return;
  }
  QTextDocument::FindFlags flags;
  if (backward) flags |= QTextDocument::FindBackward;
  if (caseButton_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
  QTextCursor found =
      editor_->document()->find(text, editor_->textCursor(), flags);
  if (found.isNull()) {
    // Wrap around.
    QTextCursor start(editor_->document());
    if (backward) start.movePosition(QTextCursor::End);
    found = editor_->document()->find(text, start, flags);
  }
  if (!found.isNull()) {
    editor_->setTextCursor(found);
    editor_->centerCursor();
  }
  updateCount();
}

void FindBar::replaceOne() {
  if (editor_->isReadOnly()) return;
  QTextCursor c = editor_->textCursor();
  const Qt::CaseSensitivity cs =
      caseButton_->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
  if (c.hasSelection() &&
      c.selectedText().compare(findEdit_->text(), cs) == 0) {
    c.insertText(replaceEdit_->text());
  }
  find(false);
}

void FindBar::replaceAll() {
  const QString text = findEdit_->text();
  if (text.isEmpty() || editor_->isReadOnly()) return;
  QTextDocument::FindFlags flags;
  if (caseButton_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
  QTextCursor edit(editor_->document());
  edit.beginEditBlock();
  QTextCursor c(editor_->document());
  for (;;) {
    c = editor_->document()->find(text, c, flags);
    if (c.isNull()) break;
    c.insertText(replaceEdit_->text());
  }
  edit.endEditBlock();
  updateCount();
}

void FindBar::updateCount() {
  const QString text = findEdit_->text();
  if (text.isEmpty()) {
    count_->clear();
    return;
  }
  QTextDocument::FindFlags flags;
  if (caseButton_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
  int total = 0;
  int current = 0;
  const int position = editor_->textCursor().selectionStart();
  QTextCursor c(editor_->document());
  for (;;) {
    c = editor_->document()->find(text, c, flags);
    if (c.isNull() || total > 9999) break;
    ++total;
    if (c.selectionStart() == position) current = total;
  }
  count_->setText(total == 0 ? tr("No results")
                             : tr("%1 of %2").arg(current).arg(total));
}

void FindBar::close() {
  hide();
  editor_->setFocus();
}

void FindBar::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape) {
    close();
    return;
  }
  QWidget::keyPressEvent(event);
}
