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

TEST_CASE("Gadget creation round-trips through IsGadget/GetGadgetText/FreeGadget", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(200, 10, 10, 200, 200, PBString("Test"));
    CHECK(pbIsGadget(1) == 0);

    CHECK(pbButtonGadget(1, 10, 10, 100, 30, PBString("Click me")) == 1);
    CHECK(pbIsGadget(1) == 1);
    CHECK(pbGetGadgetText(1).bytes() == "Click me");

    CHECK(pbTextGadget(2, 10, 50, 100, 20, PBString("Label")) == 1);
    CHECK(pbStringGadget(3, 10, 80, 100, 20, PBString("init")) == 1);
    CHECK(pbCheckBoxGadget(4, 10, 110, 100, 20, PBString("Check")) == 1);
    CHECK(pbFrameGadget(5, 10, 140, 150, 40, PBString("Frame")) == 1);

    CHECK(pbFreeGadget(1) == 1);
    CHECK(pbIsGadget(1) == 0);
    CHECK(pbFreeGadget(1) == 0); // Already freed - harmless.

    pbCloseWindow(200);
}

TEST_CASE("GetGadgetText/SetGadgetText dispatch correctly per real gadget type", "[runtime][guilib]") {
    // Oracle-verified: ButtonGadget, TextGadget, StringGadget,
    // CheckBoxGadget, and FrameGadget all support GetGadgetText/
    // SetGadgetText for their own primary display text (button/checkbox
    // label, static label, entry content, frame title respectively).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(201, 10, 10, 200, 200, PBString("Test"));
    pbButtonGadget(1, 10, 10, 100, 30, PBString("Original"));
    pbTextGadget(2, 10, 50, 100, 20, PBString("Original"));
    pbStringGadget(3, 10, 80, 100, 20, PBString("Original"));
    pbCheckBoxGadget(4, 10, 110, 100, 20, PBString("Original"));
    pbFrameGadget(5, 10, 140, 150, 40, PBString("Original"));

    for (std::int64_t id = 1; id <= 5; ++id) {
        CHECK(pbGetGadgetText(id).bytes() == "Original");
        pbSetGadgetText(id, PBString("Changed"));
        CHECK(pbGetGadgetText(id).bytes() == "Changed");
    }

    pbCloseWindow(201);
}

TEST_CASE("GetGadgetState/SetGadgetState round-trip on a CheckBoxGadget", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(202, 10, 10, 200, 200, PBString("Test"));
    pbCheckBoxGadget(1, 10, 10, 150, 20, PBString("Check"));

    CHECK(pbGetGadgetState(1) == 0); // Oracle-verified: unchecked by default.
    CHECK(pbSetGadgetState(1, 1) == 1);
    CHECK(pbGetGadgetState(1) == 1);
    CHECK(pbSetGadgetState(1, 0) == 1);
    CHECK(pbGetGadgetState(1) == 0);

    pbCloseWindow(202);
}

TEST_CASE("SetGadgetState/SetGadgetText do not queue a spurious #PB_Event_Gadget", "[runtime][guilib]") {
    // Oracle-verified real bug this project's own gadget-event signal
    // wiring hit and fixed: gtk_toggle_button_set_active/gtk_entry_set_text
    // fire the same "toggled"/"changed" GTK signals a real user
    // click/keystroke does, but real PB's own SetGadgetState/SetGadgetText
    // generate no event at all - confirmed directly (a drained event poll
    // loop immediately after each call reports zero gadget events).
    // Fixed by blocking the signal handler around just that one
    // programmatic change (see pbSetGadgetState/pbSetGadgetText's own doc
    // comments).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(203, 10, 10, 200, 200, PBString("Test"));
    pbCheckBoxGadget(1, 10, 10, 150, 20, PBString("Check"));
    pbStringGadget(2, 10, 50, 150, 20, PBString("init"));
    drainEvents();

    pbSetGadgetState(1, 1);
    pbSetGadgetText(2, PBString("changed via code"));
    drainEvents();
    CHECK(pbWindowEvent() == 0);

    pbCloseWindow(203);
}

TEST_CASE("A real click (gtk_button_clicked) queues #PB_Event_Gadget with #PB_EventType_LeftClick",
          "[runtime][guilib]") {
    // gtk_button_clicked is a public GTK API that emits the exact same
    // "clicked" signal a real pointer click does - the gadget-event
    // equivalent of the window close-button test's own direct signal-
    // emission technique, for the same reason (no window manager in this
    // headless environment to mediate a real pointer event reliably).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(204, 10, 10, 200, 200, PBString("Test"));
    pbButtonGadget(1, 10, 10, 100, 30, PBString("Click me"));
    drainEvents();

    GtkWidget* button = detail::gadgetTable().at(1);
    gtk_button_clicked(GTK_BUTTON(button));

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventWindow() == 204);
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbCloseWindow(204);
}

TEST_CASE("A real toggle (gtk_toggle_button_set_active) queues #PB_Event_Gadget with "
          "#PB_EventType_LeftClick for a CheckBoxGadget",
          "[runtime][guilib]") {
    // Oracle-verified: a checkbox toggle reports LeftClick (0), not Change
    // (768), as its own EventType() - unlike StringGadget's own Change
    // event below. This test deliberately does NOT go through
    // pbSetGadgetState (which blocks the signal specifically to avoid this
    // event - see the spurious-event test above); it drives the same
    // gtk_toggle_button_set_active call a real click ultimately triggers,
    // unblocked, to verify the signal wiring itself still works.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(205, 10, 10, 200, 200, PBString("Test"));
    pbCheckBoxGadget(1, 10, 10, 150, 20, PBString("Check"));
    drainEvents();

    GtkWidget* checkbox = detail::gadgetTable().at(1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), TRUE);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbCloseWindow(205);
}

TEST_CASE("Real typing (gtk_entry_set_text via GtkEditable) queues #PB_Event_Gadget with "
          "#PB_EventType_Change for a StringGadget",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(206, 10, 10, 200, 200, PBString("Test"));
    pbStringGadget(1, 10, 10, 150, 20, PBString(""));
    drainEvents();

    // gtk_entry_set_text itself is what pbSetGadgetText uses too, but that
    // function deliberately blocks its own "changed" handler - calling the
    // raw GTK API directly here instead exercises the unblocked signal
    // path a real keystroke drives.
    GtkWidget* entry = detail::gadgetTable().at(1);
    gtk_entry_set_text(GTK_ENTRY(entry), "typed");

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 768); // #PB_EventType_Change

    pbCloseWindow(206);
}

TEST_CASE("A new gadget is placed into the most recently opened window, not an earlier one",
          "[runtime][guilib]") {
    // Oracle-verified: gadget-creation functions take no window parameter
    // at all - PB places a new gadget into whichever window was most
    // recently opened (see guilib.hpp's own activeWindowId() doc comment).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(207, 10, 10, 100, 100, PBString("First"));
    pbOpenWindow(208, 150, 10, 100, 100, PBString("Second"));
    pbButtonGadget(1, 10, 10, 50, 20, PBString("B"));
    drainEvents();

    GtkWidget* button = detail::gadgetTable().at(1);
    GtkWidget* secondWindow = detail::windowTable().at(208);
    CHECK(gtk_widget_get_toplevel(button) == secondWindow);

    pbCloseWindow(207);
    pbCloseWindow(208);
}

TEST_CASE("Closing a window frees its own gadgets too", "[runtime][guilib]") {
    // Oracle-verified: IsGadget on a gadget that belonged to a just-closed
    // window returns 0 (closing a window implicitly frees its own
    // gadgets).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(209, 10, 10, 100, 100, PBString("Test"));
    pbButtonGadget(1, 10, 10, 50, 20, PBString("B"));
    CHECK(pbIsGadget(1) == 1);

    pbCloseWindow(209);
    CHECK(pbIsGadget(1) == 0);
}
