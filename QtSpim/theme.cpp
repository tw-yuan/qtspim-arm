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

#include "theme.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyleFactory>
#include <QStyleHints>
#include <QSvgRenderer>

#include <cmath>

namespace Theme {

QString css(const QColor& color) { return color.name(QColor::HexRgb); }

static double luminance(const QColor& c) {
  auto lin = [](double v) {
    v /= 255.0;
    return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * lin(c.red()) + 0.7152 * lin(c.green()) +
         0.0722 * lin(c.blue());
}

QColor accent() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
  QColor system = QGuiApplication::palette().color(QPalette::Accent);
  // White text on the accent needs a contrast of at least 4.5:1.
  if (system.isValid() && (1.05 / (luminance(system) + 0.05)) >= 4.5) {
    return system;
  }
#endif
  return kAccentDefault;
}

QFont uiFont(qreal pointSizeDelta) {
  QFont f = QGuiApplication::font();
  if (pointSizeDelta != 0) f.setPointSizeF(f.pointSizeF() + pointSizeDelta);
  return f;
}

QFont monoFont(int pointSize) {
  static QString family;
  if (family.isEmpty()) {
    const QStringList families = QFontDatabase::families();
    for (const char* candidate : {"SF Mono", "Menlo"}) {
      if (families.contains(QString::fromLatin1(candidate))) {
        family = QString::fromLatin1(candidate);
        break;
      }
    }
    if (family.isEmpty()) {
      family = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    }
  }
  QFont f(family);
  f.setPointSize(pointSize);
  f.setStyleHint(QFont::Monospace);
  f.setFixedPitch(true);
  return f;
}

static QPixmap renderSvg(const QByteArray& svg, int size, qreal dpr) {
  QSvgRenderer renderer(svg);
  QPixmap pm(QSize(size, size) * dpr);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  renderer.render(&p);
  p.end();
  pm.setDevicePixelRatio(dpr);
  return pm;
}

QIcon icon(const QString& name, const QColor& color, int size) {
  static QHash<QString, QIcon> cache;
  const QString key = name + css(color) + QString::number(size);
  auto it = cache.find(key);
  if (it != cache.end()) return it.value();

  QFile f(QStringLiteral(":/icons/") + name + QStringLiteral(".svg"));
  if (!f.open(QIODevice::ReadOnly)) return QIcon();
  const QByteArray source = f.readAll();

  auto tinted = [&](const QColor& c) {
    QByteArray svg = source;
    svg.replace("currentColor", css(c).toLatin1());
    svg.replace("stroke-width=\"2\"", "stroke-width=\"1.75\"");
    return svg;
  };
  QIcon result;
  for (qreal dpr : {1.0, 2.0}) {
    result.addPixmap(renderSvg(tinted(color), size, dpr), QIcon::Normal);
    result.addPixmap(renderSvg(tinted(color), size, dpr), QIcon::Active);
    result.addPixmap(renderSvg(tinted(kTextDisabled), size, dpr),
                     QIcon::Disabled);
  }
  cache.insert(key, result);
  return result;
}

static QString styleSheet() {
  const QString a = css(accent());
  QString s = QStringLiteral(R"(
QMainWindow, QDialog { background: %WINDOW%; }
QToolTip { color: %TEXT%; background: #FFFFFF; border: 1px solid %SEP%; padding: 4px 6px; }

/* Toolbar: icon above text. */
QToolBar#mainToolBar { background: %CHROME%; border: 0; border-bottom: 1px solid %SEP%; padding: 4px 8px; spacing: 2px; }
QToolBar#mainToolBar QToolButton { color: %TEXT%; border: 0; border-radius: 5px; padding: 4px 6px; min-width: 46px; font-size: 11px; }
QToolBar#mainToolBar QToolButton:hover { background: %CONTROL%; }
QToolBar#mainToolBar QToolButton:pressed { background: #E2E2E7; }
QToolBar#mainToolBar QToolButton:disabled { color: %DISABLED%; }
QToolBar#mainToolBar QToolButton#primaryButton { background: %ACCENT%; color: #FFFFFF; font-weight: 600; }
QToolBar#mainToolBar QToolButton#primaryButton:disabled { background: %CONTROL%; color: %DISABLED%; }
QToolBar#mainToolBar QToolButton#stepButton:enabled { background: %ACCENTTINT%; color: %ACCENTTEXT%; font-weight: 600; }
QToolBar#mainToolBar QToolButton#stopButton:enabled { background: %ERRORTINT%; color: %ERRORTEXT%; font-weight: 600; }

/* Panels */
QWidget#registerPanel, QWidget#bottomPanel { background: %PANEL%; }
QWidget#panelHeader { background: transparent; }
QLabel#panelTitle { font-weight: 600; color: %TEXT%; }
QLabel#groupTitle { font-size: 11px; font-weight: 600; color: %TEXT3%; }
QLabel#hint { color: %TEXT3%; }
QSplitter::handle { background: %SEP%; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical { height: 1px; }

/* Segmented controls and tab-like buttons. */
QWidget[segmented="true"] { background: %CONTROL%; border-radius: 5px; }
QToolButton[segment="true"] { border: 0; border-radius: 3px; padding: 2px 10px; color: %TEXT%; background: transparent; }
QToolButton[segment="true"]:checked { background: #FFFFFF; }
QToolButton[tabButton="true"] { border: 0; border-radius: 4px; padding: 4px 10px; color: %TEXT2%; background: transparent; }
QToolButton[tabButton="true"]:checked { background: %CONTROL%; color: %TEXT%; font-weight: 600; }
QToolButton[pill="true"] { border: 1px solid %SEP%; border-radius: 4px; padding: 0 7px; background: #FFFFFF; color: %TEXT%; font-size: 11px; }
QToolButton[pill="true"]:hover { background: %CHROME%; }
QToolButton[link="true"] { border: 0; background: transparent; color: %ACCENT%; padding: 0; }

/* Status bar */
QWidget#statusBar { background: %CHROME%; border-top: 1px solid %SEP%; }

/* Lists and tables */
QTableView { border: 0; background: transparent; gridline-color: transparent; selection-background-color: %ACCENTTINT%; selection-color: %TEXT%; }
QHeaderView::section { background: transparent; border: 0; color: %TEXT3%; font-size: 11px; padding: 2px 6px; }
QListWidget { border: 0; background: transparent; }

QPushButton#primary { background: %ACCENT%; color: #FFFFFF; border: 0; border-radius: 5px; padding: 6px 14px; font-weight: 600; }
QPushButton#primary:disabled { background: %CONTROL%; color: %DISABLED%; }
QLabel#hint[error="true"] { color: %ERROR%; }
QLabel#sectionTitle { font-size: 12px; font-weight: 600; color: %TEXT3%; }

/* Welcome page */
QWidget#welcomePage, QWidget#welcomePage QScrollArea, QWidget#welcomePage QScrollArea > QWidget > QWidget { background: %WINDOW%; }
QLabel#welcomeTitle { font-size: 26px; font-weight: 600; color: %TEXT%; }
QLabel#welcomeSubtitle { font-size: 14px; color: %TEXT2%; }
QPushButton#welcomeCard { background: %WINDOW%; border: 1px solid %SEP%; border-radius: 6px; text-align: left; }
QPushButton#welcomeCard:hover { border-color: %ACCENT%; background: #FAFBFE; }
QPushButton#welcomeCard:focus { border: 2px solid %ACCENT%; }
QLabel#cardTitle { font-size: 14px; font-weight: 600; color: %TEXT%; }
QLabel#cardText { color: %TEXT3%; }
QWidget#listBox { background: %WINDOW%; border: 1px solid %SEP%; border-radius: 6px; }
QPushButton#listRow { border: 0; border-radius: 0; background: transparent; text-align: left; }
QPushButton#resumeBar { background: %ACCENTTINT%; border: 1px solid #C9DBF5; border-radius: 6px; text-align: left; }
QPushButton#resumeBar:hover { border-color: %ACCENT%; }
QPushButton#resumeBar:focus { border: 2px solid %ACCENT%; }
QPushButton#listRow:hover { background: %CHROME%; }
QPushButton#listRow:focus { background: %ACCENTTINT%; }
QLabel#rowTitle { font-weight: 600; color: %TEXT%; }
QLabel#rowText { color: %TEXT3%; }
QLabel#rowTag { color: %ACCENTTEXT%; background: %ACCENTTINT%; border-radius: 4px; padding: 1px 7px; font-size: 11px; }
QLabel#stepNumber { color: %ACCENTTEXT%; background: %ACCENTTINT%; border-radius: 11px; font-size: 11px; font-weight: 600; }

/* Editor */
QWidget#editorHeader { background: %WINDOW%; border-bottom: 1px solid %SEP%; }
QWidget#findBar { background: %CHROME%; border-bottom: 1px solid %SEP%; }

/* Bottom panel */
QWidget#panelHeader { background: %PANEL%; border-bottom: 1px solid %SEP%; }
QToolButton#collapseButton { border: 0; border-radius: 4px; background: transparent; padding: 2px; }
QToolButton#collapseButton:hover { background: %CONTROL%; }
QPlainTextEdit#consoleView { background: %WINDOW%; }
QWidget#consoleInput { background: %PANEL%; border-top: 1px solid %SEP%; }
QWidget#consoleInput[waiting="true"] { background: %ACCENTTINT%; }
QLabel#inputPrompt { color: %ACCENTTEXT%; }
QLineEdit#consoleLine { background: %WINDOW%; border: 1px solid %SEP%; border-radius: 6px; padding: 4px 8px; }
QWidget#consoleInput[waiting="true"] QLineEdit#consoleLine { border: 2px solid %ACCENT%; padding: 3px 7px; }
QWidget#problemsList { background: %WINDOW%; }
QWidget#errorCard { background: %ERRORTINT%; border-radius: 6px; }
QWidget#warningCard { background: #FFF8E6; border-radius: 6px; }
QLabel#problemLine { font-weight: 600; color: %TEXT%; }
QLabel#problemText { color: %TEXT%; }
QLabel#problemRaw { color: %TEXT3%; }
QTableView#memoryTable { background: %WINDOW%; }
QTableView#memoryTable QHeaderView::section { border-bottom: 1px solid %SEP%; padding: 3px 6px; }

/* Registers */
QLabel#syscallCard { background: %ACCENTTINT%; color: %ACCENTTEXT%; border-radius: 6px; padding: 8px 10px; }
QTableView#registerTable { background: transparent; }

/* Status bar */
QLabel#statusText { color: %TEXT%; }

/* Settings */
QWidget#settingsCard { background: %PANEL%; border: 1px solid %SEP%; border-radius: 6px; }
QLabel#note { color: #7A4B00; background: #FFF8E6; border-radius: 4px; padding: 6px 8px; }
QLabel#optionText { color: %TEXT3%; font-size: 12px; }
)");
  s.replace("%WINDOW%", css(kWindow));
  s.replace("%CHROME%", css(kChrome));
  s.replace("%PANEL%", css(kPanel));
  s.replace("%SEP%", css(kSeparator));
  s.replace("%CONTROL%", css(kControl));
  s.replace("%TEXT%", css(kText));
  s.replace("%TEXT2%", css(kText2));
  s.replace("%TEXT3%", css(kText3));
  s.replace("%DISABLED%", css(kTextDisabled));
  s.replace("%ACCENTTINT%", css(kAccentTint));
  s.replace("%ACCENTTEXT%", css(kAccentText));
  s.replace("%ERROR%", css(kError));
  s.replace("%ERRORTINT%", css(kErrorTint));
  s.replace("%ERRORTEXT%", css(kErrorText));
  s.replace("%ACCENT%", a);
  return s;
}

void apply(QApplication& app) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  // Light only, even when the system uses a dark appearance.
  QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
#endif
#ifndef Q_OS_MACOS
  app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
#endif
  QPalette p = app.palette();
  p.setColor(QPalette::Window, kWindow);
  p.setColor(QPalette::WindowText, kText);
  p.setColor(QPalette::Base, kWindow);
  p.setColor(QPalette::AlternateBase, kPanel);
  p.setColor(QPalette::Text, kText);
  p.setColor(QPalette::Button, kChrome);
  p.setColor(QPalette::ButtonText, kText);
  p.setColor(QPalette::ToolTipBase, kWindow);
  p.setColor(QPalette::ToolTipText, kText);
  p.setColor(QPalette::PlaceholderText, kText3);
  p.setColor(QPalette::Highlight, kAccentTint);
  p.setColor(QPalette::HighlightedText, kText);
  p.setColor(QPalette::Link, accent());
  p.setColor(QPalette::Disabled, QPalette::Text, kTextDisabled);
  p.setColor(QPalette::Disabled, QPalette::WindowText, kTextDisabled);
  p.setColor(QPalette::Disabled, QPalette::ButtonText, kTextDisabled);
  app.setPalette(p);
  app.setStyleSheet(styleSheet());
}

}  // namespace Theme
