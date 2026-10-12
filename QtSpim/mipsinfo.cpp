/* SPIM S20 MIPS simulator.
   MIPS names and plain-language explanations used by the interface.

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

#include "mipsinfo.h"

#include <QCoreApplication>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

namespace {

struct OpEntry {
  const char* name;
  int type;
};

// CPU/op.h lists every instruction, pseudo-instruction and directive the
// parser accepts, with its type (and defines the type constants).  Only the
// name and the type are used here, so the parser's token values (Y_..._OP)
// are not needed.
const OpEntry kOps[] = {
#undef OP
#define OP(NAME, OPCODE, TYPE, R_OPCODE) {NAME, TYPE},
#include "op.h"
#undef OP
};

class MipsText {
  Q_DECLARE_TR_FUNCTIONS(Mips)
};

struct OpTable {
  QHash<QString, int> types;
  QStringList instructions;
  QStringList directives;

  OpTable() {
    for (const OpEntry& op : kOps) {
      const QString name = QString::fromLatin1(op.name);
      if (types.contains(name)) continue;
      types.insert(name, op.type);
      if (op.type == ASM_DIR) {
        directives << name;
      } else {
        instructions << name;
      }
    }
  }
};

const OpTable& ops() {
  static const OpTable table;
  return table;
}

// Instructions students use most; they win ties between suggestions.
const QSet<QString>& commonInstructions() {
  static const QSet<QString> common = {
      "add",  "addi", "addiu", "addu", "and",  "andi",    "b",    "beq",
      "beqz", "bge",  "bgt",   "ble",  "blt",  "bne",     "bnez", "div",
      "j",    "jal",  "jr",    "la",   "lb",   "lbu",     "lh",   "li",
      "lui",  "lw",   "mfhi",  "mflo", "move", "mul",     "mult", "nop",
      "nor",  "or",   "ori",   "sb",   "sh",   "sll",     "slt",  "slti",
      "sra",  "srl",  "sub",   "subu", "sw",   "syscall", "xor",  "xori"};
  return common;
}

int editDistance(const QString& a, const QString& b) {
  QVector<int> prev(b.size() + 1), cur(b.size() + 1);
  for (int j = 0; j <= b.size(); ++j) prev[j] = j;
  for (int i = 1; i <= a.size(); ++i) {
    cur[0] = i;
    for (int j = 1; j <= b.size(); ++j) {
      const int cost = a[i - 1] == b[j - 1] ? 0 : 1;
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
      // Transposition ("lw" typed as "wl").
      if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
        cur[j] = std::min(cur[j], prev[j - 1]);
      }
    }
    std::swap(prev, cur);
  }
  return prev[b.size()];
}

// How each kind of instruction is written, for "write it like this" hints.
QString operandFormat(const QString& name) {
  static const QHash<QString, QString> pseudo = {
      {"li", "$rd, number"},       {"la", "$rd, label"},
      {"move", "$rd, $rs"},        {"b", "label"},
      {"beqz", "$rs, label"},      {"bnez", "$rs, label"},
      {"blt", "$rs, $rt, label"},  {"bgt", "$rs, $rt, label"},
      {"ble", "$rs, $rt, label"},  {"bge", "$rs, $rt, label"},
      {"bltu", "$rs, $rt, label"}, {"bgtu", "$rs, $rt, label"},
      {"bleu", "$rs, $rt, label"}, {"bgeu", "$rs, $rt, label"},
      {"mul", "$rd, $rs, $rt"},    {"rem", "$rd, $rs, $rt"},
      {"remu", "$rd, $rs, $rt"},   {"not", "$rd, $rs"},
      {"neg", "$rd, $rs"},         {"negu", "$rd, $rs"},
      {"abs", "$rd, $rs"},         {"seq", "$rd, $rs, $rt"},
      {"sne", "$rd, $rs, $rt"},    {"sge", "$rd, $rs, $rt"},
      {"sgt", "$rd, $rs, $rt"},    {"sle", "$rd, $rs, $rt"},
      {"li.s", "$fd, number"},     {"li.d", "$fd, number"},
      {"l.s", "$ft, address"},     {"l.d", "$ft, address"},
      {"s.s", "$ft, address"},     {"s.d", "$ft, address"},
      {"ld", "$rt, address"},      {"sd", "$rt, address"},
      {"ulw", "$rt, address"},     {"usw", "$rt, address"},
      {"ulh", "$rt, address"},     {"ush", "$rt, address"},
  };
  const auto p = pseudo.find(name);
  if (p != pseudo.end()) return name + ' ' + p.value();

  switch (ops().types.value(name, -1)) {
    case R3_TYPE_INST:
      return name + " $rd, $rs, $rt";
    case R3sh_TYPE_INST:
      return name + " $rd, $rt, $rs";
    case R2sh_TYPE_INST:
      return name + " $rd, $rt, shift";
    case I2_TYPE_INST:
      return name + " $rt, $rs, number";
    case I2a_TYPE_INST:
      return name + " $rt, offset($rs)";
    case I1t_TYPE_INST:
      return name + " $rt, number";
    case I1s_TYPE_INST:
      return name + " $rs, number";
    case B2_TYPE_INST:
      return name + " $rs, $rt, label";
    case B1_TYPE_INST:
      return name + " $rs, label";
    case J_TYPE_INST:
      return name + " label";
    case R1s_TYPE_INST:
      return name + " $rs";
    case R1d_TYPE_INST:
      return name + " $rd";
    case R2st_TYPE_INST:
      return name + " $rs, $rt";
    case R2ds_TYPE_INST:
      return name + " $rd, $rs";
    case R2td_TYPE_INST:
      return name + " $rt, $rd";
    case NOARG_TYPE_INST:
      return name;
    case FP_R3_TYPE_INST:
      return name + " $fd, $fs, $ft";
    case FP_R2ds_TYPE_INST:
      return name + " $fd, $fs";
    case FP_R2ts_TYPE_INST:
      return name + " $rt, $fs";
    case FP_I2a_TYPE_INST:
      return name + " $ft, offset($rs)";
    case FP_CMP_TYPE_INST:
      return name + " $fs, $ft";
    default:
      return QString();
  }
}

QString joinAlternatives(const QStringList& words) {
  switch (words.size()) {
    case 0:
      return QString();
    case 1:
      return words[0];
    case 2:
      return MipsText::tr("%1 or %2").arg(words[0], words[1]);
    default:
      return MipsText::tr("%1, %2, or %3").arg(words[0], words[1], words[2]);
  }
}

// Splits the code part of a source line into the label(s) and the rest.
struct Tokens {
  QStringList labels;
  QString first;  // instruction or directive
  int firstColumn = -1;
  QString second;  // next word, if any
  QString rest;    // operands
  int restColumn = -1;
};

Tokens tokenize(const QString& line) {
  Tokens t;
  const QString code = Mips::stripComment(line);
  static const QRegularExpression labelRe(
      QStringLiteral("^\\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\\s*:"));
  int pos = 0;
  for (;;) {
    const auto m = labelRe.match(code.mid(pos));
    if (!m.hasMatch()) break;
    t.labels << m.captured(1);
    pos += m.capturedEnd(0);
  }
  static const QRegularExpression wordRe(QStringLiteral("\\S+"));
  auto m = wordRe.match(code, pos);
  if (!m.hasMatch()) return t;
  // The instruction ends at white space or at the first comma or quote.
  QString word = m.captured(0);
  static const QRegularExpression opEnd(QStringLiteral("[,\"(]"));
  const int cut = word.indexOf(opEnd);
  if (cut > 0) word.truncate(cut);
  t.first = word;
  t.firstColumn = m.capturedStart(0);
  const int restStart = t.firstColumn + word.size();
  t.rest = code.mid(restStart).trimmed();
  t.restColumn = restStart;
  const auto m2 = wordRe.match(code, restStart);
  if (m2.hasMatch()) t.second = m2.captured(0).section(QLatin1Char(','), 0, 0);
  return t;
}

}  // namespace

namespace Mips {

OpKind opKind(const QString& name) {
  const auto it = ops().types.find(name);
  if (it == ops().types.end()) return OpKind::None;
  if (it.value() == ASM_DIR) return OpKind::Directive;
  if (it.value() == PSEUDO_OP) return OpKind::Pseudo;
  return OpKind::Instruction;
}

const QStringList& instructionNames() { return ops().instructions; }

const QStringList& directiveNames() { return ops().directives; }

bool isRegister(const QString& token) {
  static const QRegularExpression re(QStringLiteral(
      "^\\$(?:[0-9]|[12][0-9]|3[01]|zero|at|v[01]|a[0-3]|t[0-9]|s[0-8]|"
      "k[01]|gp|sp|fp|ra|f(?:[0-9]|[12][0-9]|3[01]))$"));
  return re.match(token).hasMatch();
}

QStringList suggestions(const QString& word, const QStringList& candidates,
                        int max) {
  const QString w = word.toLower();
  if (w.isEmpty()) return {};
  struct Scored {
    double score;
    QString name;
  };
  QVector<Scored> scored;
  const int limit = w.size() <= 3 ? 2 : 3;
  for (const QString& c : candidates) {
    const int d = editDistance(w, c);
    if (d == 0 || d > limit) continue;
    double s = d;
    if (c.startsWith(w)) s -= 1.0;
    if (commonInstructions().contains(c)) s -= 0.3;
    scored.push_back({s, c});
  }
  std::stable_sort(
      scored.begin(), scored.end(),
      [](const Scored& a, const Scored& b) { return a.score < b.score; });
  QStringList result;
  for (const Scored& s : scored) {
    if (result.size() == max) break;
    result << s.name;
  }
  return result;
}

QString registerName(int n) {
  static const char* const names[32] = {
      "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2",
      "t3",   "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5",
      "s6",   "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
  return (0 <= n && n < 32) ? QString::fromLatin1(names[n]) : QString();
}

QString registerUsage(int n) {
  switch (n) {
    case -1:
      return MipsText::tr(
          "Program counter: address of the next instruction to execute");
    case -2:
      return MipsText::tr("High word of a mult result, remainder of a div");
    case -3:
      return MipsText::tr("Low word of a mult result, quotient of a div");
    case 0:
      return MipsText::tr("Always 0");
    case 1:
      return MipsText::tr("Reserved for the assembler");
    case 2:
    case 3:
      return MipsText::tr("Function results and the syscall number");
    case 26:
    case 27:
      return MipsText::tr("Reserved for the exception handler");
    case 28:
      return MipsText::tr("Global pointer");
    case 29:
      return MipsText::tr("Stack pointer");
    case 30:
      return MipsText::tr("Frame pointer");
    case 31:
      return MipsText::tr("Return address");
    default:
      break;
  }
  if (4 <= n && n <= 7) return MipsText::tr("Function arguments");
  if ((8 <= n && n <= 15) || n == 24 || n == 25) {
    return MipsText::tr("Temporaries (not preserved across calls)");
  }
  if (16 <= n && n <= 23) {
    return MipsText::tr("Saved (preserved across calls)");
  }
  return QString();
}

bool syscall(int number, Syscall* out) {
  struct Entry {
    int number;
    const char* name;
    const char* title;
    const char* description;
    bool input;
  };
  static const Entry entries[] = {
      {1, "print_int", QT_TRANSLATE_NOOP("Mips", "print integer"),
       QT_TRANSLATE_NOOP("Mips", "prints the integer in $a0"), false},
      {2, "print_float", QT_TRANSLATE_NOOP("Mips", "print float"),
       QT_TRANSLATE_NOOP("Mips", "prints the float in $f12"), false},
      {3, "print_double", QT_TRANSLATE_NOOP("Mips", "print double"),
       QT_TRANSLATE_NOOP("Mips", "prints the double in $f12"), false},
      {4, "print_string", QT_TRANSLATE_NOOP("Mips", "print string"),
       QT_TRANSLATE_NOOP("Mips",
                         "prints the zero-terminated string at the address "
                         "in $a0"),
       false},
      {5, "read_int", QT_TRANSLATE_NOOP("Mips", "read integer"),
       QT_TRANSLATE_NOOP("Mips", "the number read is stored in $v0"), true},
      {6, "read_float", QT_TRANSLATE_NOOP("Mips", "read float"),
       QT_TRANSLATE_NOOP("Mips", "the number read is stored in $f0"), true},
      {7, "read_double", QT_TRANSLATE_NOOP("Mips", "read double"),
       QT_TRANSLATE_NOOP("Mips", "the number read is stored in $f0"), true},
      {8, "read_string", QT_TRANSLATE_NOOP("Mips", "read string"),
       QT_TRANSLATE_NOOP("Mips",
                         "reads a line into the buffer at $a0, at most $a1 - "
                         "1 characters"),
       true},
      {9, "sbrk", QT_TRANSLATE_NOOP("Mips", "allocate memory"),
       QT_TRANSLATE_NOOP("Mips",
                         "allocates $a0 bytes; their address is stored in "
                         "$v0"),
       false},
      {10, "exit", QT_TRANSLATE_NOOP("Mips", "exit"),
       QT_TRANSLATE_NOOP("Mips", "ends the program"), false},
      {11, "print_char", QT_TRANSLATE_NOOP("Mips", "print character"),
       QT_TRANSLATE_NOOP("Mips", "prints the character in $a0"), false},
      {12, "read_char", QT_TRANSLATE_NOOP("Mips", "read character"),
       QT_TRANSLATE_NOOP("Mips", "the character read is stored in $v0"), true},
      {13, "open", QT_TRANSLATE_NOOP("Mips", "open file"),
       QT_TRANSLATE_NOOP("Mips",
                         "opens the file named at $a0 (flags $a1, mode $a2); "
                         "the file descriptor is stored in $v0"),
       false},
      {14, "read", QT_TRANSLATE_NOOP("Mips", "read from file"),
       QT_TRANSLATE_NOOP("Mips",
                         "reads $a2 bytes from file $a0 into $a1; the number "
                         "read is stored in $v0"),
       false},
      {15, "write", QT_TRANSLATE_NOOP("Mips", "write to file"),
       QT_TRANSLATE_NOOP("Mips",
                         "writes $a2 bytes from $a1 to file $a0; the number "
                         "written is stored in $v0"),
       false},
      {16, "close", QT_TRANSLATE_NOOP("Mips", "close file"),
       QT_TRANSLATE_NOOP("Mips", "closes file $a0"), false},
      {17, "exit2", QT_TRANSLATE_NOOP("Mips", "exit with a value"),
       QT_TRANSLATE_NOOP("Mips", "ends the program with exit code $a0"), false},
  };
  for (const Entry& e : entries) {
    if (e.number == number) {
      if (out != nullptr) {
        out->number = e.number;
        out->name = QString::fromLatin1(e.name);
        out->title = MipsText::tr(e.title);
        out->description = MipsText::tr(e.description);
        out->readsInput = e.input;
      }
      return true;
    }
  }
  return false;
}

QString exceptionName(int code) {
  switch (code) {
    case 0:
      return MipsText::tr("interrupt");
    case 4:
      return MipsText::tr("address error on load or instruction fetch");
    case 5:
      return MipsText::tr("address error on store");
    case 6:
      return MipsText::tr("bad instruction address");
    case 7:
      return MipsText::tr("bad data address");
    case 8:
      return MipsText::tr("syscall error");
    case 9:
      return MipsText::tr("breakpoint");
    case 10:
      return MipsText::tr("reserved instruction");
    case 11:
      return MipsText::tr("coprocessor unusable");
    case 12:
      return MipsText::tr("arithmetic overflow");
    case 13:
      return MipsText::tr("trap");
    case 15:
      return MipsText::tr("floating-point exception");
    default:
      return MipsText::tr("exception %1").arg(code);
  }
}

QString stripComment(const QString& line) {
  bool inString = false;
  bool inChar = false;
  for (int i = 0; i < line.size(); ++i) {
    const QChar c = line[i];
    if ((inString || inChar) && c == QLatin1Char('\\')) {
      ++i;
    } else if (c == QLatin1Char('"') && !inChar) {
      inString = !inString;
    } else if (c == QLatin1Char('\'') && !inString) {
      inChar = !inChar;
    } else if (c == QLatin1Char('#') && !inString && !inChar) {
      return line.left(i);
    }
  }
  return line;
}

QString formatWord(uint32_t value, int base) {
  switch (base) {
    case 10:
      return QString::number(static_cast<int32_t>(value));
    case 2:
      return QStringLiteral("%1").arg(value, 32, 2, QLatin1Char('0'));
    default:
      return QStringLiteral("0x%1").arg(value, 8, 16, QLatin1Char('0'));
  }
}

bool parseWord(const QString& text, int base, uint32_t* value) {
  QString t = text.trimmed();
  t.remove(QLatin1Char('_'));
  t.remove(QLatin1Char(' '));
  if (t.isEmpty()) return false;
  bool ok = false;
  qlonglong v = 0;
  if (t.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
    v = t.mid(2).toLongLong(&ok, 16);
  } else if (t.startsWith(QLatin1String("0b"), Qt::CaseInsensitive)) {
    v = t.mid(2).toLongLong(&ok, 2);
  } else if (t.startsWith(QLatin1Char('-')) || t.startsWith(QLatin1Char('+'))) {
    v = t.toLongLong(&ok, 10);
  } else {
    v = t.toLongLong(&ok, base == 16 || base == 2 ? base : 10);
  }
  if (!ok || v < -2147483648LL || v > 4294967295LL) return false;
  *value = static_cast<uint32_t>(v);
  return true;
}

Explanation explainParserMessage(const QString& message,
                                 const QString& lineText) {
  Explanation e;
  const Tokens t = tokenize(lineText);
  const QString code = stripComment(lineText);

  if (message.compare(QLatin1String("syntax error"), Qt::CaseInsensitive) ==
      0) {
    // Characters from a Chinese input method are the most common surprise.
    static const QRegularExpression wide(QStringLiteral("[^\\x00-\\x7F]"));
    const auto wm = wide.match(code);
    if (wm.hasMatch() && !code.contains(QLatin1Char('"'))) {
      e.text = MipsText::tr(
                   "This line contains “%1”, which the assembler does not "
                   "understand. Use half-width characters (switch the input "
                   "method to "
                   "English) for commas, spaces and parentheses.")
                   .arg(wm.captured(0));
      e.column = wm.capturedStart(0);
      e.length = 1;
      return e;
    }
    if (t.first.isEmpty()) {
      e.text = MipsText::tr(
          "This line is not written correctly. Check the label and its "
          "colon.");
      return e;
    }
    const QString first = t.first.toLower();
    if (first.startsWith(QLatin1Char('.'))) {
      if (opKind(first) != OpKind::Directive) {
        const QStringList s = suggestions(first, directiveNames());
        e.text = s.isEmpty()
                     ? MipsText::tr("“%1” is not an assembler directive.")
                           .arg(t.first)
                     : MipsText::tr(
                           "“%1” is not an assembler directive. Did "
                           "you mean %2?")
                           .arg(t.first, joinAlternatives(s));
        e.column = t.firstColumn;
        e.length = t.first.size();
        return e;
      }
    } else if (opKind(first) == OpKind::None) {
      if (t.labels.isEmpty() && opKind(t.second.toLower()) != OpKind::None &&
          !t.second.startsWith(QLatin1Char('.'))) {
        e.text = MipsText::tr(
                     "Is a colon missing after the label “%1”? Write “%1:”.")
                     .arg(t.first);
      } else {
        const QStringList s = suggestions(first, instructionNames());
        e.text =
            s.isEmpty()
                ? MipsText::tr("“%1” is not a MIPS instruction.").arg(t.first)
                : MipsText::tr(
                      "“%1” is not a MIPS instruction. Did you "
                      "mean %2?")
                      .arg(t.first, joinAlternatives(s));
      }
      e.column = t.firstColumn;
      e.length = t.first.size();
      return e;
    }
    // A known instruction or directive: look at the operands.
    static const QRegularExpression regRe(QStringLiteral("\\$[A-Za-z0-9]*"));
    auto it = regRe.globalMatch(code, std::max(0, t.restColumn));
    while (it.hasNext()) {
      const auto m = it.next();
      if (!isRegister(m.captured(0))) {
        e.text = MipsText::tr(
                     "“%1” is not a register. Use $t0–$t9, $s0–$s7, $a0–$a3, "
                     "$v0–$v1, $sp, $ra … or $0–$31.")
                     .arg(m.captured(0));
        e.column = m.capturedStart(0);
        e.length = m.capturedLength(0);
        return e;
      }
    }
    if (t.labels.contains(t.first)) {
      // Not reached: labels are split off above.
    }
    const QString format = operandFormat(first);
    if (opKind(first) == OpKind::Directive) {
      e.text = MipsText::tr("The operands of %1 are not written correctly.")
                   .arg(first);
    } else if (!format.isEmpty()) {
      e.text = MipsText::tr(
                   "The operands of %1 are not written correctly. Write it "
                   "like this: %2")
                   .arg(first, format);
    } else {
      e.text = MipsText::tr(
                   "The operands of %1 are not written correctly. Check "
                   "their number, the commas and the register names.")
                   .arg(first);
    }
    if (t.restColumn >= 0) {
      e.column = t.firstColumn;
      e.length = code.trimmed().size();
    }
    return e;
  }

  static const QRegularExpression rangeRe(QStringLiteral(
      "^immediate value \\((-?\\d+)\\) out of range \\((-?\\d+) \\.\\. "
      "(-?\\d+)\\)"));
  const auto rm = rangeRe.match(message);
  if (rm.hasMatch()) {
    e.text = MipsText::tr(
                 "The number %1 does not fit in this instruction, which takes "
                 "%2 to %3; only its low 16 bits are kept. Load large numbers "
                 "into a register with li first.")
                 .arg(rm.captured(1), rm.captured(2), rm.captured(3));
    return e;
  }

  struct Known {
    const char* prefix;
    const char* text;
  };
  static const Known known[] = {
      {"Shift distance can only be in the range 0..31",
       QT_TRANSLATE_NOOP("Mips", "The shift amount must be from 0 to 31.")},
      {"Label is defined for the second time",
       QT_TRANSLATE_NOOP("Mips",
                         "This label is already defined on another line. A "
                         "label can only be defined once.")},
      {"Warning: MIPS32 Rev 2 instruction is not implemented",
       QT_TRANSLATE_NOOP("Mips",
                         "SPIM does not implement this MIPS32 Release 2 "
                         "instruction, so this line is ignored.")},
      {"Warning:IPS32 Rev 2",
       QT_TRANSLATE_NOOP("Mips",
                         "SPIM does not implement this MIPS32 Release 2 "
                         "feature, so it is ignored.")},
      {"Can't put data in text segment",
       QT_TRANSLATE_NOOP("Mips",
                         "Data cannot go in the program (.text) section. Put "
                         ".word, .asciiz and other data after .data.")},
      {"Immediate form not allowed in bare machine",
       QT_TRANSLATE_NOOP("Mips",
                         "The bare machine does not accept a number here. Use "
                         "the immediate instruction (for example addi), or "
                         "switch to the Standard mode in Settings.")},
      {"Register 1 is reserved for assembler",
       QT_TRANSLATE_NOOP("Mips",
                         "$at ($1) is reserved for the assembler. Use another "
                         "register, such as $t0.")},
      {"Register number out of range",
       QT_TRANSLATE_NOOP("Mips", "There are only 32 registers, $0 to $31.")},
      {"FP register number out of range",
       QT_TRANSLATE_NOOP("Mips",
                         "There are only 32 floating-point registers, $f0 to "
                         "$f31.")},
      {"CC register number out of range",
       QT_TRANSLATE_NOOP("Mips", "Condition codes are numbered 0 to 7.")},
      {"Cannot use opcodes as labels",
       QT_TRANSLATE_NOOP("Mips",
                         "An instruction name cannot be used as a label. "
                         "Choose another name.")},
      {"Unknown character",
       QT_TRANSLATE_NOOP("Mips",
                         "This line contains a character the assembler does "
                         "not understand, such as a full-width comma or "
                         "space.")},
      {"Bad character",
       QT_TRANSLATE_NOOP("Mips",
                         "The string contains an unknown escape sequence. Use "
                         "\\n, \\t, \\\\ or \\\".")},
      {"Breakpoint 1 is reserved for debugger",
       QT_TRANSLATE_NOOP("Mips",
                         "break 1 is reserved for the debugger. Use another "
                         "code.")},
      {"Divide by zero",
       QT_TRANSLATE_NOOP("Mips", "This divides by the constant 0.")},
      {"REM requires 3 arguments",
       QT_TRANSLATE_NOOP("Mips",
                         "rem needs three operands: rem $rd, $rs, $rt.")},
      {"Warning: repeat directive ignored",
       QT_TRANSLATE_NOOP("Mips", "SPIM ignores this directive.")},
      {"Warning: struct directive ignored",
       QT_TRANSLATE_NOOP("Mips", "SPIM ignores this directive.")},
      {"RFE should only be used",
       QT_TRANSLATE_NOOP("Mips", "This SPIM simulates MIPS32: use eret.")},
      {"ERET should only be used",
       QT_TRANSLATE_NOOP("Mips",
                         "eret is a MIPS32 instruction and may not work "
                         "here.")},
  };
  for (const Known& k : known) {
    if (message.startsWith(QLatin1String(k.prefix))) {
      e.text = MipsText::tr(k.text);
      if (message.startsWith(QLatin1String("Cannot use opcodes")) &&
          !t.first.isEmpty()) {
        e.column = t.firstColumn;
        e.length = t.first.size();
      }
      return e;
    }
  }
  e.text = MipsText::tr("The assembler reported a problem on this line.");
  return e;
}

QString explainUndefinedSymbol(const QString& symbol) {
  if (symbol == QLatin1String("main")) {
    return MipsText::tr(
        "There is no main label. The program starts at main: — put “main:” "
        "before the first instruction and declare it with .globl main.");
  }
  return MipsText::tr(
             "The label “%1” is used but never defined. Check its spelling, "
             "or add “%1:” where it should be.")
      .arg(symbol);
}

QString explainRunError(const QString& message) {
  static const QRegularExpression nonInst(
      QStringLiteral("Attempt to execute non-instruction at (0x[0-9a-fA-F]+)"));
  auto m = nonInst.match(message);
  if (m.hasMatch()) {
    return MipsText::tr(
               "The program jumped to %1, where there is no instruction. "
               "Does main end with jr $ra or the exit syscall (li $v0, 10)?")
        .arg(m.captured(1));
  }
  if (message.startsWith(QLatin1String("Instruction references undefined"))) {
    return MipsText::tr("This instruction uses a label that is never defined.");
  }
  static const QRegularExpression unknownSyscall(
      QStringLiteral("Unknown system call: (-?\\d+)"));
  m = unknownSyscall.match(message);
  if (m.hasMatch()) {
    return MipsText::tr("There is no syscall %1. Check the number in $v0.")
        .arg(m.captured(1));
  }
  if (message.startsWith(QLatin1String("Memory address out of bounds"))) {
    return MipsText::tr(
        "The program used an address outside the data, stack and text "
        "segments.");
  }
  if (message.startsWith(QLatin1String("Odd FP double register"))) {
    return MipsText::tr(
        "Double-precision values use even-numbered registers ($f0, $f2, "
        "…).");
  }
  if (message.contains(QLatin1String("memory-mapped IO address"))) {
    return MipsText::tr(
        "The program used a memory-mapped I/O address that does not exist.");
  }
  return MipsText::tr("The simulator stopped the program.");
}

}  // namespace Mips
