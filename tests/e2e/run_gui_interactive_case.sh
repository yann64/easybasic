#!/usr/bin/env bash
# Interactive GUI e2e test runner - for cases that need real synthetic
# mouse/keyboard input (M7b's CanvasGadget is the first), not just a
# plain Debug probe. Otherwise the same shape as run_gui_case.sh (same
# SKIP_RETURN_CODE graceful-skip convention, same -d compile, same
# GDK_BACKEND=x11/NO_AT_BRIDGE=1), plus:
#   - its own private Xvfb display, so this doesn't disturb (or get
#     disturbed by) a real desktop session or another test's own display;
#   - a companion `xdotool_script` fixture file alongside input.pb/
#     expected.stdout - a plain bash fragment run with `$WID` set to the
#     case's own window id (found via `xdotool search --class`, matching
#     the compiled binary's own basename, which GTK uses as the window's
#     WM_CLASS by default) and `$DISPLAY` already exported.
#
# This exists because confirming which real mouse click/move/key-press
# produces which EventType()/GetGadgetAttribute() reading can't be done
# with a plain Debug probe the way every other oracle fact in this
# project has been - see pbCanvasGadget's own doc comment in guilib.hpp,
# and the M7b roadmap notes, for the full methodology this was built
# against.
#
# Usage: run_gui_interactive_case.sh <path-to-pbcxx> <case-dir>
set -u

SKIP_CODE=125

PBCXX="$1"
CASE_DIR="$2"
INPUT="$CASE_DIR/input.pb"
EXPECTED="$CASE_DIR/expected.stdout"
XDOTOOL_SCRIPT="$CASE_DIR/xdotool_script"
WORKDIR=$(mktemp -d)

XVFB_PID=""
cleanup() {
    if [ -n "$XVFB_PID" ]; then
        kill "$XVFB_PID" >/dev/null 2>&1
        wait "$XVFB_PID" 2>/dev/null
    fi
    rm -rf "$WORKDIR"
}
trap cleanup EXIT

if ! command -v Xvfb >/dev/null 2>&1 || ! command -v xdotool >/dev/null 2>&1; then
    echo "SKIP: $CASE_DIR - Xvfb/xdotool not available"
    exit $SKIP_CODE
fi

BIN="$WORKDIR/case_bin"

for extra in "$CASE_DIR"/*; do
    base=$(basename "$extra")
    if [ "$base" != "input.pb" ] && [ "$base" != "expected.stdout" ] && [ "$base" != "xdotool_script" ]; then
        cp "$extra" "$WORKDIR/"
    fi
done

if ! "$PBCXX" "$INPUT" -o "$BIN" -d > "$WORKDIR/compile.log" 2>&1; then
    if grep -q "pkg-config gtk+-3.0' failed" "$WORKDIR/compile.log"; then
        echo "SKIP: $CASE_DIR - GTK3 dev files not available"
        exit $SKIP_CODE
    fi
    echo "FAIL: $CASE_DIR - pbcxx failed to compile"
    cat "$WORKDIR/compile.log"
    exit 1
fi

# Find a free display number for this test's own private Xvfb - tried
# sequentially rather than picked at random, so a failure here is at
# least reproducible; 90-99 is far enough above any display a real
# desktop session or another tool would normally claim.
DISPLAY_NUM=""
for candidate in $(seq 90 99); do
    if [ ! -e "/tmp/.X${candidate}-lock" ]; then
        DISPLAY_NUM="$candidate"
        break
    fi
done
if [ -z "$DISPLAY_NUM" ]; then
    echo "SKIP: $CASE_DIR - no free Xvfb display number found"
    exit $SKIP_CODE
fi

Xvfb ":$DISPLAY_NUM" -screen 0 1024x768x24 >"$WORKDIR/xvfb.log" 2>&1 &
XVFB_PID=$!
export DISPLAY=":$DISPLAY_NUM"

# Give Xvfb a moment to actually start listening before anything tries
# to connect to it.
for _ in $(seq 1 50); do
    if [ -e "/tmp/.X11-unix/X${DISPLAY_NUM}" ]; then
        break
    fi
    sleep 0.1
done

( cd "$WORKDIR" && GDK_BACKEND=x11 NO_AT_BRIDGE=1 ./case_bin ) > "$WORKDIR/actual.stdout" 2>"$WORKDIR/actual.stderr" &
APP_PID=$!

# Wait for the window to actually appear before driving any input at it -
# xdotool's own WM_CLASS match on the compiled binary's basename avoids
# needing every case's own input.pb to agree on a fixed window title.
WID=""
for _ in $(seq 1 50); do
    WID=$(xdotool search --class "case_bin" 2>/dev/null | head -1)
    if [ -n "$WID" ]; then
        break
    fi
    sleep 0.1
done
if [ -z "$WID" ]; then
    kill "$APP_PID" >/dev/null 2>&1
    echo "FAIL: $CASE_DIR - compiled binary's own window never appeared"
    cat "$WORKDIR/actual.stderr"
    exit 1
fi
export WID

# The window id existing (just confirmed above) doesn't mean X considers
# it viewable/focusable yet - with no window manager running on this
# private Xvfb display, an `xdotool windowfocus` attempted too early
# fails with a real `BadMatch` on `X_SetInputFocus` (confirmed directly
# while developing this script), which otherwise made the whole case
# flaky rather than reliably pass or fail. Retrying here, once, centrally,
# means `xdotool_script` itself doesn't need its own retry logic.
for _ in $(seq 1 50); do
    if xdotool windowfocus --sync "$WID" >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done

# shellcheck disable=SC1090
source "$XDOTOOL_SCRIPT"

# The case's own input.pb is expected to exit on its own once it's seen
# however many events it's waiting for - this loop (rather than a plain
# blocking `wait`) guards against a script/implementation mismatch
# hanging the whole test suite instead of just failing this one case.
EXITED=0
for _ in $(seq 1 100); do
    if ! kill -0 "$APP_PID" 2>/dev/null; then
        EXITED=1
        break
    fi
    sleep 0.1
done
if [ "$EXITED" -eq 0 ]; then
    kill "$APP_PID" >/dev/null 2>&1
    echo "FAIL: $CASE_DIR - compiled binary never exited on its own after the input script finished"
    cat "$WORKDIR/actual.stderr"
    exit 1
fi
wait "$APP_PID" 2>/dev/null

if ! diff -u "$EXPECTED" "$WORKDIR/actual.stdout"; then
    echo "FAIL: $CASE_DIR - stdout mismatch"
    exit 1
fi

echo "PASS: $CASE_DIR"
exit 0
