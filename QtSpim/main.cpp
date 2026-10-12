/* SPIM S20 MIPS simulator.
   Terminal interface for SPIM simulator.

   Copyright (c) 1990-2022, James R. Larus.
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

#include <clocale>

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QTranslator>

#include "mainwindow.h"
#include "theme.h"
#include "spimview.h"

static CommandLine parseCommandLine(QStringList args);

SpimView* Window = nullptr;  // the classic window, once it is shown
QApplication* App;

// Traditional Chinese when the system (or the setting) asks for it.
static void installTranslations(QApplication& app) {
  const QString language =
      QSettings().value(QStringLiteral("UI/Language")).toString();
  const QLocale locale =
      language.isEmpty() ? QLocale::system() : QLocale(language);
  const bool traditionalChinese =
      language != QLatin1String("en") &&
      locale.language() == QLocale::Chinese &&
      (locale.script() == QLocale::TraditionalChineseScript ||
       locale.territory() == QLocale::Taiwan ||
       locale.territory() == QLocale::HongKong ||
       locale.territory() == QLocale::Macau);
  if (!traditionalChinese) {
    return;
  }
  auto* ours = new QTranslator(&app);
  if (ours->load(QStringLiteral(":/translations/qtspim_zh_TW.qm"))) {
    app.installTranslator(ours);
  }
  // Qt's own strings (standard buttons and dialogs).
  auto* qt = new QTranslator(&app);
  if (qt->load(QLocale(QLocale::Chinese, QLocale::TraditionalChineseScript,
                       QLocale::Taiwan),
               QStringLiteral("qtbase"), QStringLiteral("_"),
               QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
    app.installTranslator(qt);
  }
}

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  setlocale(
      LC_NUMERIC,
      "C");  // Qt Linux has wrong default
             // https://doc.qt.io/qt-5/qcoreapplication.html#locale-settings
  QCoreApplication::setOrganizationName("LarusStone");
  QCoreApplication::setOrganizationDomain("larusstone.org");
  QCoreApplication::setApplicationName("QtSpim");
  QGuiApplication::setApplicationDisplayName(QStringLiteral("QtSpim ARM"));

  App = &a;

  // Initialize Spim
  //
  message_out.i = 1;
  console_out.i = 2;

  installTranslations(a);
  Theme::apply(a);

  MainWindow win;
  const CommandLine commandLine = parseCommandLine(a.arguments());
  win.start(commandLine);

  return a.exec();
}

static CommandLine parseCommandLine(QStringList args) {
  CommandLine result;
  for (int i = 1; i < args.length(); i++) {
#ifdef WIN32
    if (args[i][0] == '/') {
      args[i][0] = '-';
    }
#endif
    if ((args[i] == "-asm") || (args[i] == "-a")) {
      bare_machine = false;
      delayed_branches = false;
      delayed_loads = false;
    } else if ((args[i] == "-bare") || (args[i] == "-b")) {
      bare_machine = true;
      delayed_branches = true;
      delayed_loads = true;
      quiet = true;
    } else if ((args[i] == "-delayed_branches") || (args[i] == "-db")) {
      delayed_branches = true;
    } else if ((args[i] == "-delayed_loads") || (args[i] == "-dl")) {
      delayed_loads = true;
    } else if ((args[i] == "-exception") || (args[i] == "-e") ||
               (args[i] == "-trap") || (args[i] == "-t")) {
      result.setHandler = true;
      result.loadHandler = true;
    } else if ((args[i] == "-noexception") || (args[i] == "-ne") ||
               (args[i] == "-notrap") || (args[i] == "-nt")) {
      result.setHandler = true;
      result.loadHandler = false;
    } else if (((args[i] == "-exception_file") || (args[i] == "-ef") ||
                (args[i] == "-trap_file") || (args[i] == "-tf")) &&
               i + 1 < args.length()) {
      result.setHandler = true;
      result.loadHandler = true;
      result.handlerFile = args[++i];
    } else if ((args[i] == "-mapped_io") || (args[i] == "-mio")) {
      mapped_io = true;
    } else if ((args[i] == "-nomapped_io") || (args[i] == "-nmio")) {
      mapped_io = false;
    } else if ((args[i] == "-pseudo") || (args[i] == "-p")) {
      accept_pseudo_insts = true;
    } else if ((args[i] == "-nopseudo") || (args[i] == "-np")) {
      accept_pseudo_insts = false;
    } else if ((args[i] == "-quiet") || (args[i] == "-q")) {
      quiet = true;
    } else if ((args[i] == "-noquiet") || (args[i] == "-nq")) {
      quiet = false;
    } else if (((args[i] == "-stext") || (args[i] == "-st")) &&
               i + 1 < args.length()) {
      initial_text_size = args[++i].toInt();
    } else if (((args[i] == "-sdata") || (args[i] == "-sd")) &&
               i + 1 < args.length()) {
      initial_data_size = args[++i].toInt();
    } else if (((args[i] == "-ldata") || (args[i] == "-ld")) &&
               i + 1 < args.length()) {
      initial_data_limit = args[++i].toInt();
    } else if (((args[i] == "-sstack") || (args[i] == "-ss")) &&
               i + 1 < args.length()) {
      initial_stack_size = args[++i].toInt();
    } else if (((args[i] == "-lstack") || (args[i] == "-ls")) &&
               i + 1 < args.length()) {
      initial_stack_limit = args[++i].toInt();
    } else if (((args[i] == "-sktext") || (args[i] == "-skt")) &&
               i + 1 < args.length()) {
      initial_k_text_size = args[++i].toInt();
    } else if (((args[i] == "-skdata") || (args[i] == "-skd")) &&
               i + 1 < args.length()) {
      initial_k_data_size = args[++i].toInt();
    } else if (((args[i] == "-lkdata") || (args[i] == "-lkd")) &&
               i + 1 < args.length()) {
      initial_k_data_limit = args[++i].toInt();
    } else if ((args[i] == "-file") || (args[i] == "-f")) {
      result.files = args.mid(i + 1);
      return result;
    } else if (args[i].startsWith("-psn_")) {
      // Process serial number that older macOS versions pass to apps.
    } else if (args[i][0] != '-') {
      result.files = args.mid(i);
      return result;
    } else {
      QMessageBox::information(
          nullptr, QStringLiteral("QtSpim ARM"),
          QString("Unknown argument: %1 (ignored)\n\n"
                  "Usage: QtSpim\n"
                  "\t-bare\t\t\tBare machine (no pseudo-ops, delayed branches "
                  "and loads)\n"
                  "\t-asm\t\t\tExtended machine (pseudo-ops, no delayed "
                  "branches and loads) (default)\n"
                  "\t-delayed_branches\tExecute delayed branches\n"
                  "\t-delayed_loads\t\tExecute delayed loads\n"
                  "\t-exception\t\tLoad exception handler (default)\n"
                  "\t-noexception\t\tDo not load exception handler\n"
                  "\t-exception_file <file>\tSpecify exception handler in "
                  "place of default\n"
                  "\t-quiet\t\t\tDo not print warnings\n"
                  "\t-noquiet\t\tPrint warnings (default)\n"
                  "\t-mapped_io\t\tEnable memory-mapped IO\n"
                  "\t-nomapped_io\t\tDo not enable memory-mapped IO (default)\n"
                  "\t-file <file> ...\tAssembly code file(s)\n"
                  "\t\n"
                  "\tIf argument does not start with a '-', it and all "
                  "subsequent arguments are file names.\n")
              .arg(args[i]));
    }
  }
  return result;
}
