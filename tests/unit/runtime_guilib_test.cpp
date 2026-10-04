#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include <easybasic/runtime/guilib.hpp>
#include <easybasic/runtime/threadlib.hpp> // pbElapsedMilliseconds

using namespace easybasic::runtime;

namespace {
/// GUI tests need a real, connectable display (X11 via a real desktop or
/// Xvfb) - `gtk_init_check` (unlike plain `gtk_init`, which aborts the
/// whole process on failure) reports this cleanly, so every test case
/// below skips rather than fails when none is available, the same
/// graceful-skip convention `diff_against_pbcompilerc.sh` already uses
/// when `pbcompilerc` itself isn't installed. Most CI runners have no
/// display at all; this dev environment does (a Wayland session plus a
/// separate Xvfb :99 for GUI testing specifically - see the M7b roadmap
/// notes on why `GDK_BACKEND=x11` has to be forced there).
bool hasDisplay() {
    static bool checked = [] {
        int argc = 0;
        char** argv = nullptr;
        return gtk_init_check(&argc, &argv) == TRUE;
    }();
    return checked;
}

/// Drains whatever startup events a freshly-opened window generates
/// (move/resize/repaint - oracle-verified real PB generates some too)
/// before a test asserts on "no event pending". A plain `while
/// (pbWindowEvent() != 0) {}` isn't reliable here: `pbWindowEvent()` only
/// pumps events GTK has *already* received from the X server
/// (`gtk_events_pending()`), so a tight loop can see "nothing pending"
/// for a moment before a configure/draw event that's still in flight over
/// the socket actually arrives, and exit too early - exactly the timing
/// mistake this project's own oracle testing made (and corrected) earlier
/// in M7b with a `.pb` test script. Waiting for a short quiet streak
/// (matching the `.pb`-side fix) avoids that.
void drainEvents() {
    int quietStreak = 0;
    while (quietStreak < 5) {
        if (pbWindowEvent() == 0) {
            quietStreak++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            quietStreak = 0;
        }
    }
}
} // namespace

TEST_CASE("pbOpenWindow/pbIsWindow/pbCloseWindow round-trip", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    CHECK(pbIsWindow(100) == 0);
    pbOpenWindow(100, 10, 10, 100, 100, PBString("Test"));
    CHECK(pbIsWindow(100) == 1);
    pbCloseWindow(100);
    CHECK(pbIsWindow(100) == 0);
}

TEST_CASE("pbIsWindow/pbCloseWindow return a harmless default for an unknown window ID",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    CHECK(pbIsWindow(999999) == 0);
    CHECK(pbCloseWindow(999999) == 0);
}

TEST_CASE("pbWindowEvent returns 0 when no event is pending", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(101, 10, 10, 100, 100, PBString("Test"));
    drainEvents();
    CHECK(pbWindowEvent() == 0);
    pbCloseWindow(101);
}

TEST_CASE("A delete-event (the real close-button signal) queues #PB_Event_CloseWindow, "
          "and does not destroy the window itself",
          "[runtime][guilib]") {
    // Verified this way (direct signal emission) rather than via a real
    // user click/window-manager interaction, which this headless
    // environment has no window manager to mediate (confirmed: xdotool's
    // own windowclose bypasses the WM_DELETE_WINDOW protocol entirely
    // without one, destroying the GdkWindow directly instead) - this
    // exercises the exact same code path (GTK's own "delete-event" signal
    // delivery to pbcxx's own handler) a real close-button click would.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(102, 10, 10, 100, 100, PBString("Test"));
    GtkWidget* window = detail::windowTable().at(102);
    GdkEvent* fakeEvent = gdk_event_new(GDK_DELETE);
    gboolean returnValue = GDK_EVENT_PROPAGATE;
    g_signal_emit_by_name(window, "delete-event", fakeEvent, &returnValue);
    gdk_event_free(fakeEvent);

    CHECK(returnValue == TRUE); // Oracle-verified: the window itself isn't destroyed by this signal alone.
    CHECK(pbIsWindow(102) == 1);
    CHECK(pbWindowEvent() == 2); // #PB_Event_CloseWindow
    CHECK(pbEventWindow() == 102);
    pbCloseWindow(102);
    CHECK(pbIsWindow(102) == 0);
}

TEST_CASE("pbWaitWindowEvent returns 0 after its timeout elapses with no event", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(103, 10, 10, 100, 100, PBString("Test"));
    drainEvents();
    std::int64_t before = pbElapsedMilliseconds();
    std::int64_t result = pbWaitWindowEvent(100);
    std::int64_t elapsed = pbElapsedMilliseconds() - before;
    CHECK(result == 0);
    CHECK(elapsed >= 80); // Loose lower bound to avoid flakiness.
    pbCloseWindow(103);
}
