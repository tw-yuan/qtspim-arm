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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <QApplication>
#include <QByteArray>
#include <QMessageBox>

#include "simulator.h"

#include "spim.h"
#include "string-stream.h"
#include "spim-utils.h"

// SPIM globals
//
bool bare_machine;        /* => simulate bare machine */
bool accept_pseudo_insts; /* => parse pseudo instructions  */
bool delayed_branches;    /* => simulate delayed branches */
bool delayed_loads;       /* => simulate delayed loads */
bool quiet;               /* => no warning messages */
char *exception_file_name =
    0; /* The path from which to load the exception handler, if desired */
bool mapped_io;        /* => activate memory-mapped IO */
int spim_return_value; /* Value returned when spim exits */

port message_out;
port console_out;

// The interface functions the simulator core calls (CPU/spim.h).  They
// forward everything to the Simulator object (simulator.cpp).

static QByteArray formatMessage(const char *fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);
  const int length = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);
  if (length <= 0) {
    return QByteArray();
  }
  QByteArray buf(length, '\0');
  std::vsnprintf(buf.data(), static_cast<size_t>(length) + 1, fmt, args);
  // As in the original QtSpim, output stops at a NUL character (print_char
  // of 0 prints nothing).
  const int nul = buf.indexOf('\0');
  if (nul >= 0) {
    buf.truncate(nul);
  }
  return buf;
}

int console_input_available() {
  return Simulator::instance()->coreInputAvailable() ? 1 : 0;
}

void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const QByteArray message = formatMessage(fmt, args);
  va_end(args);

  Simulator::instance()->coreError(QString::fromUtf8(message));
}

void run_error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const QByteArray message = formatMessage(fmt, args);
  va_end(args);

  Simulator::instance()->coreRunError(QString::fromUtf8(message));
}

void fatal_error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const QByteArray message = formatMessage(fmt, args);
  va_end(args);

  QMessageBox::critical(nullptr, QApplication::applicationDisplayName(),
                        QString::fromUtf8(message));
  std::exit(1);
}

char get_console_char() {
  return static_cast<char>(Simulator::instance()->coreReadChar());
}

void put_console_char(char c) {
  Simulator::instance()->coreOutput(true, QByteArray(1, c));
}

void read_input(char *str, int str_size) {
  while ((1 < str_size) && (!force_break)) {
    char ch = get_console_char();

    *str = ch;
    str += 1;
    str_size -= 1;

    if (ch == '\n') {
      break;
    }
  }

  if (0 < str_size) {
    *str = '\0';
  }
}

void write_output(port fp, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const QByteArray text = formatMessage(fmt, args);
  va_end(args);

  if (fp.i == message_out.i) {
    Simulator::instance()->coreOutput(false, text);
  } else if (fp.i == console_out.i) {
    Simulator::instance()->coreOutput(true, text);
  }
}
