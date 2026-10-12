/* SPIM S20 MIPS simulator.
   The list of assembler errors and warnings.

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

#include "problemspanel.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include "mipsinfo.h"
#include "theme.h"

QString ProblemsPanel::explain(const Problem& p, const QString& lineText,
                               int* column, int* length) {
  if (column != nullptr) *column = -1;
  if (length != nullptr) *length = 0;
  switch (p.kind) {
    case Problem::Kind::Parser: {
      const Mips::Explanation e =
          Mips::explainParserMessage(p.message, lineText);
      if (column != nullptr) *column = e.column;
      if (length != nullptr) *length = e.length;
      return e.text;
    }
    case Problem::Kind::UndefinedSymbol: {
      if (column != nullptr && length != nullptr) {
        const int at = Mips::stripComment(lineText).indexOf(p.symbol);
        if (at >= 0) {
          *column = at;
          *length = p.symbol.size();
        }
      }
      return Mips::explainUndefinedSymbol(p.symbol);
    }
    case Problem::Kind::Other:
      break;
  }
  return p.message;
}

QList<EditorMark> ProblemsPanel::marksFor(const QList<Problem>& problems,
                                          const QStringList& sourceLines) {
  QList<EditorMark> marks;
  for (const Problem& p : problems) {
    if (p.line <= 0) continue;
    const QString lineText =
        p.line <= sourceLines.size() ? sourceLines[p.line - 1] : QString();
    EditorMark m;
    m.line = p.line;
    m.message = explain(p, lineText, &m.column, &m.length);
    m.warning = p.severity == Problem::Severity::Warning;
    marks << m;
  }
  return marks;
}

ProblemsPanel::ProblemsPanel(QWidget* parent) : QWidget(parent) {
  list_ = new QWidget;
  list_->setObjectName(QStringLiteral("problemsList"));
  listLayout_ = new QVBoxLayout(list_);
  listLayout_->setContentsMargins(12, 8, 12, 12);
  listLayout_->setSpacing(8);
  listLayout_->addStretch(1);

  scroll_ = new QScrollArea(this);
  scroll_->setWidget(list_);
  scroll_->setWidgetResizable(true);
  scroll_->setFrameShape(QFrame::NoFrame);

  empty_ = new QLabel(this);
  empty_->setObjectName(QStringLiteral("hint"));
  empty_->setAlignment(Qt::AlignCenter);
  empty_->setWordWrap(true);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(scroll_);
  layout->addWidget(empty_);
  setProblems({}, {}, false);
}

void ProblemsPanel::setProblems(const QList<Problem>& problems,
                                const QStringList& sourceLines,
                                bool assembled) {
  // Remove the old rows (all but the final stretch).
  while (listLayout_->count() > 1) {
    QLayoutItem* item = listLayout_->takeAt(0);
    delete item->widget();
    delete item;
  }
  count_ = problems.size();
  scroll_->setVisible(!problems.isEmpty());
  empty_->setVisible(problems.isEmpty());
  empty_->setText(assembled
                      ? tr("No problems. The program assembled successfully.")
                      : tr("Not assembled yet. Press Run (%1) or Assemble "
                           "(%2) to check the program.")
                            .arg(QKeySequence(Qt::CTRL | Qt::Key_R)
                                     .toString(QKeySequence::NativeText),
                                 QKeySequence(Qt::CTRL | Qt::Key_B)
                                     .toString(QKeySequence::NativeText)));
  if (problems.isEmpty()) return;

  bool syntaxError = false;
  bool anyError = false;
  int row = 0;
  for (const Problem& p : problems) {
    const bool warning = p.severity == Problem::Severity::Warning;
    anyError |= !warning;
    if (p.kind == Problem::Kind::Parser &&
        p.message.compare(QLatin1String("syntax error"), Qt::CaseInsensitive) ==
            0) {
      syntaxError = true;
    }
    const QString lineText = (p.line > 0 && p.line <= sourceLines.size())
                                 ? sourceLines[p.line - 1]
                                 : QString();

    auto* card = new QWidget;
    card->setObjectName(warning ? QStringLiteral("warningCard")
                                : QStringLiteral("errorCard"));
    card->setAttribute(Qt::WA_StyledBackground);
    auto* icon = new QLabel(card);
    icon->setPixmap(
        (warning
             ? Theme::icon(QStringLiteral("triangle-alert"), Theme::kPaused, 16)
             : Theme::icon(QStringLiteral("circle-x"), Theme::kError, 16))
            .pixmap(16, 16));
    icon->setAlignment(Qt::AlignTop);
    icon->setAccessibleName(warning ? tr("Warning") : tr("Error"));

    auto* where =
        new QLabel(p.line > 0 ? tr("Line %1").arg(p.line)
                              : (warning ? tr("Warning") : tr("Error")),
                   card);
    where->setObjectName(QStringLiteral("problemLine"));
    auto* text = new QLabel(explain(p, lineText), card);
    text->setObjectName(QStringLiteral("problemText"));
    text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* raw = new QLabel(p.raw, card);
    raw->setObjectName(QStringLiteral("problemRaw"));
    raw->setFont(Theme::monoFont(11));
    raw->setWordWrap(true);
    raw->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* body = new QVBoxLayout;
    body->setSpacing(3);
    auto* header = new QHBoxLayout;
    header->addWidget(where);
    header->addStretch(1);
    if (p.line > 0) {
      auto* go = new QToolButton(card);
      go->setText(tr("Go to Line"));
      go->setProperty("link", true);
      go->setCursor(Qt::PointingHandCursor);
      const int line = p.line;
      connect(go, &QToolButton::clicked, this,
              [this, line] { emit lineActivated(line); });
      header->addWidget(go);
    }
    body->addLayout(header);
    body->addWidget(text);
    body->addWidget(raw);

    auto* cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(10, 8, 10, 8);
    cardLayout->setSpacing(8);
    cardLayout->addWidget(icon, 0, Qt::AlignTop);
    cardLayout->addLayout(body, 1);
    listLayout_->insertWidget(row++, card);
  }

  if (syntaxError) {
    auto* note = new QLabel(
        tr("SPIM stops reading the file at the first syntax error. Fix it "
           "and run again to see any later errors."));
    note->setObjectName(QStringLiteral("hint"));
    note->setWordWrap(true);
    listLayout_->insertWidget(row++, note);
  }
  if (anyError) {
    auto* runAnyway = new QToolButton;
    runAnyway->setText(tr("Run anyway"));
    runAnyway->setToolTip(
        tr("Run the program as the original SPIM would, despite the errors"));
    runAnyway->setProperty("link", true);
    runAnyway->setCursor(Qt::PointingHandCursor);
    connect(runAnyway, &QToolButton::clicked, this,
            &ProblemsPanel::runAnywayRequested);
    listLayout_->insertWidget(row++, runAnyway, 0, Qt::AlignLeft);
  }
}
