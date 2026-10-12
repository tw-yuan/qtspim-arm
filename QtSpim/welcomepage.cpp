/* SPIM S20 MIPS simulator.
   The welcome page shown when no program is open.

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

#include "welcomepage.h"

#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "theme.h"

namespace {

// A large button with an icon, a title and one line of explanation.
QPushButton* actionCard(const QString& icon, const QString& title,
                        const QString& text, QWidget* parent) {
  auto* b = new QPushButton(parent);
  b->setObjectName(QStringLiteral("welcomeCard"));
  b->setCursor(Qt::PointingHandCursor);
  b->setAccessibleName(title);
  b->setAccessibleDescription(text);
  auto* iconLabel = new QLabel(b);
  iconLabel->setPixmap(Theme::icon(icon, Theme::accent(), 24).pixmap(24, 24));
  auto* titleLabel = new QLabel(title, b);
  titleLabel->setObjectName(QStringLiteral("cardTitle"));
  auto* textLabel = new QLabel(text, b);
  textLabel->setObjectName(QStringLiteral("cardText"));
  textLabel->setWordWrap(true);
  for (QLabel* l : {iconLabel, titleLabel, textLabel}) {
    l->setAttribute(Qt::WA_TransparentForMouseEvents);
  }
  auto* layout = new QVBoxLayout(b);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(6);
  layout->addWidget(iconLabel);
  layout->addSpacing(4);
  layout->addWidget(titleLabel);
  layout->addWidget(textLabel, 1);
  b->setMinimumHeight(132);
  return b;
}

QLabel* sectionTitle(const QString& text, QWidget* parent) {
  auto* l = new QLabel(text, parent);
  l->setObjectName(QStringLiteral("sectionTitle"));
  return l;
}

}  // namespace

QList<WelcomePage::Example> WelcomePage::examples() {
  return {
      {QStringLiteral("hello.s"), tr("Hello, World"),
       tr("Print a string with a syscall"), tr("Start here")},
      {QStringLiteral("sum.s"), tr("Array sum"),
       tr("Loops, labels, and loading words from memory"), tr("Loops")},
      {QStringLiteral("add.s"), tr("Add two numbers"),
       tr("Read input with read_int and print the result"), tr("Input")},
      {QStringLiteral("factorial.s"), tr("Factorial"),
       tr("Function calls, jal/jr, and saving registers on the stack"),
       tr("Functions")},
  };
}

WelcomePage::WelcomePage(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("welcomePage"));
  setAttribute(Qt::WA_StyledBackground);

  auto* content = new QWidget;
  content->setObjectName(QStringLiteral("welcomeContent"));
  content->setMaximumWidth(780);

  auto* title = new QLabel(tr("Welcome to QtSpim ARM"), content);
  title->setObjectName(QStringLiteral("welcomeTitle"));
  auto* subtitle = new QLabel(
      tr("Write and run MIPS32 assembly programs, and watch how they work "
         "one instruction at a time."),
      content);
  subtitle->setObjectName(QStringLiteral("welcomeSubtitle"));
  subtitle->setWordWrap(true);

  // Back to the program in the editor, when there is one.
  resume_ = new QPushButton(content);
  resume_->setObjectName(QStringLiteral("resumeBar"));
  resume_->setCursor(Qt::PointingHandCursor);
  auto* resumeIcon = new QLabel(resume_);
  resumeIcon->setPixmap(
      Theme::icon(QStringLiteral("file-code"), Theme::accent(), 18)
          .pixmap(18, 18));
  resumeName_ = new QLabel(resume_);
  resumeName_->setObjectName(QStringLiteral("rowTitle"));
  auto* resumeText =
      new QLabel(tr("Back to the program you were working on."), resume_);
  resumeText->setObjectName(QStringLiteral("rowText"));
  auto* resumeArrow = new QLabel(resume_);
  resumeArrow->setPixmap(
      Theme::icon(QStringLiteral("arrow-right"), Theme::accent(), 18)
          .pixmap(18, 18));
  for (QLabel* l : {resumeIcon, resumeName_, resumeText, resumeArrow}) {
    l->setAttribute(Qt::WA_TransparentForMouseEvents);
  }
  auto* resumeLayout = new QHBoxLayout(resume_);
  resumeLayout->setContentsMargins(14, 10, 14, 10);
  resumeLayout->setSpacing(10);
  resumeLayout->addWidget(resumeIcon);
  resumeLayout->addWidget(resumeName_);
  resumeLayout->addWidget(resumeText, 1);
  resumeLayout->addWidget(resumeArrow);
  resume_->setMinimumHeight(44);
  resume_->hide();
  connect(resume_, &QPushButton::clicked, this, &WelcomePage::resumeRequested);

  auto* cards = new QHBoxLayout;
  cards->setSpacing(12);
  QPushButton* newCard =
      actionCard(QStringLiteral("file-plus"), tr("New Program"),
                 tr("Start from a template with .data, .text and main already "
                    "written."),
                 content);
  QPushButton* openCard = actionCard(
      QStringLiteral("folder-open"), tr("Open File…"),
      tr("Open a homework or lab file (.s, .as or .asm). You can also drag "
         "it into this window."),
      content);
  QPushButton* examplesCard = actionCard(
      QStringLiteral("book-open"), tr("Examples"),
      tr("Short programs with comments, each about one idea."), content);
  cards->addWidget(newCard);
  cards->addWidget(openCard);
  cards->addWidget(examplesCard);
  connect(newCard, &QPushButton::clicked, this, &WelcomePage::newRequested);
  connect(openCard, &QPushButton::clicked, this, &WelcomePage::openRequested);

  // Examples
  examplesBox_ = new QWidget(content);
  examplesBox_->setObjectName(QStringLiteral("listBox"));
  examplesBox_->setAttribute(Qt::WA_StyledBackground);
  auto* examplesLayout = new QVBoxLayout(examplesBox_);
  examplesLayout->setContentsMargins(0, 4, 0, 4);
  examplesLayout->setSpacing(0);
  for (const Example& e : examples()) {
    auto* row = new QPushButton(examplesBox_);
    row->setObjectName(QStringLiteral("listRow"));
    row->setCursor(Qt::PointingHandCursor);
    row->setAccessibleName(e.title);
    auto* icon = new QLabel(row);
    icon->setPixmap(Theme::icon(QStringLiteral("file-code"), Theme::kText3, 18)
                        .pixmap(18, 18));
    auto* name = new QLabel(e.title, row);
    name->setObjectName(QStringLiteral("rowTitle"));
    auto* text = new QLabel(e.description, row);
    text->setObjectName(QStringLiteral("rowText"));
    auto* tag = new QLabel(e.tag, row);
    tag->setObjectName(QStringLiteral("rowTag"));
    for (QLabel* l : {icon, name, text, tag}) {
      l->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    auto* l = new QHBoxLayout(row);
    l->setContentsMargins(14, 8, 14, 8);
    l->setSpacing(10);
    l->addWidget(icon);
    l->addWidget(name);
    l->addWidget(text, 1);
    l->addWidget(tag);
    row->setMinimumHeight(40);
    const QString file = e.file;
    connect(row, &QPushButton::clicked, this,
            [this, file] { emit exampleRequested(file); });
    examplesLayout->addWidget(row);
  }

  // Recent files
  recentBox_ = new QWidget(content);
  recentBox_->setObjectName(QStringLiteral("listBox"));
  recentBox_->setAttribute(Qt::WA_StyledBackground);
  recentLayout_ = new QVBoxLayout(recentBox_);
  recentLayout_->setContentsMargins(0, 4, 0, 4);
  recentLayout_->setSpacing(0);

  // Quick start
  auto* steps = new QWidget(content);
  auto* stepsLayout = new QVBoxLayout(steps);
  stepsLayout->setContentsMargins(0, 0, 0, 0);
  stepsLayout->setSpacing(8);
  const QStringList stepTexts = {
      tr("Write a program in the editor, or open your homework file."),
      tr("Press %1 to run it. Output appears in the Console.")
          .arg(QKeySequence(Qt::CTRL | Qt::Key_R)
                   .toString(QKeySequence::NativeText)),
      tr("Click a line number to set a breakpoint, then press F10 to step "
         "and watch the registers change."),
  };
  for (int i = 0; i < stepTexts.size(); ++i) {
    auto* number = new QLabel(QString::number(i + 1), steps);
    number->setObjectName(QStringLiteral("stepNumber"));
    number->setAlignment(Qt::AlignCenter);
    number->setFixedSize(22, 22);
    auto* text = new QLabel(stepTexts[i], steps);
    text->setWordWrap(true);
    auto* row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(number, 0, Qt::AlignTop);
    row->addWidget(text, 1);
    stepsLayout->addLayout(row);
  }

  auto* footer = new QLabel(
      tr("QtSpim ARM uses the simulator of SPIM 9.1.24 unchanged. It is an "
         "unofficial fork of QtSpim and is not endorsed by James R. Larus."),
      content);
  footer->setObjectName(QStringLiteral("hint"));
  footer->setWordWrap(true);

  auto* columns = new QHBoxLayout;
  columns->setSpacing(24);
  auto* left = new QVBoxLayout;
  left->setSpacing(8);
  left->addWidget(sectionTitle(tr("Examples"), content));
  left->addWidget(examplesBox_);
  left->addSpacing(12);
  left->addWidget(sectionTitle(tr("Recent"), content));
  left->addWidget(recentBox_);
  left->addStretch(1);
  auto* right = new QVBoxLayout;
  right->setSpacing(8);
  right->addWidget(sectionTitle(tr("Quick start"), content));
  right->addWidget(steps);
  right->addStretch(1);
  columns->addLayout(left, 3);
  columns->addLayout(right, 2);

  auto* layout = new QVBoxLayout(content);
  layout->setContentsMargins(32, 40, 32, 32);
  layout->setSpacing(0);
  layout->addWidget(title);
  layout->addSpacing(6);
  layout->addWidget(subtitle);
  layout->addSpacing(20);
  layout->addWidget(resume_);
  layout->addSpacing(12);
  layout->addLayout(cards);
  layout->addSpacing(28);
  layout->addLayout(columns);
  layout->addSpacing(28);
  layout->addWidget(footer);

  auto* center = new QWidget;
  auto* centerLayout = new QHBoxLayout(center);
  centerLayout->setContentsMargins(0, 0, 0, 0);
  centerLayout->addStretch(1);
  centerLayout->addWidget(content, 100);
  centerLayout->addStretch(1);

  auto* scroll = new QScrollArea(this);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setWidget(center);
  auto* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->addWidget(scroll);

  connect(examplesCard, &QPushButton::clicked, this, [this, scroll] {
    scroll->ensureWidgetVisible(examplesBox_);
    QWidget* first = examplesBox_->findChild<QPushButton*>();
    if (first != nullptr) first->setFocus();
  });
  setRecentFiles({});
}

void WelcomePage::setRecentFiles(const QStringList& files) {
  while (QLayoutItem* item = recentLayout_->takeAt(0)) {
    delete item->widget();
    delete item;
  }
  int shown = 0;
  for (const QString& path : files) {
    if (path.isEmpty() || shown == 5) continue;
    ++shown;
    const QFileInfo info(path);
    auto* row = new QPushButton(recentBox_);
    row->setObjectName(QStringLiteral("listRow"));
    row->setCursor(Qt::PointingHandCursor);
    row->setAccessibleName(info.fileName());
    row->setToolTip(QDir::toNativeSeparators(path));
    auto* icon = new QLabel(row);
    icon->setPixmap(Theme::icon(QStringLiteral("file-text"), Theme::kText3, 18)
                        .pixmap(18, 18));
    auto* name = new QLabel(info.fileName(), row);
    name->setObjectName(QStringLiteral("rowTitle"));
    QString dir = QDir::toNativeSeparators(info.absolutePath());
    const QString home = QDir::toNativeSeparators(QDir::homePath());
    if (dir.startsWith(home)) dir = QLatin1Char('~') + dir.mid(home.size());
    auto* where = new QLabel(dir, row);
    where->setObjectName(QStringLiteral("rowText"));
    for (QLabel* l : {icon, name, where}) {
      l->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    auto* l = new QHBoxLayout(row);
    l->setContentsMargins(14, 8, 14, 8);
    l->setSpacing(10);
    l->addWidget(icon);
    l->addWidget(name);
    l->addWidget(where, 1);
    row->setMinimumHeight(40);
    connect(row, &QPushButton::clicked, this,
            [this, path] { emit recentRequested(path); });
    recentLayout_->addWidget(row);
  }
  if (shown == 0) {
    auto* none = new QLabel(tr("No recent files."), recentBox_);
    none->setObjectName(QStringLiteral("hint"));
    none->setContentsMargins(14, 10, 14, 10);
    recentLayout_->addWidget(none);
  }
}

void WelcomePage::setCurrentDocument(const QString& name) {
  resumeName_->setText(tr("Continue editing %1").arg(name));
  resume_->setAccessibleName(tr("Continue editing %1").arg(name));
  resume_->setVisible(!name.isEmpty());
}
