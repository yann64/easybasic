#!/usr/bin/env bash
# Golden e2e test runner for GUI (M7b+) test cases - otherwise identical to
# run_case.sh, but skips gracefully (ctest's SKIP_RETURN_CODE, wired up in
# tests/CMakeLists.txt) rather than failing when this environment can't
# actually run a GUI program: no GTK3 dev files to compile against, or no
# usable display to open a real window on. The same graceful-skip
# convention scripts/diff_against_pbcompilerc.sh already uses when
# pbcompilerc itself isn't installed.
#
# Usage: run_gui_case.sh <path-to-pbcxx> <case-dir>
set -u

SKIP_CODE=125

PBCXX="$1"
CASE_DIR="$2"
INPUT="$CASE_DIR/input.pb"
EXPECTED="$CASE_DIR/expected.stdout"
WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT

BIN="$WORKDIR/case_bin"

# Copy over any extra fixture files a case ships alongside input.pb/
# expected.stdout (e.g. gui_image's own test.bmp for LoadImage) - run from
# $WORKDIR, not $CASE_DIR, the same reason run_case.sh's own comment gives,
# so a case can reference one by a plain relative filename.
for extra in "$CASE_DIR"/*; do
    base=$(basename "$extra")
    if [ "$base" != "input.pb" ] && [ "$base" != "expected.stdout" ]; then
        cp "$extra" "$WORKDIR/"
    fi
done

# Always compiled in debug mode (-d): a plain build strips every Debug
# statement to nothing, matching run_case.sh's own convention.
if ! "$PBCXX" "$INPUT" -o "$BIN" -d > "$WORKDIR/compile.log" 2>&1; then
    if grep -q "pkg-config gtk+-3.0' failed" "$WORKDIR/compile.log"; then
        echo "SKIP: $CASE_DIR - GTK3 dev files not available"
        exit $SKIP_CODE
    fi
    echo "FAIL: $CASE_DIR - pbcxx failed to compile"
    cat "$WORKDIR/compile.log"
    exit 1
fi

# GDK_BACKEND=x11 avoids GTK silently preferring a Wayland compositor over
# an Xvfb X11 display also present in the environment (this project's own
# dev machine, and Xvfb-based CI runners, both hit this - see the M7b
# roadmap notes). Forcing it is harmless on a plain-X11-only environment.
# NO_AT_BRIDGE=1 skips GTK's own accessibility (AT-SPI) D-Bus bridge, which
# has nothing to connect to here anyway - see tests/lsan-suppressions.txt's
# matching entry for the LeakSanitizer false positive this otherwise causes
# under an ASan/UBSan build.
if ! ( cd "$WORKDIR" && GDK_BACKEND=x11 NO_AT_BRIDGE=1 ./case_bin ) > "$WORKDIR/actual.stdout" 2>"$WORKDIR/actual.stderr"; then
    if grep -qi "cannot open display\|no protocol specified\|cannot connect to" "$WORKDIR/actual.stderr"; then
        echo "SKIP: $CASE_DIR - no usable display available"
        exit $SKIP_CODE
    fi
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
