/* SPIM S20 MIPS simulator.
   The Settings dialog.

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

#include "settingsdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include "theme.h"

namespace {

QLabel* heading(const QString& text, QWidget* parent) {
  auto* l = new QLabel(text, parent);
  l->setObjectName(QStringLiteral("sectionTitle"));
  return l;
}

QLabel* explanation(const QString& text, QWidget* parent) {
  auto* l = new QLabel(text, parent);
  l->setObjectName(QStringLiteral("optionText"));
  l->setWordWrap(true);
  l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  return l;
}

QWidget* card(QWidget* parent) {
  auto* w = new QWidget(parent);
  w->setObjectName(QStringLiteral("settingsCard"));
  w->setAttribute(Qt::WA_StyledBackground);
  return w;
}

QToolButton* segment(const QString& text, QWidget* parent) {
  auto* b = new QToolButton(parent);
  b->setText(text);
  b->setCheckable(true);
  b->setAutoExclusive(true);
  b->setProperty("segment", true);
  return b;
}

QWidget* segmented(QToolButton* a, QToolButton* b, QWidget* parent) {
  auto* w = new QWidget(parent);
  w->setProperty("segmented", true);
  w->setAttribute(Qt::WA_StyledBackground);
  a->setParent(w);
  b->setParent(w);
  auto* l = new QHBoxLayout(w);
  l->setContentsMargins(2, 2, 2, 2);
  l->setSpacing(2);
  l->addWidget(a);
  l->addWidget(b);
  return w;
}

// A radio button with a line of explanation under it.
QWidget* option(QRadioButton* button, const QString& text, QWidget* parent) {
  auto* w = new QWidget(parent);
  auto* l = new QVBoxLayout(w);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(2);
  button->setParent(w);
  l->addWidget(button);
  if (!text.isEmpty()) {
    QLabel* e = explanation(text, w);
    e->setContentsMargins(24, 0, 0, 0);
    l->addWidget(e);
  }
  return w;
}

}  // namespace

SettingsDialog::SettingsDialog(const AppSettings& settings, QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(tr("Settings"));
  setObjectName(QStringLiteral("settingsDialog"));
  setMinimumWidth(660);

  // --- Simulator ---
  QWidget* simCard = card(this);
  standard_ = new QRadioButton(tr("Standard (recommended)"));
  bare_ = new QRadioButton(tr("Bare machine"));
  custom_ = new QRadioButton(tr("Custom"));
  mode_ = new QButtonGroup(this);
  mode_->addButton(standard_, 0);
  mode_->addButton(bare_, 1);
  mode_->addButton(custom_, 2);

  customBox_ = new QWidget(simCard);
  bareBox_ = new QCheckBox(tr("Bare machine"), customBox_);
  delayedBranches_ = new QCheckBox(tr("Delayed branches"), customBox_);
  delayedLoads_ = new QCheckBox(tr("Delayed loads"), customBox_);
  pseudo_ = new QCheckBox(tr("Pseudo-instructions"), customBox_);
  mappedIo_ = new QCheckBox(tr("Memory-mapped I/O"), customBox_);
  quiet_ =
      new QCheckBox(tr("Do not print exception messages (quiet)"), customBox_);
  auto* customLayout = new QGridLayout(customBox_);
  customLayout->setContentsMargins(24, 0, 0, 0);
  customLayout->setHorizontalSpacing(24);
  customLayout->addWidget(bareBox_, 0, 0);
  customLayout->addWidget(pseudo_, 0, 1);
  customLayout->addWidget(delayedBranches_, 1, 0);
  customLayout->addWidget(delayedLoads_, 1, 1);
  customLayout->addWidget(mappedIo_, 2, 0);
  customLayout->addWidget(quiet_, 2, 1);

  auto* simLayout = new QVBoxLayout(simCard);
  simLayout->setContentsMargins(14, 10, 14, 10);
  simLayout->setSpacing(6);
  simLayout->addWidget(option(
      standard_,
      tr("Pseudo-instructions such as li, la and move are allowed; branches "
         "and loads take effect at once (no delay slots). These are QtSpim's "
         "defaults."),
      simCard));
  simLayout->addWidget(option(
      bare_,
      tr("Only real MIPS instructions; branches and loads have delay slots, "
         "as on the hardware."),
      simCard));
  simLayout->addWidget(
      option(custom_, tr("Choose each option below."), simCard));
  simLayout->addWidget(customBox_);
  QLabel* course = explanation(
      tr("Use the settings your course requires: the same program can give "
         "different results with and without delay slots."),
      simCard);
  course->setObjectName(QStringLiteral("note"));
  simLayout->addWidget(course);

  // --- Exception handler ---
  QWidget* handlerCard = card(this);
  builtIn_ = new QRadioButton(tr("Built-in"));
  fromFile_ = new QRadioButton(tr("From a file…"));
  noHandler_ = new QRadioButton(tr("None"));
  handler_ = new QButtonGroup(this);
  handler_->addButton(builtIn_, 0);
  handler_->addButton(fromFile_, 1);
  handler_->addButton(noHandler_, 2);
  handlerPath_ = new QLineEdit(handlerCard);
  handlerPath_->setPlaceholderText(tr("Exception handler file"));
  chooseButton_ = new QToolButton(handlerCard);
  chooseButton_->setText(tr("Choose…"));
  auto* pathRow = new QHBoxLayout;
  pathRow->setContentsMargins(24, 0, 0, 0);
  pathRow->addWidget(handlerPath_, 1);
  pathRow->addWidget(chooseButton_);
  auto* handlerLayout = new QVBoxLayout(handlerCard);
  handlerLayout->setContentsMargins(14, 10, 14, 10);
  handlerLayout->setSpacing(4);
  handlerLayout->addWidget(option(
      builtIn_,
      tr("SPIM's standard handler. It also calls main and ends the program."),
      handlerCard));
  handlerLayout->addWidget(option(fromFile_, QString(), handlerCard));
  handlerLayout->addLayout(pathRow);
  handlerLayout->addWidget(
      option(noHandler_,
             tr("For programs that provide their own __start and handler."),
             handlerCard));

  // --- Editor and display ---
  QWidget* displayCard = card(this);
  fontSize_ = new QSpinBox(displayCard);
  fontSize_->setRange(10, 28);
  fontSize_->setSuffix(tr(" pt"));
  tab4_ = segment(tr("4 spaces"), nullptr);
  tab8_ = segment(tr("8 spaces"), nullptr);
  hex_ = segment(tr("Hexadecimal"), nullptr);
  dec_ = segment(tr("Decimal"), nullptr);
  showKernel_ = new QCheckBox(tr("Show kernel code and data"), displayCard);
  showKernel_->setToolTip(
      tr("Show the exception handler in Memory, and step into it"));
  language_ = new QComboBox(displayCard);
  language_->addItem(tr("Follow the system"), QString());
  language_->addItem(QStringLiteral("繁體中文"), QStringLiteral("zh_TW"));
  language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
  languageNote_ = explanation(tr("Takes effect the next time QtSpim ARM "
                                 "starts."),
                              displayCard);
  languageNote_->hide();

  auto* form = new QFormLayout(displayCard);
  form->setContentsMargins(14, 10, 14, 10);
  form->setHorizontalSpacing(16);
  form->setVerticalSpacing(6);
  form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  form->addRow(tr("Font size"), fontSize_);
  form->addRow(tr("Tab width"), segmented(tab4_, tab8_, displayCard));
  form->addRow(tr("Numbers in registers and memory"),
               segmented(hex_, dec_, displayCard));
  form->addRow(QString(), showKernel_);
  form->addRow(tr("Interface language"), language_);
  form->addRow(QString(), languageNote_);

  // --- Buttons ---
  auto* defaults = new QPushButton(tr("Restore Defaults"), this);
  auto* cancel = new QPushButton(tr("Cancel"), this);
  auto* save = new QPushButton(tr("Save"), this);
  save->setObjectName(QStringLiteral("primary"));
  save->setDefault(true);
  auto* buttons = new QHBoxLayout;
  buttons->addWidget(defaults);
  buttons->addStretch(1);
  buttons->addWidget(cancel);
  buttons->addWidget(save);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(20, 14, 20, 16);
  layout->setSpacing(6);
  layout->addWidget(heading(tr("Simulator"), this));
  layout->addWidget(simCard);
  layout->addSpacing(4);
  layout->addWidget(heading(tr("Exception handler"), this));
  layout->addWidget(handlerCard);
  layout->addSpacing(4);
  layout->addWidget(heading(tr("Editor and display"), this));
  layout->addWidget(displayCard);
  layout->addSpacing(8);
  layout->addLayout(buttons);

  connect(mode_, &QButtonGroup::idClicked, this, &SettingsDialog::applyMode);
  for (QCheckBox* c : {bareBox_, delayedBranches_, delayedLoads_, pseudo_,
                       mappedIo_, quiet_}) {
    connect(c, &QCheckBox::toggled, this, &SettingsDialog::updateMode);
  }
  connect(handler_, &QButtonGroup::idClicked, this, [this](int id) {
    handlerPath_->setEnabled(id == 1);
    chooseButton_->setEnabled(id == 1);
    if (id == 1 && handlerPath_->text().isEmpty()) chooseHandler();
  });
  connect(chooseButton_, &QToolButton::clicked, this,
          &SettingsDialog::chooseHandler);
  connect(language_, &QComboBox::currentIndexChanged, this, [this] {
    languageNote_->setVisible(language_->currentData().toString() !=
                              originalLanguage_);
  });
  connect(defaults, &QPushButton::clicked, this, [this] {
    AppSettings d;
    d.language = language_->currentData().toString();
    load(d);
  });
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
  connect(save, &QPushButton::clicked, this, &QDialog::accept);

  originalLanguage_ = settings.language;
  load(settings);
}

void SettingsDialog::load(const AppSettings& s) {
  {
    const QSignalBlocker b0(bareBox_), b1(delayedBranches_), b2(delayedLoads_),
        b3(pseudo_), b4(mappedIo_), b5(quiet_);
    bareBox_->setChecked(s.bareMachine);
    delayedBranches_->setChecked(s.delayedBranches);
    delayedLoads_->setChecked(s.delayedLoads);
    pseudo_->setChecked(s.acceptPseudoInsts);
    mappedIo_->setChecked(s.mappedIo);
    quiet_->setChecked(s.quiet);
  }
  const bool standard = !s.bareMachine && s.acceptPseudoInsts &&
                        !s.delayedBranches && !s.delayedLoads && !s.mappedIo &&
                        !s.quiet;
  const bool bare = s.bareMachine && !s.acceptPseudoInsts &&
                    s.delayedBranches && s.delayedLoads && !s.mappedIo;
  (standard ? standard_ : bare ? bare_ : custom_)->setChecked(true);
  customBox_->setEnabled(custom_->isChecked());

  if (!s.loadExceptionHandler) {
    noHandler_->setChecked(true);
  } else if (s.exceptionHandlerFile.isEmpty()) {
    builtIn_->setChecked(true);
  } else {
    fromFile_->setChecked(true);
  }
  handlerPath_->setText(s.exceptionHandlerFile);
  handlerPath_->setEnabled(fromFile_->isChecked());
  chooseButton_->setEnabled(fromFile_->isChecked());

  fontSize_->setValue(s.fontSize);
  (s.tabWidth == 4 ? tab4_ : tab8_)->setChecked(true);
  (s.base == 10 ? dec_ : hex_)->setChecked(true);
  originalBase_ = s.base;
  showKernel_->setChecked(s.showKernel);
  const int lang = language_->findData(s.language);
  language_->setCurrentIndex(lang < 0 ? 0 : lang);
}

void SettingsDialog::applyMode(int mode) {
  const QSignalBlocker b0(bareBox_), b1(delayedBranches_), b2(delayedLoads_),
      b3(pseudo_), b4(mappedIo_), b5(quiet_);
  if (mode == 0) {
    bareBox_->setChecked(false);
    pseudo_->setChecked(true);
    delayedBranches_->setChecked(false);
    delayedLoads_->setChecked(false);
    mappedIo_->setChecked(false);
    quiet_->setChecked(false);
  } else if (mode == 1) {
    bareBox_->setChecked(true);
    pseudo_->setChecked(false);
    delayedBranches_->setChecked(true);
    delayedLoads_->setChecked(true);
    mappedIo_->setChecked(false);
  }
  customBox_->setEnabled(mode == 2);
}

void SettingsDialog::updateMode() {
  // Changing an option by hand is the custom mode.
  custom_->setChecked(true);
  customBox_->setEnabled(true);
}

void SettingsDialog::chooseHandler() {
  const QString file = QFileDialog::getOpenFileName(
      this, tr("Exception Handler"), handlerPath_->text(),
      tr("Assembly files (*.s *.as *.asm);;All files (*)"));
  if (!file.isEmpty()) {
    handlerPath_->setText(file);
    fromFile_->setChecked(true);
    handlerPath_->setEnabled(true);
    chooseButton_->setEnabled(true);
  } else if (handlerPath_->text().isEmpty()) {
    builtIn_->setChecked(true);
    handlerPath_->setEnabled(false);
    chooseButton_->setEnabled(false);
  }
}

AppSettings SettingsDialog::settings() const {
  AppSettings s;
  s.bareMachine = bareBox_->isChecked();
  s.acceptPseudoInsts = pseudo_->isChecked();
  s.delayedBranches = delayedBranches_->isChecked();
  s.delayedLoads = delayedLoads_->isChecked();
  s.mappedIo = mappedIo_->isChecked();
  s.quiet = quiet_->isChecked();
  s.loadExceptionHandler = !noHandler_->isChecked();
  s.exceptionHandlerFile =
      fromFile_->isChecked() ? handlerPath_->text().trimmed() : QString();
  s.fontSize = fontSize_->value();
  s.tabWidth = tab4_->isChecked() ? 4 : 8;
  s.base = dec_->isChecked() ? 10 : (hex_->isChecked() ? 16 : originalBase_);
  s.showKernel = showKernel_->isChecked();
  s.language = language_->currentData().toString();
  return s;
}
