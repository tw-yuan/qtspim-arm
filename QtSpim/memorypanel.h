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

#ifndef MEMORYPANEL_H
#define MEMORYPANEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>
#include <QWidget>

#include <cstdint>

class QStackedWidget;
class QTableView;
class QToolButton;

// Words of the data segment or the stack, four to a row.  Runs of rows that
// are all zero are folded into one row, as in the original QtSpim.
class WordsModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  enum class Segment { Data, Stack };
  enum Column {
    AddressColumn,
    LabelColumn,
    Word0,
    Word1,
    Word2,
    Word3,
    TextColumn,
    ColumnCount
  };
  static constexpr int TagRole = Qt::UserRole + 1;

  explicit WordsModel(Segment segment, QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  void setBase(int base);
  void setShowKernel(bool show) { showKernel_ = show; }
  void setEditable(bool editable) { editable_ = editable; }
  void refresh();
  // Row showing |addr|, or -1.
  int rowOf(uint32_t addr) const;
  bool isSectionRow(int row) const;
  QString plainText() const;

 signals:
  void editRejected(const QString& message);
  void valueEdited();

 private:
  enum class Kind { Section, Words, Zeros };
  struct Row {
    Kind kind;
    uint32_t addr;
    uint32_t end;  // Zeros: last address of the run; Section: title index
    QString title;
  };
  void addRange(uint32_t from, uint32_t to);
  bool readWord(uint32_t addr, uint32_t* value) const;
  QString ascii(uint32_t addr) const;

  Segment segment_;
  QVector<Row> rows_;
  QHash<uint32_t, QStringList> tags_;  // word address -> registers
  int base_ = 16;
  bool showKernel_ = false;
  bool editable_ = true;
  uint32_t highlight_ = 0;  // stack: the row with $sp
};

// The program's machine instructions, with their source lines.
class CodeModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  enum Column {
    AddressColumn,
    LabelColumn,
    EncodingColumn,
    InstructionColumn,
    SourceColumn,
    ColumnCount
  };

  explicit CodeModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;

  void setShowKernel(bool show) { showKernel_ = show; }
  void setBase(int base) { base_ = base; }
  void rebuild();   // after loading a program
  void updatePc();  // after running
  int rowOf(uint32_t addr) const { return rowOfAddress_.value(addr, -1); }
  QString plainText() const;

 private:
  struct Row {
    bool section = false;
    uint32_t addr = 0;
    uint32_t encoding = 0;
    QString label;
    QString instruction;
    QString source;
    bool user = false;
  };
  QVector<Row> rows_;
  QHash<uint32_t, int> rowOfAddress_;
  uint32_t pc_ = 0;
  bool showKernel_ = false;
  int base_ = 16;
};

class MemoryPanel : public QWidget {
  Q_OBJECT

 public:
  enum class View { Data, Stack, Code };

  explicit MemoryPanel(QWidget* parent = nullptr);

  // The Data / Stack / Machine code control, shown in the bottom panel's
  // header while this panel is visible.
  QWidget* viewSelector() const { return selector_; }
  void setView(View view);
  View view() const { return view_; }

  void setBase(int base);
  void setShowKernel(bool show);
  void setEditable(bool editable);
  void programLoaded();
  void refresh();
  QString plainText(View view) const;

 signals:
  void valueEdited();
  void message(const QString& text);

 private:
  QTableView* makeView(QAbstractTableModel* model);

  WordsModel* data_;
  WordsModel* stack_;
  CodeModel* code_;
  QTableView* dataView_;
  QTableView* stackView_;
  QTableView* codeView_;
  QStackedWidget* views_;
  QWidget* selector_;
  QToolButton* dataButton_;
  QToolButton* stackButton_;
  QToolButton* codeButton_;
  View view_ = View::Data;
};

#endif  // MEMORYPANEL_H
