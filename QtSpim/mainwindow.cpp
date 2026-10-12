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

#include "mainwindow.h"

#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringConverter>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "codeeditor.h"
#include "consolepanel.h"
#include "findbar.h"
#include "memorypanel.h"
#include "mipsinfo.h"
#include "problemspanel.h"
#include "registerpanel.h"
#include "simulator.h"
#include "theme.h"
#include "welcomepage.h"

// The classic window; it also brings the simulator core's headers.
#include "spimview.h"

// Set by CMakeLists.txt.
#ifndef QTSPIM_ARM_VERSION
#define QTSPIM_ARM_VERSION "9.1.24-arm (development build)"
#endif

namespace {

// The value the original QtSpim stores for its built-in exception handler.
const char kStandardHandler[] = "<<SPIM Exception Handler>>";

const char kTemplate[] =
    "# A new program. Write your code after main: and press Run.\n"
    "\n"
    "\t.data\n"
    "msg:\t.asciiz\t\"Hello\\n\"\n"
    "\n"
    "\t.text\n"
    "\t.globl\tmain\n"
    "main:\n"
    "\tli\t$v0, 4\t\t# print_string(msg)\n"
    "\tla\t$a0, msg\n"
    "\tsyscall\n"
    "\n"
    "\tjr\t$ra\t\t# return to the startup code, which ends the program\n";

QString fileFilter() {
  return MainWindow::tr("Assembly files (*.s *.as *.asm);;All files (*)");
}

QPixmap dot(const QColor& color) {
  const qreal dpr = 2.0;
  QPixmap pm(QSize(10, 10) * dpr);
  pm.setDevicePixelRatio(dpr);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawEllipse(QRectF(1, 1, 8, 8));
  return pm;
}

QToolButton* tabButton(const QString& icon, QWidget* parent) {
  auto* b = new QToolButton(parent);
  b->setCheckable(true);
  b->setAutoExclusive(true);
  b->setProperty("tabButton", true);
  b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  b->setIcon(Theme::icon(icon, Theme::kText2, 16));
  b->setIconSize(QSize(16, 16));
  b->setCursor(Qt::PointingHandCursor);
  return b;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), sim_(Simulator::instance()) {
  setObjectName(QStringLiteral("mainWindow"));
  setWindowTitle(QStringLiteral("QtSpim ARM"));
  setUnifiedTitleAndToolBarOnMac(true);
  setAcceptDrops(true);

  createActions();

  welcome_ = new WelcomePage(this);
  QWidget* editorArea = createEditorArea();
  bottomPanel_ = createBottomPanel();
  registers_ = new RegisterPanel(this);

  vsplit_ = new QSplitter(Qt::Vertical);
  vsplit_->addWidget(editorArea);
  vsplit_->addWidget(bottomPanel_);
  vsplit_->setStretchFactor(0, 3);
  vsplit_->setStretchFactor(1, 2);
  vsplit_->setChildrenCollapsible(false);
  hsplit_ = new QSplitter(Qt::Horizontal);
  hsplit_->addWidget(vsplit_);
  hsplit_->addWidget(registers_);
  hsplit_->setStretchFactor(0, 1);
  hsplit_->setStretchFactor(1, 0);
  hsplit_->setChildrenCollapsible(false);
  registers_->setMinimumWidth(240);

  auto* workspace = new QWidget(this);
  auto* workspaceLayout = new QVBoxLayout(workspace);
  workspaceLayout->setContentsMargins(0, 0, 0, 0);
  workspaceLayout->setSpacing(0);
  workspaceLayout->addWidget(hsplit_, 1);
  statusBar_ = createStatusBar();
  workspaceLayout->addWidget(statusBar_);

  pages_ = new QStackedWidget(this);
  pages_->addWidget(welcome_);
  pages_->addWidget(workspace);
  setCentralWidget(pages_);

  createMenus();
  createToolBar();
  restoreLayout();
  readSettings();

  // Simulator.
  connect(sim_, &Simulator::stateChanged, this, &MainWindow::onStateChanged);
  connect(sim_, &Simulator::machineChanged, this,
          &MainWindow::onMachineChanged);
  connect(sim_, &Simulator::programLoaded, this, &MainWindow::onProgramLoaded);
  connect(sim_, &Simulator::aboutToExecute, this, [this] {
    if (sim_->classicMode()) return;
    registers_->beginExecution();
  });
  connect(sim_, &Simulator::consoleOutput, this, [this](const QString& text) {
    if (!sim_->classicMode()) console_->appendOutput(text);
  });
  connect(sim_, &Simulator::simulatorMessage, this,
          [this](const QString& text, bool error) {
            if (!sim_->classicMode()) console_->appendMessage(text, error);
          });
  connect(sim_, &Simulator::consoleCleared, this, [this] {
    if (!sim_->classicMode()) console_->clear();
  });
  connect(sim_, &Simulator::inputRequested, this,
          &MainWindow::onInputRequested);
  connect(console_, &ConsolePanel::inputSubmitted, sim_,
          &Simulator::provideInput);

  // Editor.
  connect(editor_->document(), &QTextDocument::modificationChanged, this,
          &QWidget::setWindowModified);
  connect(editor_, &QPlainTextEdit::textChanged, this, [this] {
    if (settingDocument_ || sim_->isBusy()) return;
    if (sim_->state() != Simulator::State::NoProgram &&
        sim_->state() != Simulator::State::NeedsAssembly &&
        editor_->toPlainText() != sim_->assembledSource()) {
      sim_->invalidate();
    }
  });
  connect(editor_, &CodeEditor::breakpointsChanged, this, [this] {
    sim_->setBreakpointLines(editor_->breakpointLines());
    memory_->refresh();
  });
  connect(editor_, &CodeEditor::filesDropped, this,
          [this](const QStringList& files) {
            if (!files.isEmpty() && maybeSave()) openFile(files.first());
          });

  // Welcome page.
  connect(welcome_, &WelcomePage::newRequested, this, &MainWindow::newFile);
  connect(welcome_, &WelcomePage::openRequested, this, &MainWindow::open);
  connect(welcome_, &WelcomePage::exampleRequested, this,
          &MainWindow::openExample);
  connect(welcome_, &WelcomePage::recentRequested, this,
          [this](const QString& path) { openFile(path); });

  // Panels.
  connect(problems_, &ProblemsPanel::lineActivated, editor_,
          &CodeEditor::goToLine);
  connect(problems_, &ProblemsPanel::runAnywayRequested, this,
          &MainWindow::runAnyway);
  connect(registers_, &RegisterPanel::baseChanged, this, [this](int base) {
    settings_.base = base;
    memory_->setBase(base);
    hexAct_->setChecked(base == 16);
    decAct_->setChecked(base == 10);
    binAct_->setChecked(base == 2);
  });
  connect(registers_, &RegisterPanel::valueEdited, this, [this] {
    memory_->refresh();
    updateStatus();
    updateExecutionLine();
  });
  connect(memory_, &MemoryPanel::valueEdited, this,
          [this] { registers_->refresh(); });
  connect(memory_, &MemoryPanel::message, this,
          [this](const QString& text) { statusText_->setText(text); });

  pages_->setCurrentWidget(welcome_);
  onStateChanged();
  // Files opened from the Finder.
  qApp->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::FileOpen) {
    const QString file = static_cast<QFileOpenEvent*>(event)->file();
    // The classic window, like the original QtSpim, does not open files
    // from the Finder.
    if (!file.isEmpty() && !sim_->isBusy() && !sim_->classicMode() &&
        maybeSave()) {
      openFile(file);
    }
    return true;
  }
  return QMainWindow::eventFilter(watched, event);
}

MainWindow::~MainWindow() = default;

// --- Building the window ---

void MainWindow::createActions() {
  auto action = [this](const QString& text, const QString& icon = QString()) {
    auto* a = new QAction(text, this);
    if (!icon.isEmpty()) a->setIcon(Theme::icon(icon, Theme::kText, 18));
    return a;
  };
  auto tip = [](QAction* a, const QString& text) {
    const QString keys = a->shortcut().toString(QKeySequence::NativeText);
    a->setToolTip(keys.isEmpty() ? text
                                 : QStringLiteral("%1 (%2)").arg(text, keys));
    a->setStatusTip(text);
  };

  newAct_ = action(tr("New"), QStringLiteral("file-plus"));
  newAct_->setShortcut(QKeySequence::New);
  tip(newAct_, tr("Start a new program"));
  connect(newAct_, &QAction::triggered, this, &MainWindow::newFile);

  openAct_ = action(tr("Open…"), QStringLiteral("folder-open"));
  openAct_->setIconText(tr("Open"));
  openAct_->setShortcut(QKeySequence::Open);
  tip(openAct_, tr("Open an assembly file"));
  connect(openAct_, &QAction::triggered, this, &MainWindow::open);

  saveAct_ = action(tr("Save"), QStringLiteral("save"));
  saveAct_->setShortcut(QKeySequence::Save);
  tip(saveAct_, tr("Save the program"));
  connect(saveAct_, &QAction::triggered, this, &MainWindow::save);

  saveAsAct_ = action(tr("Save As…"));
  saveAsAct_->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAct_, &QAction::triggered, this, &MainWindow::saveAs);

  closeAct_ = action(tr("Close"));
  closeAct_->setShortcut(QKeySequence::Close);
  connect(closeAct_, &QAction::triggered, this, &MainWindow::closeDocument);

  exportLogAct_ = action(tr("Export Log…"), QStringLiteral("download"));
  connect(exportLogAct_, &QAction::triggered, this, &MainWindow::exportLog);

  printAct_ = action(tr("Print…"), QStringLiteral("printer"));
  printAct_->setShortcut(QKeySequence::Print);
  connect(printAct_, &QAction::triggered, this, &MainWindow::print);

  quitAct_ = action(tr("Quit QtSpim ARM"));
  quitAct_->setShortcut(QKeySequence::Quit);
  quitAct_->setMenuRole(QAction::QuitRole);
  connect(quitAct_, &QAction::triggered, this, &QWidget::close);

  // Edit
  undoAct_ = action(tr("Undo"));
  undoAct_->setShortcut(QKeySequence::Undo);
  redoAct_ = action(tr("Redo"));
  redoAct_->setShortcut(QKeySequence::Redo);
  cutAct_ = action(tr("Cut"));
  cutAct_->setShortcut(QKeySequence::Cut);
  copyAct_ = action(tr("Copy"));
  copyAct_->setShortcut(QKeySequence::Copy);
  pasteAct_ = action(tr("Paste"));
  pasteAct_->setShortcut(QKeySequence::Paste);
  selectAllAct_ = action(tr("Select All"));
  selectAllAct_->setShortcut(QKeySequence::SelectAll);
  findAct_ = action(tr("Find…"), QStringLiteral("search"));
  findAct_->setShortcut(QKeySequence::Find);
  replaceAct_ = action(tr("Find and Replace…"));
  replaceAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F));
  findNextAct_ = action(tr("Find Next"));
  findNextAct_->setShortcut(QKeySequence::FindNext);
  findPreviousAct_ = action(tr("Find Previous"));
  findPreviousAct_->setShortcut(QKeySequence::FindPrevious);
  goToLineAct_ = action(tr("Go to Line…"));
  goToLineAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
  commentAct_ = action(tr("Toggle Comment"));
  commentAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));

  // Simulator
  runAct_ = action(tr("Run"), QStringLiteral("play"));
  runAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  runAct_->setIcon(Theme::icon(QStringLiteral("play"), Qt::white, 18));
  tip(runAct_, tr("Assemble and run the program from the start"));
  connect(runAct_, &QAction::triggered, this, &MainWindow::run);

  assembleAct_ = action(tr("Assemble"), QStringLiteral("hammer"));
  assembleAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
  tip(assembleAct_, tr("Check the program and load it without running it"));
  connect(assembleAct_, &QAction::triggered, this, [this] { assemble(); });

  stepAct_ = action(tr("Step"), QStringLiteral("arrow-down-to-line"));
  stepAct_->setShortcuts(
      {QKeySequence(Qt::Key_F10), QKeySequence(Qt::CTRL | Qt::Key_Apostrophe)});
  stepAct_->setIcon(Theme::icon(QStringLiteral("arrow-down-to-line"),
                                Theme::kAccentText, 18));
  tip(stepAct_, tr("Execute one instruction"));
  connect(stepAct_, &QAction::triggered, this, &MainWindow::step);

  continueAct_ = action(tr("Continue"), QStringLiteral("step-forward"));
  continueAct_->setShortcuts(
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R), QKeySequence(Qt::Key_F5)});
  tip(continueAct_, tr("Run until the next breakpoint or the end"));
  connect(continueAct_, &QAction::triggered, this, &MainWindow::resume);

  pauseAct_ = action(tr("Pause"), QStringLiteral("pause"));
  tip(pauseAct_, tr("Pause the running program"));
  connect(pauseAct_, &QAction::triggered, sim_, &Simulator::pause);

  stopAct_ = action(tr("Stop"), QStringLiteral("square"));
  stopAct_->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Period),
                          QKeySequence(Qt::SHIFT | Qt::Key_F5)});
  stopAct_->setIcon(
      Theme::icon(QStringLiteral("square"), Theme::kErrorText, 18));
  tip(stopAct_, tr("End the program"));
  connect(stopAct_, &QAction::triggered, sim_, &Simulator::stop);

  resetAct_ = action(tr("Reset"), QStringLiteral("rotate-ccw"));
  resetAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
  tip(resetAct_, tr("Load the program again and clear registers and memory"));
  connect(resetAct_, &QAction::triggered, this, &MainWindow::reset);

  toggleBreakpointAct_ =
      action(tr("Toggle Breakpoint"), QStringLiteral("circle-dot"));
  toggleBreakpointAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backslash));
  connect(toggleBreakpointAct_, &QAction::triggered, this,
          [this] { editor_->toggleBreakpoint(editor_->currentLineNumber()); });
  clearBreakpointsAct_ = action(tr("Remove All Breakpoints"));
  connect(clearBreakpointsAct_, &QAction::triggered, this,
          [this] { editor_->clearBreakpoints(); });

  clearRegistersAct_ = action(tr("Clear Registers"));
  connect(clearRegistersAct_, &QAction::triggered, this, [this] {
    registers_->beginExecution();
    sim_->resetRegisters();
  });
  runParametersAct_ = action(tr("Run Parameters…"));
  connect(runParametersAct_, &QAction::triggered, this,
          &MainWindow::showRunParameters);
  symbolsAct_ = action(tr("Show Symbols…"));
  connect(symbolsAct_, &QAction::triggered, this, &MainWindow::showSymbols);

  settingsAct_ = action(tr("Settings…"), QStringLiteral("sliders-horizontal"));
  settingsAct_->setIconText(tr("Settings"));
  settingsAct_->setShortcut(QKeySequence::Preferences);
  if (settingsAct_->shortcut().isEmpty()) {
    settingsAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
  }
  settingsAct_->setMenuRole(QAction::PreferencesRole);
  tip(settingsAct_, tr("Simulator, exception handler, editor and language"));
  connect(settingsAct_, &QAction::triggered, this, &MainWindow::showSettings);

  // View
  consoleAct_ = action(tr("Console"), QStringLiteral("terminal"));
  consoleAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
  connect(consoleAct_, &QAction::triggered, this,
          [this] { showBottomTab(ConsoleTab); });
  memoryAct_ = action(tr("Memory"), QStringLiteral("table-2"));
  memoryAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
  connect(memoryAct_, &QAction::triggered, this,
          [this] { showBottomTab(MemoryTab); });
  problemsAct_ = action(tr("Problems"), QStringLiteral("triangle-alert"));
  problemsAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
  connect(problemsAct_, &QAction::triggered, this,
          [this] { showBottomTab(ProblemsTab); });
  registersAct_ = action(tr("Registers"), QStringLiteral("cpu"));
  registersAct_->setCheckable(true);
  registersAct_->setChecked(true);
  registersAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_0));
  connect(registersAct_, &QAction::toggled, this,
          &MainWindow::setRegistersVisible);
  bottomAct_ = action(tr("Bottom Panel"), QStringLiteral("panel-bottom"));
  bottomAct_->setCheckable(true);
  bottomAct_->setChecked(true);
  bottomAct_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
  connect(bottomAct_, &QAction::toggled, this,
          [this](bool on) { setBottomCollapsed(!on); });
  hexAct_ = action(tr("Hexadecimal"));
  decAct_ = action(tr("Decimal"));
  binAct_ = action(tr("Binary"));
  auto* baseGroup = new QActionGroup(this);
  for (QAction* a : {hexAct_, decAct_, binAct_}) {
    a->setCheckable(true);
    baseGroup->addAction(a);
  }
  connect(hexAct_, &QAction::triggered, this, [this] { setBase(16); });
  connect(decAct_, &QAction::triggered, this, [this] { setBase(10); });
  connect(binAct_, &QAction::triggered, this, [this] { setBase(2); });
  restoreLayoutAct_ = action(tr("Restore Layout"));
  connect(restoreLayoutAct_, &QAction::triggered, this,
          &MainWindow::restoreLayout);
  classicAct_ = action(tr("Classic Interface"), QStringLiteral("history"));
  classicAct_->setIconText(tr("Classic"));
  tip(classicAct_, tr("Switch to the original QtSpim interface"));
  connect(classicAct_, &QAction::triggered, this,
          [this] { showClassic(false, CommandLine()); });

  // Help
  helpAct_ = action(tr("QtSpim Help"), QStringLiteral("circle-help"));
  helpAct_->setIconText(tr("Help"));
  helpAct_->setShortcut(QKeySequence::HelpContents);
  tip(helpAct_, tr("Open the SPIM manual"));
  connect(helpAct_, &QAction::triggered, this, &MainWindow::showHelp);
  aboutAct_ = action(tr("About QtSpim ARM"));
  aboutAct_->setMenuRole(QAction::AboutRole);
  connect(aboutAct_, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createMenus() {
  QMenu* file = menuBar()->addMenu(tr("&File"));
  file->addAction(newAct_);
  file->addAction(openAct_);
  recentMenu_ = file->addMenu(tr("Open Recent"));
  examplesMenu_ = file->addMenu(tr("Examples"));
  for (const WelcomePage::Example& e : WelcomePage::examples()) {
    QAction* a = examplesMenu_->addAction(e.title);
    const QString name = e.file;
    connect(a, &QAction::triggered, this, [this, name] { openExample(name); });
  }
  file->addSeparator();
  file->addAction(saveAct_);
  file->addAction(saveAsAct_);
  file->addAction(closeAct_);
  file->addSeparator();
  file->addAction(exportLogAct_);
  file->addAction(printAct_);
  file->addSeparator();
  file->addAction(quitAct_);

  QMenu* edit = menuBar()->addMenu(tr("&Edit"));
  edit->addAction(undoAct_);
  edit->addAction(redoAct_);
  edit->addSeparator();
  edit->addAction(cutAct_);
  edit->addAction(copyAct_);
  edit->addAction(pasteAct_);
  edit->addAction(selectAllAct_);
  edit->addSeparator();
  edit->addAction(findAct_);
  edit->addAction(replaceAct_);
  edit->addAction(findNextAct_);
  edit->addAction(findPreviousAct_);
  edit->addSeparator();
  edit->addAction(goToLineAct_);
  edit->addAction(commentAct_);

  QMenu* simulator = menuBar()->addMenu(tr("&Simulator"));
  simulator->addAction(runAct_);
  simulator->addAction(assembleAct_);
  simulator->addAction(stepAct_);
  simulator->addAction(continueAct_);
  simulator->addAction(pauseAct_);
  simulator->addAction(stopAct_);
  simulator->addAction(resetAct_);
  simulator->addSeparator();
  simulator->addAction(toggleBreakpointAct_);
  simulator->addAction(clearBreakpointsAct_);
  simulator->addSeparator();
  simulator->addAction(clearRegistersAct_);
  simulator->addAction(runParametersAct_);
  simulator->addAction(symbolsAct_);
  simulator->addSeparator();
  simulator->addAction(settingsAct_);

  QMenu* view = menuBar()->addMenu(tr("&View"));
  view->addAction(consoleAct_);
  view->addAction(memoryAct_);
  view->addAction(problemsAct_);
  view->addSeparator();
  view->addAction(registersAct_);
  view->addAction(bottomAct_);
  QMenu* numbers = view->addMenu(tr("Number Format"));
  numbers->addAction(hexAct_);
  numbers->addAction(decAct_);
  numbers->addAction(binAct_);
  view->addSeparator();
  view->addAction(restoreLayoutAct_);
  view->addAction(classicAct_);

  QMenu* help = menuBar()->addMenu(tr("&Help"));
  help->addAction(helpAct_);
  help->addAction(aboutAct_);

  // The editing actions work on the editor.
  connect(undoAct_, &QAction::triggered, this, [this] { editor_->undo(); });
  connect(redoAct_, &QAction::triggered, this, [this] { editor_->redo(); });
  connect(cutAct_, &QAction::triggered, this, [this] { editor_->cut(); });
  connect(copyAct_, &QAction::triggered, this, [this] {
    QWidget* w = QApplication::focusWidget();
    if (auto* text = qobject_cast<QPlainTextEdit*>(w)) {
      text->copy();
    } else if (auto* line = qobject_cast<QLineEdit*>(w)) {
      line->copy();
    } else {
      editor_->copy();
    }
  });
  connect(pasteAct_, &QAction::triggered, this, [this] {
    QWidget* w = QApplication::focusWidget();
    if (auto* line = qobject_cast<QLineEdit*>(w)) {
      line->paste();
    } else {
      editor_->paste();
    }
  });
  connect(selectAllAct_, &QAction::triggered, this, [this] {
    QWidget* w = QApplication::focusWidget();
    if (auto* text = qobject_cast<QPlainTextEdit*>(w)) {
      text->selectAll();
    } else if (auto* line = qobject_cast<QLineEdit*>(w)) {
      line->selectAll();
    } else {
      editor_->selectAll();
    }
  });
  connect(findAct_, &QAction::triggered, this,
          [this] { findBar_->showFind(); });
  connect(replaceAct_, &QAction::triggered, this,
          [this] { findBar_->showReplace(); });
  connect(findNextAct_, &QAction::triggered, this,
          [this] { findBar_->findNext(); });
  connect(findPreviousAct_, &QAction::triggered, this,
          [this] { findBar_->findPrevious(); });
  connect(goToLineAct_, &QAction::triggered, this, [this] {
    bool ok = false;
    const int line = QInputDialog::getInt(
        this, tr("Go to Line"), tr("Line number:"),
        editor_->currentLineNumber(), 1, editor_->blockCount(), 1, &ok);
    if (ok) editor_->goToLine(line);
  });
  connect(commentAct_, &QAction::triggered, this,
          [this] { editor_->toggleComment(); });
  connect(editor_, &QPlainTextEdit::undoAvailable, undoAct_,
          &QAction::setEnabled);
  connect(editor_, &QPlainTextEdit::redoAvailable, redoAct_,
          &QAction::setEnabled);
  undoAct_->setEnabled(false);
  redoAct_->setEnabled(false);
}

void MainWindow::createToolBar() {
  QToolBar* bar = addToolBar(tr("Toolbar"));
  bar->setObjectName(QStringLiteral("mainToolBar"));
  bar->setMovable(false);
  bar->setFloatable(false);
  bar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  bar->setIconSize(QSize(18, 18));
  bar->setContextMenuPolicy(Qt::PreventContextMenu);
  bar->addAction(newAct_);
  bar->addAction(openAct_);
  bar->addAction(saveAct_);
  bar->addSeparator();
  bar->addAction(runAct_);
  bar->addAction(stepAct_);
  bar->addAction(continueAct_);
  bar->addAction(stopAct_);
  bar->addAction(resetAct_);
  auto* spacer = new QWidget(bar);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  bar->addWidget(spacer);
  bar->addAction(classicAct_);
  bar->addAction(settingsAct_);
  bar->addAction(helpAct_);
  bar->widgetForAction(runAct_)->setObjectName(QStringLiteral("primaryButton"));
  bar->widgetForAction(stepAct_)->setObjectName(QStringLiteral("stepButton"));
  bar->widgetForAction(stopAct_)->setObjectName(QStringLiteral("stopButton"));
  for (QAction* a : bar->actions()) {
    if (QWidget* w = bar->widgetForAction(a)) {
      w->setAccessibleName(a->iconText());
      w->setCursor(Qt::PointingHandCursor);
    }
  }
}

QWidget* MainWindow::createEditorArea() {
  auto* area = new QWidget(this);
  auto* header = new QWidget(area);
  header->setObjectName(QStringLiteral("editorHeader"));
  header->setAttribute(Qt::WA_StyledBackground);
  fileLabel_ = new QLabel(header);
  fileLabel_->setObjectName(QStringLiteral("panelTitle"));
  assembleLabel_ = new QLabel(header);
  assembleLabel_->setObjectName(QStringLiteral("hint"));
  auto* headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(14, 7, 14, 7);
  headerLayout->addWidget(fileLabel_);
  headerLayout->addStretch(1);
  headerLayout->addWidget(assembleLabel_);

  editor_ = new CodeEditor(area);
  findBar_ = new FindBar(editor_, area);
  auto* layout = new QVBoxLayout(area);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(header);
  layout->addWidget(findBar_);
  layout->addWidget(editor_, 1);
  return area;
}

QWidget* MainWindow::createBottomPanel() {
  auto* panel = new QWidget(this);
  panel->setObjectName(QStringLiteral("bottomPanel"));
  panel->setAttribute(Qt::WA_StyledBackground);

  console_ = new ConsolePanel(panel);
  memory_ = new MemoryPanel(panel);
  problems_ = new ProblemsPanel(panel);
  bottomStack_ = new QStackedWidget(panel);
  bottomStack_->addWidget(console_);
  bottomStack_->addWidget(memory_);
  bottomStack_->addWidget(problems_);

  auto* header = new QWidget(panel);
  header->setObjectName(QStringLiteral("panelHeader"));
  tabButtons_[ConsoleTab] = tabButton(QStringLiteral("terminal"), header);
  tabButtons_[MemoryTab] = tabButton(QStringLiteral("table-2"), header);
  tabButtons_[ProblemsTab] =
      tabButton(QStringLiteral("triangle-alert"), header);
  tabButtons_[ConsoleTab]->setText(tr("Console"));
  tabButtons_[MemoryTab]->setText(tr("Memory"));
  tabButtons_[ProblemsTab]->setText(tr("Problems"));
  for (int i = 0; i < 3; ++i) {
    connect(tabButtons_[i], &QToolButton::clicked, this,
            [this, i] { showBottomTab(i); });
  }

  // Tools on the right that belong to the current tab.
  bottomTools_ = new QStackedWidget(header);
  auto* clear = new QToolButton;
  clear->setText(tr("Clear"));
  clear->setIcon(Theme::icon(QStringLiteral("trash-2"), Theme::kText2, 15));
  clear->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  clear->setProperty("pill", true);
  clear->setToolTip(tr("Clear the console"));
  connect(clear, &QToolButton::clicked, console_, &ConsolePanel::clear);
  auto* consoleTools = new QWidget;
  auto* consoleToolsLayout = new QHBoxLayout(consoleTools);
  consoleToolsLayout->setContentsMargins(0, 0, 0, 0);
  consoleToolsLayout->addStretch(1);
  consoleToolsLayout->addWidget(clear);
  auto* memoryTools = new QWidget;
  auto* memoryToolsLayout = new QHBoxLayout(memoryTools);
  memoryToolsLayout->setContentsMargins(0, 0, 0, 0);
  memoryToolsLayout->addStretch(1);
  memory_->viewSelector()->setParent(memoryTools);
  memoryToolsLayout->addWidget(memory_->viewSelector());
  bottomTools_->addWidget(consoleTools);
  bottomTools_->addWidget(memoryTools);
  bottomTools_->addWidget(new QWidget);

  collapseButton_ = new QToolButton(header);
  collapseButton_->setObjectName(QStringLiteral("collapseButton"));
  collapseButton_->setAutoRaise(true);
  collapseButton_->setIconSize(QSize(16, 16));
  collapseButton_->setCursor(Qt::PointingHandCursor);
  connect(collapseButton_, &QToolButton::clicked, this,
          [this] { setBottomCollapsed(!bottomCollapsed_); });

  auto* headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(8, 5, 8, 5);
  headerLayout->setSpacing(2);
  for (QToolButton* b : tabButtons_) headerLayout->addWidget(b);
  headerLayout->addStretch(1);
  headerLayout->addWidget(bottomTools_);
  headerLayout->addSpacing(6);
  headerLayout->addWidget(collapseButton_);

  auto* layout = new QVBoxLayout(panel);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(header);
  layout->addWidget(bottomStack_, 1);

  showBottomTab(ConsoleTab);
  setBottomCollapsed(false);
  return panel;
}

QWidget* MainWindow::createStatusBar() {
  auto* bar = new QWidget(this);
  bar->setObjectName(QStringLiteral("statusBar"));
  bar->setAttribute(Qt::WA_StyledBackground);
  statusDot_ = new QLabel(bar);
  statusText_ = new QLabel(bar);
  statusText_->setTextFormat(Qt::RichText);
  statusText_->setObjectName(QStringLiteral("statusText"));
  statusText_->setAccessibleName(tr("Status"));
  pcLabel_ = new QLabel(bar);
  pcLabel_->setObjectName(QStringLiteral("hint"));
  pcLabel_->setFont(Theme::monoFont(11));
  auto pill = [this, bar]() {
    auto* b = new QToolButton(bar);
    b->setProperty("pill", true);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip(tr("Change in Settings"));
    connect(b, &QToolButton::clicked, this, &MainWindow::showSettings);
    return b;
  };
  pseudoPill_ = pill();
  delayPill_ = pill();
  ioPill_ = pill();
  auto* layout = new QHBoxLayout(bar);
  layout->setContentsMargins(12, 4, 12, 4);
  layout->setSpacing(8);
  layout->addWidget(statusDot_);
  layout->addWidget(statusText_, 1);
  layout->addWidget(pcLabel_);
  layout->addSpacing(6);
  layout->addWidget(pseudoPill_);
  layout->addWidget(delayPill_);
  layout->addWidget(ioPill_);
  return bar;
}

// --- Documents ---

QString MainWindow::documentName() const {
  if (!filePath_.isEmpty()) return QFileInfo(filePath_).fileName();
  return untitledName_.isEmpty() ? QStringLiteral("untitled.s") : untitledName_;
}

void MainWindow::setDocument(const QString& text, const QString& path,
                             const QString& name) {
  settingDocument_ = true;
  filePath_ = path;
  untitledName_ = name;
  editor_->setPlainText(text);
  editor_->clearMarks();
  editor_->clearBreakpoints();
  editor_->setCodeLines({});
  editor_->document()->setModified(false);
  settingDocument_ = false;
  hasDocument_ = true;
  pages_->setCurrentIndex(1);
  setWindowFilePath(path);
  setWindowTitle(QStringLiteral("%1[*] — QtSpim ARM").arg(documentName()));
  setWindowModified(false);
  problems_->setProblems({}, {}, false);
  tabButtons_[ProblemsTab]->setText(tr("Problems"));
  sim_->invalidate();
  editor_->setFocus();
  onStateChanged();
}

void MainWindow::newFile() {
  if (sim_->isBusy() || !maybeSave()) return;
  setDocument(QString::fromUtf8(kTemplate), QString(),
              QStringLiteral("untitled.s"));
  // Put the cursor on the first instruction.
  editor_->goToLine(9);
}

void MainWindow::open() {
  if (sim_->isBusy() || !maybeSave()) return;
  QString dir = QDir::homePath();
  if (!filePath_.isEmpty()) {
    dir = QFileInfo(filePath_).absolutePath();
  } else if (!recentFiles_.isEmpty() && !recentFiles_.first().isEmpty()) {
    dir = QFileInfo(recentFiles_.first()).absolutePath();
  }
  const QString path =
      QFileDialog::getOpenFileName(this, tr("Open"), dir, fileFilter());
  if (!path.isEmpty()) openFile(path);
}

bool MainWindow::openFile(const QString& path) {
  if (sim_->isBusy()) return false;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(
        this, tr("Open"),
        tr("Cannot open %1:\n%2")
            .arg(QDir::toNativeSeparators(path), file.errorString()));
    recentFiles_.removeAll(path);
    updateRecentFiles();
    return false;
  }
  const QByteArray bytes = file.readAll();
  // Assembly files are usually ASCII or UTF-8; fall back to Latin-1.
  auto decoder = QStringDecoder(QStringDecoder::Utf8);
  QString text = decoder.decode(bytes);
  if (decoder.hasError()) text = QString::fromLatin1(bytes);
  setDocument(text, QFileInfo(path).absoluteFilePath(), QString());
  addRecentFile(filePath_);
  assemble();
  return true;
}

void MainWindow::openExample(const QString& name) {
  if (sim_->isBusy() || !maybeSave()) return;
  QFile file(QStringLiteral(":/examples/") + name);
  if (!file.open(QIODevice::ReadOnly)) return;
  setDocument(QString::fromUtf8(file.readAll()), QString(), name);
  assemble();
}

bool MainWindow::save() {
  if (!hasDocument_) return false;
  if (filePath_.isEmpty()) return saveAs();
  return saveTo(filePath_);
}

bool MainWindow::saveAs() {
  if (!hasDocument_) return false;
  QString suggestion = filePath_;
  if (suggestion.isEmpty()) {
    QString dir = QDir::homePath();
    if (!recentFiles_.isEmpty() && !recentFiles_.first().isEmpty()) {
      dir = QFileInfo(recentFiles_.first()).absolutePath();
    }
    suggestion = QDir(dir).filePath(documentName());
  }
  QString path = QFileDialog::getSaveFileName(this, tr("Save As"), suggestion,
                                              fileFilter());
  if (path.isEmpty()) return false;
  if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".s");
  return saveTo(path);
}

bool MainWindow::saveTo(const QString& path) {
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(
        this, tr("Save"),
        tr("Cannot save %1:\n%2")
            .arg(QDir::toNativeSeparators(path), file.errorString()));
    return false;
  }
  file.write(editor_->toPlainText().toUtf8());
  if (!file.commit()) {
    QMessageBox::warning(
        this, tr("Save"),
        tr("Cannot save %1:\n%2")
            .arg(QDir::toNativeSeparators(path), file.errorString()));
    return false;
  }
  filePath_ = QFileInfo(path).absoluteFilePath();
  untitledName_.clear();
  editor_->document()->setModified(false);
  setWindowFilePath(filePath_);
  setWindowTitle(QStringLiteral("%1[*] — QtSpim ARM").arg(documentName()));
  addRecentFile(filePath_);
  updateEditorHeader();
  return true;
}

bool MainWindow::maybeSave() {
  if (!hasDocument_ || !editor_->document()->isModified()) return true;
  QMessageBox box(
      QMessageBox::Warning, tr("Save Changes"),
      tr("Do you want to save the changes to %1?").arg(documentName()),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
      isVisible() ? this : nullptr);
  box.setInformativeText(
      tr("Your changes will be lost if you don't save "
         "them."));
  box.setDefaultButton(QMessageBox::Save);
  switch (box.exec()) {
    case QMessageBox::Save:
      return save();
    case QMessageBox::Discard:
      return true;
    default:
      return false;
  }
}

void MainWindow::closeDocument() {
  if (!hasDocument_ || sim_->isBusy() || !maybeSave()) return;
  hasDocument_ = false;
  filePath_.clear();
  settingDocument_ = true;
  editor_->clear();
  editor_->document()->setModified(false);
  settingDocument_ = false;
  setWindowFilePath(QString());
  setWindowTitle(QStringLiteral("QtSpim ARM"));
  pages_->setCurrentWidget(welcome_);
  sim_->start();
  onStateChanged();
}

void MainWindow::addRecentFile(const QString& path) {
  recentFiles_.removeAll(path);
  recentFiles_.prepend(path);
  while (recentFiles_.size() > recentFilesLength_) recentFiles_.removeLast();
  updateRecentFiles();
  writeSettings();
}

void MainWindow::updateRecentFiles() {
  recentMenu_->clear();
  for (const QString& path : recentFiles_) {
    if (path.isEmpty()) continue;
    QAction* a = recentMenu_->addAction(QFileInfo(path).fileName());
    a->setToolTip(QDir::toNativeSeparators(path));
    connect(a, &QAction::triggered, this, [this, path] {
      if (maybeSave()) openFile(path);
    });
  }
  recentMenu_->setEnabled(!recentMenu_->isEmpty());
  welcome_->setRecentFiles(recentFiles_);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
  const QList<QUrl> urls = event->mimeData()->urls();
  if (!urls.isEmpty() && urls.first().isLocalFile() && maybeSave()) {
    openFile(urls.first().toLocalFile());
  }
}

// --- Simulation ---

bool MainWindow::needsAssembly() const {
  switch (sim_->state()) {
    case Simulator::State::NoProgram:
    case Simulator::State::NeedsAssembly:
    case Simulator::State::AssemblyFailed:
      return true;
    default:
      return editor_->toPlainText() != sim_->assembledSource();
  }
}

bool MainWindow::assemble() {
  if (!hasDocument_ || sim_->isBusy()) return false;
  console_->flush();
  sim_->setBreakpointLines(editor_->breakpointLines());
  const QString argv0 = filePath_.isEmpty() ? documentName() : filePath_;
  const bool ok = sim_->assemble(editor_->toPlainText(), documentName(), argv0);
  updateProblems();
  if (!ok) {
    showBottomTab(ProblemsTab);
    for (const Problem& p : sim_->problems()) {
      if (p.severity == Problem::Severity::Error && p.line > 0) {
        editor_->revealLine(p.line);
        break;
      }
    }
  }
  return ok;
}

void MainWindow::run() {
  if (sim_->state() == Simulator::State::Running) {
    sim_->pause();
    return;
  }
  if (!hasDocument_ || sim_->isBusy()) return;
  if (sim_->state() != Simulator::State::Ready || needsAssembly()) {
    if (!assemble()) return;
  }
  showBottomTab(ConsoleTab);
  sim_->run();
}

void MainWindow::step() {
  if (!hasDocument_ || sim_->isBusy()) return;
  if (sim_->state() != Simulator::State::Paused &&
      sim_->state() != Simulator::State::Ready) {
    if (!assemble()) return;
  } else if (needsAssembly()) {
    if (!assemble()) return;
  }
  sim_->step();
}

void MainWindow::resume() {
  if (sim_->state() == Simulator::State::Paused) sim_->resume();
}

void MainWindow::reset() {
  if (!hasDocument_ || sim_->isBusy()) return;
  assemble();
}

void MainWindow::runAnyway() {
  if (sim_->state() != Simulator::State::AssemblyFailed) return;
  sim_->ignoreErrors();
  showBottomTab(ConsoleTab);
  sim_->run();
}

void MainWindow::onStateChanged() {
  if (sim_->classicMode()) return;
  const Simulator::State state = sim_->state();
  const bool busy = sim_->isBusy();
  const bool waiting = state == Simulator::State::WaitingForInput;

  QString prompt;
  if (waiting) {
    Mips::Syscall info;
    const int number = sim_->waitingSyscall();
    if (Mips::syscall(number, &info)) {
      const int line = sim_->currentLine();
      prompt = line > 0
                   ? tr("The program called %1 (syscall %2) on line %3 and is "
                        "waiting for your input: %4.")
                         .arg(info.name)
                         .arg(number)
                         .arg(line)
                         .arg(info.description)
                   : tr("The program called %1 (syscall %2) and is waiting "
                        "for your input: %3.")
                         .arg(info.name)
                         .arg(number)
                         .arg(info.description);
    }
  }
  console_->setInputState(state == Simulator::State::Running, waiting, prompt,
                          sim_->waitingSyscall() == 12);
  editor_->setReadOnly(busy);
  registers_->setEditable(!busy);
  memory_->setEditable(!busy);
  registers_->setPlaceholder(
      state == Simulator::State::AssemblyFailed
          ? tr("Fix the errors and press Run. The registers and memory "
               "appear here once the program assembles.")
          : QString());
  updateActions();
  updateStatus();
  updateEditorHeader();
  updateExecutionLine();
  if (!busy) {
    registers_->refresh();
    memory_->refresh();
  }
  if (closeAfterStop_ && !busy) {
    closeAfterStop_ = false;
    QTimer::singleShot(0, this, &QWidget::close);
  }
}

void MainWindow::onMachineChanged() {
  if (sim_->classicMode()) return;
  console_->flush();
  registers_->refresh();
  memory_->refresh();
  updateStatus();
  updateExecutionLine();
}

void MainWindow::onProgramLoaded() {
  if (sim_->classicMode()) return;
  registers_->clearChanges();
  memory_->programLoaded();
  QSet<int> lines;
  if (sim_->state() != Simulator::State::NoProgram) lines = sim_->codeLines();
  editor_->setCodeLines(lines);
}

void MainWindow::onInputRequested() {
  if (sim_->classicMode()) return;
  if (bottomCollapsed_) setBottomCollapsed(false);
  showBottomTab(ConsoleTab);
  registers_->refresh();
  updateExecutionLine();
  console_->focusInput();
}

void MainWindow::updateActions() {
  using State = Simulator::State;
  const State state = sim_->state();
  const bool doc = hasDocument_;
  const bool busy = sim_->isBusy();
  const bool running = state == State::Running;
  const bool paused = state == State::Paused;

  runAct_->setEnabled(doc && (running || !busy));
  if (running) {
    runAct_->setText(tr("Pause"));
    runAct_->setIcon(Theme::icon(QStringLiteral("pause"), Qt::white, 18));
    runAct_->setToolTip(tr("Pause the running program"));
  } else {
    runAct_->setText(tr("Run"));
    runAct_->setIcon(Theme::icon(QStringLiteral("play"), Qt::white, 18));
    runAct_->setToolTip(tr("Assemble and run the program from the start (%1)")
                            .arg(QKeySequence(Qt::CTRL | Qt::Key_R)
                                     .toString(QKeySequence::NativeText)));
  }
  assembleAct_->setEnabled(doc && !busy);
  stepAct_->setEnabled(doc && !busy && state != State::AssemblyFailed &&
                       (paused || state == State::Ready || needsAssembly()));
  continueAct_->setEnabled(paused && !needsAssembly());
  pauseAct_->setEnabled(running);
  stopAct_->setEnabled(busy || paused);
  resetAct_->setEnabled(doc && !busy);

  for (QAction* a : {newAct_, openAct_, closeAct_}) a->setEnabled(!busy);
  saveAct_->setEnabled(doc);
  saveAsAct_->setEnabled(doc);
  closeAct_->setEnabled(doc && !busy);
  recentMenu_->setEnabled(!busy && !recentMenu_->isEmpty());
  examplesMenu_->setEnabled(!busy);
  for (QAction* a : {cutAct_, pasteAct_, replaceAct_, commentAct_}) {
    a->setEnabled(doc && !busy);
  }
  for (QAction* a :
       {findAct_, findNextAct_, findPreviousAct_, goToLineAct_, selectAllAct_,
        copyAct_, toggleBreakpointAct_, clearBreakpointsAct_}) {
    a->setEnabled(doc);
  }
  clearRegistersAct_->setEnabled(!busy);
  runParametersAct_->setEnabled(!busy);
  symbolsAct_->setEnabled(!busy);
  settingsAct_->setEnabled(!busy);
  classicAct_->setEnabled(!busy);
  exportLogAct_->setEnabled(!busy);
  printAct_->setEnabled(!busy);
}

QString MainWindow::pauseDescription() const {
  const int line = sim_->currentLine();
  QString head;
  if (line > 0) {
    const SourceLocation loc = sim_->location(sim_->programCounter());
    const QString where = loc.count > 1
                              ? tr("line %1 (machine instruction %2 of %3)")
                                    .arg(line)
                                    .arg(loc.index + 1)
                                    .arg(loc.count)
                              : tr("line %1").arg(line);
    head = sim_->pauseReason() == Simulator::PauseReason::Breakpoint
               ? tr("Stopped at the breakpoint on %1").arg(where)
               : tr("Paused on %1").arg(where);
  } else {
    head = tr("Paused at 0x%1, outside your program")
               .arg(sim_->programCounter(), 8, 16, QLatin1Char('0'));
  }
  QString tail = tr("press Step or Continue");
  if (sim_->lastException() >= 0) {
    const int exceptionLine = sim_->location(sim_->lastExceptionPc()).line;
    const QString name = Mips::exceptionName(sim_->lastException());
    tail = exceptionLine > 0
               ? tr("exception on line %1: %2").arg(exceptionLine).arg(name)
               : tr("exception: %1").arg(name);
  }
  return QStringLiteral("<b>%1</b> — %2")
      .arg(head.toHtmlEscaped(), tail.toHtmlEscaped());
}

void MainWindow::updateStatus() {
  using State = Simulator::State;
  QString text;
  QColor color = Theme::kTextDisabled;
  auto bold = [](const QString& head, const QString& tail) {
    return QStringLiteral("<b>%1</b> — %2")
        .arg(head.toHtmlEscaped(), tail.toHtmlEscaped());
  };
  switch (sim_->state()) {
    case State::NoProgram:
    case State::NeedsAssembly:
      if (!hasDocument_) {
        text = tr("Open or create a program to begin.");
      } else if (sim_->state() == State::NeedsAssembly) {
        text = bold(tr("Edited"), tr("press Run to assemble and run it"));
      } else {
        text = bold(tr("Ready"), tr("press Run"));
      }
      break;
    case State::Ready:
      color = Theme::kSuccess;
      text = bold(tr("Assembled"),
                  tr("press Run, or Step to go one instruction at a time"));
      break;
    case State::AssemblyFailed: {
      color = Theme::kError;
      int first = 0;
      for (const Problem& p : sim_->problems()) {
        if (p.severity == Problem::Severity::Error && p.line > 0) {
          first = p.line;
          break;
        }
      }
      const int n = sim_->errorCount();
      QString detail;
      if (first > 0) {
        detail = n == 1
                     ? tr("1 error, on line %1").arg(first)
                     : tr("%1 errors, the first on line %2").arg(n).arg(first);
      } else {
        detail = n == 1 ? tr("1 error") : tr("%1 errors").arg(n);
      }
      text = bold(tr("Could not assemble"), detail);
      break;
    }
    case State::Running:
      color = Theme::accent();
      text = bold(tr("Running…"), tr("press Pause or Stop"));
      break;
    case State::Paused:
      color = Theme::kPaused;
      text = pauseDescription();
      break;
    case State::WaitingForInput: {
      color = Theme::accent();
      Mips::Syscall info;
      const int line = sim_->currentLine();
      const QString what = Mips::syscall(sim_->waitingSyscall(), &info)
                               ? info.name
                               : QStringLiteral("syscall");
      text = bold(
          tr("Waiting for input"),
          line > 0
              ? tr("%1 on line %2; type in the Console").arg(what).arg(line)
              : tr("%1; type in the Console").arg(what));
      break;
    }
    case State::Finished:
      color = Theme::kSuccess;
      text = bold(tr("Finished"), tr("exit code %1").arg(sim_->exitCode()));
      break;
    case State::Stopped:
      text = bold(tr("Stopped"), tr("press Run to start again"));
      break;
    case State::RunError:
      color = Theme::kError;
      text = bold(tr("Error"), Mips::explainRunError(sim_->runErrorMessage()));
      break;
  }
  statusDot_->setPixmap(dot(color));
  statusText_->setText(text);
  statusText_->setToolTip(
      sim_->state() == State::RunError ? sim_->runErrorMessage() : QString());

  pcLabel_->setText(sim_->hasProgram()
                        ? QStringLiteral("PC 0x%1").arg(sim_->programCounter(),
                                                        8, 16, QLatin1Char('0'))
                        : QString());
  pseudoPill_->setText(settings_.acceptPseudoInsts
                           ? tr("Pseudo-instructions: on")
                           : tr("Pseudo-instructions: off"));
  delayPill_->setText(settings_.delayedBranches || settings_.delayedLoads
                          ? tr("Delay slots: on")
                          : tr("Delay slots: off"));
  ioPill_->setText(tr("Memory-mapped I/O"));
  ioPill_->setVisible(settings_.mappedIo);
}

void MainWindow::updateEditorHeader() {
  using State = Simulator::State;
  fileLabel_->setText(documentName());
  switch (sim_->state()) {
    case State::NoProgram:
      assembleLabel_->clear();
      break;
    case State::NeedsAssembly:
      assembleLabel_->setText(tr("Edited"));
      break;
    case State::AssemblyFailed:
      assembleLabel_->setText(
          QStringLiteral("<span style='color:%1'>✕ %2</span>")
              .arg(Theme::css(Theme::kError),
                   sim_->errorCount() == 1
                       ? tr("1 error")
                       : tr("%1 errors").arg(sim_->errorCount())));
      break;
    default:
      assembleLabel_->setText(
          QStringLiteral("<span style='color:%1'>✓</span> %2")
              .arg(Theme::css(Theme::kSuccess),
                   sim_->instructionCount() == 1
                       ? tr("Assembled · 1 instruction")
                       : tr("Assembled · %1 instructions")
                             .arg(sim_->instructionCount())));
      break;
  }
}

void MainWindow::updateExecutionLine() {
  const Simulator::State state = sim_->state();
  if (state == Simulator::State::Paused ||
      state == Simulator::State::WaitingForInput) {
    editor_->setExecutionLine(sim_->currentLine(),
                              state == Simulator::State::WaitingForInput);
  } else {
    editor_->setExecutionLine(0, false);
  }
}

void MainWindow::updateProblems() {
  const QStringList lines = editor_->toPlainText().split(QLatin1Char('\n'));
  const QList<Problem>& problems = sim_->problems();
  problems_->setProblems(problems, lines, true);
  editor_->setMarks(ProblemsPanel::marksFor(problems, lines));
  tabButtons_[ProblemsTab]->setText(
      problems.isEmpty() ? tr("Problems")
                         : tr("Problems %1").arg(problems.size()));
}

// --- View ---

void MainWindow::showBottomTab(int tab) {
  if (bottomCollapsed_) setBottomCollapsed(false);
  bottomStack_->setCurrentIndex(tab);
  bottomTools_->setCurrentIndex(tab);
  tabButtons_[tab]->setChecked(true);
}

void MainWindow::setBottomCollapsed(bool collapsed) {
  if (collapsed == bottomCollapsed_ && bottomStack_->isVisible() != collapsed) {
    // Already in that state.
  }
  if (collapsed && !bottomCollapsed_ && vsplit_ != nullptr) {
    bottomSizes_ = vsplit_->sizes();
  }
  bottomCollapsed_ = collapsed;
  bottomStack_->setVisible(!collapsed);
  bottomTools_->setVisible(!collapsed);
  collapseButton_->setIcon(Theme::icon(
      collapsed ? QStringLiteral("chevron-up") : QStringLiteral("chevron-down"),
      Theme::kText2, 16));
  collapseButton_->setToolTip(collapsed ? tr("Show the panel")
                                        : tr("Hide the panel"));
  if (bottomAct_ != nullptr) {
    const QSignalBlocker block(bottomAct_);
    bottomAct_->setChecked(!collapsed);
  }
  if (!collapsed && vsplit_ != nullptr && bottomSizes_.size() == 2 &&
      bottomSizes_[1] > 80) {
    vsplit_->setSizes(bottomSizes_);
  }
}

void MainWindow::setRegistersVisible(bool visible) {
  registers_->setVisible(visible);
  const QSignalBlocker block(registersAct_);
  registersAct_->setChecked(visible);
}

void MainWindow::restoreLayout() {
  hsplit_->setSizes({880, 300});
  vsplit_->setSizes({460, 260});
  setRegistersVisible(true);
  setBottomCollapsed(false);
}

void MainWindow::setBase(int base) { registers_->setBase(base); }

// --- Dialogs ---

void MainWindow::showSettings() {
  if (sim_->isBusy()) return;
  SettingsDialog dialog(settings_, this);
  if (dialog.exec() != QDialog::Accepted) return;
  applySettings(dialog.settings());
  writeSettings();
}

void MainWindow::applySettings(const AppSettings& s) {
  const bool simulatorChanged = !s.sameSimulator(settings_);
  settings_ = s;
  editor_->setEditorFont(s.fontSize);
  editor_->setTabWidth(s.tabWidth);
  console_->setFontSize(s.fontSize);
  console_->setRawInput(s.mappedIo);
  registers_->setBase(s.base);
  memory_->setBase(s.base);
  memory_->setShowKernel(s.showKernel);
  sim_->stepIntoKernel = s.showKernel;
  applySimulatorSettings();
  if (simulatorChanged && !sim_->isBusy()) {
    if (hasDocument_ && sim_->state() != Simulator::State::NoProgram) {
      sim_->invalidate();
    } else {
      sim_->start();
    }
  }
  updateStatus();
}

void MainWindow::applySimulatorSettings() {
  bare_machine = settings_.bareMachine;
  accept_pseudo_insts = settings_.acceptPseudoInsts;
  delayed_branches = settings_.delayedBranches;
  delayed_loads = settings_.delayedLoads;
  mapped_io = settings_.mappedIo;
  quiet = settings_.quiet;
  sim_->loadExceptionHandler = settings_.loadExceptionHandler;
  sim_->exceptionHandlerFile = settings_.exceptionHandlerFile;
}

void MainWindow::showRunParameters() {
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Run Parameters"));
  auto* address = new QLineEdit(&dialog);
  address->setFont(Theme::monoFont(12));
  address->setPlaceholderText(tr("__start (default)"));
  if (sim_->startAddress != 0) {
    address->setText(QStringLiteral("0x%1").arg(sim_->startAddress, 8, 16,
                                                QLatin1Char('0')));
  }
  auto* arguments = new QLineEdit(sim_->programArguments, &dialog);
  arguments->setFont(Theme::monoFont(12));
  auto* note = new QLabel(
      tr("The arguments are passed to main in $a0 (argc) and $a1 (argv). "
         "They take effect the next time the program is loaded."),
      &dialog);
  note->setObjectName(QStringLiteral("hint"));
  note->setWordWrap(true);
  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  auto* form = new QFormLayout(&dialog);
  form->addRow(tr("Start address"), address);
  form->addRow(tr("Arguments"), arguments);
  form->addRow(note);
  form->addRow(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) return;
  uint32_t start = 0;
  if (!address->text().trimmed().isEmpty() &&
      !Mips::parseWord(address->text(), 16, &start)) {
    QMessageBox::warning(this, tr("Run Parameters"),
                         tr("“%1” is not an address.").arg(address->text()));
    return;
  }
  sim_->startAddress = start;
  sim_->programArguments = arguments->text();
  writeSettings();
  sim_->invalidate();
}

void MainWindow::showSymbols() {
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Symbols"));
  dialog.resize(520, 480);
  auto* view = new QPlainTextEdit(&dialog);
  view->setReadOnly(true);
  view->setFont(Theme::monoFont(12));
  QStringList lines;
  for (const QString& line :
       sim_->symbolListing().split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
    // "g\tname at 0x00400000" or "\tname at 0x...".
    const bool global = line.startsWith(QLatin1Char('g'));
    QString rest = line.mid(line.indexOf(QLatin1Char('\t')) + 1);
    const int at = rest.lastIndexOf(QStringLiteral(" at "));
    if (at < 0) continue;
    lines << QStringLiteral("%1  %2 %3")
                 .arg(rest.mid(at + 4),
                      (global ? QStringLiteral("global")
                              : QStringLiteral("      ")),
                      rest.left(at));
  }
  lines.sort();
  view->setPlainText(tr("Address     Scope  Name") + QLatin1Char('\n') +
                     lines.join(QLatin1Char('\n')));
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  auto* layout = new QVBoxLayout(&dialog);
  layout->addWidget(view);
  layout->addWidget(buttons);
  dialog.exec();
}

QString MainWindow::logText(bool registers, bool code, bool data, bool stack,
                            bool console) const {
  QString out;
  auto section = [&out](const QString& title, const QString& body) {
    out += QStringLiteral("==== %1 ====\n%2\n\n").arg(title, body);
  };
  if (registers) section(tr("Registers"), registers_->plainText());
  if (code)
    section(tr("Machine code"), memory_->plainText(MemoryPanel::View::Code));
  if (data) section(tr("Data"), memory_->plainText(MemoryPanel::View::Data));
  if (stack) section(tr("Stack"), memory_->plainText(MemoryPanel::View::Stack));
  if (console) section(tr("Console"), console_->text());
  return out;
}

namespace {

// Asks which parts to include in a log or printout.
bool chooseParts(QWidget* parent, const QString& title, bool parts[5]) {
  QDialog dialog(parent);
  dialog.setWindowTitle(title);
  const QString names[5] = {MainWindow::tr("Registers"),
                            MainWindow::tr("Machine code"),
                            MainWindow::tr("Data"), MainWindow::tr("Stack"),
                            MainWindow::tr("Console")};
  QCheckBox* boxes[5];
  auto* layout = new QVBoxLayout(&dialog);
  auto* label = new QLabel(MainWindow::tr("Include:"), &dialog);
  layout->addWidget(label);
  for (int i = 0; i < 5; ++i) {
    boxes[i] = new QCheckBox(names[i], &dialog);
    boxes[i]->setChecked(true);
    layout->addWidget(boxes[i]);
  }
  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog,
                   &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);
  layout->addWidget(buttons);
  if (dialog.exec() != QDialog::Accepted) return false;
  for (int i = 0; i < 5; ++i) parts[i] = boxes[i]->isChecked();
  return true;
}

}  // namespace

void MainWindow::exportLog() {
  bool parts[5];
  if (!chooseParts(this, tr("Export Log"), parts)) return;
  QString base =
      filePath_.isEmpty()
          ? QDir::home().filePath(QFileInfo(documentName()).completeBaseName())
          : QFileInfo(filePath_).absolutePath() + QLatin1Char('/') +
                QFileInfo(filePath_).completeBaseName();
  const QString path = QFileDialog::getSaveFileName(
      this, tr("Export Log"), base + QStringLiteral("-log.txt"),
      tr("Text files (*.txt);;All files (*)"));
  if (path.isEmpty()) return;
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, tr("Export Log"),
                         tr("Cannot write %1.").arg(path));
    return;
  }
  file.write(
      logText(parts[0], parts[1], parts[2], parts[3], parts[4]).toUtf8());
  if (!file.commit()) {
    QMessageBox::warning(this, tr("Export Log"),
                         tr("Cannot write %1.").arg(path));
  }
}

void MainWindow::print() {
  bool parts[5];
  if (!chooseParts(this, tr("Print"), parts)) return;
  QPrinter printer;
  QPrintDialog dialog(&printer, this);
  dialog.setWindowTitle(tr("Print"));
  if (dialog.exec() != QDialog::Accepted) return;
  QTextDocument document;
  document.setDefaultFont(Theme::monoFont(9));
  QString text = documentName() + QStringLiteral("\n\n");
  if (hasDocument_) text += editor_->toPlainText() + QStringLiteral("\n\n");
  text += logText(parts[0], parts[1], parts[2], parts[3], parts[4]);
  document.setPlainText(text);
  document.print(&printer);
}

void MainWindow::showHelp() {
  // On the Mac, the help collection and Assistant are inside the app bundle,
  // wherever it is installed: <bundle>/Contents/{Resources/doc,MacOS}.
  const QString appDir = QCoreApplication::applicationDirPath();
  const QString helpFiles[] = {
      qEnvironmentVariable("PROGRAMFILES(x86)") +
          QStringLiteral("/QtSpim/help/qtspim.qhc"),
      appDir + QStringLiteral("/../Resources/doc/qtspim.qhc"),
      QStringLiteral("/usr/lib/qtspim/help/qtspim.qhc"),
  };
  const QString assistants[] = {
      QStringLiteral("assistant"),
      appDir + QStringLiteral("/Assistant"),
      QStringLiteral("/usr/lib/qtspim/bin/assistant"),
  };
  for (int i = 0; i < 3; ++i) {
    if (!QFileInfo::exists(helpFiles[i])) continue;
    auto* process = new QProcess(this);
    process->start(assistants[i],
                   {QStringLiteral("-collectionFile"), helpFiles[i]});
    if (!process->waitForStarted()) {
      QMessageBox::information(
          this, tr("QtSpim Help"),
          tr("Cannot start the help browser (Qt Assistant). Check the "
             "installation."));
    }
    return;
  }
  QMessageBox::information(
      this, tr("QtSpim Help"),
      tr("Cannot find the QtSpim help files. Check the installation."));
}

void MainWindow::showAbout() {
  QMessageBox box(
      QMessageBox::NoIcon, tr("About QtSpim ARM"),
      QStringLiteral("<p style='font-size:15pt'><b>QtSpim ARM</b>"
                     "</p><p>") +
          tr("Version %1").arg(QStringLiteral(QTSPIM_ARM_VERSION)) +
          QStringLiteral("</p><p>") +
          tr("QtSpim ARM is an unofficial fork of QtSpim. It is "
             "not affiliated with or endorsed by James R. "
             "Larus.") +
          QStringLiteral("</p><p><a href='https://github.com/tw-yuan/"
                         "qtspim-arm'>https://github.com/tw-yuan/qtspim-arm"
                         "</a></p><hr><p>") +
          QStringLiteral(SPIM_VERSION) +
          QStringLiteral("</p><p>SPIM is a simulator of the MIPS R3000 "
                         "processor.<br>Copyright (c) 1990-2015, James R. "
                         "Larus (larus@larusstone.org).<br>SPIM is "
                         "distributed under a BSD license.<br><a "
                         "href='https://sourceforge.net/projects/"
                         "spimsimulator/'>https://sourceforge.net/projects/"
                         "spimsimulator/</a></p><p>") +
          tr("Icons: Lucide (lucide.dev), ISC license.") +
          QStringLiteral("</p><p>QtSpim ARM is linked to the Qt library, "
                         "which is distributed under the GNU Lesser General "
                         "Public License version 3.</p>"),
      QMessageBox::Ok, this);
  box.setIconPixmap(QPixmap(QStringLiteral(":/icons/qtspim.png")));
  box.exec();
}

// --- Settings ---

void MainWindow::readSettings() {
  QSettings s;
  s.beginGroup(QStringLiteral("Spim"));
  settings_.quiet = s.value(QStringLiteral("Quiet"), false).toBool();
  settings_.bareMachine =
      s.value(QStringLiteral("BareMachine"), false).toBool();
  settings_.acceptPseudoInsts =
      s.value(QStringLiteral("AcceptPseudoInsts"), true).toBool();
  settings_.delayedBranches =
      s.value(QStringLiteral("DelayedBranches"), false).toBool();
  settings_.delayedLoads =
      s.value(QStringLiteral("DelayedLoads"), false).toBool();
  settings_.mappedIo = s.value(QStringLiteral("MappedIO"), false).toBool();
  settings_.loadExceptionHandler =
      s.value(QStringLiteral("LoadExceptionHandler"), true).toBool();
  QString handler =
      s.value(QStringLiteral("ExceptionHandlerFileName")).toString();
  if (handler == QLatin1String(kStandardHandler)) handler.clear();
  settings_.exceptionHandlerFile = handler;
  sim_->startAddress = s.value(QStringLiteral("StartingAddress"), 0).toUInt();
  sim_->programArguments =
      s.value(QStringLiteral("CommandLineArguments")).toString();
  s.endGroup();

  settings_.fontSize =
      qBound(10, s.value(QStringLiteral("Editor/FontSize"), 13).toInt(), 28);
  settings_.tabWidth =
      s.value(QStringLiteral("Editor/TabWidth"), 8).toInt() == 4 ? 4 : 8;
  settings_.base =
      s.value(QStringLiteral("RegWin/RegisterDisplayBase"), 16).toInt();
  if (settings_.base != 2 && settings_.base != 10) settings_.base = 16;
  settings_.showKernel =
      s.value(QStringLiteral("View/ShowKernel"), false).toBool();
  settings_.language = s.value(QStringLiteral("UI/Language")).toString();

  s.beginGroup(QStringLiteral("FileMenu"));
  recentFilesLength_ =
      qBound(4, s.value(QStringLiteral("RecentFilesLength"), 8).toInt(), 20);
  recentFiles_.clear();
  for (int i = 0; i < recentFilesLength_; ++i) {
    // The key is "RecentFile" followed by the character with code i, as in
    // the original QtSpim.
    const QString file =
        s.value(QStringLiteral("RecentFile") + QString(QChar(i))).toString();
    if (!file.isEmpty()) recentFiles_ << file;
  }
  s.endGroup();
  updateRecentFiles();

  s.beginGroup(QStringLiteral("NewUI"));
  restoreGeometry(s.value(QStringLiteral("Geometry")).toByteArray());
  if (s.contains(QStringLiteral("HSplitter"))) {
    hsplit_->restoreState(s.value(QStringLiteral("HSplitter")).toByteArray());
    vsplit_->restoreState(s.value(QStringLiteral("VSplitter")).toByteArray());
  }
  setRegistersVisible(
      s.value(QStringLiteral("RegistersVisible"), true).toBool());
  s.endGroup();
  if (!s.contains(QStringLiteral("NewUI/Geometry"))) resize(1200, 780);

  applySettings(settings_);
}

void MainWindow::writeSettings() {
  QSettings s;
  s.beginGroup(QStringLiteral("Spim"));
  s.setValue(QStringLiteral("Quiet"), quiet);
  s.setValue(QStringLiteral("BareMachine"), bare_machine);
  s.setValue(QStringLiteral("AcceptPseudoInsts"), accept_pseudo_insts);
  s.setValue(QStringLiteral("DelayedBranches"), delayed_branches);
  s.setValue(QStringLiteral("DelayedLoads"), delayed_loads);
  s.setValue(QStringLiteral("MappedIO"), mapped_io);
  s.setValue(QStringLiteral("LoadExceptionHandler"),
             sim_->loadExceptionHandler);
  s.setValue(QStringLiteral("ExceptionHandlerFileName"),
             sim_->exceptionHandlerFile.isEmpty()
                 ? QString::fromLatin1(kStandardHandler)
                 : sim_->exceptionHandlerFile);
  s.setValue(QStringLiteral("StartingAddress"), sim_->startAddress);
  s.setValue(QStringLiteral("CommandLineArguments"), sim_->programArguments);
  s.endGroup();

  s.setValue(QStringLiteral("Editor/FontSize"), settings_.fontSize);
  s.setValue(QStringLiteral("Editor/TabWidth"), settings_.tabWidth);
  s.setValue(QStringLiteral("RegWin/RegisterDisplayBase"), settings_.base);
  s.setValue(QStringLiteral("View/ShowKernel"), settings_.showKernel);
  s.setValue(QStringLiteral("UI/Language"), settings_.language);

  s.beginGroup(QStringLiteral("FileMenu"));
  s.setValue(QStringLiteral("RecentFilesLength"), recentFilesLength_);
  for (int i = 0; i < recentFilesLength_; ++i) {
    s.setValue(QStringLiteral("RecentFile") + QString(QChar(i)),
               i < recentFiles_.size() ? recentFiles_[i] : QString());
  }
  s.endGroup();

  if (isVisible()) {
    s.beginGroup(QStringLiteral("NewUI"));
    s.setValue(QStringLiteral("Geometry"), saveGeometry());
    s.setValue(QStringLiteral("HSplitter"), hsplit_->saveState());
    s.setValue(QStringLiteral("VSplitter"), vsplit_->saveState());
    // The panel is hidden with the workspace on the welcome page, so save
    // what the View menu says.
    s.setValue(QStringLiteral("RegistersVisible"), registersAct_->isChecked());
    s.endGroup();
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (sim_->isBusy()) {
    closeAfterStop_ = true;
    sim_->stop();
    event->ignore();
    return;
  }
  if (!maybeSave()) {
    event->ignore();
    return;
  }
  writeSettings();
  event->accept();
}

// --- Starting, and the classic interface ---

void MainWindow::start(const CommandLine& commandLine) {
  startupFiles_ = commandLine.files;
  if (commandLine.setHandler) {
    settings_.loadExceptionHandler = commandLine.loadHandler;
    if (!commandLine.handlerFile.isEmpty()) {
      settings_.exceptionHandlerFile = commandLine.handlerFile;
    }
  }
  // The command line may have changed the SPIM globals.
  settings_.bareMachine = bare_machine;
  settings_.acceptPseudoInsts = accept_pseudo_insts;
  settings_.delayedBranches = delayed_branches;
  settings_.delayedLoads = delayed_loads;
  settings_.mappedIo = mapped_io;
  settings_.quiet = quiet;
  applySimulatorSettings();
  sim_->start();

  if (QSettings().value(QStringLiteral("UI/Classic"), false).toBool()) {
    showClassic(true, commandLine);
    return;
  }
  show();
  if (!commandLine.files.isEmpty()) openFile(commandLine.files.first());
}

void MainWindow::showClassic(bool atStartup, const CommandLine& commandLine) {
  if (sim_->isBusy()) return;
  writeSettings();
  const bool created = classic_ == nullptr;
  if (created) {
    // The original window reads the same settings (the SPIM globals).
    classic_ = new SpimView;
    Window = classic_;
    classic_->setStyleSheet(SpimView::ClassicStyleSheet());
    classic_->SetSwitchToNewInterface([this] { showNew(); });
    classic_->SetQuitCheck([this] { return maybeSave(); });
  }
  classic_->SetExceptionHandler(sim_->exceptionHandlerFile.isEmpty()
                                    ? QString::fromLatin1(kStandardHandler)
                                    : sim_->exceptionHandlerFile,
                                sim_->loadExceptionHandler);
  sim_->setClassicMode(true);
  QSettings().setValue(QStringLiteral("UI/Classic"), true);
  hide();
  classic_->SpimConsole->show();
  classic_->show();
  if (atStartup) {
    // As the original QtSpim starts.
    classic_->sim_ReinitializeSimulator();
    for (const QString& file : commandLine.files) {
      if (!file.isEmpty()) {
        QByteArray name = QFile::encodeName(file);
        read_assembly_file(name.data());
      }
    }
  }
  classic_->DisplayTextSegments(true);
  classic_->UpdateDataDisplay();
  classic_->raise();
  classic_->activateWindow();
}

void MainWindow::showNew() {
  if (classic_ == nullptr) return;
  sim_->setClassicMode(false);
  QSettings().setValue(QStringLiteral("UI/Classic"), false);
  // The classic window may have changed the settings and loaded another
  // program.
  settings_.bareMachine = bare_machine;
  settings_.acceptPseudoInsts = accept_pseudo_insts;
  settings_.delayedBranches = delayed_branches;
  settings_.delayedLoads = delayed_loads;
  settings_.mappedIo = mapped_io;
  settings_.quiet = quiet;
  settings_.loadExceptionHandler = classic_->LoadsExceptionHandler();
  const QString handler = classic_->ExceptionHandlerFile();
  settings_.exceptionHandlerFile =
      handler == QLatin1String(kStandardHandler) ? QString() : handler;
  applySettings(settings_);
  classic_->SpimConsole->hide();
  classic_->hide();
  if (hasDocument_) {
    sim_->invalidate();
  } else {
    sim_->start();
    QString file = classic_->CurrentFile();
    if (file.isEmpty() && !startupFiles_.isEmpty()) file = startupFiles_.first();
    show();
    if (!file.isEmpty() && QFileInfo::exists(file)) openFile(file);
  }
  show();
  raise();
  activateWindow();
  memory_->programLoaded();
  onMachineChanged();
  onStateChanged();
  writeSettings();
}
