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

#ifndef REGISTERPANEL_H
#define REGISTERPANEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QWidget>

#include <cstdint>

class QLabel;
class QStackedWidget;
class QTableView;
class QToolButton;

// Integer or floating-point registers, grouped by their use.
class RegisterModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  enum class Bank { Integer, FloatingPoint };
  enum Column { NameColumn, NumberColumn, ValueColumn, ColumnCount };

  explicit RegisterModel(Bank bank, QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  bool isGroupRow(int row) const;
  void setBase(int base);
  void setEditable(bool editable) { editable_ = editable; }
  // Remembers the current values; refresh() then marks the changed ones.
  void snapshot();
  void clearChanges();
  void refresh();

 signals:
  void editRejected(const QString& message);
  void valueEdited();

 private:
  enum class Kind {
    Group,
    Gpr,
    Pc,
    Hi,
    Lo,
    Epc,
    Cause,
    BadVAddr,
    Status,
    Fir,
    Fcsr,
    Single,
    Double
  };
  struct Row {
    Kind kind;
    int index;      // register number
    QString title;  // group title
  };
  uint64_t raw(const Row& row) const;
  QString name(const Row& row) const;
  QString format(const Row& row) const;
  QString tooltip(const Row& row) const;

  Bank bank_;
  QVector<Row> rows_;
  QVector<uint64_t> before_;
  QVector<bool> changed_;
  int base_ = 16;
  bool editable_ = true;
};

class RegisterPanel : public QWidget {
  Q_OBJECT

 public:
  explicit RegisterPanel(QWidget* parent = nullptr);

  int base() const { return base_; }
  void setBase(int base);
  void setEditable(bool editable);
  void setFontSize(int pointSize);

  void beginExecution();  // before running or stepping
  void clearChanges();    // after loading a program
  void refresh();         // after the machine changed
  // Shown instead of the registers (e.g. while there are assembler errors);
  // an empty message shows the registers.
  void setPlaceholder(const QString& message);

  QString plainText() const;  // for Export Log and Print

 signals:
  void baseChanged(int base);
  void valueEdited();

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void showBank(RegisterModel::Bank bank);
  void updateSyscallCard();
  QTableView* makeView(RegisterModel* model);

  RegisterModel* intModel_;
  RegisterModel* fpModel_;
  QTableView* intView_;
  QTableView* fpView_;
  QStackedWidget* views_;
  QToolButton* hexButton_;
  QToolButton* decButton_;
  QToolButton* intTab_;
  QToolButton* fpTab_;
  QLabel* syscallCard_;
  QLabel* placeholder_;
  QWidget* placeholderBox_;
  QLabel* hint_;
  QWidget* content_;
  int base_ = 16;
};

#endif  // REGISTERPANEL_H
