/* SPIM S20 MIPS simulator.
   The main window of the new interface.

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>

#include "settingsdialog.h"

class CodeEditor;
class ConsolePanel;
class FindBar;
class MemoryPanel;
class ProblemsPanel;
class QAction;
class QLabel;
class QMenu;
class QSplitter;
class QStackedWidget;
class QToolButton;
class RegisterPanel;
class Simulator;
class SpimView;
class WelcomePage;

// Options from the command line (see parseCommandLine in main.cpp).
struct CommandLine {
  QStringList files;
  bool setHandler = false;
  bool loadHandler = true;
  QString handlerFile;  // empty = keep
};

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  // Shows the interface the user chose last time and opens |commandLine|'s
  // files.
  void start(const CommandLine& commandLine);
  bool maybeSave();

 protected:
  void closeEvent(QCloseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  enum BottomTab { ConsoleTab, MemoryTab, ProblemsTab };

  // Building the window.
  void createActions();
  void createMenus();
  void createToolBar();
  QWidget* createEditorArea();
  QWidget* createBottomPanel();
  QWidget* createStatusBar();

  // Documents.
  void newFile();
  void open();
  bool openFile(const QString& path);
  void openExample(const QString& file);
  bool save();
  bool saveAs();
  bool saveTo(const QString& path);
  void closeDocument();
  void setDocument(const QString& text, const QString& path,
                   const QString& name);
  void addRecentFile(const QString& path);
  void updateRecentFiles();
  QString documentName() const;

  // Simulation.
  bool assemble();
  bool needsAssembly() const;
  void run();
  void step();
  void resume();
  void reset();
  void runAnyway();
  void onStateChanged();
  void onMachineChanged();
  void onProgramLoaded();
  void onInputRequested();
  void updateActions();
  void updateStatus();
  void updateEditorHeader();
  void updateExecutionLine();
  void updateProblems();
  QString pauseDescription() const;

  // View.
  void showBottomTab(int tab);
  void setBottomCollapsed(bool collapsed);
  void setRegistersVisible(bool visible);
  void restoreLayout();
  void setBase(int base);

  // Dialogs.
  void showSettings();
  void showRunParameters();
  void showSymbols();
  void exportLog();
  void print();
  void showHelp();
  void showAbout();
  QString logText(bool registers, bool code, bool data, bool stack,
                  bool console) const;

  // Settings.
  void readSettings();
  void writeSettings();
  void applySettings(const AppSettings& settings);
  void applySimulatorSettings();

  // The classic QtSpim window.
  void showClassic(bool atStartup, const CommandLine& commandLine);
  void showNew();

  Simulator* sim_;
  AppSettings settings_;

  // Document.
  QString filePath_;      // empty = not saved yet
  QString untitledName_;  // name of an unsaved document
  bool hasDocument_ = false;
  bool settingDocument_ = false;
  QStringList recentFiles_;
  int recentFilesLength_ = 8;

  // Widgets.
  QStackedWidget* pages_;
  WelcomePage* welcome_;
  QSplitter* hsplit_;
  QSplitter* vsplit_;
  CodeEditor* editor_;
  FindBar* findBar_;
  QLabel* fileLabel_;
  QLabel* assembleLabel_;
  QWidget* bottomPanel_;
  QStackedWidget* bottomStack_;
  QStackedWidget* bottomTools_;
  QToolButton* tabButtons_[3];
  QToolButton* collapseButton_;
  ConsolePanel* console_;
  MemoryPanel* memory_;
  ProblemsPanel* problems_;
  RegisterPanel* registers_;
  QWidget* statusBar_;
  QLabel* statusDot_;
  QLabel* statusText_;
  QLabel* pcLabel_;
  QToolButton* pseudoPill_;
  QToolButton* delayPill_;
  QToolButton* ioPill_;
  bool bottomCollapsed_ = false;
  QList<int> bottomSizes_;
  bool closeAfterStop_ = false;

  SpimView* classic_ = nullptr;
  QStringList startupFiles_;  // given on the command line

  // Actions.
  QAction* newAct_;
  QAction* openAct_;
  QAction* saveAct_;
  QAction* saveAsAct_;
  QAction* closeAct_;
  QAction* exportLogAct_;
  QAction* printAct_;
  QAction* quitAct_;
  QMenu* recentMenu_;
  QMenu* examplesMenu_;
  QAction* undoAct_;
  QAction* redoAct_;
  QAction* cutAct_;
  QAction* copyAct_;
  QAction* pasteAct_;
  QAction* selectAllAct_;
  QAction* findAct_;
  QAction* replaceAct_;
  QAction* findNextAct_;
  QAction* findPreviousAct_;
  QAction* goToLineAct_;
  QAction* commentAct_;
  QAction* runAct_;
  QAction* assembleAct_;
  QAction* stepAct_;
  QAction* continueAct_;
  QAction* pauseAct_;
  QAction* stopAct_;
  QAction* resetAct_;
  QAction* toggleBreakpointAct_;
  QAction* clearBreakpointsAct_;
  QAction* clearRegistersAct_;
  QAction* runParametersAct_;
  QAction* symbolsAct_;
  QAction* settingsAct_;
  QAction* consoleAct_;
  QAction* memoryAct_;
  QAction* problemsAct_;
  QAction* registersAct_;
  QAction* bottomAct_;
  QAction* hexAct_;
  QAction* decAct_;
  QAction* binAct_;
  QAction* restoreLayoutAct_;
  QAction* classicAct_;
  QAction* helpAct_;
  QAction* aboutAct_;
};

#endif  // MAINWINDOW_H
