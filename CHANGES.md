# Changes from the original QtSpim

QtSpim ARM compared with the original SPIM / QtSpim 9.1.24 (SourceForge SVN
r764, commit `6cb6a08` in this repository). QtSpim ARM is an unofficial fork;
see [`README.md`](README.md).

## Simulator behaviour

The simulator must behave exactly like the original. The regression tests in
[`tests/`](tests/README.md) were recorded with the unmodified r764 build and
pass on every build of QtSpim ARM (x86_64 and arm64).

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Instructions, pseudo-instructions, system calls, exceptions, delay slots, memory layout, output format | — | **unchanged** |
| NaN produced by an invalid floating-point operation (0/0, inf − inf, 0 × inf, sqrt(−1)) | taken from the host CPU: `0xffc00000` on x86, `0x7fc00000` on a native arm64 build | `0xffc00000` on every host (the x86 result) |
| Float-to-integer conversion (`cvt.w`, `trunc.w`, `round.w`, `floor.w`, `ceil.w`) of NaN, infinity or a value outside the int32 range | C cast (undefined): `0x80000000` on x86; saturated, and NaN → 0, on a native arm64 build | `0x80000000` on every host (the x86 result) |
| Everything on x86 | — | unchanged, including the two cases above |

The only change in the simulator core is in `CPU/run.cpp` (the two rows above).
Behaviour of the original that looks like a bug is kept on purpose; examples
are listed in [`tests/README.md`](tests/README.md#original-spim-behaviour-recorded-here).

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

## Command-line `spim`

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Behaviour and version string | — | unchanged, apart from the two floating-point cases above on arm64 |
| Default exception handler path | `/usr/share/spim/exceptions.s` (`spim/Makefile`) | `<prefix>/share/spim/exceptions.s` with CMake (`/usr/local/...` by default); `spim/Makefile` is unchanged |

## Repository

| | Original 9.1.24 | QtSpim ARM |
|---|---|---|
| Tests | six torture tests in `Tests/`; `tt.alu.bare.s` no longer assembles | the same plus 36 more test runs: 42 in all, comparing stdout, stderr and exit status with recordings from the original build, run by `scripts/run-tests.sh` |
| Documentation | `README`, `Documentation/` | also `README.md`, `CHANGES.md`, `docs/BUILDING.md`, `tests/README.md` |

## Not changed

`PCSpim/`, `xspim/`, `Setup/`, `Tests/`, `Documentation/`, `spim/`,
`CPU/exceptions.s`, the parser and scanner, and every file in `CPU/` other than
`run.cpp`. The original `README`, `QtSpim/QtSpim.pro` and
`QtSpim/macinfo.plist` are kept as they were.
