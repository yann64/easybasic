#!/usr/bin/env bash
# Golden e2e test runner: compiles tests/e2e/<case>/input.pb with pbcxx,
# runs it, and diffs stdout against tests/e2e/<case>/expected.stdout.
#
# Usage: run_case.sh <path-to-pbcxx> <case-dir>
set -u

PBCXX="$1"
CASE_DIR="$2"
INPUT="$CASE_DIR/input.pb"
EXPECTED="$CASE_DIR/expected.stdout"
WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT

BIN="$WORKDIR/case_bin"

# Always compiled in debug mode (-d): a plain build strips every Debug
# statement to nothing (matches real PureBasic's own non-`-d` behavior), so
# `-d` is what makes these golden tests' output checkable at all.
if ! "$PBCXX" "$INPUT" -o "$BIN" -d > "$WORKDIR/compile.log" 2>&1; then
    echo "FAIL: $CASE_DIR - pbcxx failed to compile"
    cat "$WORKDIR/compile.log"
    exit 1
fi

# Run from inside WORKDIR (not wherever this script itself happens to be
# invoked from) so a test program using a plain relative filename (e.g. the
# filelib test's own scratch files) writes somewhere unique and already
# cleaned up by the trap above - a real portability gap this project's own
# first-ever Windows CI run (M6) caught the hard way, via tests that
# instead hardcoded absolute `/tmp/...` paths (not a real path on Windows).
if ! ( cd "$WORKDIR" && ./case_bin ) > "$WORKDIR/actual.stdout" 2>"$WORKDIR/actual.stderr"; then
    echo "FAIL: $CASE_DIR - compiled binary exited non-zero"
    cat "$WORKDIR/actual.stderr"
    exit 1
fi

if ! diff -u "$EXPECTED" "$WORKDIR/actual.stdout"; then
    echo "FAIL: $CASE_DIR - stdout mismatch"
    exit 1
fi

echo "PASS: $CASE_DIR"
exit 0
