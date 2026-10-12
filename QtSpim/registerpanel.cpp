/* SPIM S20 MIPS simulator.
   The register panel.

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

#include "registerpanel.h"

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QStackedWidget>
#include <QStyle>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

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

uint32_t floatBits(float f) {
  uint32_t bits;
  std::memcpy(&bits, &f, sizeof bits);
  return bits;
}

uint64_t doubleBits(double d) {
  uint64_t bits;
  std::memcpy(&bits, &d, sizeof bits);
  return bits;
}

}  // namespace

// --- RegisterModel ---

RegisterModel::RegisterModel(Bank bank, QObject* parent)
    : QAbstractTableModel(parent), bank_(bank) {
  auto group = [this](const QString& title) {
    rows_.push_back({Kind::Group, 0, title});
  };
  auto gpr = [this](int n) { rows_.push_back({Kind::Gpr, n, QString()}); };
  if (bank == Bank::Integer) {
    group(tr("Program counter and mult/div results"));
    rows_.push_back({Kind::Pc, 0, QString()});
    rows_.push_back({Kind::Hi, 0, QString()});
    rows_.push_back({Kind::Lo, 0, QString()});
    group(tr("Results and arguments"));
    for (int n = 2; n <= 7; ++n) gpr(n);
    group(tr("Temporaries (not preserved across calls)"));
    for (int n = 8; n <= 15; ++n) gpr(n);
    gpr(24);
    gpr(25);
    group(tr("Saved (preserved across calls)"));
    for (int n = 16; n <= 23; ++n) gpr(n);
    group(tr("Pointers"));
    for (int n = 28; n <= 31; ++n) gpr(n);
    group(tr("Reserved"));
    gpr(0);
    gpr(1);
    gpr(26);
    gpr(27);
    group(tr("Exceptions (coprocessor 0)"));
    rows_.push_back({Kind::Epc, 14, QString()});
    rows_.push_back({Kind::Cause, 13, QString()});
    rows_.push_back({Kind::BadVAddr, 8, QString()});
    rows_.push_back({Kind::Status, 12, QString()});
  } else {
    group(tr("Control"));
    rows_.push_back({Kind::Fir, 0, QString()});
    rows_.push_back({Kind::Fcsr, 31, QString()});
    group(tr("Single precision"));
    for (int n = 0; n < FGR_LENGTH; ++n) {
      rows_.push_back({Kind::Single, n, QString()});
    }
    group(tr("Double precision"));
    for (int n = 0; n < FPR_LENGTH; ++n) {
      rows_.push_back({Kind::Double, n, QString()});
    }
  }
  before_.fill(0, rows_.size());
  changed_.fill(false, rows_.size());
}

int RegisterModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : rows_.size();
}

int RegisterModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : ColumnCount;
}

bool RegisterModel::isGroupRow(int row) const {
  return 0 <= row && row < rows_.size() && rows_[row].kind == Kind::Group;
}

uint64_t RegisterModel::raw(const Row& row) const {
  if (FPR == nullptr) return 0;  // before the simulator is initialized
  switch (row.kind) {
    case Kind::Group:
      return 0;
    case Kind::Gpr:
      return static_cast<uint32_t>(R[row.index]);
    case Kind::Pc:
      return PC;
    case Kind::Hi:
      return static_cast<uint32_t>(HI);
    case Kind::Lo:
      return static_cast<uint32_t>(LO);
    case Kind::Epc:
      return static_cast<uint32_t>(CP0_EPC);
    case Kind::Cause:
      return static_cast<uint32_t>(CP0_Cause);
    case Kind::BadVAddr:
      return static_cast<uint32_t>(CP0_BadVAddr);
    case Kind::Status:
      return static_cast<uint32_t>(CP0_Status);
    case Kind::Fir:
      return static_cast<uint32_t>(FIR);
    case Kind::Fcsr:
      return static_cast<uint32_t>(FCSR);
    case Kind::Single:
      return floatBits(FGR[row.index]);
    case Kind::Double:
      return doubleBits(FPR[row.index]);
  }
  return 0;
}

QString RegisterModel::name(const Row& row) const {
  switch (row.kind) {
    case Kind::Group:
      return row.title;
    case Kind::Gpr:
      return QLatin1Char('$') + Mips::registerName(row.index);
    case Kind::Pc:
      return QStringLiteral("PC");
    case Kind::Hi:
      return QStringLiteral("HI");
    case Kind::Lo:
      return QStringLiteral("LO");
    case Kind::Epc:
      return QStringLiteral("EPC");
    case Kind::Cause:
      return QStringLiteral("Cause");
    case Kind::BadVAddr:
      return QStringLiteral("BadVAddr");
    case Kind::Status:
      return QStringLiteral("Status");
    case Kind::Fir:
      return QStringLiteral("FIR");
    case Kind::Fcsr:
      return QStringLiteral("FCSR");
    case Kind::Single:
      return QStringLiteral("$f%1").arg(row.index);
    case Kind::Double:
      return QStringLiteral("$f%1").arg(2 * row.index);
  }
  return QString();
}

QString RegisterModel::format(const Row& row) const {
  const uint64_t v = raw(row);
  if (row.kind == Kind::Single) {
    float f;
    const uint32_t bits = static_cast<uint32_t>(v);
    std::memcpy(&f, &bits, sizeof f);
    if (base_ == 10) return QString::number(double(f), 'g', 9);
    return Mips::formatWord(bits, base_);
  }
  if (row.kind == Kind::Double) {
    double d;
    std::memcpy(&d, &v, sizeof d);
    if (base_ == 10) return QString::number(d, 'g', 17);
    if (base_ == 2) {
      return QStringLiteral("%1").arg(v, 64, 2, QLatin1Char('0'));
    }
    return QStringLiteral("0x%1").arg(v, 16, 16, QLatin1Char('0'));
  }
  return Mips::formatWord(static_cast<uint32_t>(v), base_);
}

QString RegisterModel::tooltip(const Row& row) const {
  switch (row.kind) {
    case Kind::Group:
      return QString();
    case Kind::Gpr:
      return QStringLiteral("$%1 (r%2) — %3")
          .arg(Mips::registerName(row.index))
          .arg(row.index)
          .arg(Mips::registerUsage(row.index));
    case Kind::Pc:
      return QStringLiteral("PC — ") + Mips::registerUsage(-1);
    case Kind::Hi:
      return QStringLiteral("HI — ") + Mips::registerUsage(-2);
    case Kind::Lo:
      return QStringLiteral("LO — ") + Mips::registerUsage(-3);
    case Kind::Epc:
      return tr(
          "EPC — address of the instruction that caused the last "
          "exception");
    case Kind::Cause: {
      const int code = (static_cast<uint32_t>(raw(row)) >> 2) & 0x1f;
      return tr("Cause — cause of the last exception (exception code in bits "
                "2–6; now %1: %2)")
          .arg(code)
          .arg(Mips::exceptionName(code));
    }
    case Kind::BadVAddr:
      return tr(
          "BadVAddr — memory address that caused the last address "
          "error");
    case Kind::Status:
      return tr("Status — interrupt mask and enable bits");
    case Kind::Fir:
      return tr("FIR — floating-point implementation register");
    case Kind::Fcsr:
      return tr("FCSR — floating-point condition flags and control");
    case Kind::Single:
      return tr("$f%1 — single-precision floating point").arg(row.index);
    case Kind::Double:
      return tr("$f%1 and $f%2 — double-precision floating point")
          .arg(2 * row.index)
          .arg(2 * row.index + 1);
  }
  return QString();
}

QVariant RegisterModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};
  const Row& row = rows_[index.row()];
  const bool group = row.kind == Kind::Group;
  const bool changed = changed_[index.row()];

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      if (group) return index.column() == NameColumn ? row.title : QVariant();
      switch (index.column()) {
        case NameColumn:
          return name(row);
        case NumberColumn:
          if (row.kind == Kind::Gpr)
            return QStringLiteral("r%1").arg(row.index);
          return QVariant();
        case ValueColumn:
          return format(row);
      }
      return {};
    case Qt::ToolTipRole:
      return tooltip(row);
    case Qt::AccessibleTextRole:
      if (group) return row.title;
      return name(row) + QLatin1Char(' ') + format(row);
    case Qt::FontRole: {
      if (group) {
        QFont f = Theme::uiFont(-2);
        f.setWeight(QFont::DemiBold);
        return f;
      }
      if (index.column() == NumberColumn) return Theme::monoFont(10);
      QFont f = Theme::monoFont(12);
      if (changed && index.column() != NumberColumn) f.setBold(true);
      return f;
    }
    case Qt::ForegroundRole:
      if (group || index.column() == NumberColumn) return Theme::kText3;
      return Theme::kText;
    case Qt::BackgroundRole:
      if (changed && !group) return Theme::kChanged;
      return {};
    case Qt::TextAlignmentRole:
      if (group) return int(Qt::AlignLeft | Qt::AlignBottom);
      return int(Qt::AlignLeft | Qt::AlignVCenter);
  }
  return {};
}

Qt::ItemFlags RegisterModel::flags(const QModelIndex& index) const {
  if (!index.isValid() || isGroupRow(index.row())) return Qt::ItemIsEnabled;
  Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  const Row& row = rows_[index.row()];
  if (editable_ && index.column() == ValueColumn &&
      !(row.kind == Kind::Gpr && row.index == 0) && FPR != nullptr) {
    f |= Qt::ItemIsEditable;
  }
  return f;
}

bool RegisterModel::setData(const QModelIndex& index, const QVariant& value,
                            int role) {
  if (role != Qt::EditRole || !(flags(index) & Qt::ItemIsEditable)) {
    return false;
  }
  const Row& row = rows_[index.row()];
  const QString text = value.toString().trimmed();
  if (text == format(row)) return false;

  if (row.kind == Kind::Single || row.kind == Kind::Double) {
    bool ok = false;
    if (text.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
      const qulonglong bits = text.mid(2).toULongLong(&ok, 16);
      if (ok && row.kind == Kind::Single && bits <= 0xffffffffULL) {
        const uint32_t b = static_cast<uint32_t>(bits);
        std::memcpy(&FGR[row.index], &b, sizeof b);
      } else if (ok && row.kind == Kind::Double) {
        std::memcpy(&FPR[row.index], &bits, sizeof bits);
      } else {
        ok = false;
      }
    } else {
      const double d = text.toDouble(&ok);
      if (ok && row.kind == Kind::Single) FGR[row.index] = float(d);
      if (ok && row.kind == Kind::Double) FPR[row.index] = d;
    }
    if (!ok) {
      emit editRejected(tr("“%1” is not a floating-point number.").arg(text));
      return false;
    }
  } else {
    uint32_t v = 0;
    if (!Mips::parseWord(text, base_, &v)) {
      emit editRejected(
          tr("“%1” is not a 32-bit value. Type a decimal number, or a hex "
             "number starting with 0x.")
              .arg(text));
      return false;
    }
    const reg_word w = static_cast<reg_word>(v);
    switch (row.kind) {
      case Kind::Gpr:
        R[row.index] = w;
        break;
      case Kind::Pc:
        PC = v;
        break;
      case Kind::Hi:
        HI = w;
        break;
      case Kind::Lo:
        LO = w;
        break;
      case Kind::Epc:
        CP0_EPC = w;
        break;
      case Kind::Cause:
        CP0_Cause = w;
        break;
      case Kind::BadVAddr:
        CP0_BadVAddr = w;
        break;
      case Kind::Status:
        CP0_Status = w;
        break;
      case Kind::Fir:
        FIR = w;
        break;
      case Kind::Fcsr:
        FCSR = w;
        break;
      default:
        return false;
    }
  }
  emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(2));
  emit valueEdited();
  return true;
}

void RegisterModel::setBase(int base) {
  base_ = base;
  if (!rows_.isEmpty()) {
    emit dataChanged(index(0, ValueColumn),
                     index(rows_.size() - 1, ValueColumn));
  }
}

void RegisterModel::snapshot() {
  for (int i = 0; i < rows_.size(); ++i) before_[i] = raw(rows_[i]);
}

void RegisterModel::clearChanges() {
  changed_.fill(false);
  snapshot();
  refresh();
}

void RegisterModel::refresh() {
  for (int i = 0; i < rows_.size(); ++i) {
    changed_[i] = rows_[i].kind != Kind::Group && raw(rows_[i]) != before_[i];
  }
  if (!rows_.isEmpty()) {
    emit dataChanged(index(0, 0), index(rows_.size() - 1, ColumnCount - 1));
  }
}

// --- RegisterPanel ---

namespace {

QToolButton* segment(const QString& text, QWidget* parent) {
  auto* b = new QToolButton(parent);
  b->setText(text);
  b->setCheckable(true);
  b->setAutoExclusive(true);
  b->setProperty("segment", true);
  b->setCursor(Qt::PointingHandCursor);
  return b;
}

QToolButton* tabButton(const QString& text, QWidget* parent) {
  auto* b = new QToolButton(parent);
  b->setText(text);
  b->setCheckable(true);
  b->setAutoExclusive(true);
  b->setProperty("tabButton", true);
  b->setCursor(Qt::PointingHandCursor);
  return b;
}

}  // namespace

RegisterPanel::RegisterPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("registerPanel"));
  setAttribute(Qt::WA_StyledBackground);

  auto* title = new QLabel(tr("Registers"), this);
  title->setObjectName(QStringLiteral("panelTitle"));

  auto* baseControl = new QWidget(this);
  baseControl->setProperty("segmented", true);
  baseControl->setAttribute(Qt::WA_StyledBackground);
  hexButton_ = segment(tr("Hex"), baseControl);
  decButton_ = segment(tr("Dec"), baseControl);
  hexButton_->setToolTip(tr("Show values in hexadecimal"));
  decButton_->setToolTip(tr("Show values in decimal"));
  auto* baseLayout = new QHBoxLayout(baseControl);
  baseLayout->setContentsMargins(2, 2, 2, 2);
  baseLayout->setSpacing(2);
  baseLayout->addWidget(hexButton_);
  baseLayout->addWidget(decButton_);

  auto* header = new QHBoxLayout;
  header->setContentsMargins(12, 10, 10, 6);
  header->addWidget(title);
  header->addStretch(1);
  header->addWidget(baseControl);

  intTab_ = tabButton(tr("Integer"), this);
  fpTab_ = tabButton(tr("Floating point"), this);
  intTab_->setChecked(true);
  auto* tabs = new QHBoxLayout;
  tabs->setContentsMargins(10, 0, 10, 6);
  tabs->setSpacing(2);
  tabs->addWidget(intTab_);
  tabs->addWidget(fpTab_);
  tabs->addStretch(1);

  syscallCard_ = new QLabel(this);
  syscallCard_->setObjectName(QStringLiteral("syscallCard"));
  syscallCard_->setWordWrap(true);
  syscallCard_->setTextFormat(Qt::RichText);
  syscallCard_->hide();

  intModel_ = new RegisterModel(RegisterModel::Bank::Integer, this);
  fpModel_ = new RegisterModel(RegisterModel::Bank::FloatingPoint, this);
  intView_ = makeView(intModel_);
  fpView_ = makeView(fpModel_);
  views_ = new QStackedWidget(this);
  views_->addWidget(intView_);
  views_->addWidget(fpView_);

  hint_ = new QLabel(tr("Yellow: registers changed by the last step. Hover "
                        "over a register to see what it is for."),
                     this);
  hint_->setObjectName(QStringLiteral("hint"));
  hint_->setWordWrap(true);
  hint_->setContentsMargins(12, 6, 12, 10);

  content_ = new QWidget(this);
  auto* contentLayout = new QVBoxLayout(content_);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);
  contentLayout->addLayout(tabs);
  auto* cardBox = new QHBoxLayout;
  cardBox->setContentsMargins(10, 0, 10, 8);
  cardBox->addWidget(syscallCard_);
  contentLayout->addLayout(cardBox);
  contentLayout->addWidget(views_, 1);
  contentLayout->addWidget(hint_);

  placeholderBox_ = new QWidget(this);
  placeholder_ = new QLabel(placeholderBox_);
  placeholder_->setObjectName(QStringLiteral("hint"));
  placeholder_->setWordWrap(true);
  placeholder_->setAlignment(Qt::AlignCenter);
  auto* placeholderLayout = new QVBoxLayout(placeholderBox_);
  placeholderLayout->setContentsMargins(24, 24, 24, 24);
  placeholderLayout->addStretch(1);
  placeholderLayout->addWidget(placeholder_);
  placeholderLayout->addStretch(1);
  placeholderBox_->hide();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addLayout(header);
  layout->addWidget(content_, 1);
  layout->addWidget(placeholderBox_, 1);

  setBase(16);
  connect(hexButton_, &QToolButton::clicked, this, [this] { setBase(16); });
  connect(decButton_, &QToolButton::clicked, this, [this] { setBase(10); });
  connect(intTab_, &QToolButton::clicked, this,
          [this] { showBank(RegisterModel::Bank::Integer); });
  connect(fpTab_, &QToolButton::clicked, this,
          [this] { showBank(RegisterModel::Bank::FloatingPoint); });
  for (RegisterModel* m : {intModel_, fpModel_}) {
    connect(m, &RegisterModel::editRejected, this, [this](const QString& msg) {
      hint_->setText(msg);
      hint_->setProperty("error", true);
      hint_->style()->unpolish(hint_);
      hint_->style()->polish(hint_);
    });
    connect(m, &RegisterModel::valueEdited, this, &RegisterPanel::valueEdited);
  }
}

QTableView* RegisterPanel::makeView(RegisterModel* model) {
  auto* view = new QTableView(this);
  view->setModel(model);
  view->setObjectName(QStringLiteral("registerTable"));
  view->setFrameShape(QFrame::NoFrame);
  view->setShowGrid(false);
  view->setWordWrap(false);
  view->horizontalHeader()->hide();
  view->verticalHeader()->hide();
  view->verticalHeader()->setDefaultSectionSize(24);
  view->setSelectionBehavior(QAbstractItemView::SelectRows);
  view->setSelectionMode(QAbstractItemView::SingleSelection);
  view->setEditTriggers(QAbstractItemView::DoubleClicked |
                        QAbstractItemView::EditKeyPressed |
                        QAbstractItemView::AnyKeyPressed);
  view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  view->horizontalHeader()->setSectionResizeMode(RegisterModel::NameColumn,
                                                 QHeaderView::Fixed);
  view->horizontalHeader()->setSectionResizeMode(RegisterModel::NumberColumn,
                                                 QHeaderView::Fixed);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setColumnWidth(RegisterModel::NameColumn, 84);
  view->setColumnWidth(RegisterModel::NumberColumn, 40);
  view->setAccessibleName(tr("Registers"));
  for (int r = 0; r < model->rowCount(); ++r) {
    if (model->isGroupRow(r)) {
      view->setSpan(r, 0, 1, RegisterModel::ColumnCount);
      view->setRowHeight(r, r == 0 ? 22 : 30);
    }
  }
  return view;
}

void RegisterPanel::setBase(int base) {
  base_ = base;
  hexButton_->setChecked(base == 16);
  decButton_->setChecked(base == 10);
  if (base == 2) {
    // Neither segment: binary is chosen from the menu.
    hexButton_->setAutoExclusive(false);
    decButton_->setAutoExclusive(false);
    hexButton_->setChecked(false);
    decButton_->setChecked(false);
    hexButton_->setAutoExclusive(true);
    decButton_->setAutoExclusive(true);
  }
  intModel_->setBase(base);
  fpModel_->setBase(base);
  intView_->setColumnWidth(RegisterModel::NameColumn, base == 2 ? 70 : 84);
  emit baseChanged(base);
}

void RegisterPanel::setEditable(bool editable) {
  intModel_->setEditable(editable);
  fpModel_->setEditable(editable);
}

void RegisterPanel::setFontSize(int) {}

void RegisterPanel::showBank(RegisterModel::Bank bank) {
  const bool integer = bank == RegisterModel::Bank::Integer;
  intTab_->setChecked(integer);
  fpTab_->setChecked(!integer);
  views_->setCurrentIndex(integer ? 0 : 1);
}

void RegisterPanel::beginExecution() {
  intModel_->snapshot();
  fpModel_->snapshot();
}

void RegisterPanel::clearChanges() {
  intModel_->clearChanges();
  fpModel_->clearChanges();
  updateSyscallCard();
}

void RegisterPanel::refresh() {
  intModel_->refresh();
  fpModel_->refresh();
  updateSyscallCard();
  hint_->setText(
      tr("Yellow: registers changed by the last step. Hover over "
         "a register to see what it is for."));
  hint_->setProperty("error", false);
  hint_->style()->unpolish(hint_);
  hint_->style()->polish(hint_);
}

void RegisterPanel::setPlaceholder(const QString& message) {
  placeholder_->setText(message);
  placeholderBox_->setVisible(!message.isEmpty());
  content_->setVisible(message.isEmpty());
}

void RegisterPanel::updateSyscallCard() {
  Simulator* sim = Simulator::instance();
  const bool stopped = sim->state() == Simulator::State::Paused ||
                       sim->state() == Simulator::State::WaitingForInput;
  if (!stopped || FPR == nullptr || !sim->atSyscall()) {
    syscallCard_->hide();
    return;
  }
  const int number = R[REG_V0];
  Mips::Syscall info;
  if (Mips::syscall(number, &info)) {
    syscallCard_->setText(
        tr("<b>syscall %1 · %2 (%3)</b><br>$v0 = %1 selects this service: "
           "%4.")
            .arg(number)
            .arg(info.name, info.title.toHtmlEscaped(),
                 info.description.toHtmlEscaped()));
  } else {
    syscallCard_->setText(
        tr("<b>syscall</b><br>$v0 = %1 is not a SPIM system call.")
            .arg(number));
  }
  syscallCard_->show();
}

void RegisterPanel::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu(this);
  const struct {
    QString text;
    int base;
  } items[] = {{tr("Hexadecimal"), 16}, {tr("Decimal"), 10}, {tr("Binary"), 2}};
  for (const auto& item : items) {
    QAction* a = menu.addAction(item.text);
    a->setCheckable(true);
    a->setChecked(base_ == item.base);
    const int base = item.base;
    connect(a, &QAction::triggered, this, [this, base] { setBase(base); });
  }
  menu.exec(event->globalPos());
}

QString RegisterPanel::plainText() const {
  QString out;
  for (RegisterModel* m : {intModel_, fpModel_}) {
    for (int r = 0; r < m->rowCount(); ++r) {
      if (m->isGroupRow(r)) {
        out += QLatin1Char('\n') + m->index(r, 0).data().toString() +
               QLatin1Char('\n');
        continue;
      }
      out += QStringLiteral("%1 %2 %3\n")
                 .arg(m->index(r, 0).data().toString(), -9)
                 .arg(m->index(r, 1).data().toString(), -4)
                 .arg(m->index(r, 2).data().toString());
    }
  }
  return out;
}
