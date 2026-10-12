/* SPIM S20 MIPS simulator.
   The memory panel: data segment, stack and machine code.

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

#include "memorypanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include <cstdlib>
#include <cstring>

#include "mipsinfo.h"
#include "simulator.h"
#include "theme.h"

#include "spim.h"
#include "string-stream.h"
#include "spim-utils.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"

namespace {

QString hex8(uint32_t v) {
  return QStringLiteral("0x%1").arg(v, 8, 16, QLatin1Char('0'));
}

// Labels at addresses [from, to).
QString labelsIn(uint32_t from, uint32_t to) {
  const QMap<uint32_t, QString>& labels = Simulator::instance()->labels();
  QStringList names;
  for (auto it = labels.lowerBound(from); it != labels.end() && it.key() < to;
       ++it) {
    names << it.value();
  }
  return names.join(QStringLiteral(", "));
}

// Draws a word with small tags for the registers that point to it.
class WordDelegate : public QStyledItemDelegate {
 public:
  using QStyledItemDelegate::QStyledItemDelegate;

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override {
    QStyledItemDelegate::paint(painter, option, index);
    const QString tag = index.data(WordsModel::TagRole).toString();
    if (tag.isEmpty()) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QFont f = Theme::monoFont(10);
    f.setBold(true);
    painter->setFont(f);
    const QFontMetrics fm(f);
    const int w = fm.horizontalAdvance(tag) + 8;
    const int h = fm.height() + 2;
    const QRect r(option.rect.right() - w - 4, option.rect.center().y() - h / 2,
                  w, h);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Theme::kAccentTint);
    painter->drawRoundedRect(r, Theme::kRadiusTag, Theme::kRadiusTag);
    painter->setPen(Theme::kAccentText);
    painter->drawText(r, Qt::AlignCenter, tag);
    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override {
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    const QString tag = index.data(WordsModel::TagRole).toString();
    if (!tag.isEmpty()) {
      s.rwidth() +=
          QFontMetrics(Theme::monoFont(10)).horizontalAdvance(tag) + 16;
    }
    return s;
  }
};

}  // namespace

// --- WordsModel ---

WordsModel::WordsModel(Segment segment, QObject* parent)
    : QAbstractTableModel(parent), segment_(segment) {}

int WordsModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : rows_.size();
}

int WordsModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : ColumnCount;
}

bool WordsModel::readWord(uint32_t addr, uint32_t* value) const {
  if (data_seg != nullptr && DATA_BOT <= addr && addr < data_top) {
    *value = static_cast<uint32_t>(data_seg[(addr - DATA_BOT) >> 2]);
  } else if (stack_seg != nullptr && stack_bot <= addr && addr < STACK_TOP) {
    *value = static_cast<uint32_t>(stack_seg[(addr - stack_bot) >> 2]);
  } else if (k_data_seg != nullptr && K_DATA_BOT <= addr && addr < k_data_top) {
    *value = static_cast<uint32_t>(k_data_seg[(addr - K_DATA_BOT) >> 2]);
  } else {
    return false;
  }
  return true;
}

QString WordsModel::ascii(uint32_t addr) const {
  QString s;
  for (uint32_t a = addr; a < addr + 16; a += 4) {
    uint32_t w = 0;
    if (!readWord(a, &w)) {
      s += QStringLiteral("    ");
      continue;
    }
    // Bytes in memory order (the simulator stores words in host order,
    // like its byte array data_seg_b).
    unsigned char bytes[4];
    std::memcpy(bytes, &w, 4);
    for (unsigned char b : bytes) {
      s += (b >= 32 && b < 127) ? QChar(b) : QChar(u'·');
    }
  }
  return s;
}

void WordsModel::addRange(uint32_t from, uint32_t to) {
  auto plainRow = [this](uint32_t a) {
    if (!labelsIn(a, a + 16).isEmpty()) return false;
    for (uint32_t w = a; w < a + 16; w += 4) {
      uint32_t v = 1;
      if (!readWord(w, &v) || v != 0 || tags_.contains(w)) return false;
    }
    return true;
  };
  uint32_t a = from & ~0xFu;
  while (a < to && a >= (from & ~0xFu)) {
    if (plainRow(a)) {
      uint32_t b = a + 16;
      while (b < to && b > a && plainRow(b)) b += 16;
      if (b - a >= 32) {
        rows_.push_back({Kind::Zeros, a, b - 1, QString()});
        if (b <= a) break;
        a = b;
        continue;
      }
    }
    rows_.push_back({Kind::Words, a, 0, QString()});
    if (a + 16 < a) break;  // wrapped around
    a += 16;
  }
}

void WordsModel::refresh() {
  QVector<Row> old = rows_;
  rows_.clear();
  tags_.clear();
  if (data_seg == nullptr) {
    beginResetModel();
    endResetModel();
    return;
  }
  for (int r = 2; r < 32; ++r) {
    if (r == 26 || r == 27) continue;
    const uint32_t v = static_cast<uint32_t>(R[r]);
    if (v != 0 && (v & 3) == 0) {
      tags_[v] << QLatin1Char('$') + Mips::registerName(r);
    }
  }
  if (segment_ == Segment::Data) {
    if (showKernel_) {
      rows_.push_back({Kind::Section, DATA_BOT, 0, tr("User data")});
    }
    addRange(DATA_BOT, data_top);
    if (showKernel_) {
      rows_.push_back({Kind::Section, K_DATA_BOT, 0, tr("Kernel data")});
      addRange(K_DATA_BOT, k_data_top);
    }
  } else {
    const uint32_t sp = static_cast<uint32_t>(R[REG_SP]);
    uint32_t from = sp & ~0xFu;
    if (from < stack_bot || from >= STACK_TOP) from = stack_bot & ~0xFu;
    highlight_ = sp & ~0xFu;
    addRange(from, STACK_TOP);
  }

  bool same = old.size() == rows_.size();
  for (int i = 0; same && i < rows_.size(); ++i) {
    same = old[i].kind == rows_[i].kind && old[i].addr == rows_[i].addr &&
           old[i].end == rows_[i].end;
  }
  if (same) {
    if (!rows_.isEmpty()) {
      emit dataChanged(index(0, 0), index(rows_.size() - 1, ColumnCount - 1));
    }
  } else {
    // Keep the new rows while the model is reset.
    QVector<Row> fresh = rows_;
    rows_ = old;
    beginResetModel();
    rows_ = fresh;
    endResetModel();
  }
}

int WordsModel::rowOf(uint32_t addr) const {
  for (int i = 0; i < rows_.size(); ++i) {
    const Row& r = rows_[i];
    if (r.kind == Kind::Words && r.addr <= addr && addr < r.addr + 16) return i;
    if (r.kind == Kind::Zeros && r.addr <= addr && addr <= r.end) return i;
  }
  return -1;
}

bool WordsModel::isSectionRow(int row) const {
  return 0 <= row && row < rows_.size() && rows_[row].kind == Kind::Section;
}

void WordsModel::setBase(int base) {
  base_ = base;
  if (!rows_.isEmpty()) {
    emit dataChanged(index(0, Word0), index(rows_.size() - 1, Word3));
  }
}

QVariant WordsModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
  switch (section) {
    case AddressColumn:
      return tr("Address");
    case LabelColumn:
      return tr("Label");
    case Word0:
      return QStringLiteral("+0");
    case Word1:
      return QStringLiteral("+4");
    case Word2:
      return QStringLiteral("+8");
    case Word3:
      return QStringLiteral("+c");
    case TextColumn:
      return tr("Text");
  }
  return {};
}

QVariant WordsModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};
  const Row& row = rows_[index.row()];
  const int col = index.column();
  const bool word = col >= Word0 && col <= Word3;
  const uint32_t wordAddr = row.addr + 4 * (col - Word0);

  if (row.kind == Kind::Section) {
    if (role == Qt::DisplayRole && col == AddressColumn) return row.title;
    if (role == Qt::FontRole) {
      QFont f = Theme::uiFont(-2);
      f.setWeight(QFont::DemiBold);
      return f;
    }
    if (role == Qt::ForegroundRole) return Theme::kText3;
    return {};
  }

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch (col) {
        case AddressColumn:
          return hex8(row.addr);
        case LabelColumn:
          return row.kind == Kind::Words ? labelsIn(row.addr, row.addr + 16)
                                         : QString();
        case TextColumn:
          if (row.kind == Kind::Zeros) {
            return tr("all 0 up to %1").arg(hex8(row.end));
          }
          return ascii(row.addr);
        default:
          if (word) {
            uint32_t v = 0;
            if (row.kind == Kind::Zeros) return Mips::formatWord(0, base_);
            if (!readWord(wordAddr, &v)) return QString();
            return Mips::formatWord(v, base_);
          }
      }
      return {};
    case TagRole:
      if (word && row.kind == Kind::Words) {
        return tags_.value(wordAddr).join(QLatin1Char(' '));
      }
      return {};
    case Qt::ToolTipRole:
      if (word && row.kind == Kind::Words) {
        uint32_t v = 0;
        if (!readWord(wordAddr, &v)) return {};
        QString tip = tr("%1: %2 (decimal %3)")
                          .arg(hex8(wordAddr), hex8(v))
                          .arg(static_cast<int32_t>(v));
        const QStringList tags = tags_.value(wordAddr);
        if (!tags.isEmpty()) {
          tip += QLatin1Char('\n') +
                 tr("Pointed to by %1").arg(tags.join(QStringLiteral(", ")));
        }
        return tip;
      }
      return {};
    case Qt::FontRole:
      if (col == LabelColumn) {
        QFont f = Theme::monoFont(12);
        f.setWeight(QFont::DemiBold);
        return f;
      }
      if (col == TextColumn && row.kind == Kind::Zeros)
        return Theme::uiFont(-1);
      return Theme::monoFont(12);
    case Qt::ForegroundRole:
      if (col == AddressColumn) return Theme::kText3;
      if (col == LabelColumn) return Theme::kSynLabel;
      if (col == TextColumn) return Theme::kText3;
      if (word) {
        uint32_t v = 0;
        if (row.kind == Kind::Zeros || (readWord(wordAddr, &v) && v == 0)) {
          return Theme::kTextDisabled;
        }
      }
      return Theme::kText;
    case Qt::BackgroundRole:
      if (segment_ == Segment::Stack && row.kind == Kind::Words &&
          row.addr == highlight_) {
        return QColor("#F3F6FC");
      }
      return {};
  }
  return {};
}

Qt::ItemFlags WordsModel::flags(const QModelIndex& index) const {
  if (!index.isValid() || isSectionRow(index.row())) return Qt::ItemIsEnabled;
  Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  const Row& row = rows_[index.row()];
  if (editable_ && row.kind == Kind::Words && index.column() >= Word0 &&
      index.column() <= Word3) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool WordsModel::setData(const QModelIndex& index, const QVariant& value,
                         int role) {
  if (role != Qt::EditRole || !(flags(index) & Qt::ItemIsEditable)) {
    return false;
  }
  const Row& row = rows_[index.row()];
  const uint32_t addr = row.addr + 4 * (index.column() - Word0);
  uint32_t current = 0;
  if (!readWord(addr, &current)) return false;
  const QString text = value.toString().trimmed();
  if (text == Mips::formatWord(current, base_)) return false;
  uint32_t v = 0;
  if (!Mips::parseWord(text, base_, &v)) {
    emit editRejected(tr("“%1” is not a 32-bit value. Type a decimal "
                         "number, or a hex number starting with 0x.")
                          .arg(text));
    return false;
  }
  set_mem_word(addr, static_cast<reg_word>(v));
  emit dataChanged(index.siblingAtColumn(0),
                   index.siblingAtColumn(ColumnCount - 1));
  emit valueEdited();
  return true;
}

QString WordsModel::plainText() const {
  QString out;
  for (int r = 0; r < rows_.size(); ++r) {
    QStringList cells;
    for (int c = 0; c < ColumnCount; ++c) {
      cells << index(r, c).data().toString();
    }
    out += cells.join(QLatin1Char('\t')) + QLatin1Char('\n');
  }
  return out;
}

// --- CodeModel ---

CodeModel::CodeModel(QObject* parent) : QAbstractTableModel(parent) {}

int CodeModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : rows_.size();
}

int CodeModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : ColumnCount;
}

void CodeModel::rebuild() {
  beginResetModel();
  rows_.clear();
  rowOfAddress_.clear();
  pc_ = PC;
  if (text_seg != nullptr) {
    Simulator* sim = Simulator::instance();
    str_stream ss;
    ss_init(&ss);
    auto scan = [&](instruction** segment, mem_addr bottom, mem_addr top) {
      for (mem_addr a = bottom; a < top; a += BYTES_PER_WORD) {
        instruction* inst = segment[(a - bottom) >> 2];
        if (inst == nullptr) continue;
        ss_clear(&ss);
        format_an_inst(&ss, read_mem_inst(a), a);
        // "[0x00400000]\t0x8fa40000  lw $4, 0($29)   ; 183: lw $a0 0($sp)"
        QString line = QString::fromUtf8(ss_to_string(&ss)).trimmed();
        if (line.startsWith(QLatin1Char('*'))) line.remove(0, 1);
        Row row;
        row.addr = a;
        row.user = sim->isUserAddress(a);
        const int tab = line.indexOf(QLatin1Char('\t'));
        QString rest = tab >= 0 ? line.mid(tab + 1) : line;
        row.encoding = rest.left(10).toUInt(nullptr, 16);
        rest = rest.mid(10).trimmed();
        const int semi = rest.indexOf(QStringLiteral("; "));
        if (semi >= 0) {
          row.instruction = rest.left(semi).trimmed();
          row.source = rest.mid(semi + 2).simplified();
        } else {
          row.instruction = rest;
        }
        if (row.user && row.source.isEmpty()) {
          const SourceLocation loc = sim->location(a);
          row.source = tr("↳ line %1 (%2 of %3)")
                           .arg(loc.line)
                           .arg(loc.index + 1)
                           .arg(loc.count);
        }
        row.label = labelsIn(a, a + 4);
        rowOfAddress_.insert(a, rows_.size());
        rows_.push_back(row);
      }
    };
    if (showKernel_) {
      Row section;
      section.section = true;
      section.label = tr("User text");
      rows_.push_back(section);
    }
    scan(text_seg, TEXT_BOT, text_top);
    if (showKernel_) {
      Row section;
      section.section = true;
      section.label = tr("Kernel text");
      rows_.push_back(section);
      scan(k_text_seg, K_TEXT_BOT, k_text_top);
    }
    free(ss.buf);
  }
  endResetModel();
}

void CodeModel::updatePc() {
  const int oldRow = rowOf(pc_);
  pc_ = PC;
  const int newRow = rowOf(pc_);
  for (int r : {oldRow, newRow}) {
    if (r >= 0) emit dataChanged(index(r, 0), index(r, ColumnCount - 1));
  }
}

QVariant CodeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
  switch (section) {
    case AddressColumn:
      return tr("Address");
    case LabelColumn:
      return tr("Label");
    case EncodingColumn:
      return tr("Encoding");
    case InstructionColumn:
      return tr("Instruction");
    case SourceColumn:
      return tr("Source");
  }
  return {};
}

QVariant CodeModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};
  const Row& row = rows_[index.row()];
  const int col = index.column();
  if (row.section) {
    if (role == Qt::DisplayRole && col == AddressColumn) return row.label;
    if (role == Qt::FontRole) {
      QFont f = Theme::uiFont(-2);
      f.setWeight(QFont::DemiBold);
      return f;
    }
    if (role == Qt::ForegroundRole) return Theme::kText3;
    return {};
  }
  const bool current = row.addr == pc_ && pc_ != 0;
  switch (role) {
    case Qt::DisplayRole:
      switch (col) {
        case AddressColumn:
          return hex8(row.addr);
        case LabelColumn:
          return row.label;
        case EncodingColumn:
          return hex8(row.encoding);
        case InstructionColumn:
          return row.instruction;
        case SourceColumn:
          return row.source;
      }
      return {};
    case Qt::DecorationRole:
      if (col == AddressColumn) {
        if (current) {
          return Theme::icon(QStringLiteral("chevron-right"), Theme::accent(),
                             14);
        }
        if (inst_is_breakpoint(row.addr)) {
          return Theme::icon(QStringLiteral("circle-dot"), Theme::kBreakpoint,
                             14);
        }
        // Keep the addresses aligned.
        static const QIcon blank = [] {
          QPixmap pm(14, 14);
          pm.fill(Qt::transparent);
          return QIcon(pm);
        }();
        return blank;
      }
      return {};
    case Qt::FontRole: {
      QFont f = Theme::monoFont(12);
      if (col == LabelColumn || (current && col == InstructionColumn)) {
        f.setWeight(QFont::DemiBold);
      }
      return f;
    }
    case Qt::ForegroundRole:
      if (col == AddressColumn || col == EncodingColumn) return Theme::kText3;
      if (col == LabelColumn) return Theme::kSynLabel;
      if (!row.user) return Theme::kText3;
      if (col == SourceColumn) return Theme::kText2;
      return Theme::kText;
    case Qt::BackgroundRole:
      if (current) return Theme::kCurrentLine;
      return {};
    case Qt::ToolTipRole:
      if (!row.user) {
        return tr(
            "Not part of your program (startup code or exception "
            "handler)");
      }
      return {};
  }
  return {};
}

QString CodeModel::plainText() const {
  QString out;
  for (const Row& row : rows_) {
    if (row.section) {
      out += QLatin1Char('\n') + row.label + QLatin1Char('\n');
      continue;
    }
    out +=
        QStringLiteral("[%1] %2  %3 %4; %5\n")
            .arg(hex8(row.addr), hex8(row.encoding))
            .arg(row.label.isEmpty() ? QString() : row.label + QLatin1Char(':'),
                 -10)
            .arg(row.instruction, -32)
            .arg(row.source);
  }
  return out;
}

// --- MemoryPanel ---

MemoryPanel::MemoryPanel(QWidget* parent) : QWidget(parent) {
  data_ = new WordsModel(WordsModel::Segment::Data, this);
  stack_ = new WordsModel(WordsModel::Segment::Stack, this);
  code_ = new CodeModel(this);
  dataView_ = makeView(data_);
  stackView_ = makeView(stack_);
  codeView_ = makeView(code_);
  for (QTableView* v : {dataView_, stackView_}) {
    v->setItemDelegate(new WordDelegate(v));
    v->setColumnWidth(WordsModel::AddressColumn, 104);
    v->setColumnWidth(WordsModel::LabelColumn, 90);
    for (int c = WordsModel::Word0; c <= WordsModel::Word3; ++c) {
      v->setColumnWidth(c, 140);
    }
  }
  codeView_->setColumnWidth(CodeModel::AddressColumn, 124);
  codeView_->setColumnWidth(CodeModel::LabelColumn, 90);
  codeView_->setColumnWidth(CodeModel::EncodingColumn, 104);
  codeView_->setColumnWidth(CodeModel::InstructionColumn, 260);

  views_ = new QStackedWidget(this);
  views_->addWidget(dataView_);
  views_->addWidget(stackView_);
  views_->addWidget(codeView_);
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(views_);

  selector_ = new QWidget(this);
  selector_->setProperty("segmented", true);
  selector_->setAttribute(Qt::WA_StyledBackground);
  auto* selectorLayout = new QHBoxLayout(selector_);
  selectorLayout->setContentsMargins(2, 2, 2, 2);
  selectorLayout->setSpacing(2);
  auto make = [this, selectorLayout](const QString& text, View view) {
    auto* b = new QToolButton(selector_);
    b->setText(text);
    b->setCheckable(true);
    b->setAutoExclusive(true);
    b->setProperty("segment", true);
    b->setCursor(Qt::PointingHandCursor);
    connect(b, &QToolButton::clicked, this, [this, view] { setView(view); });
    selectorLayout->addWidget(b);
    return b;
  };
  dataButton_ = make(tr("Data"), View::Data);
  stackButton_ = make(tr("Stack"), View::Stack);
  codeButton_ = make(tr("Machine code"), View::Code);
  dataButton_->setChecked(true);

  for (WordsModel* m : {data_, stack_}) {
    connect(m, &WordsModel::editRejected, this, &MemoryPanel::message);
    connect(m, &WordsModel::valueEdited, this, &MemoryPanel::valueEdited);
  }
}

QTableView* MemoryPanel::makeView(QAbstractTableModel* model) {
  auto* view = new QTableView(this);
  view->setModel(model);
  view->setObjectName(QStringLiteral("memoryTable"));
  view->setFrameShape(QFrame::NoFrame);
  view->setShowGrid(false);
  view->setWordWrap(false);
  view->setCornerButtonEnabled(false);
  view->verticalHeader()->hide();
  view->verticalHeader()->setDefaultSectionSize(22);
  view->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft |
                                                Qt::AlignVCenter);
  view->horizontalHeader()->setHighlightSections(false);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setSelectionBehavior(QAbstractItemView::SelectItems);
  view->setSelectionMode(QAbstractItemView::SingleSelection);
  view->setEditTriggers(QAbstractItemView::DoubleClicked |
                        QAbstractItemView::EditKeyPressed);
  view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  view->setAccessibleName(tr("Memory"));
  return view;
}

void MemoryPanel::setView(View view) {
  view_ = view;
  dataButton_->setChecked(view == View::Data);
  stackButton_->setChecked(view == View::Stack);
  codeButton_->setChecked(view == View::Code);
  views_->setCurrentIndex(static_cast<int>(view));
  if (view == View::Code) {
    const int row = code_->rowOf(PC);
    if (row >= 0) {
      codeView_->scrollTo(code_->index(row, 0),
                          QAbstractItemView::PositionAtCenter);
    }
  }
}

void MemoryPanel::setBase(int base) {
  data_->setBase(base);
  stack_->setBase(base);
  code_->setBase(base);
}

void MemoryPanel::setShowKernel(bool show) {
  data_->setShowKernel(show);
  stack_->setShowKernel(show);
  code_->setShowKernel(show);
  programLoaded();
}

void MemoryPanel::setEditable(bool editable) {
  data_->setEditable(editable);
  stack_->setEditable(editable);
}

void MemoryPanel::programLoaded() {
  code_->rebuild();
  refresh();
}

void MemoryPanel::refresh() {
  data_->refresh();
  stack_->refresh();
  code_->updatePc();
  if (view_ == View::Code) {
    const int row = code_->rowOf(PC);
    if (row >= 0) {
      codeView_->scrollTo(code_->index(row, 0),
                          QAbstractItemView::EnsureVisible);
    }
  }
}

QString MemoryPanel::plainText(View view) const {
  switch (view) {
    case View::Data:
      return data_->plainText();
    case View::Stack:
      return stack_->plainText();
    case View::Code:
      return code_->plainText();
  }
  return QString();
}
