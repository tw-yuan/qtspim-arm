/* SPIM S20 MIPS simulator.
   Colors, fonts and icons of the QtSpim ARM interface.

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

#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QString>

class QApplication;

// The interface is light only (docs/UI_DESIGN.md, section 6). All colors are
// defined here so that widgets never hard-code them.
namespace Theme {

// Surfaces
inline const QColor kWindow("#FFFFFF");
inline const QColor kChrome("#F6F6F8");     // toolbar, status bar
inline const QColor kPanel("#FBFBFC");      // side and bottom panels
inline const QColor kSeparator("#E3E3E8");  // 1 px lines
inline const QColor kControl("#EDEDF0");    // segmented control track

// Text
inline const QColor kText("#1D1D1F");
inline const QColor kText2("#4A4A50");
inline const QColor kText3("#6E6E73");
inline const QColor kTextDisabled("#A1A1A6");

// Accent and states
inline const QColor kAccentDefault("#0A5BC4");
inline const QColor kAccentTint("#E8F0FB");
inline const QColor kAccentText("#0A4A9E");
inline const QColor kPaused("#C27803");
inline const QColor kError("#B42318");
inline const QColor kErrorText("#8A1C12");
inline const QColor kErrorTint("#FDF0EF");
inline const QColor kSuccess("#127A3A");
inline const QColor kSuccessTint("#E7F5EC");

// Highlights
inline const QColor kCurrentLine("#FFF4C2");
inline const QColor kWaitingLine("#E8F0FB");
inline const QColor kChanged("#FFF1BF");
inline const QColor kBreakpoint("#D93025");

// Syntax
inline const QColor kSynOpcode("#9B2393");
inline const QColor kSynDirective("#0B5E8A");
inline const QColor kSynRegister("#A0430A");
inline const QColor kSynNumber("#1C00CF");
inline const QColor kSynString("#1A6B2B");
inline const QColor kSynLabel("#0F4C81");
inline const QColor kSynComment("#6E7781");

// Corner radii (squarer, as the owner asked).
constexpr int kRadiusCard = 6;
constexpr int kRadiusButton = 5;
constexpr int kRadiusTag = 4;

// The accent color: the system accent where Qt reports one and it is dark
// enough for white text, otherwise the default blue.
QColor accent();

// Fonts. The UI uses the system font (San Francisco and PingFang TC on macOS);
// code and numbers use SF Mono, falling back to Menlo, then to the system
// fixed-width font.
QFont uiFont(qreal pointSizeDelta = 0);
QFont monoFont(int pointSize = 13);

// A Lucide icon from :/icons, drawn in |color|. Disabled state uses the
// disabled text color.
QIcon icon(const QString& name, const QColor& color = kText, int size = 20);

// Sets the light palette, style and style sheet of the application.
void apply(QApplication& app);

// "#RRGGBB" of a color, for style sheets.
QString css(const QColor& color);

}  // namespace Theme

#endif  // THEME_H
