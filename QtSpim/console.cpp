/* SPIM S20 MIPS simulator.
   Terminal interface for SPIM simulator.

   Copyright (c) 1990-2010, James R. Larus.
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

#include "spimview.h"
#include "simulator.h"

Console::Console(QWidget* parent) : QPlainTextEdit(parent) {
  setWindowTitle("Console");
  resize(800, 600);

  setUndoRedoEnabled(false);
  appendPlainText(QString(""));

  QFont courier("Courier");
  setFont(courier);
}

void Console::WriteOutput(QString out) {
  activateWindow();
  moveCursor(QTextCursor::End);
  insertPlainText(out);
  ensureCursorVisible();
}

// Typed characters go to the Simulator object, which hands them to the
// program (get_console_char in spim_support.cpp waits for them in a nested
// event loop, as this class used to).
bool Console::InputAvailable() {
  return Simulator::instance()->coreInputAvailable();
}

void Console::Clear() {
  setPlainText("");
  Simulator::instance()->clearInput();
}

void Console::keyPressEvent(QKeyEvent* /*e*/) {
  // Ignore
}

void Console::keyReleaseEvent(QKeyEvent* e) {
  QString key = e->text();

  if (key != "") {
    if (!mapped_io) {
      WriteOutput(key);  // Do not echo input when using mem mapped IO
    }

    // Release the read (if any) that is blocked waiting for input.
    //
    Simulator::instance()->provideInput(key);
  }
}

void Console::mouseMoveEvent(QMouseEvent* /*e*/) {
  // Ignore
}

void Console::mousePressEvent(QMouseEvent* /*e*/) {
  // Ignore
}

void Console::closeEvent(QCloseEvent* event) {
  Window->ui->action_Win_Console->setChecked(false);
  event->accept();
}

void Console::hideEvent(QHideEvent* event) {
  Window->ui->action_Win_Console->setChecked(false);
  event->accept();
}

void Console::showEvent(QShowEvent* event) {
  if (Window != NULL) {
    Window->ui->action_Win_Console->setChecked(true);
  }
  event->accept();
}
