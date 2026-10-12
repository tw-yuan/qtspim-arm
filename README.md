# QtSpim ARM

> **Unofficial fork.** QtSpim ARM is not affiliated with, maintained by or
> endorsed by James R. Larus or the original SPIM project. For the original
> SPIM and QtSpim, see <https://spimsimulator.sourceforge.net/> and
> <https://sourceforge.net/projects/spimsimulator/>.

QtSpim ARM is a fork of [SPIM / QtSpim](https://spimsimulator.sourceforge.net/),
James R. Larus's MIPS32 simulator, made to run natively on current Macs. It
starts from SPIM 9.1.24 (SourceForge SVN r764). The upstream README is kept
unchanged in [`README`](README).

## Goals

- A native macOS app for Apple Silicon and Intel (Universal Binary), built with Qt 6.
- Signed with an Apple Developer ID and notarized, so it opens without Gatekeeper warnings.
- A built-in program editor and a modern, Mac-style interface for students, in
  Traditional Chinese or English, with the original QtSpim windows one click
  away.

**The simulator must behave exactly like the original SPIM 9.1.24.** SPIM is a
teaching tool, and assignments are often graded with the original. Any change
in instruction semantics, system calls, exception handling, delay slots,
memory layout or output format is treated as a bug. Regression tests recorded
from the original build check this; see [`regression/README.md`](regression/README.md).

## Status

Work in progress. Test builds are on the
[Releases](https://github.com/tw-yuan/qtspim-arm/releases) page.
[`CHANGES.md`](CHANGES.md) lists every difference from the original.

| Part | State |
|---|---|
| Regression tests against the original SPIM 9.1.24 | done |
| CMake build, Universal Binary, Qt 6 | done; verified on macOS 27 (Apple M3, arm64 and x86_64 under Rosetta) |
| Identical floating-point results on Apple Silicon and Intel | done |
| App identity (QtSpim ARM, `net.yuan-tw.qtspim`) | done |
| Signing, notarization and DMG | planned |
| Built-in editor, new interface ([design](docs/UI_DESIGN.md)), Traditional Chinese | done; the classic QtSpim interface is kept |

## Download and install

QtSpim ARM runs on macOS 13 or later, on Apple Silicon and Intel Macs.

1. Download `QtSpim-ARM-<version>.dmg` from
   [Releases](https://github.com/tw-yuan/qtspim-arm/releases), open it and drag
   **QtSpim ARM** into **Applications**.
2. The test builds are not yet signed with an Apple Developer ID or
   notarized, so the first time macOS refuses to open the app. Open it once,
   then go to **System Settings › Privacy & Security**, find "QtSpim ARM was
   blocked" near the bottom and click **Open Anyway**. Alternatively, run this
   in Terminal:

   ```bash
   xattr -dr com.apple.quarantine "/Applications/QtSpim ARM.app"
   ```

   After that the app opens normally.

## The interface

QtSpim ARM opens in a new, single-window interface: a program editor with
syntax highlighting, breakpoints in the margin and error messages explained in
plain words; the console, memory and assembler problems below it; and the
registers on the right, with the ones the last step changed highlighted. Run,
Step and Continue use the same simulator as the original. The interface is in
Traditional Chinese when the system is, and in English otherwise (Settings can
change it).

The original QtSpim windows are still there: click **Classic** in the toolbar,
and **Switch to New Interface** to come back.

## Building from source

See [`docs/BUILDING.md`](docs/BUILDING.md). In short:

```bash
brew install cmake bison flex ninja
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.3/macos   # a Universal Qt 6.11
cmake --build build
scripts/run-tests.sh build/spim
```

## Third-party material

- Icons: [Lucide](https://lucide.dev) 1.55.0 (`lucide-static`), ISC License;
  see [`QtSpim/icons/LICENSE`](QtSpim/icons/LICENSE). Apple's SF Symbols and SF
  fonts are not included.
- Qt is used under the GNU Lesser General Public License version 3.

## License

SPIM is Copyright (c) 1990-2023, by James R. Larus. All rights reserved.
The changes in this fork are Copyright (c) 2026, qtspim-arm contributors, and
are distributed under the same license.

SPIM is distributed under a BSD license:

```
Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of
    conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or other materials
    provided with the distribution.

    Neither the name of the James R. Larus nor the names of its contributors may be used to
    endorse or promote products derived from this software without specific prior written
    permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

QtSpim ARM is linked dynamically to the [Qt](https://www.qt.io/) library,
which is distributed under the GNU Lesser General Public License version 3
(<https://www.gnu.org/licenses/lgpl-3.0.html>). The Qt source code is
available from <https://download.qt.io/>.

## Third-party assets

None beyond the original SPIM sources and Qt. Any icons or fonts added later
will be listed here with their licenses.
