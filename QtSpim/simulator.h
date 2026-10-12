/* SPIM S20 MIPS simulator.
   The interface's view of the SPIM simulator core.

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

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTemporaryDir>

#include <cstdint>

class QEventLoop;

// One message from the assembler, or an undefined symbol.
struct Problem {
  enum class Severity { Error, Warning };
  enum class Kind { Parser, UndefinedSymbol, Other };
  Severity severity = Severity::Error;
  Kind kind = Kind::Other;
  int line = 0;     // 1-based source line, 0 = none
  QString message;  // SPIM's message, e.g. "syntax error"
  QString raw;      // SPIM's whole message, as printed by spim
  QString context;  // source text SPIM printed with the message
  QString symbol;   // for undefined symbols
};

// Where an instruction came from in the user's source.
struct SourceLocation {
  int line = 0;   // 1-based
  int index = 0;  // 0-based position among the line's machine instructions
  int count = 1;  // machine instructions the line expanded to
};

// Wraps the global state of the simulator core (CPU/) for the interface.
//
// The core is not reentrant and calls back into the interface through the
// functions in spim_support.cpp, so there is exactly one Simulator.  The
// program runs on the GUI thread in slices, processing events in between,
// as the original QtSpim did; console input waits in a nested event loop.
// The core itself is used exactly as QtSpim and spim use it: programs are
// loaded from a file with initialize_world() and read_assembly_file(), and
// run with run_program().
class Simulator : public QObject {
  Q_OBJECT

 public:
  enum class State {
    NoProgram,       // nothing assembled yet
    NeedsAssembly,   // the source or the settings changed
    AssemblyFailed,  // the assembler reported errors
    Ready,           // assembled, not started
    Running,
    Paused,           // after a step, a breakpoint or Pause
    WaitingForInput,  // a read syscall is waiting for the console
    Finished,         // exit or exit2 syscall
    Stopped,          // Stop
    RunError,         // the simulator stopped the program with an error
  };

  enum class PauseReason { None, Step, Breakpoint, User };

  static Simulator* instance();

  // Initializes the simulator without a program (at startup, or after the
  // settings changed).
  void start();

  // The classic QtSpim window is in use: the core's callbacks behave as in
  // the original QtSpim (spimview.cpp and console.cpp show them).
  void setClassicMode(bool classic) { classic_ = classic; }
  bool classicMode() const { return classic_; }
  void clearInput() {
    input_.clear();
    inputBytes_.clear();
  }

  // --- Settings (the SPIM globals bare_machine etc. are set directly) ---
  bool loadExceptionHandler = true;
  QString exceptionHandlerFile;  // empty = the built-in handler
  uint32_t startAddress = 0;     // 0 = __start
  QString programArguments;
  bool stepIntoKernel = false;  // single-step through kernel code

  // --- Program ---

  // Writes |source| to a temporary file named |fileName| and loads it into a
  // freshly initialized simulator, then prepares the stack and PC to run.
  // Returns true if the program can run (no assembler errors).
  bool assemble(const QString& source, const QString& fileName,
                const QString& argv0);
  // Loads the program again from the same source (Reset).
  bool reassemble();
  // The source or the settings changed.
  void invalidate();
  // Lets a program with assembler errors run, as the original SPIM does.
  void ignoreErrors();

  State state() const { return state_; }
  PauseReason pauseReason() const { return pauseReason_; }
  bool isBusy() const {
    return state_ == State::Running || state_ == State::WaitingForInput;
  }
  bool canResume() const { return state_ == State::Paused; }
  bool hasProgram() const;

  const QList<Problem>& problems() const { return problems_; }
  int errorCount() const;
  int instructionCount() const { return userAddresses_.size(); }
  QString assembledSource() const { return source_; }

  // Source mapping for the user's program.
  bool isUserAddress(uint32_t addr) const { return locations_.contains(addr); }
  SourceLocation location(uint32_t addr) const {
    return locations_.value(addr);
  }
  int currentLine() const;  // line of PC, 0 if PC is not in the program
  uint32_t programCounter() const;
  QSet<int> codeLines() const;  // lines that produced machine code
  QList<uint32_t> addressesOfLine(int line) const {
    return lineAddresses_.value(line);
  }
  // Labels and their addresses (from the symbol table).
  const QMap<uint32_t, QString>& labels() const { return labels_; }

  // Original instruction encoding at |addr| (also under a breakpoint).
  bool instructionAt(uint32_t addr, uint32_t* encoding) const;
  // PC is at a syscall instruction.
  bool atSyscall() const;

  // --- Breakpoints, by source line ---
  void setBreakpointLines(const QSet<int>& lines);
  bool lineHasCode(int line) const { return lineAddresses_.contains(line); }

  // --- Execution ---
  void run();     // from the start (assembles first if needed: caller)
  void step();    // one source-level step
  void resume();  // Continue
  void pause();
  void stop();

  // Exit code of the last exit/exit2 syscall.
  int exitCode() const { return exitCode_; }
  // The last run error from the core and its explanation.
  QString runErrorMessage() const { return runErrorMessage_; }
  // Exceptions raised in the last execution slice (Cause code), or -1.
  int lastException() const { return lastException_; }
  uint32_t lastExceptionPc() const { return lastExceptionPc_; }
  bool resetRegisters();  // Simulator › Clear Registers

  // --- Console ---
  void provideInput(const QString& text);
  bool inputPending() const { return coreInputAvailable(); }
  // The read syscall waiting for input (5, 6, 7, 8 or 12), or 0.
  int waitingSyscall() const { return waitingSyscall_; }

  // --- Callbacks from spim_support.cpp (the core) ---
  void coreOutput(bool console, const QByteArray& bytes);
  void coreError(const QString& text);
  void coreRunError(const QString& text);
  int coreReadChar();  // blocks in a nested event loop; 0 if stopped
  bool coreInputAvailable() const {
    return !input_.isEmpty() || !inputBytes_.isEmpty();
  }

  // The symbol table, as print_symbols writes it.
  QString symbolListing();

 signals:
  void stateChanged();
  // Registers and memory may have changed.
  void machineChanged();
  // About to execute: panels record values to highlight changes.
  void aboutToExecute();
  void consoleOutput(const QString& text);
  // A message from the simulator (an error or warning while running).
  void simulatorMessage(const QString& text, bool error);
  void consoleCleared();
  void inputRequested();
  void programLoaded();

 private:
  Simulator();
  void initializeWorld();
  QString builtInExceptionHandler();
  void buildSourceMap(const QHash<uint32_t, void*>& before);
  void parseProblems(const QStringList& messages);
  void collectLabels();
  void addLabel(uint32_t addr, const QString& name);
  void applyBreakpoints();
  void initializePcAndStack();
  void setState(State s, PauseReason reason = PauseReason::None);
  enum class Mode { Run, Continue, Step };
  void execute(Mode mode);
  bool runSlice(int steps, bool contBkpt, bool* breakpoint);
  void finishRun();

  bool classic_ = false;
  State state_ = State::NoProgram;
  PauseReason pauseReason_ = PauseReason::None;
  QTemporaryDir tempDir_;
  QString handlerPath_;
  QString source_;
  QString fileName_;
  QString argv0_;
  QString loadedPath_;

  QList<Problem> problems_;
  QStringList loadMessages_;
  bool loading_ = false;
  bool capturing_ = false;
  QString captured_;

  QHash<uint32_t, SourceLocation> locations_;
  QMap<int, QList<uint32_t>> lineAddresses_;
  QList<uint32_t> userAddresses_;
  QHash<uint32_t, uint32_t> encodings_;
  QMap<uint32_t, QString> labels_;
  QSet<int> breakpointLines_;
  QSet<uint32_t> breakpoints_;

  bool stopRequested_ = false;
  bool pauseRequested_ = false;
  bool runError_ = false;
  QString runErrorMessage_;
  int exitCode_ = 0;
  int lastException_ = -1;
  uint32_t lastExceptionPc_ = 0;

  QString input_;
  QByteArray inputBytes_;  // the current character, as UTF-8
  QEventLoop* inputLoop_ = nullptr;
  int waitingSyscall_ = 0;
};

#endif  // SIMULATOR_H
