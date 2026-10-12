#!/usr/bin/env bash
#
# Run the spim regression tests and compare the results with the golden
# outputs recorded from the original SPIM 9.1.24 (SVN r764).
#
# Copyright (c) 2026, qtspim-arm contributors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# Neither the name of the James R. Larus nor the names of its contributors may be
# used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Usage: scripts/run-tests.sh [--record] [--keep] SPIM [TEST...]
#
# Runs every test in tests/manifest.txt (or only the named tests; shell
# patterns such as 'tt.*' are allowed) with the given spim executable and
# compares standard output, standard error and the exit status with
# tests/golden/<test>.{stdout,stderr,status}.  Any difference is shown as a
# unified diff and makes the script exit with status 1.
#
#   --record  Write golden files for tests that do not have any yet.
#             Existing golden files are never overwritten.  Golden files
#             must only be recorded with the original SPIM 9.1.24 (r764)
#             build; see tests/README.md.
#   --keep    Keep the directory with the actual outputs and print its path.
#
# Environment:
#   SPIM_TEST_TIMEOUT  Seconds before a test is killed (default 60).
#
# The script works with the bash 3.2 that ships with macOS.

set -u

usage() {
  echo "usage: $0 [--record] [--keep] SPIM [TEST...]" >&2
  exit 2
}

record=0
keep=0
while [ $# -gt 0 ]; do
  case "$1" in
    --record) record=1; shift ;;
    --keep) keep=1; shift ;;
    -h|--help) usage ;;
    --) shift; break ;;
    -*) echo "$0: unknown option: $1" >&2; usage ;;
    *) break ;;
  esac
done
[ $# -ge 1 ] || usage

spim="$1"
shift
if [ ! -f "$spim" ] || [ ! -x "$spim" ]; then
  echo "$0: not an executable file: $spim" >&2
  exit 2
fi
spim="$(cd "$(dirname "$spim")" && pwd)/$(basename "$spim")"

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root" || exit 2

manifest=tests/manifest.txt
golden=tests/golden
timeout="${SPIM_TEST_TIMEOUT:-60}"

out="$(mktemp -d "${TMPDIR:-/tmp}/spim-tests.XXXXXX")" || exit 2
if [ "$keep" -eq 0 ]; then
  trap 'rm -rf "$out"' EXIT
fi

# Run spim with an empty environment: spim copies the environment onto the
# simulated stack, so it would otherwise change $sp and the stack contents.
#
# The timeout is a separate watchdog process that kills spim.  It must not
# use alarm() or setitimer() in the spim process (e.g. perl -e 'alarm ...;
# exec ...'): spim drives the simulated CP0 timer with ITIMER_REAL and
# ignores SIGALRM, so an inherited alarm would both fail to stop spim and
# delay the simulated timer.
#
# Usage: run_spim INPUT STDOUT STDERR ARGS...
run_spim() {
  local input="$1" stdout="$2" stderr="$3" pid watchdog status
  shift 3
  env -i "$spim" "$@" <"$input" >"$stdout" 2>"$stderr" &
  pid=$!
  (
    trap 'kill "$sleeper" 2>/dev/null; exit 0' TERM
    sleep "$timeout" &
    sleeper=$!
    wait "$sleeper"
    kill -KILL "$pid" 2>/dev/null
  ) &
  watchdog=$!
  wait "$pid"
  status=$?
  kill -TERM "$watchdog" 2>/dev/null
  wait "$watchdog" 2>/dev/null
  return "$status"
}

trim() {
  local s="$1"
  s="${s#"${s%%[![:space:]]*}"}"
  s="${s%"${s##*[![:space:]]}"}"
  printf '%s' "$s"
}

selected() {
  [ $# -gt 1 ] || return 0
  local name="$1" pat
  shift
  for pat in "$@"; do
    # shellcheck disable=SC2254 # $pat is meant to be a glob pattern.
    case "$name" in
      $pat) return 0 ;;
    esac
  done
  return 1
}

show_diff() {
  local expected="$1" actual="$2" label="$3" lines
  lines=$(diff -u --label "golden/$label" --label "actual/$label" \
    "$expected" "$actual" | wc -l)
  diff -u --label "golden/$label" --label "actual/$label" \
    "$expected" "$actual" | head -n 200
  if [ "$lines" -gt 200 ]; then
    echo "... ($((lines - 200)) more lines of diff)"
  fi
}

passed=0
failed=0
recorded=0
failed_names=""
names=""

while IFS='|' read -r name input args || [ -n "$name" ]; do
  name="$(trim "$name")"
  case "$name" in
    ''|'#'*) continue ;;
  esac
  input="$(trim "${input:-}")"
  args="$(trim "${args:-}")"
  names="$names $name"
  selected "$name" "$@" || continue

  if [ "$input" = "-" ] || [ -z "$input" ]; then
    input=/dev/null
  elif [ ! -f "$input" ]; then
    echo "FAIL $name: missing input file $input"
    failed=$((failed + 1))
    failed_names="$failed_names $name"
    continue
  fi

  argv=()
  read -r -a argv <<<"$args"
  run_spim "$input" "$out/$name.stdout" "$out/$name.stderr" \
    ${argv[@]+"${argv[@]}"}
  echo "$?" >"$out/$name.status"

  if [ ! -f "$golden/$name.stdout" ] && [ ! -f "$golden/$name.stderr" ] &&
    [ ! -f "$golden/$name.status" ]; then
    if [ "$record" -eq 1 ]; then
      mkdir -p "$golden"
      cp "$out/$name.stdout" "$out/$name.stderr" "$out/$name.status" "$golden/"
      echo "RECORDED $name (status $(cat "$out/$name.status"))"
      recorded=$((recorded + 1))
    else
      echo "FAIL $name: no golden files in $golden"
      failed=$((failed + 1))
      failed_names="$failed_names $name"
    fi
    continue
  fi

  ok=1
  for kind in stdout stderr status; do
    if [ ! -f "$golden/$name.$kind" ]; then
      [ "$ok" -eq 1 ] && echo "FAIL $name"
      echo "  missing golden file $golden/$name.$kind"
      ok=0
    elif ! cmp -s "$golden/$name.$kind" "$out/$name.$kind"; then
      [ "$ok" -eq 1 ] && echo "FAIL $name"
      show_diff "$golden/$name.$kind" "$out/$name.$kind" "$name.$kind"
      ok=0
    fi
  done
  if [ "$ok" -eq 1 ]; then
    echo "ok   $name"
    passed=$((passed + 1))
  else
    if [ "$(cat "$out/$name.status")" = "137" ]; then
      echo "  (spim was killed after ${timeout}s; see SPIM_TEST_TIMEOUT)"
    fi
    failed=$((failed + 1))
    failed_names="$failed_names $name"
  fi
done <"$manifest"

# Golden files that no test in the manifest uses.
for f in "$golden"/*.stdout "$golden"/*.stderr "$golden"/*.status; do
  [ -f "$f" ] || continue
  base="$(basename "$f")"
  base="${base%.*}"
  case " $names " in
    *" $base "*) ;;
    *) echo "warning: $f does not belong to any test in $manifest" ;;
  esac
done

if [ $# -gt 0 ] && [ $((passed + failed + recorded)) -eq 0 ]; then
  echo "$0: no test in $manifest matches: $*" >&2
  exit 2
fi

echo
echo "$passed passed, $failed failed$([ "$recorded" -gt 0 ] && echo ", $recorded recorded")"
[ "$keep" -eq 1 ] && echo "actual outputs: $out"
if [ "$failed" -gt 0 ]; then
  echo "failed:$failed_names"
  exit 1
fi
exit 0
