#!/usr/bin/env bash
# Differential e2e test: compiles the same .pb program with both the real
# PureBasic compiler (pbcompilerc) and pbcxx, runs both, and diffs stdout.
# The oracle's live output IS the expected output - no hand-written fixture
# needed. See docs/developer/oracle-testing.md.
#
# Skips gracefully (exit 0, printing SKIP) when pbcompilerc isn't installed,
# since it won't be present on most CI runners (no PureBasic license there)
# - this suite is local-dev-machine-plus-self-hosted-runner only.
#
# Usage: diff_against_pbcompilerc.sh <path-to-pbcxx> <program.pb>
set -u

PBCXX="$1"
PROGRAM="$2"

PBCOMPILERC="${PBCOMPILERC:-pbcompilerc}"
if ! command -v "$PBCOMPILERC" >/dev/null 2>&1; then
    echo "SKIP: $PROGRAM - pbcompilerc not found on this machine"
    exit 0
fi

WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT

# Both compilers are invoked in debug mode (-d): a plain (non-debug) PB
# build strips every Debug statement to nothing, so -d is what makes the
# comparison meaningful at all. Both also print PB's own oracle-verified
# "[Debugger]  <value>" prefix, so no output normalization is needed.
cp "$PROGRAM" "$WORKDIR/program.pb"
( cd "$WORKDIR" && "$PBCOMPILERC" program.pb -d -o oracle_bin ) > "$WORKDIR/oracle_compile.log" 2>&1
ORACLE_COMPILE_RC=$?
if [ $ORACLE_COMPILE_RC -ne 0 ]; then
    echo "FAIL: $PROGRAM - pbcompilerc itself failed to compile the program"
    cat "$WORKDIR/oracle_compile.log"
    exit 1
fi

if ! "$PBCXX" "$PROGRAM" -o "$WORKDIR/pbcxx_bin" -d > "$WORKDIR/pbcxx_compile.log" 2>&1; then
    echo "FAIL: $PROGRAM - pbcxx failed to compile"
    cat "$WORKDIR/pbcxx_compile.log"
    exit 1
fi

# Run from inside WORKDIR, not wherever this script itself was invoked from
# - see run_case.sh's own identical comment on why (a test program using a
# plain relative filename needs somewhere unique and already cleaned up by
# the trap above).
( cd "$WORKDIR" && ./oracle_bin ) > "$WORKDIR/oracle.stdout" 2>&1
( cd "$WORKDIR" && ./pbcxx_bin ) > "$WORKDIR/pbcxx.stdout" 2>&1

if ! diff -u "$WORKDIR/oracle.stdout" "$WORKDIR/pbcxx.stdout"; then
    echo "FAIL: $PROGRAM - pbcxx's output disagrees with the real pbcompilerc"
    exit 1
fi

echo "PASS: $PROGRAM"
exit 0
