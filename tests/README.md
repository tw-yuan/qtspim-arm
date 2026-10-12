# Regression tests

These tests pin down the behaviour of the original SPIM 9.1.24 (SourceForge SVN
r764, commit `6cb6a08` on `main`), so that the port can be checked against it.
SPIM is a teaching tool: assignments are graded with the original SPIM, so any
change in instruction semantics, system calls, exception handling, delay slots,
memory layout or output format is a bug.

## Running

```bash
scripts/run-tests.sh path/to/spim            # all tests
scripts/run-tests.sh path/to/spim 'tt.*'     # only some tests (shell patterns)
scripts/run-tests.sh --keep path/to/spim     # keep the actual outputs
```

The script runs every test in `tests/manifest.txt` and compares the standard
output, standard error and exit status with `tests/golden/<test>.stdout`,
`.stderr` and `.status`. Any difference is printed as a unified diff, and the
script exits with status 1. It needs only bash 3.2, `diff`, `cmp` and (for the
per-test timeout, `SPIM_TEST_TIMEOUT`, default 60 s) `perl`, all of which
macOS includes.

spim is run from the repository root with an **empty environment**
(`env -i`). spim copies its environment onto the simulated stack, so `$sp` and
the stack contents would otherwise depend on the machine. Test programs and
input files are given as paths relative to the repository root, because spim
puts them in error messages and on the stack too.

## Layout

| Path | Contents |
|---|---|
| `manifest.txt` | One line per test: name, stdin file, spim arguments |
| `golden/` | Expected stdout, stderr and exit status of each test |
| `cases/` | Supplementary test programs (`*.s`) and their input (`*.in`, `data/`) |

The manifest runs the upstream torture tests in `Tests/` with the arguments
from `spim/Makefile` (`test` and `test_bare` targets), plus `Tests/tt.io.s`
(memory-mapped I/O, input from `cases/tt.io.in`) and `helloworld.s`. The
programs in `cases/` cover:

| Area | Tests |
|---|---|
| System calls | `syscall_print`, `syscall_read`, `syscall_sbrk`, `sbrk_limit`, `sbrk_limit.ldata`, `syscall_exit`, `syscall_exit2`, `syscall_file`, `syscall_unknown` |
| Exceptions | `exceptions`, `exceptions.quiet`, `exc_bad_pc`, `exc_no_handler`, `exc_custom_handler` |
| Delay slots | `delay_branch`, `delay_branch.delayed`, `delay_load`, `delay_load.delayed`, `delay_both.delayed` |
| Bare machine | `bare`, `bare_pseudo` |
| Integer instructions and pseudo-instructions | `int_arith` |
| Floating point | `fp_arith`, `fp_host_dependent` |
| Byte order | `endian` |
| Memory layout and startup state | `data_layout`, `argv`, `argv.extra` |
| Assembler diagnostics | `parse_errors`, `syntax_error`, `missing_file` |
| Command line and interactive mode | `usage`, `interactive` |

## How the golden files were made

The golden files were recorded with `scripts/run-tests.sh --record` from a
`spim` built from the **unmodified** source (`git archive 6cb6a08`) with the
upstream `spim/Makefile`:

| | |
|---|---|
| Host | Ubuntu 24.04, x86_64 |
| Compiler | GCC 13.3.0 (`g++ -O`, as in `spim/Makefile`) |
| bison / flex | GNU Bison 3.8.2 / flex 2.6.4 |
| C library | glibc 2.39 |

They were then checked against other builds of the same unmodified source:

| Build | Result |
|---|---|
| clang 18, x86_64 | all tests pass |
| GCC 13.3, aarch64 (qemu-user, `-fsigned-char` as on Apple platforms) | all pass except `fp_host_dependent` |
| clang 18, aarch64 (qemu-user, `-fsigned-char`) | all pass except `fp_host_dependent` (same diff) |

`--record` only writes golden files for tests that have none, and it never
overwrites existing ones.

## Rules

* **Never edit or re-record a golden file to make a test pass.** If the output
  differs from the original SPIM, find out why and report it. Golden files
  change only when a new test is added, and then they must be recorded with
  the original r764 build (or the Homebrew `spim` 9.1.24, which is built from
  r764).
* Golden files contain only behaviour that is the same on every host. NaN
  values are compared as raw bits and never printed with `print_float` /
  `print_double`, because C libraries format NaN signs differently (glibc
  prints `-nan`, macOS prints `nan`).
* The version banner is not printed by any test, so a version number change
  does not touch the golden files.

## Behaviour that depends on the host CPU

`fp_host_dependent` records floating-point results that SPIM takes directly from
the host FPU and C compiler, using the values of the original x86 build:

* The NaN produced by an invalid operation (0/0, inf − inf, 0 × inf, sqrt(−1)):
  x86 gives `0xffc00000` / `0xfff8000000000000`, arm64 gives `0x7fc00000` /
  `0x7ff8000000000000`.
* Float-to-integer conversions (`cvt.w`, `trunc.w`, `round.w`, `floor.w`,
  `ceil.w`) of values out of the `int32` range, of infinities and of NaN: SPIM
  uses a C cast, which is undefined behaviour in that case. x86 gives
  `0x80000000`; arm64 saturates to `0x7fffffff` / `0x80000000` and converts NaN
  to `0`.

A native arm64 build of the unmodified source fails this test. That is a real
behaviour difference between the arm64 port and the x86 original, not a test
problem. Fixing it would require a change in `CPU/`, which needs the project
owner's decision.

## Original SPIM behaviour recorded here

These may look like bugs, but they are what SPIM 9.1.24 does, so the port must
keep them:

* `Tests/tt.alu.bare.s` no longer assembles: newer SPIM versions reject some of
  its immediates (`addi $4 $0 0xff98`, `ori $4 $3 -1`, …) and stop at a
  syntax error (`ctc3`). The run ends with "Instruction references undefined
  symbol".
* Only the last word after `-file <program>` reaches the program: spim treats
  every further word that does not start with `-` as a new program file name
  (`argv.extra`).
* For some assembler errors the reported line number is that of the next line
  (`parse_errors`).
* A label followed by `.align` is moved to the aligned address (see the comment
  in `cases/syscall_print.s`).
* In interactive mode, `print_all_regs` without an argument also swallows the
  next input line (`interactive`).
* `bnel` that is not taken skips the next instruction even without
  `-delayed_branches` (`delay_branch`).
* `round.w` adds 0.5 and truncates toward zero, so `round.w.s -2.5` gives −2
  and `round.w.s -1.7` gives −1 (`fp_arith`).
* The command-line spim's data segment is 128 KB (`spim/Makefile` sets
  `-DDATA_SIZE=131072`), and `sbrk` can grow it to 1 MB unless `-ldata` gives a
  larger limit (`sbrk_limit`, `sbrk_limit.ldata`).

A test that jumps to a word-aligned address with no instruction
(`jr 0x00410000`) is not included: the default exception handler skips to the
next word, which raises the same exception again, so spim loops forever.
