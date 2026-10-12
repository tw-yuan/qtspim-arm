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

#include "simulator.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>

#include <algorithm>
#include <cstdlib>

#include "spim.h"
#include "string-stream.h"
#include "spim-utils.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"

namespace {

// Instructions run between event processing, as in the original QtSpim.
constexpr int kRunSlice = 100000;
// Single steps through code outside the user's program (the startup code
// and the exception handler) before a step gives up and pauses there.
constexpr int kSkipLimit = 1000000;

QStringDecoder& consoleDecoder() {
  static QStringDecoder decoder(QStringDecoder::Utf8);
  return decoder;
}

// Text of a source line in SPIM's error message, for comparing it with the
// editor's lines.
QString normalized(QString s) {
  s = s.simplified();
  return s;
}

}  // namespace

Simulator* Simulator::instance() {
  static Simulator* simulator = new Simulator;
  return simulator;
}

Simulator::Simulator() : QObject(nullptr) {}

bool Simulator::hasProgram() const {
  return state_ != State::NoProgram && state_ != State::NeedsAssembly &&
         state_ != State::AssemblyFailed;
}

int Simulator::errorCount() const {
  int n = 0;
  for (const Problem& p : problems_) {
    if (p.severity == Problem::Severity::Error) ++n;
  }
  return n;
}

int Simulator::currentLine() const {
  const auto it = locations_.find(PC);
  return it == locations_.end() ? 0 : it->line;
}

uint32_t Simulator::programCounter() const { return PC; }

QSet<int> Simulator::codeLines() const {
  return QSet<int>(lineAddresses_.keyBegin(), lineAddresses_.keyEnd());
}

bool Simulator::instructionAt(uint32_t addr, uint32_t* encoding) const {
  const auto it = encodings_.find(addr);
  if (it == encodings_.end()) return false;
  *encoding = it.value();
  return true;
}

bool Simulator::atSyscall() const {
  uint32_t encoding;
  if (!instructionAt(PC, &encoding)) {
    // Not the user's code: look at the instruction itself.
    instruction* inst = nullptr;
    if (TEXT_BOT <= PC && PC < text_top) {
      inst = text_seg[(PC - TEXT_BOT) >> 2];
    } else if (K_TEXT_BOT <= PC && PC < k_text_top) {
      inst = k_text_seg[(PC - K_TEXT_BOT) >> 2];
    }
    if (inst == nullptr || inst_is_breakpoint(PC)) return false;
    encoding = ENCODING(inst);
  }
  return (encoding & 0xFC00003F) == 0x0000000C;
}

QString Simulator::builtInExceptionHandler() {
  if (handlerPath_.isEmpty() && tempDir_.isValid()) {
    // Copy exactly the resource's bytes (see the history of spimview.cpp:
    // writing it as a C string appended unrelated memory on macOS).
    const QString path = tempDir_.filePath(QStringLiteral("exceptions.s"));
    QFile resource(QStringLiteral(":/exceptions.s"));
    QFile file(path);
    if (resource.open(QIODevice::ReadOnly) &&
        file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      file.write(resource.readAll());
      file.close();
      handlerPath_ = path;
    }
  }
  return handlerPath_;
}

void Simulator::initializeWorld() {
  if (loadExceptionHandler) {
    QString path = exceptionHandlerFile;
    bool builtIn = path.isEmpty();
    if (!builtIn && !QFileInfo::exists(path)) {
      Problem p;
      p.severity = Problem::Severity::Warning;
      p.kind = Problem::Kind::Other;
      p.message = tr("The exception handler %1 was not found, so the "
                     "built-in exception handler is used.")
                      .arg(path);
      p.raw = p.message;
      problems_ << p;
      builtIn = true;
    }
    if (builtIn) path = builtInExceptionHandler();
    QByteArray name = QFile::encodeName(path);
    initialize_world(name.data(), !builtIn);
  } else {
    initialize_world(nullptr, true);
  }
}

void Simulator::start() {
  if (isBusy()) return;
  problems_.clear();
  loading_ = true;
  initializeWorld();
  loading_ = false;
  locations_.clear();
  lineAddresses_.clear();
  userAddresses_.clear();
  encodings_.clear();
  labels_.clear();
  breakpoints_.clear();
  input_.clear();
  inputBytes_.clear();
  setState(State::NoProgram);
  emit programLoaded();
  emit machineChanged();
}

bool Simulator::assemble(const QString& source, const QString& fileName,
                         const QString& argv0) {
  if (isBusy()) return false;
  source_ = source;
  fileName_ = fileName.isEmpty() ? QStringLiteral("untitled.s") : fileName;
  argv0_ = argv0.isEmpty() ? fileName_ : argv0;
  loadedPath_ = tempDir_.filePath(fileName_);
  QFile file(loadedPath_);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    problems_.clear();
    Problem p;
    p.message = tr("Cannot write the temporary file %1.").arg(loadedPath_);
    p.raw = p.message;
    problems_ << p;
    setState(State::AssemblyFailed);
    return false;
  }
  file.write(source.toUtf8());
  file.close();
  return reassemble();
}

bool Simulator::reassemble() {
  if (isBusy() || loadedPath_.isEmpty()) return false;

  problems_.clear();
  loadMessages_.clear();
  runError_ = false;
  runErrorMessage_.clear();
  exitCode_ = 0;
  spim_return_value = 0;
  lastException_ = -1;
  input_.clear();
  inputBytes_.clear();
  pauseReason_ = PauseReason::None;

  // The exception handler's instructions, to tell them from the program's.
  loading_ = true;
  initializeWorld();
  QHash<uint32_t, void*> before;
  for (mem_addr a = TEXT_BOT; a < text_top; a += BYTES_PER_WORD) {
    instruction* inst = text_seg[(a - TEXT_BOT) >> 2];
    if (inst != nullptr) before.insert(a, inst);
  }
  for (mem_addr a = K_TEXT_BOT; a < k_text_top; a += BYTES_PER_WORD) {
    instruction* inst = k_text_seg[(a - K_TEXT_BOT) >> 2];
    if (inst != nullptr) before.insert(a, inst);
  }

  QByteArray name = QFile::encodeName(loadedPath_);
  read_assembly_file(name.data());
  loading_ = false;

  buildSourceMap(before);
  parseProblems(loadMessages_);

  char* undefined = undefined_symbol_string();
  if (undefined != nullptr) {
    const QStringList symbols = QString::fromUtf8(undefined).split(
        QLatin1Char('\n'), Qt::SkipEmptyParts);
    free(undefined);
    const QStringList lines = source_.split(QLatin1Char('\n'));
    for (const QString& symbol : symbols) {
      Problem p;
      p.severity = Problem::Severity::Error;
      p.kind = Problem::Kind::UndefinedSymbol;
      p.symbol = symbol;
      p.message = QStringLiteral("undefined symbol: %1").arg(symbol);
      p.raw = QStringLiteral("The following symbols are undefined:\n%1")
                  .arg(symbol);
      // The first line that uses the symbol.
      const QRegularExpression use(
          QStringLiteral("(^|[^A-Za-z0-9_.$])%1($|[^A-Za-z0-9_.$:])")
              .arg(QRegularExpression::escape(symbol)));
      for (int i = 0; i < lines.size(); ++i) {
        QString code = lines[i];
        const int hash = code.indexOf(QLatin1Char('#'));
        if (hash >= 0) code.truncate(hash);
        if (use.match(code).hasMatch()) {
          p.line = i + 1;
          p.context = lines[i];
          break;
        }
      }
      problems_ << p;
    }
  }

  collectLabels();
  breakpoints_.clear();
  applyBreakpoints();
  initializePcAndStack();

  consoleDecoder().resetState();
  emit consoleCleared();
  setState(errorCount() > 0 ? State::AssemblyFailed : State::Ready);
  emit programLoaded();
  emit machineChanged();
  return errorCount() == 0;
}

void Simulator::invalidate() {
  if (isBusy()) return;
  if (state_ != State::NoProgram) setState(State::NeedsAssembly);
}

void Simulator::ignoreErrors() {
  if (state_ != State::AssemblyFailed || loadedPath_.isEmpty()) return;
  setState(State::Ready);
  applyBreakpoints();
}

void Simulator::buildSourceMap(const QHash<uint32_t, void*>& before) {
  locations_.clear();
  lineAddresses_.clear();
  userAddresses_.clear();
  encodings_.clear();

  auto scan = [&](instruction** segment, mem_addr bottom, mem_addr top) {
    int lastLine = 0;
    mem_addr lastAddr = 0;
    for (mem_addr a = bottom; a < top; a += BYTES_PER_WORD) {
      instruction* inst = segment[(a - bottom) >> 2];
      if (inst == nullptr || before.value(a) == inst) {
        lastLine = 0;
        continue;
      }
      // SOURCE is "N: text" for the first instruction of a line and NULL
      // for the other instructions a pseudo-instruction expands to.
      int line = 0;
      if (SOURCE(inst) != nullptr) {
        line = std::atoi(SOURCE(inst));
      } else if (lastLine > 0 && lastAddr + BYTES_PER_WORD == a) {
        line = lastLine;
      }
      if (line <= 0) {
        lastLine = 0;
        continue;
      }
      SourceLocation loc;
      loc.line = line;
      locations_.insert(a, loc);
      lineAddresses_[line].append(a);
      userAddresses_.append(a);
      encodings_.insert(a, static_cast<uint32_t>(ENCODING(inst)));
      lastLine = line;
      lastAddr = a;
    }
  };
  scan(text_seg, TEXT_BOT, text_top);
  scan(k_text_seg, K_TEXT_BOT, k_text_top);

  for (auto it = lineAddresses_.cbegin(); it != lineAddresses_.cend(); ++it) {
    const QList<uint32_t>& addrs = it.value();
    for (int i = 0; i < addrs.size(); ++i) {
      SourceLocation& loc = locations_[addrs[i]];
      loc.index = i;
      loc.count = addrs.size();
    }
  }
}

void Simulator::parseProblems(const QStringList& messages) {
  static const QRegularExpression parserRe(
      QStringLiteral("^spim: \\(parser\\) ([^\\n]*?) on line (\\d+) of file "
                     "[^\\n]*\\n(.*)$"),
      QRegularExpression::DotMatchesEverythingOption);
  const QStringList lines = source_.split(QLatin1Char('\n'));

  for (const QString& text : messages) {
    Problem p;
    // Show the program's name, not the temporary file's path.
    p.raw = QString(text).replace(loadedPath_, fileName_).trimmed();
    const auto m = parserRe.match(text);
    if (m.hasMatch()) {
      p.kind = Problem::Kind::Parser;
      p.message = m.captured(1);
      const int reported = m.captured(2).toInt();
      // The source text SPIM printed: "\t  <line>\n\t  <caret>".
      QString context = m.captured(3);
      if (context.startsWith(QLatin1Char('\t'))) context.remove(0, 1);
      if (context.startsWith(QLatin1String("  "))) context.remove(0, 2);
      context = context.section(QLatin1Char('\n'), 0, 0);
      p.context = context;
      // For some errors SPIM reports the next line, because the scanner has
      // already read the end of the line (regression/README.md).  Use the
      // line whose text matches.
      const QString wanted = normalized(context);
      auto matches = [&](int line) {
        return 1 <= line && line <= lines.size() &&
               normalized(lines[line - 1]) == wanted;
      };
      if (matches(reported)) {
        p.line = reported;
      } else if (matches(reported - 1)) {
        p.line = reported - 1;
      } else {
        p.line = qBound(1, reported, std::max(1, int(lines.size())));
      }
      const bool warning =
          p.message.startsWith(QLatin1String("Warning")) ||
          p.message.startsWith(QLatin1String("immediate value")) ||
          p.message.startsWith(QLatin1String("Shift distance")) ||
          p.message.contains(QLatin1String("should only be used"));
      p.severity =
          warning ? Problem::Severity::Warning : Problem::Severity::Error;
    } else {
      p.kind = Problem::Kind::Other;
      p.message = p.raw;
      p.severity = p.raw.startsWith(QLatin1String("Warning"))
                       ? Problem::Severity::Warning
                       : Problem::Severity::Error;
    }
    problems_ << p;
  }
}

void Simulator::collectLabels() {
  labels_.clear();
  static const QRegularExpression re(
      QStringLiteral("^(g?)\\t(.*) at 0x([0-9a-fA-F]+)$"));
  const QStringList lines =
      symbolListing().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
  for (const QString& line : lines) {
    const auto m = re.match(line);
    if (!m.hasMatch()) continue;
    bool ok = false;
    const uint32_t addr = m.captured(3).toUInt(&ok, 16);
    if (!ok || addr == 0) continue;
    const QString name = m.captured(2);
    addLabel(addr, name);
  }

  // print_symbols only knows the global labels: SPIM removes a file's local
  // labels from the symbol table once the file is read.  The labels that
  // instructions use (la $t0, array; bnez $t1, loop) are still known to
  // those instructions, and a label on a line with code is at that line's
  // first instruction.
  for (uint32_t addr : userAddresses_) {
    instruction* inst = addr >= K_TEXT_BOT
                            ? k_text_seg[(addr - K_TEXT_BOT) >> 2]
                            : text_seg[(addr - TEXT_BOT) >> 2];
    if (inst == nullptr || EXPR(inst) == nullptr) continue;
    const label* l = EXPR(inst)->symbol;
    if (l != nullptr && l->addr != 0 && !l->const_flag && l->name != nullptr) {
      addLabel(static_cast<uint32_t>(l->addr), QString::fromUtf8(l->name));
    }
  }
  static const QRegularExpression labelRe(
      QStringLiteral("^\\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\\s*:"));
  const QStringList sourceLines = source_.split(QLatin1Char('\n'));
  for (int i = 0; i < sourceLines.size(); ++i) {
    const auto addrs = lineAddresses_.constFind(i + 1);
    if (addrs == lineAddresses_.constEnd() || addrs->isEmpty()) continue;
    const auto m = labelRe.match(sourceLines[i]);
    if (m.hasMatch()) addLabel(addrs->first(), m.captured(1));
  }
}

void Simulator::addLabel(uint32_t addr, const QString& name) {
  QString& names = labels_[addr];
  QStringList list =
      names.isEmpty() ? QStringList() : names.split(QStringLiteral(", "));
  if (list.contains(name)) return;
  list << name;
  // The program's labels before the exception handler's (__start, __eoth).
  std::stable_sort(list.begin(), list.end(),
                   [](const QString& a, const QString& b) {
                     return !a.startsWith(QLatin1Char('_')) &&
                            b.startsWith(QLatin1Char('_'));
                   });
  names = list.join(QStringLiteral(", "));
}

QString Simulator::symbolListing() {
  captured_.clear();
  capturing_ = true;
  print_symbols();
  capturing_ = false;
  return captured_;
}

void Simulator::setBreakpointLines(const QSet<int>& lines) {
  breakpointLines_ = lines;
  if (!hasProgram()) return;
  QSet<uint32_t> wanted;
  for (int line : lines) {
    const QList<uint32_t> addrs = lineAddresses_.value(line);
    if (!addrs.isEmpty()) wanted.insert(addrs.first());
  }
  for (uint32_t addr : QSet<uint32_t>(breakpoints_)) {
    if (!wanted.contains(addr)) {
      delete_breakpoint(addr);
      breakpoints_.remove(addr);
    }
  }
  for (uint32_t addr : wanted) {
    if (!breakpoints_.contains(addr)) {
      add_breakpoint(addr);
      breakpoints_.insert(addr);
    }
  }
}

void Simulator::applyBreakpoints() {
  for (int line : breakpointLines_) {
    const QList<uint32_t> addrs = lineAddresses_.value(line);
    if (addrs.isEmpty() || breakpoints_.contains(addrs.first())) continue;
    add_breakpoint(addrs.first());
    breakpoints_.insert(addrs.first());
  }
}

void Simulator::initializePcAndStack() {
  PC = startAddress != 0 ? startAddress : starting_address();
  QString commandLine = argv0_;
  if (!programArguments.trimmed().isEmpty()) {
    commandLine += QLatin1Char(' ') + programArguments;
  }
  const QByteArray args = commandLine.toLocal8Bit();
  initialize_stack(args.constData());
}

bool Simulator::resetRegisters() {
  if (isBusy()) return false;
  initialize_registers();
  emit machineChanged();
  return true;
}

void Simulator::setState(State s, PauseReason reason) {
  state_ = s;
  pauseReason_ = reason;
  emit stateChanged();
}

void Simulator::run() {
  if (state_ != State::Ready) return;
  if (PC == 0) initializePcAndStack();
  execute(Mode::Run);
}

void Simulator::step() {
  if (state_ != State::Ready && state_ != State::Paused) return;
  if (PC == 0) initializePcAndStack();
  execute(Mode::Step);
}

void Simulator::resume() {
  if (state_ != State::Paused) return;
  execute(Mode::Continue);
}

void Simulator::pause() {
  if (state_ == State::Running) pauseRequested_ = true;
}

void Simulator::stop() {
  if (state_ == State::Running || state_ == State::WaitingForInput) {
    stopRequested_ = true;
    force_break = true;
    if (inputLoop_ != nullptr) inputLoop_->quit();
  } else if (state_ == State::Paused) {
    setState(State::Stopped);
  }
}

bool Simulator::runSlice(int steps, bool contBkpt, bool* breakpoint) {
  bool continuable = false;
  *breakpoint = run_program(PC, steps, false, contBkpt, &continuable);
  return continuable;
}

void Simulator::execute(Mode mode) {
  emit aboutToExecute();
  const bool fromStart = state_ == State::Ready;
  stopRequested_ = false;
  pauseRequested_ = false;
  runError_ = false;
  lastException_ = -1;

  // A single step usually finishes at once; only show "Running" when it
  // does not (input, a long detour through kernel code).
  state_ = State::Running;
  if (mode != Mode::Step) emit stateChanged();

  auto finished = [&](bool breakpoint, bool continuable) -> bool {
    if (runError_) {
      setState(State::RunError);
    } else if (breakpoint) {
      setState(State::Paused, PauseReason::Breakpoint);
    } else if (!continuable) {
      exitCode_ = spim_return_value;
      setState(State::Finished);
    } else if (stopRequested_) {
      setState(State::Stopped);
    } else if (pauseRequested_) {
      setState(State::Paused, PauseReason::User);
    } else {
      return false;
    }
    return true;
  };

  if (mode == Mode::Step) {
    bool breakpoint = false;
    bool continuable = true;
    bool done = false;
    // From the start, run the startup code up to the program's first
    // instruction; otherwise execute one instruction.
    if (!fromStart || stepIntoKernel) {
      continuable = runSlice(1, true, &breakpoint);
      if (PC == EXCEPTION_ADDR) {
        lastException_ = CP0_ExCode;
        lastExceptionPc_ = CP0_EPC;
      }
      done = finished(breakpoint, continuable);
    }
    int skipped = 0;
    while (!done && !stepIntoKernel && !isUserAddress(PC)) {
      continuable = runSlice(1, true, &breakpoint);
      if (PC == EXCEPTION_ADDR && lastException_ < 0) {
        lastException_ = CP0_ExCode;
        lastExceptionPc_ = CP0_EPC;
      }
      done = finished(breakpoint, continuable);
      if (++skipped % 10000 == 0 && !done) {
        if (skipped == 10000) emit stateChanged();
        QCoreApplication::processEvents();
        done = finished(false, true);
      }
      if (skipped >= kSkipLimit && !done) {
        setState(State::Paused, PauseReason::Step);
        done = true;
      }
    }
    if (!done) setState(State::Paused, PauseReason::Step);
  } else {
    bool contBkpt = mode == Mode::Continue;
    for (;;) {
      bool breakpoint = false;
      const bool continuable = runSlice(kRunSlice, contBkpt, &breakpoint);
      contBkpt = false;
      if (finished(breakpoint, continuable)) break;
      QCoreApplication::processEvents();
      if (finished(false, true)) break;
    }
  }
  stopRequested_ = false;
  pauseRequested_ = false;
  emit machineChanged();
}

void Simulator::provideInput(const QString& text) {
  input_ += text;
  if (inputLoop_ != nullptr) inputLoop_->quit();
}

// --- Callbacks from the core ---

void Simulator::coreOutput(bool console, const QByteArray& bytes) {
  // Console output may split a UTF-8 character between print_char calls.
  const QString text =
      console ? consoleDecoder().decode(bytes) : QString::fromUtf8(bytes);
  if (capturing_ && !console) {
    captured_ += text;
  } else if (loading_ && !console) {
    // "Loaded: <exception handler>" and the like.
  } else if (console) {
    emit consoleOutput(text);
  } else {
    emit simulatorMessage(text, false);
  }
}

void Simulator::coreError(const QString& text) {
  if (loading_) {
    loadMessages_ << text;
  } else {
    emit simulatorMessage(text, true);
  }
}

void Simulator::coreRunError(const QString& text) {
  if (classic_) {
    // The original QtSpim shows the message and lets the user go on.
    emit simulatorMessage(text, true);
    return;
  }
  // The command-line spim abandons the program on a run error; so does the
  // new interface (the original QtSpim asked whether to go on).
  runError_ = true;
  runErrorMessage_ = text.trimmed();
  force_break = true;
  emit simulatorMessage(text, true);
}

int Simulator::coreReadChar() {
  if (classic_) {
    // As the original console: wait for a key in a nested event loop; a
    // nested request gets a newline.
    if (inputLoop_ != nullptr) return '\n';
    if (input_.isEmpty() && inputBytes_.isEmpty()) emit inputRequested();
    while (input_.isEmpty() && inputBytes_.isEmpty()) {
      QEventLoop loop;
      inputLoop_ = &loop;
      loop.exec();
      inputLoop_ = nullptr;
    }
  }
  while (input_.isEmpty() && inputBytes_.isEmpty()) {
    if (stopRequested_) {
      force_break = true;
      return 0;
    }
    waitingSyscall_ = R[REG_V0];
    setState(State::WaitingForInput);
    emit inputRequested();
    QEventLoop loop;
    inputLoop_ = &loop;
    loop.exec();
    inputLoop_ = nullptr;
    waitingSyscall_ = 0;
    if (stopRequested_) {
      force_break = true;
      return 0;
    }
    state_ = State::Running;
    emit stateChanged();
  }
  // Input is passed to the program as UTF-8 bytes.
  if (inputBytes_.isEmpty()) {
    inputBytes_ = input_.left(1).toUtf8();
    input_.remove(0, 1);
  }
  const char c = inputBytes_.at(0);
  inputBytes_.remove(0, 1);
  return c == '\r' ? '\n' : static_cast<unsigned char>(c);
}
