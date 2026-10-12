# Changes from the original QtSpim

QtSpim ARM compared with the original SPIM / QtSpim 9.1.24 (SourceForge SVN
r764, commit `6cb6a08` in this repository). QtSpim ARM is an unofficial fork;
see [`README.md`](README.md).

## Simulator behaviour

The simulator must behave exactly like the original. The regression tests in
[`regression/`](regression/README.md) were recorded with the unmodified r764 build and
pass on every build of QtSpim ARM (x86_64 and arm64).

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Instructions, pseudo-instructions, system calls, exceptions, delay slots, memory layout, output format | — | **unchanged** |
| NaN produced by an invalid floating-point operation (0/0, inf − inf, 0 × inf, sqrt(−1)) | taken from the host CPU: `0xffc00000` on x86, `0x7fc00000` on a native arm64 build | `0xffc00000` on every host (the x86 result) |
| Float-to-integer conversion (`cvt.w`, `trunc.w`, `round.w`, `floor.w`, `ceil.w`) of NaN, infinity or a value outside the int32 range | C cast (undefined): `0x80000000` on x86; saturated, and NaN → 0, on a native arm64 build | `0x80000000` on every host (the x86 result) |
| Everything on x86 | — | unchanged, including the two cases above |
| A syntax error at the very end of a file that has no final newline (for example `add $t0, $t1` with an operand missing) | spim and QtSpim hang: the scanner loops forever while printing the line | the error is reported as usual and loading stops |

The only changes in the simulator core are in `CPU/run.cpp` (the two
floating-point rows) and `CPU/scanner.l` (the last row).
Behaviour of the original that looks like a bug is kept on purpose; examples
are listed in [`regression/README.md`](regression/README.md#original-spim-behaviour-recorded-here).

## Platform and build

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Mac architectures | Intel only; Apple Silicon needs Rosetta 2 | Universal Binary, arm64 + x86_64 |
| Qt | Qt 5 (`QRegExp`, `QDesktopWidget`) | Qt 6 (releases use Qt 6.11), without Qt5Compat; the sources still compile with Qt 5.15 |
| Build system | qmake (`QtSpim/QtSpim.pro`) and `spim/Makefile` | CMake (`CMakeLists.txt`); the qmake project and the Makefile are kept |
| Minimum macOS | not set in `Info.plist` | macOS 13 |
| Help files | built with `qcollectiongenerator` (removed from newer Qt) | built with `qhelpgenerator` into the app bundle |
| Signing and notarization | none | planned (Developer ID, notarized DMG) |

## The QtSpim application

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| App bundle | `QtSpim.app` | `QtSpim ARM.app` (can be installed next to the original) |
| Bundle ID | `com.larus.qtspim` | `net.yuan-tw.qtspim` |
| Version in `Info.plist` | `9.1.23` (outdated) | `9.1.24` plus a build number; releases are tagged `v9.1.24-arm.N` |
| Retina displays | `Info.plist` lacks `NSHighResolutionCapable` | `NSHighResolutionCapable` set |
| Dark mode | tab labels unreadable; register, text, data and message windows only partly light, with some text black on dark | all text readable; the windows look as in light mode |
| Help menu | runs Assistant only from `/Applications/QtSpim.app` | looks inside the running app bundle, wherever it is installed (Assistant is to be bundled when packaging) |
| About box and window title | QtSpim | QtSpim ARM, stating that it is an unofficial fork; the original credits are kept |
| Settings (recent files, window layout, …) | stored as `LarusStone` / `QtSpim` | unchanged, so existing settings still load |
| Built-in exception handler | written to a temporary file as a C string, which can pick up bytes that follow it in memory | copied by size; the original code made the macOS build hang at startup |

A plain Qt 6 port would also have broken three things that QtSpim ARM keeps
working like the original: the exception handler Reset button in Settings
(`QSignalMapper::mapped` was removed in Qt 6), the address computed when you
right-click a data word (the words are separated by non-breaking spaces), and
the keys under which recent files are saved.

## The new interface

QtSpim ARM opens in a new interface ([`docs/UI_DESIGN.md`](docs/UI_DESIGN.md)).
The original windows are still available as the classic interface
(**Classic** in the toolbar), and behave as before. Both use the same
simulator in the same way: the program is loaded from a file with
`initialize_world()` and `read_assembly_file()` and run with `run_program()`.

| | Original QtSpim 9.1.24 | New interface |
|---|---|---|
| Layout | separate register, text, data, message and console windows | one window: editor; console, memory and problems below; registers on the right |
| Writing programs | in another editor, then File › Reinitialize and Load File | built-in editor; Run writes the editor's text to a temporary file and loads it |
| Assembler errors | a message box per message | marked on the line, explained in plain words with spelling suggestions and listed under Problems with SPIM's own message; a program with errors runs only with **Run anyway** (the original ran it) |
| Run errors (`run_error`, e.g. a jump to an address without an instruction) | a message box; OK lets the program go on | the program stops, as in the command-line `spim` (the classic interface keeps the box) |
| Exception messages and other `error()` output while running | a message box and the messages window | printed in red in the console |
| Stepping | one machine instruction | one machine instruction; the first Step runs the startup code up to `main`, and a step over code outside the program (the exception handler) finishes it, unless **Show kernel code** is on |
| Breakpoints | on addresses, in the text window; a dialog when one is hit | on source lines, in the editor's margin; the status bar says where the program stopped |
| Console input | each key goes to the program at once | a line, sent with Return; `read_char` and memory-mapped I/O still get each key at once |
| Language | English | Traditional Chinese or English (the classic interface stays English) |
| Appearance | follows the system | light, also when the system is dark |
| Settings | `LarusStone` / `QtSpim` | the same file and keys, plus editor and display settings; `UI/Classic` remembers the interface |
| Opening files | File › Load File (`*.a *.s *.asm`) | Open (`*.s *.as *.asm`), drag and drop, or from the Finder |

## Command-line `spim`

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Behaviour and version string | — | unchanged, apart from the two floating-point cases above on arm64 |
| Default exception handler path | `/usr/share/spim/exceptions.s` (`spim/Makefile`) | `<prefix>/share/spim/exceptions.s` with CMake (`/usr/local/...` by default); `spim/Makefile` is unchanged |

## Repository

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Tests | six torture tests in `Tests/`; `tt.alu.bare.s` no longer assembles | the same plus 37 more test runs in `regression/`: 43 in all, comparing stdout, stderr and exit status with recordings from the original build (one, for the hang above, recorded with the fixed build), run by `scripts/run-tests.sh` |
| Documentation | `README`, `Documentation/` | also `README.md`, `CHANGES.md`, `docs/BUILDING.md`, `docs/UI_DESIGN.md`, `regression/README.md` |
| Examples | — | four short example programs in `QtSpim/examples/` (BSD license), opened from the welcome page |

## Not changed

`PCSpim/`, `xspim/`, `Setup/`, `Tests/`, `Documentation/`, `spim/`,
`CPU/exceptions.s`, the parser and scanner, and every file in `CPU/` other than
`run.cpp` and `scanner.l`. The original `README` and `QtSpim/macinfo.plist`
are kept as they were; `QtSpim/QtSpim.pro` only lists the new sources.
