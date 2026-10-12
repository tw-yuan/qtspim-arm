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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QSpinBox;
class QToolButton;

struct AppSettings {
  // Simulator (the SPIM globals and the exception handler).
  bool bareMachine = false;
  bool acceptPseudoInsts = true;
  bool delayedBranches = false;
  bool delayedLoads = false;
  bool mappedIo = false;
  bool quiet = false;
  bool loadExceptionHandler = true;
  QString exceptionHandlerFile;  // empty = built-in
  // Editor and display.
  int fontSize = 13;
  int tabWidth = 8;
  int base = 16;
  bool showKernel = false;
  QString language;  // "" = follow the system, "zh_TW", "en"

  bool sameSimulator(const AppSettings& o) const {
    return bareMachine == o.bareMachine &&
           acceptPseudoInsts == o.acceptPseudoInsts &&
           delayedBranches == o.delayedBranches &&
           delayedLoads == o.delayedLoads && mappedIo == o.mappedIo &&
           quiet == o.quiet && loadExceptionHandler == o.loadExceptionHandler &&
           exceptionHandlerFile == o.exceptionHandlerFile;
  }
};

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  SettingsDialog(const AppSettings& settings, QWidget* parent = nullptr);
  AppSettings settings() const;

 private:
  void load(const AppSettings& s);
  void updateMode();
  void applyMode(int mode);
  void chooseHandler();

  QButtonGroup* mode_;
  QRadioButton* standard_;
  QRadioButton* bare_;
  QRadioButton* custom_;
  QCheckBox* bareBox_;
  QCheckBox* delayedBranches_;
  QCheckBox* delayedLoads_;
  QCheckBox* pseudo_;
  QCheckBox* mappedIo_;
  QCheckBox* quiet_;
  QWidget* customBox_;
  QButtonGroup* handler_;
  QRadioButton* builtIn_;
  QRadioButton* fromFile_;
  QRadioButton* noHandler_;
  QLineEdit* handlerPath_;
  QToolButton* chooseButton_;
  QSpinBox* fontSize_;
  QToolButton* tab4_;
  QToolButton* tab8_;
  QToolButton* hex_;
  QToolButton* dec_;
  QCheckBox* showKernel_;
  QComboBox* language_;
  QLabel* languageNote_;
  int originalBase_ = 16;
  QString originalLanguage_;
};

#endif  // SETTINGSDIALOG_H
