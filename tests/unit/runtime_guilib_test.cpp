#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <easybasic/runtime/guilib.hpp>
#include <easybasic/runtime/mathlib.hpp> // pbRGB/pbRed/pbGreen/pbBlue - Requester round-trip tests
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
///
/// Deliberately unconditionally `false` on Haiku, real display or not -
/// this file's own ~90 `pbOpenWindow` calls across ~100 `TEST_CASE`s,
/// all run back-to-back in one process, reliably crash Haiku's own
/// GTK3-on-Wayland stack after a few hundred cumulative window create/
/// destroy cycles (confirmed directly: a minimal, ~20-line GTK3-only
/// reproducer using none of this project's own code crashes the same
/// way within ~300-400 cycles) - see the M7b roadmap's own "Haiku GUI
/// testing" notes for the full investigation (three distinct crash
/// signatures found, one of them *inside* `app_server` itself, wedging
/// the whole GUI session until a hard reboot) and
/// github.com/X547/wayland-server's own issue tracker (filed from a
/// fork at github.com/yann64/wayland-server/issues/1) for the upstream
/// report. Not a case of "no display available" at all (a real one
/// exists) - bypassed anyway, deliberately, until that's fixed upstream,
/// rather than risk wedging the one self-hosted Haiku CI runner this
/// project has. `SysTrayIcon`'s own much smaller test file (9 cases) is
/// left running on Haiku - nowhere near the churn needed to trigger this.
bool hasDisplay() {
#if defined(__HAIKU__)
    return false;
#else
    static bool checked = [] {
        int argc = 0;
        char** argv = nullptr;
        return gtk_init_check(&argc, &argv) == TRUE;
    }();
    return checked;
#endif
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

/// A minimal, valid 24-bit BMP (4x2 pixels - width chosen so each row is
/// already 4-byte-aligned, no padding bytes to get right) written byte by
/// byte, so `pbLoadImage` tests below have a real file to load without
/// this project's own unit tests depending on an external asset (nothing
/// else in this test file does either).
std::filesystem::path writeTinyBmp() {
    std::filesystem::path path = std::filesystem::temp_directory_path() / "easybasic_guilib_test.bmp";
    constexpr int width = 4;
    constexpr int height = 2;
    constexpr int rowBytes = width * 3; // Already a multiple of 4 - no padding needed.
    constexpr int pixelDataSize = rowBytes * height;
    constexpr int fileSize = 14 + 40 + pixelDataSize;
    std::vector<unsigned char> bytes(fileSize, 0);
    auto put16 = [&](std::size_t offset, std::uint16_t v) {
        bytes[offset] = static_cast<unsigned char>(v & 0xff);
        bytes[offset + 1] = static_cast<unsigned char>((v >> 8) & 0xff);
    };
    auto put32 = [&](std::size_t offset, std::uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            bytes[offset + i] = static_cast<unsigned char>((v >> (8 * i)) & 0xff);
        }
    };
    bytes[0] = 'B';
    bytes[1] = 'M';
    put32(2, static_cast<std::uint32_t>(fileSize));
    put32(10, 54); // Pixel data offset.
    put32(14, 40); // DIB header size (BITMAPINFOHEADER).
    put32(18, width);
    put32(22, height);
    put16(26, 1);  // Planes.
    put16(28, 24); // Bits per pixel.
    put32(34, static_cast<std::uint32_t>(pixelDataSize));
    for (int i = 0; i < pixelDataSize; ++i) {
        bytes[54 + i] = 0x80; // Flat gray - the exact color doesn't matter, only that it loads.
    }
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return path;
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

namespace {
/// `pbMessageRequester` blocks the calling thread inside `gtk_dialog_run`'s
/// own nested main loop until a button is clicked - there's no window-
/// manager-less click-simulation trick available here the way there was
/// for a gadget click (no on-screen coordinates are known ahead of time,
/// and the call doesn't return control until a response happens anyway).
/// Scheduled with `g_timeout_add` *before* calling `pbMessageRequester`,
/// this fires safely from the very same nested loop `gtk_dialog_run` pumps
/// (both service the same default `GMainContext`) and finds the dialog by
/// its own type rather than needing a widget reference threaded through.
gboolean autoRespond(gpointer userData) {
    auto response = static_cast<GtkResponseType>(reinterpret_cast<std::intptr_t>(userData));
    GList* windows = gtk_window_list_toplevels();
    for (GList* w = windows; w != nullptr; w = w->next) {
        auto* widget = static_cast<GtkWidget*>(w->data);
        if (GTK_IS_DIALOG(widget) != 0) {
            gtk_dialog_response(GTK_DIALOG(widget), response);
            break;
        }
    }
    g_list_free(windows);
    return G_SOURCE_REMOVE;
}

/// A more general version of `autoRespond` for the Requester family (M7b):
/// runs an arbitrary `setup` callback against the found dialog *before*
/// responding - e.g. setting a `GtkFileChooser`'s own filename, or typing
/// into an `InputRequester` dialog's own entry - rather than only ever
/// choosing which button to click.
gboolean dialogActionTrampoline(gpointer userData) {
    auto* action = static_cast<std::function<void(GtkDialog*)>*>(userData);
    GList* windows = gtk_window_list_toplevels();
    for (GList* w = windows; w != nullptr; w = w->next) {
        auto* widget = static_cast<GtkWidget*>(w->data);
        if (GTK_IS_DIALOG(widget) != 0) {
            (*action)(GTK_DIALOG(widget));
            break;
        }
    }
    g_list_free(windows);
    delete action;
    return G_SOURCE_REMOVE;
}

void scheduleDialogAction(std::function<void(GtkDialog*)> action) {
    g_timeout_add(50, dialogActionTrampoline, new std::function<void(GtkDialog*)>(std::move(action)));
}

/// Finds the first descendant of `widget` that's an instance of `type` -
/// used to reach `InputRequester`'s own `GtkEntry`, which isn't exposed
/// any other way (unlike a `GtkFileChooserDialog`, whose own filename/
/// folder is set directly on the dialog object itself via the
/// `GtkFileChooser` interface, needing no child lookup at all).
GtkWidget* findDescendantOfType(GtkWidget* widget, GType type) {
    if (G_TYPE_CHECK_INSTANCE_TYPE(widget, type) != 0) {
        return widget;
    }
    if (GTK_IS_CONTAINER(widget) == 0) {
        return nullptr;
    }
    GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
    GtkWidget* found = nullptr;
    for (GList* c = children; c != nullptr; c = c->next) {
        found = findDescendantOfType(GTK_WIDGET(c->data), type);
        if (found != nullptr) {
            break;
        }
    }
    g_list_free(children);
    return found;
}
} // namespace

TEST_CASE("MessageRequester's only button returns #PB_MessageRequester_Yes, not _Ok",
          "[runtime][guilib]") {
    // Oracle-verified, somewhat surprising finding: clicking the sole
    // button on a plain Ok-type dialog returns 6 (#PB_MessageRequester_Yes)
    // in real PB, not 0 (#PB_MessageRequester_Ok) - see
    // pbMessageRequester's own doc comment for why this is replicated
    // exactly rather than "cleaned up".
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_OK));
    CHECK(pbMessageRequester(PBString("Title"), PBString("Text")) == 6);
}

TEST_CASE("MessageRequester YesNo reports Yes/No correctly", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_YES));
    CHECK(pbMessageRequester(PBString("Title"), PBString("Text"), 1) == 6); // #PB_MessageRequester_Yes

    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_NO));
    CHECK(pbMessageRequester(PBString("Title"), PBString("Text"), 1) == 7); // #PB_MessageRequester_No
}

TEST_CASE("MessageRequester YesNoCancel reports Cancel correctly", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbMessageRequester(PBString("Title"), PBString("Text"), 2) == 2); // #PB_MessageRequester_Cancel
}

TEST_CASE("WindowID returns the real window handle, usable to re-find the window",
          "[runtime][guilib]") {
    // Oracle-verified: unlike every other GUI handle this project has
    // simplified to a clean 1/0, WindowID()'s own return value is genuinely
    // load-bearing - it's what CreateMenu/CreateStatusBar's own second
    // argument must be (see pbWindowID's own doc comment).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    CHECK(pbWindowID(300) == 0); // unknown window ID
    pbOpenWindow(300, 10, 10, 100, 100, PBString("Test"));
    std::int64_t handle = pbWindowID(300);
    CHECK(handle != 0);
    CHECK(reinterpret_cast<GtkWidget*>(handle) == detail::windowTable().at(300));
    pbCloseWindow(300);
}

TEST_CASE("CreateMenu/IsMenu/FreeMenu round-trip", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(301, 10, 10, 200, 100, PBString("Test"));
    CHECK(pbIsMenu(1) == 0);
    CHECK(pbCreateMenu(1, pbWindowID(301)) == 1);
    CHECK(pbIsMenu(1) == 1);
    pbFreeMenu(1);
    CHECK(pbIsMenu(1) == 0);
    pbCloseWindow(301);
}

TEST_CASE("Menu building (MenuTitle/MenuItem/MenuBar/OpenSubMenu/CloseSubMenu) + text/state accessors",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(302, 10, 10, 200, 100, PBString("Test"));
    pbCreateMenu(1, pbWindowID(302));
    pbMenuTitle(PBString("File"));
    pbMenuItem(10, PBString("Open"));
    pbMenuBar(); // separator
    pbOpenSubMenu(PBString("Recent"));
    pbMenuItem(11, PBString("A"));
    pbCloseSubMenu();
    pbMenuItem(12, PBString("Quit"));
    pbMenuTitle(PBString("Edit"));
    pbMenuItem(13, PBString("Cut"));

    CHECK(pbGetMenuItemText(1, 10).bytes() == "Open");
    pbSetMenuItemText(1, 10, PBString("Renamed"));
    CHECK(pbGetMenuItemText(1, 10).bytes() == "Renamed");

    CHECK(pbGetMenuTitleText(1, 0).bytes() == "File");
    CHECK(pbGetMenuTitleText(1, 1).bytes() == "Edit");
    pbSetMenuTitleText(1, 0, PBString("Fichier"));
    CHECK(pbGetMenuTitleText(1, 0).bytes() == "Fichier");

    // Oracle-verified default: a plain MenuItem starts unchecked.
    CHECK(pbGetMenuItemState(1, 10) == 0);
    pbSetMenuItemState(1, 10, 1);
    CHECK(pbGetMenuItemState(1, 10) == 1);
    pbSetMenuItemState(1, 10, 0);
    CHECK(pbGetMenuItemState(1, 10) == 0);

    CHECK(pbMenuHeight() > 0);
    CHECK(pbMenuID(1) == reinterpret_cast<std::int64_t>(detail::menuTable().at(1)));

    pbCloseWindow(302);
}

TEST_CASE("DisableMenuItem/HideMenu affect sensitivity/visibility, not tracked state",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(303, 10, 10, 200, 100, PBString("Test"));
    pbCreateMenu(1, pbWindowID(303));
    pbMenuTitle(PBString("File"));
    pbMenuItem(10, PBString("Open"));

    GtkWidget* item = detail::menuItemWidgets().at(1).at(10);
    pbDisableMenuItem(1, 10, 1);
    CHECK(gtk_widget_get_sensitive(item) == FALSE);
    pbDisableMenuItem(1, 10, 0);
    CHECK(gtk_widget_get_sensitive(item) == TRUE);

    GtkWidget* menuBar = detail::menuTable().at(1);
    pbHideMenu(1, 1);
    CHECK(gtk_widget_get_visible(menuBar) == FALSE);
    pbHideMenu(1, 0);
    CHECK(gtk_widget_get_visible(menuBar) == TRUE);

    pbCloseWindow(303);
}

TEST_CASE("A real menu item activation queues #PB_Event_Menu and leaves GetMenuItemState unchanged",
          "[runtime][guilib]") {
    // Oracle-verified (via xdotool click simulation against the real
    // pbcompilerc binary under Xvfb): clicking a plain MenuItem does *not*
    // change its own checked state - only an explicit SetMenuItemState()
    // call does. `gtk_menu_item_activate` here is the programmatic
    // equivalent of `gtk_button_clicked` in the gadget tests above - it
    // drives the exact same "activate" signal path a real click does.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(304, 10, 10, 200, 100, PBString("Test"));
    pbCreateMenu(1, pbWindowID(304));
    pbMenuTitle(PBString("File"));
    pbMenuItem(10, PBString("Open"));
    drainEvents();

    GtkWidget* item = detail::menuItemWidgets().at(1).at(10);
    gtk_menu_item_activate(GTK_MENU_ITEM(item));

    CHECK(pbWindowEvent() == 1); // #PB_Event_Menu
    CHECK(pbEventMenu() == 10);
    CHECK(pbEventWindow() == 304);
    CHECK(pbGetMenuItemState(1, 10) == 0);

    pbCloseWindow(304);
}

TEST_CASE("Closing a window frees its own menus and status bars too", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(305, 10, 10, 200, 100, PBString("Test"));
    pbCreateMenu(1, pbWindowID(305));
    pbCreateStatusBar(1, pbWindowID(305));
    CHECK(pbIsMenu(1) == 1);
    CHECK(pbIsStatusBar(1) == 1);

    pbCloseWindow(305);
    CHECK(pbIsMenu(1) == 0);
    CHECK(pbIsStatusBar(1) == 0);
}

TEST_CASE("CreateStatusBar/AddStatusBarField/StatusBarText round-trip", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(306, 10, 10, 300, 100, PBString("Test"));
    CHECK(pbIsStatusBar(1) == 0);
    CHECK(pbCreateStatusBar(1, pbWindowID(306)) == 1);
    CHECK(pbIsStatusBar(1) == 1);

    pbAddStatusBarField(100);
    pbAddStatusBarField(-65535); // #PB_Ignore
    CHECK(pbStatusBarText(1, 0, PBString("Area 1")) == 1);
    CHECK(pbStatusBarText(1, 1, PBString("Area 2"), 8) == 1); // #PB_StatusBar_Right
    CHECK(pbStatusBarText(1, 2, PBString("No such field")) == 0);

    GtkWidget* frame0 = detail::statusBarFieldFrames().at(1).at(0);
    GtkWidget* label0 = gtk_bin_get_child(GTK_BIN(frame0));
    CHECK(std::string(gtk_label_get_text(GTK_LABEL(label0))) == "Area 1");

    CHECK(pbStatusBarHeight(1) > 0);
    CHECK(pbStatusBarID(1) == reinterpret_cast<std::int64_t>(detail::statusBarTable().at(1)));

    pbFreeStatusBar(1);
    CHECK(pbIsStatusBar(1) == 0);

    pbCloseWindow(306);
}

// M7b's fifth GUI slice: the Image library. GdkPixbuf needs no GTK/display
// initialization at all (unlike every window/gadget/menu test above), so
// none of these are gated on hasDisplay().

TEST_CASE("CreateImage/IsImage/ImageWidth/ImageHeight/ImageID round-trip", "[runtime][guilib]") {
    CHECK(pbIsImage(400) == 0);
    CHECK(pbCreateImage(400, 64, 32) == 1);
    CHECK(pbIsImage(400) == 1);
    CHECK(pbImageWidth(400) == 64);
    CHECK(pbImageHeight(400) == 32);
    CHECK(pbImageID(400) != 0);
    CHECK(pbImageID(400) == reinterpret_cast<std::int64_t>(detail::imageTable().at(400)));
    pbFreeImage(400);
}

TEST_CASE("CreateImage rejects a non-positive width/height", "[runtime][guilib]") {
    CHECK(pbCreateImage(401, 0, 10) == 0);
    CHECK(pbCreateImage(401, 10, 0) == 0);
    CHECK(pbCreateImage(401, -5, 10) == 0);
    CHECK(pbIsImage(401) == 0);
}

TEST_CASE("LoadImage loads a real file; a bad path is a harmless failure", "[runtime][guilib]") {
    std::filesystem::path bmp = writeTinyBmp();
    CHECK(pbLoadImage(410, PBString(bmp.string())) == 1);
    CHECK(pbIsImage(410) == 1);
    CHECK(pbImageWidth(410) == 4);
    CHECK(pbImageHeight(410) == 2);

    CHECK(pbLoadImage(411, PBString("/nonexistent/path/does_not_exist.bmp")) == 0);
    CHECK(pbIsImage(411) == 0);

    pbFreeImage(410);
    std::remove(bmp.string().c_str());
}

TEST_CASE("IsImage/ImageWidth/ImageHeight/ImageID on an unknown id return a harmless default",
          "[runtime][guilib]") {
    CHECK(pbIsImage(999999) == 0);
    CHECK(pbImageWidth(999999) == 0);
    CHECK(pbImageHeight(999999) == 0);
    CHECK(pbImageID(999999) == 0);
}

TEST_CASE("FreeImage removes one image; re-creating the same id afterward works", "[runtime][guilib]") {
    pbCreateImage(420, 5, 5);
    CHECK(pbFreeImage(420) == 0); // Oracle-verified: no return value, always 0 - see pbFreeImage's own doc comment.
    CHECK(pbIsImage(420) == 0);
    CHECK(pbCreateImage(420, 9, 9) == 1);
    CHECK(pbImageWidth(420) == 9);
    pbFreeImage(420);
}

TEST_CASE("FreeImage(#PB_All) frees every remaining image at once", "[runtime][guilib]") {
    pbCreateImage(430, 1, 1);
    pbCreateImage(431, 1, 1);
    pbCreateImage(432, 1, 1);
    pbFreeImage(-1); // #PB_All
    CHECK(pbIsImage(430) == 0);
    CHECK(pbIsImage(431) == 0);
    CHECK(pbIsImage(432) == 0);
}

TEST_CASE("CreateImageMenu behaves like CreateMenu; MenuItem/OpenSubMenu accept an ImageID",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(307, 10, 10, 200, 100, PBString("Test"));
    CHECK(pbCreateImageMenu(1, pbWindowID(307)) == 1);
    CHECK(pbIsMenu(1) == 1);
    pbMenuTitle(PBString("File"));

    pbCreateImage(440, 16, 16);
    pbMenuItem(10, PBString("Open"), pbImageID(440));
    pbMenuItem(11, PBString("Close")); // No image - must keep working unchanged.
    pbOpenSubMenu(PBString("Recent"), pbImageID(440));
    pbMenuItem(12, PBString("A"));
    pbCloseSubMenu();

    // The real point of this test: GetMenuItemText/SetMenuItemText must
    // still find the right label even though attachMenuItemImage replaced
    // the item's direct child with an image+label box - see
    // detail::menuItemLabel's own doc comment.
    CHECK(pbGetMenuItemText(1, 10).bytes() == "Open");
    pbSetMenuItemText(1, 10, PBString("Renamed"));
    CHECK(pbGetMenuItemText(1, 10).bytes() == "Renamed");
    CHECK(pbGetMenuItemText(1, 11).bytes() == "Close"); // Unaffected, no-image item.

    pbFreeImage(440);
    pbCloseWindow(307);
}

// M7b's sixth GUI slice: ToolBar.

TEST_CASE("CreateToolBar/IsToolBar/FreeToolBar round-trip", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(308, 10, 10, 200, 100, PBString("Test"));
    CHECK(pbIsToolBar(1) == 0);
    CHECK(pbCreateToolBar(1, pbWindowID(308)) == 1);
    CHECK(pbIsToolBar(1) == 1);
    CHECK(pbFreeToolBar(1) == 0); // Oracle-verified: no return value - see pbFreeToolBar's own doc comment.
    CHECK(pbIsToolBar(1) == 0);
    pbCloseWindow(308);
}

TEST_CASE("ToolBarImageButton/ToolBarSeparator build a real toolbar; Get/SetToolBarButtonState round-trip",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(309, 10, 10, 200, 100, PBString("Test"));
    pbCreateToolBar(1, pbWindowID(309));
    pbCreateImage(450, 16, 16);

    CHECK(pbToolBarImageButton(10, pbImageID(450)) == 0); // Oracle-verified: no return value.
    CHECK(pbToolBarSeparator() == 0);                     // Oracle-verified: no return value.
    CHECK(pbToolBarImageButton(11, pbImageID(450), 1) == 0); // #PB_ToolBar_Toggle

    // Regression check for a real bug: pbToolBarImageButton originally
    // looked `imageId` up in imageTable() as if it were a plain #Image
    // number, but it's actually ImageID()'s own return value (the real
    // GdkPixbuf* pointer) - the lookup always missed, so no icon was ever
    // attached, silently, since nothing here checked for the icon widget
    // itself until now.
    GtkWidget* button10 = detail::toolBarButtonWidgets().at(1).at(10);
    CHECK(gtk_tool_button_get_icon_widget(GTK_TOOL_BUTTON(button10)) != nullptr);

    // Oracle-verified default: a fresh Toggle button starts released.
    CHECK(pbGetToolBarButtonState(1, 11) == 0);
    pbSetToolBarButtonState(1, 11, 1);
    CHECK(pbGetToolBarButtonState(1, 11) == 1);
    pbSetToolBarButtonState(1, 11, 0);
    CHECK(pbGetToolBarButtonState(1, 11) == 0);

    // A plain (non-Toggle) button has no checked state at all.
    CHECK(pbGetToolBarButtonState(1, 10) == 0);

    CHECK(pbToolBarHeight(1) > 0);
    CHECK(pbToolBarID(1) == reinterpret_cast<std::int64_t>(detail::toolBarTable().at(1)));

    pbFreeImage(450);
    pbCloseWindow(309);
}

TEST_CASE("DisableToolBarButton/ToolBarButtonText/ToolBarToolTip affect the real widget",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(310, 10, 10, 200, 100, PBString("Test"));
    pbCreateToolBar(1, pbWindowID(310), 4); // #PB_ToolBar_Text
    pbCreateImage(451, 16, 16);
    pbToolBarImageButton(10, pbImageID(451), 0, PBString("Open"));

    GtkWidget* button = detail::toolBarButtonWidgets().at(1).at(10);
    CHECK(gtk_widget_get_sensitive(button) == TRUE);
    pbDisableToolBarButton(1, 10, 1);
    CHECK(gtk_widget_get_sensitive(button) == FALSE);
    pbDisableToolBarButton(1, 10, 0);
    CHECK(gtk_widget_get_sensitive(button) == TRUE);

    CHECK(std::string(gtk_tool_button_get_label(GTK_TOOL_BUTTON(button))) == "Open");
    pbToolBarButtonText(1, 10, PBString("Renamed"));
    CHECK(std::string(gtk_tool_button_get_label(GTK_TOOL_BUTTON(button))) == "Renamed");

    CHECK(gtk_widget_get_tooltip_text(button) == nullptr);
    pbToolBarToolTip(1, 10, PBString("A tip"));
    // gtk_widget_get_tooltip_text returns a caller-owned, newly allocated
    // string (per its own GTK docs) - g_free it rather than leaking it.
    gchar* tooltip = gtk_widget_get_tooltip_text(button);
    CHECK(std::string(tooltip) == "A tip");
    g_free(tooltip);
    pbToolBarToolTip(1, 10, PBString("")); // Oracle-verified: an empty string removes it.
    CHECK(gtk_widget_get_tooltip_text(button) == nullptr);

    pbFreeImage(451);
    pbCloseWindow(310);
}

TEST_CASE("A real toolbar button click queues #PB_Event_Menu, the same route a real MenuItem uses",
          "[runtime][guilib]") {
    // Oracle-verified (via CreateToolBar's own docs): toolbar button clicks
    // are detected the same way menu events are, via EventMenu() - not
    // #PB_Event_Gadget. `g_signal_emit_by_name(..., "clicked")` here is the
    // toolbar-button equivalent of `gtk_menu_item_activate` in the menu
    // test above - GtkToolButton has no dedicated "activate" function of
    // its own to call instead.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(311, 10, 10, 200, 100, PBString("Test"));
    pbCreateToolBar(1, pbWindowID(311));
    pbCreateImage(452, 16, 16);
    pbToolBarImageButton(20, pbImageID(452));
    drainEvents();

    GtkWidget* button = detail::toolBarButtonWidgets().at(1).at(20);
    g_signal_emit_by_name(button, "clicked");

    CHECK(pbWindowEvent() == 1); // #PB_Event_Menu
    CHECK(pbEventMenu() == 20);
    CHECK(pbEventWindow() == 311);

    pbFreeImage(452);
    pbCloseWindow(311);
}

TEST_CASE("CreateToolBar reorders below an already-existing menu, above one created afterward",
          "[runtime][guilib]") {
    // Oracle-verified (CreateToolBar's own example and real-world usage):
    // a toolbar always renders below the menu bar, regardless of which was
    // created first - see pbCreateToolBar's own doc comment on why this
    // needs its own explicit check, unlike pbCreateMenu's unconditional
    // "always force myself to 0".
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(312, 10, 10, 200, 100, PBString("Test"));
    pbCreateMenu(1, pbWindowID(312));
    pbCreateToolBar(1, pbWindowID(312));

    GtkWidget* window = detail::windowTable().at(312);
    GtkWidget* vbox = gtk_bin_get_child(GTK_BIN(window));
    GtkWidget* menuWidget = detail::menuTable().at(1);
    GtkWidget* toolbarWidget = detail::toolBarTable().at(1);
    GList* children = gtk_container_get_children(GTK_CONTAINER(vbox));
    int menuPos = g_list_index(children, menuWidget);
    int toolbarPos = g_list_index(children, toolbarWidget);
    g_list_free(children);
    CHECK(menuPos >= 0);
    CHECK(toolbarPos >= 0);
    CHECK(menuPos < toolbarPos);

    pbFreeMenu(1);
    pbFreeToolBar(1);
    pbCloseWindow(312);
}

// The Requester family's remaining pieces (M7b) - MessageRequester (the
// third GUI slice) already established the "drive the real dialog via
// autoRespond/scheduleDialogAction, no xdotool" split this project's own
// GUI testing uses throughout; every test below follows it.

TEST_CASE("ColorRequester returns -1 on cancel, and round-trips an initial color on accept",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbColorRequester() == -1);

    std::int64_t initial = pbRGB(10, 20, 30);
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_OK));
    std::int64_t result = pbColorRequester(initial);
    CHECK(pbRed(result) == 10);
    CHECK(pbGreen(result) == 20);
    CHECK(pbBlue(result) == 30);
}

TEST_CASE("FontRequester returns 0 on cancel, and round-trips name/size/style/color on accept",
          "[runtime][guilib]") {
    // "Sans" is a Pango alias guaranteed to resolve on any system (unlike
    // a concrete family name such as "Arial", which may not be installed
    // in this test environment) - chosen specifically so this assertion
    // doesn't depend on the test machine's own installed fonts.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbFontRequester(PBString("Sans"), 12, 0) == 0);

    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_OK));
    std::int64_t ok = pbFontRequester(PBString("Sans"), 14, 0, pbRGB(1, 2, 3), 1 /* #PB_Font_Bold */);
    CHECK(ok != 0);
    CHECK(pbSelectedFontName().bytes() == "Sans");
    CHECK(pbSelectedFontSize() == 14);
    CHECK((pbSelectedFontStyle() & 1) != 0); // #PB_Font_Bold
    CHECK(pbSelectedFontColor() == pbRGB(1, 2, 3)); // Echoed back unchanged - see pbFontRequester's own doc comment.
}

TEST_CASE("InputRequester returns an empty string on cancel by default", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbInputRequester(PBString("T"), PBString("M"), PBString("D")).bytes().empty());
}

TEST_CASE("InputRequester returns #PB_InputRequester_Cancel on cancel when HandleCancel is set",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    PBString result = pbInputRequester(PBString("T"), PBString("M"), PBString("D"), 2); // #PB_InputRequester_HandleCancel
    CHECK(result.bytes() == "\n\t"); // See pbInputRequester's own doc comment on this not being a referenceable constant yet.
}

TEST_CASE("InputRequester returns the entry's own (possibly edited) text on accept",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    scheduleDialogAction([](GtkDialog* dialog) {
        GtkWidget* entry = findDescendantOfType(GTK_WIDGET(dialog), GTK_TYPE_ENTRY);
        REQUIRE(entry != nullptr);
        gtk_entry_set_text(GTK_ENTRY(entry), "typed value");
        gtk_dialog_response(dialog, GTK_RESPONSE_OK);
    });
    CHECK(pbInputRequester(PBString("T"), PBString("M"), PBString("Default")).bytes() == "typed value");
}

TEST_CASE("InputRequester's Password option hides the entry's own text", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    scheduleDialogAction([](GtkDialog* dialog) {
        GtkWidget* entry = findDescendantOfType(GTK_WIDGET(dialog), GTK_TYPE_ENTRY);
        REQUIRE(entry != nullptr);
        CHECK(gtk_entry_get_visibility(GTK_ENTRY(entry)) == FALSE);
        gtk_dialog_response(dialog, GTK_RESPONSE_CANCEL);
    });
    pbInputRequester(PBString("T"), PBString("M"), PBString("D"), 1); // #PB_InputRequester_Password
}

TEST_CASE("OpenFileRequester returns an empty string and SelectedFilePattern -1 on cancel",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbOpenFileRequester(PBString("T"), PBString(""), PBString("Text|*.txt"), 0).bytes().empty());
    CHECK(pbSelectedFilePattern() == -1);
}

TEST_CASE("OpenFileRequester/SaveFileRequester's own filter-pattern parsing splits labels/globs correctly",
          "[runtime][guilib]") {
    // Doesn't drive a real dialog at all - accepting (not cancelling) a
    // real GtkFileChooserDialog opened with ACTION_OPEN/ACTION_SAVE hangs
    // indefinitely in this specific sandboxed desktop environment
    // (confirmed, in isolation, outside this test suite entirely: GTK3's
    // own file-chooser machinery hands the "Open"/"Save" confirmation off
    // to xdg-desktop-portal-gtk - confirmed running - whose own file-
    // picker UI doesn't render anywhere this test can find or drive, so
    // the simulated response this project's every other dialog test
    // relies on never reaches anything real; ACTION_SELECT_FOLDER, used
    // by PathRequester below, isn't affected the same way, confirmed the
    // same isolated way). Real, interactive use is unaffected - this is
    // a test-environment limitation, not a bug in pbOpenFileRequester/
    // pbSaveFileRequester themselves - so filter-pattern parsing (the one
    // piece of real, nontrivial logic in either function, oracle-verified
    // via OpenFileRequester's own docs) is still verified directly here,
    // and the cancel path (which doesn't hit this at all) is covered by
    // the test above/below.
    auto groups = detail::parseFilterPattern("Text (*.txt)|*.txt;*.bat|PureBasic (*.pb)|*.pb|All (*.*)|*.*");
    REQUIRE(groups.size() == 3);
    CHECK(groups[0].first == "Text (*.txt)");
    REQUIRE(groups[0].second.size() == 2);
    CHECK(groups[0].second[0] == "*.txt");
    CHECK(groups[0].second[1] == "*.bat");
    CHECK(groups[1].first == "PureBasic (*.pb)");
    REQUIRE(groups[1].second.size() == 1);
    CHECK(groups[1].second[0] == "*.pb");
    CHECK(groups[2].first == "All (*.*)");
    REQUIRE(groups[2].second.size() == 1);
    CHECK(groups[2].second[0] == "*.*");
}

TEST_CASE("SaveFileRequester returns an empty string on cancel", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbSaveFileRequester(PBString("T"), PBString(""), PBString("Text|*.txt"), 0).bytes().empty());
}

TEST_CASE("PathRequester returns an empty string on cancel, and the chosen path (with a trailing "
          "slash) on accept",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    g_timeout_add(50, autoRespond, reinterpret_cast<gpointer>(GTK_RESPONSE_CANCEL));
    CHECK(pbPathRequester(PBString("T"), PBString("")).bytes().empty());

    scheduleDialogAction(
        [](GtkDialog* dialog) { gtk_dialog_response(dialog, GTK_RESPONSE_OK); });
    PBString result = pbPathRequester(PBString("T"), PBString("/tmp"));
    REQUIRE_FALSE(result.bytes().empty()); // guards the .back() below from undefined behavior
    CHECK(result.bytes().back() == '/');
}

TEST_CASE("ContainerGadget automatically captures subsequently created gadgets until CloseGadgetList",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(400, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbContainerGadget(1, 10, 10, 200, 200, 0) == 1);
    CHECK(pbIsGadget(1) == 1);
    CHECK(pbButtonGadget(2, 5, 5, 60, 20, PBString("Inside")) == 1);
    CHECK(pbCloseGadgetList() == 1);
    CHECK(pbButtonGadget(3, 5, 220, 60, 20, PBString("Outside")) == 1);

    // Oracle-verified: gadget coordinates inside a container are relative
    // to the container's own top-left, not the window's - a button
    // placed at (0,0) inside a container at (50,50) reports GadgetX()=0,
    // not 50 (this project has no GadgetX of its own yet, so this checks
    // the same fact via the real GtkFixed child allocation instead).
    GtkWidget* innerButton = detail::gadgetTable().at(2);
    GtkWidget* containerFixed = detail::containerFixedTable().at(1);
    CHECK(gtk_widget_get_parent(innerButton) == containerFixed);
    GtkWidget* outerButton = detail::gadgetTable().at(3);
    CHECK(gtk_widget_get_parent(outerButton) == detail::windowFixedTable().at(400));

    pbCloseWindow(400);
}

TEST_CASE("Freeing a container recursively frees every gadget nested inside it, at any depth",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(401, 10, 10, 300, 300, PBString("Test"));

    pbContainerGadget(10, 10, 10, 250, 250, 0);
    pbContainerGadget(11, 20, 20, 150, 150, 0);
    pbButtonGadget(12, 5, 5, 60, 20, PBString("Deep"));
    pbCloseGadgetList(); // closes 11
    pbButtonGadget(13, 5, 180, 60, 20, PBString("OuterLevel"));
    pbCloseGadgetList(); // closes 10
    pbButtonGadget(14, 5, 260, 60, 20, PBString("WindowLevel"));

    for (std::int64_t id : {10, 11, 12, 13, 14}) {
        CHECK(pbIsGadget(id) == 1);
    }

    CHECK(pbFreeGadget(10) == 1);
    CHECK(pbIsGadget(10) == 0);
    CHECK(pbIsGadget(11) == 0); // direct child
    CHECK(pbIsGadget(12) == 0); // nested two levels deep
    CHECK(pbIsGadget(13) == 0); // direct child
    CHECK(pbIsGadget(14) == 1); // window-level, unaffected - oracle-verified

    pbCloseWindow(401);
}

TEST_CASE("OpenGadgetList reopens a closed container so more gadgets can be added to it dynamically",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(402, 10, 10, 300, 300, PBString("Test"));

    pbContainerGadget(20, 10, 10, 200, 200, 0);
    pbCloseGadgetList();

    CHECK(pbOpenGadgetList(20) == 1);
    CHECK(pbButtonGadget(21, 5, 5, 60, 20, PBString("Reopened")) == 1);
    CHECK(pbCloseGadgetList() == 1);

    GtkWidget* reopenedButton = detail::gadgetTable().at(21);
    GtkWidget* containerFixed = detail::containerFixedTable().at(20);
    CHECK(gtk_widget_get_parent(reopenedButton) == containerFixed);

    // Oracle-verified: a gadget added via a later OpenGadgetList is still
    // a real child of that container, for FreeGadget's own purposes too.
    pbFreeGadget(20);
    CHECK(pbIsGadget(21) == 0);

    // A non-container (or already-freed) #Gadget is a harmless failure.
    CHECK(pbOpenGadgetList(20) == 0);
    CHECK(pbOpenGadgetList(9999) == 0);

    pbCloseWindow(402);
}

TEST_CASE("CloseGadgetList with nothing open is a harmless no-op", "[runtime][guilib]") {
    // Oracle-verified: real PB's own debugger raises a fatal error here -
    // this project deliberately simplifies to a silent no-op instead (see
    // pbCloseGadgetList's own doc comment for why), so this only checks
    // that it doesn't crash and leaves window-level gadget creation
    // working normally afterward, not that it matches the fatal text.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(403, 10, 10, 200, 200, PBString("Test"));
    CHECK(pbCloseGadgetList() == 0);
    CHECK(pbButtonGadget(1, 10, 10, 60, 20, PBString("Still works")) == 1);
    CHECK(pbIsGadget(1) == 1);
    pbCloseWindow(403);
}

TEST_CASE("PanelGadget starts with no tabs, and AddGadgetItem captures subsequently created gadgets",
          "[runtime][guilib]") {
    // Oracle-verified (PanelGadget.html's own "Remarques"): unlike
    // ContainerGadget, a freshly created Panel does *not* become the
    // current gadget-list target by itself - CountGadgetItems is 0, and
    // only AddGadgetItem (creating the first real tab) does.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(500, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbPanelGadget(1, 10, 10, 250, 250) == 1);
    CHECK(pbCountGadgetItems(1) == 0);

    CHECK(pbAddGadgetItem(1, -1, PBString("Tab1")) == 1);
    CHECK(pbCountGadgetItems(1) == 1);
    CHECK(pbButtonGadget(2, 5, 5, 60, 20, PBString("InTab1")) == 1);

    GtkWidget* button = detail::gadgetTable().at(2);
    auto* notebook = GTK_NOTEBOOK(detail::gadgetTable().at(1));
    CHECK(gtk_widget_get_parent(button) == gtk_notebook_get_nth_page(notebook, 0));

    pbCloseGadgetList();
    pbCloseWindow(500);
}

TEST_CASE("A second AddGadgetItem on the same Panel replaces, not pushes - one CloseGadgetList "
          "fully exits it",
          "[runtime][guilib]") {
    // Oracle-verified via the real PanelGadget.html example's own exact
    // sequence: two AddGadgetItem calls on the same Panel with no
    // CloseGadgetList between them, then exactly one CloseGadgetList -
    // confirmed to return all the way to window level (a second,
    // immediately following CloseGadgetList correctly hits the real
    // "nothing open" case, oracle-verified separately).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(501, 10, 10, 300, 300, PBString("Test"));

    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbButtonGadget(2, 5, 5, 60, 20, PBString("InTab1"));
    pbAddGadgetItem(1, -1, PBString("Tab2")); // same Panel, no CloseGadgetList yet
    pbButtonGadget(3, 5, 5, 60, 20, PBString("InTab2"));
    CHECK(pbCountGadgetItems(1) == 2);

    CHECK(pbCloseGadgetList() == 1); // should fully exit the Panel
    CHECK(pbButtonGadget(4, 5, 260, 60, 20, PBString("WindowLevel")) == 1);
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(4)) == detail::windowFixedTable().at(501));
    CHECK(pbCloseGadgetList() == 0); // nothing left open - a harmless no-op here

    pbCloseWindow(501);
}

TEST_CASE("A Panel nested inside another Panel's own tab pushes a genuinely new frame",
          "[runtime][guilib]") {
    // Replicates the real PanelGadget.html example's own "Sous-onglet"
    // (sub-tab) structure: a second, different Panel inside the first
    // one's own tab still needs its own CloseGadgetList, distinct from
    // the replace-not-push behavior above (which only applies to the
    // *same* Panel).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(502, 10, 10, 300, 300, PBString("Test"));

    pbPanelGadget(1, 8, 8, 280, 280);
    pbAddGadgetItem(1, -1, PBString("Outer"));
    pbPanelGadget(2, 5, 5, 260, 200);
    pbAddGadgetItem(2, -1, PBString("Inner1"));
    pbButtonGadget(3, 5, 5, 60, 20, PBString("Deep"));
    CHECK(pbCloseGadgetList() == 1); // closes inner Panel 2
    pbButtonGadget(4, 5, 210, 60, 20, PBString("OuterLevel"));
    CHECK(pbCloseGadgetList() == 1); // closes outer Panel 1
    pbButtonGadget(5, 5, 290, 60, 20, PBString("WindowLevel"));

    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(3)) ==
          gtk_notebook_get_nth_page(GTK_NOTEBOOK(detail::gadgetTable().at(2)), 0));
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(4)) ==
          gtk_notebook_get_nth_page(GTK_NOTEBOOK(detail::gadgetTable().at(1)), 0));
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(5)) == detail::windowFixedTable().at(502));

    pbCloseWindow(502);
}

TEST_CASE("GetGadgetState/SetGadgetState report and select a Panel's own active tab, without "
          "firing a spurious Change event",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(503, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbCloseGadgetList();

    CHECK(pbGetGadgetState(1) == 0); // the first tab stays active - adding more doesn't steal it

    pbSetGadgetState(1, 1);
    CHECK(pbGetGadgetState(1) == 1);

    drainEvents();
    CHECK(pbWindowEvent() == 0); // SetGadgetState queues nothing

    pbCloseWindow(503);
}

TEST_CASE("A real tab switch (not SetGadgetState) queues #PB_Event_Gadget with #PB_EventType_Change",
          "[runtime][guilib]") {
    // gtk_notebook_set_current_page, called directly (bypassing
    // pbSetGadgetState's own signal-blocking), is the GtkNotebook
    // equivalent of gtk_button_clicked in the plain-gadget tests - it
    // fires the exact same "switch-page" signal a real tab click does.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(504, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbCloseGadgetList();
    drainEvents();

    gtk_notebook_set_current_page(GTK_NOTEBOOK(detail::gadgetTable().at(1)), 1);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 768); // #PB_EventType_Change

    pbCloseWindow(504);
}

TEST_CASE("CountGadgetItems/GetGadgetItemText/SetGadgetItemText round-trip a Panel's own tabs",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(505, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbCloseGadgetList();

    CHECK(pbCountGadgetItems(1) == 2);
    CHECK(pbGetGadgetItemText(1, 0).bytes() == "Tab1");
    CHECK(pbGetGadgetItemText(1, 1).bytes() == "Tab2");
    CHECK(pbGetGadgetItemText(1, 99).bytes().empty()); // out of range - harmless failure

    CHECK(pbSetGadgetItemText(1, 1, PBString("Renamed")) == 1);
    CHECK(pbGetGadgetItemText(1, 1).bytes() == "Renamed");
    CHECK(pbSetGadgetItemText(1, 99, PBString("Nope")) == 0); // out of range

    pbCloseWindow(505);
}

TEST_CASE("RemoveGadgetItem frees every gadget nested in that tab and re-indexes the rest",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(506, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbButtonGadget(10, 5, 5, 60, 20, PBString("A"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbButtonGadget(11, 5, 5, 60, 20, PBString("B"));
    pbCloseGadgetList();

    CHECK(pbRemoveGadgetItem(1, 0) == 1);
    CHECK(pbCountGadgetItems(1) == 1);
    CHECK(pbIsGadget(10) == 0); // freed along with its own tab
    CHECK(pbIsGadget(11) == 1); // the surviving tab's own gadget, unaffected
    CHECK(pbGetGadgetItemText(1, 0).bytes() == "Tab2"); // shifted down to index 0

    CHECK(pbRemoveGadgetItem(1, 99) == 0); // out of range - harmless failure

    pbCloseWindow(506);
}

TEST_CASE("ClearGadgetItems frees every gadget nested in every tab", "[runtime][guilib]") {
    // Oracle-verified in isolation (no prior RemoveGadgetItem call on the
    // same Panel): ClearGadgetItems does recursively free every nested
    // gadget. A real, reproducible, and narrower oracle quirk - the exact
    // same sequence *after* an earlier RemoveGadgetItem call on the same
    // Panel instead leaves the survivor's own nested gadget alive
    // (IsGadget still true) despite CountGadgetItems correctly reporting
    // 0 - is deliberately *not* replicated here: this project always
    // frees recursively, the more internally consistent behavior (and
    // the same one RemoveGadgetItem itself always has), not that one
    // narrow, anomalous combination.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(507, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbButtonGadget(10, 5, 5, 60, 20, PBString("A"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbButtonGadget(11, 5, 5, 60, 20, PBString("B"));
    pbCloseGadgetList();

    CHECK(pbClearGadgetItems(1) == 1);
    CHECK(pbCountGadgetItems(1) == 0);
    CHECK(pbIsGadget(10) == 0);
    CHECK(pbIsGadget(11) == 0);

    pbCloseWindow(507);
}

TEST_CASE("OpenGadgetList reopens an existing Panel tab by its own Element index",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(508, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbCloseGadgetList();

    CHECK(pbOpenGadgetList(1, 0) == 1); // reopen tab 0, not the last-created tab 1
    CHECK(pbButtonGadget(10, 5, 5, 60, 20, PBString("AddedToTab0")) == 1);
    CHECK(pbCloseGadgetList() == 1);

    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(10)) ==
          gtk_notebook_get_nth_page(GTK_NOTEBOOK(detail::gadgetTable().at(1)), 0));
    CHECK(pbCountGadgetItems(1) == 2); // reopening doesn't create a new tab

    // An out-of-range Element, or a #Gadget that isn't a live Panel/
    // Container, is a harmless failure.
    CHECK(pbOpenGadgetList(1, 99) == 0);
    CHECK(pbOpenGadgetList(9999) == 0);

    pbCloseWindow(508);
}

TEST_CASE("AddGadgetItem's own ImageID attaches an icon beside the tab's own label, without "
          "breaking GetGadgetItemText/SetGadgetItemText",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(509, 10, 10, 300, 300, PBString("Test"));
    pbCreateImage(1, 16, 16);
    std::int64_t imageId = pbImageID(1);

    pbPanelGadget(2, 10, 10, 250, 250);
    CHECK(pbAddGadgetItem(2, -1, PBString("WithIcon"), imageId) == 1);
    pbCloseGadgetList();

    CHECK(pbGetGadgetItemText(2, 0).bytes() == "WithIcon");
    CHECK(pbSetGadgetItemText(2, 0, PBString("Renamed")) == 1);
    CHECK(pbGetGadgetItemText(2, 0).bytes() == "Renamed");

    auto* notebook = GTK_NOTEBOOK(detail::gadgetTable().at(2));
    GtkWidget* tabLabel = gtk_notebook_get_tab_label(notebook, gtk_notebook_get_nth_page(notebook, 0));
    REQUIRE(tabLabel != nullptr);
    CHECK(GTK_IS_BOX(tabLabel) != 0); // image + label, not a bare GtkLabel

    pbFreeImage(1);
    pbCloseWindow(509);
}

TEST_CASE("Freeing a Panel recursively frees every gadget nested in every tab, the same as a "
          "Container",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(510, 10, 10, 300, 300, PBString("Test"));
    pbPanelGadget(1, 10, 10, 250, 250);
    pbAddGadgetItem(1, -1, PBString("Tab1"));
    pbButtonGadget(10, 5, 5, 60, 20, PBString("A"));
    pbAddGadgetItem(1, -1, PBString("Tab2"));
    pbButtonGadget(11, 5, 5, 60, 20, PBString("B"));
    pbCloseGadgetList();

    CHECK(pbFreeGadget(1) == 1);
    CHECK(pbIsGadget(1) == 0);
    CHECK(pbIsGadget(10) == 0);
    CHECK(pbIsGadget(11) == 0);

    pbCloseWindow(510);
}

TEST_CASE("SplitterGadget's own default divider position is half of whichever dimension its "
          "orientation splits",
          "[runtime][guilib]") {
    // Oracle-verified: #PB_Splitter_Vertical means the divider *bar*
    // itself is vertical (panes side by side, split by width) - the
    // opposite of what the flag's own name might suggest at first
    // glance, confirmed directly rather than assumed (a default, no-flag
    // Splitter's own position is half its own *height* instead).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(600, 10, 10, 400, 400, PBString("Test"));

    pbButtonGadget(1, 0, 0, 0, 0, PBString("B1"));
    pbButtonGadget(2, 0, 0, 0, 0, PBString("B2"));
    CHECK(pbSplitterGadget(10, 10, 10, 300, 200, 1, 2, 1 /* #PB_Splitter_Vertical */) == 1);
    CHECK(pbGetGadgetState(10) == 150); // half of width

    pbButtonGadget(3, 0, 0, 0, 0, PBString("B3"));
    pbButtonGadget(4, 0, 0, 0, 0, PBString("B4"));
    CHECK(pbSplitterGadget(11, 10, 220, 300, 200, 3, 4, 0) == 1);
    CHECK(pbGetGadgetState(11) == 100); // half of height, no Vertical flag

    GtkWidget* paned = detail::gadgetTable().at(10);
    CHECK(gtk_orientable_get_orientation(GTK_ORIENTABLE(paned)) == GTK_ORIENTATION_HORIZONTAL);
    GtkWidget* panedDefault = detail::gadgetTable().at(11);
    CHECK(gtk_orientable_get_orientation(GTK_ORIENTABLE(panedDefault)) == GTK_ORIENTATION_VERTICAL);

    pbCloseWindow(600);
}

TEST_CASE("SplitterGadget reparents its own two already-existing child gadgets, which stay "
          "placed into its own window-level Fixed beforehand",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(601, 10, 10, 400, 400, PBString("Test"));

    pbButtonGadget(1, 0, 0, 0, 0, PBString("B1"));
    pbButtonGadget(2, 0, 0, 0, 0, PBString("B2"));
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(1)) == detail::windowFixedTable().at(601));

    CHECK(pbSplitterGadget(10, 10, 10, 300, 200, 1, 2) == 1);

    auto* paned = GTK_PANED(detail::gadgetTable().at(10));
    CHECK(gtk_paned_get_child1(paned) == detail::gadgetTable().at(1));
    CHECK(gtk_paned_get_child2(paned) == detail::gadgetTable().at(2));
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(1)) == GTK_WIDGET(paned));

    // A #Gadget1/#Gadget2 that doesn't exist is a harmless failure, not
    // a crash.
    CHECK(pbSplitterGadget(11, 10, 220, 300, 200, 1, 9999) == 0);

    pbCloseWindow(601);
}

TEST_CASE("GetGadgetState/SetGadgetState round-trip a Splitter's own divider position in pixels",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(602, 10, 10, 400, 400, PBString("Test"));
    pbButtonGadget(1, 0, 0, 0, 0, PBString("B1"));
    pbButtonGadget(2, 0, 0, 0, 0, PBString("B2"));
    pbSplitterGadget(10, 10, 10, 300, 200, 1, 2);

    CHECK(pbSetGadgetState(10, 77) == 1);
    CHECK(pbGetGadgetState(10) == 77);

    pbCloseWindow(602);
}

TEST_CASE("FirstFixed/SecondFixed keep that pane's own size unchanged when the Splitter itself "
          "is resized",
          "[runtime][guilib]") {
    // Oracle-verified directly: a FirstFixed Splitter's own divider
    // position (pixels from the start) stays unchanged after widening it
    // with ResizeGadget - the first pane keeps its own size, the second
    // absorbs the difference.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(603, 10, 10, 500, 500, PBString("Test"));
    pbButtonGadget(1, 0, 0, 0, 0, PBString("B1"));
    pbButtonGadget(2, 0, 0, 0, 0, PBString("B2"));
    pbSplitterGadget(10, 10, 10, 300, 200, 1, 2, 1 | 4 /* Vertical | FirstFixed */);

    CHECK(pbGetGadgetState(10) == 150);
    pbResizeGadget(10, 10, 10, 400, 200);
    CHECK(pbGetGadgetState(10) == 150); // unchanged - gadget1 stayed fixed

    pbCloseWindow(603);
}

TEST_CASE("GetGadgetAttribute/SetGadgetAttribute report and configure a Splitter's own two panes",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(604, 10, 10, 400, 400, PBString("Test"));
    pbButtonGadget(1, 0, 0, 0, 0, PBString("B1"));
    pbButtonGadget(2, 0, 0, 0, 0, PBString("B2"));
    pbSplitterGadget(10, 10, 10, 300, 200, 1, 2);

    CHECK(pbGetGadgetAttribute(10, 3 /* #PB_Splitter_FirstGadget */) == 1);
    CHECK(pbGetGadgetAttribute(10, 4 /* #PB_Splitter_SecondGadget */) == 2);

    CHECK(pbSetGadgetAttribute(10, 1 /* #PB_Splitter_FirstMinimumSize */, 42) == 1);
    CHECK(pbGetGadgetAttribute(10, 1) == 42);
    CHECK(pbSetGadgetAttribute(10, 2 /* #PB_Splitter_SecondMinimumSize */, 24) == 1);
    CHECK(pbGetGadgetAttribute(10, 2) == 24);

    // No #PB_Splitter_Vertical flag here, so the default orientation
    // splits by height - FirstMinimumSize applies to the child's own
    // minimum *height*, not width.
    int unusedW = -1;
    int minH = -1;
    gtk_widget_get_size_request(detail::gadgetTable().at(1), &unusedW, &minH);
    CHECK(minH == 42);

    // Oracle-verified unsupported-attribute and non-Splitter/unknown-
    // gadget cases are all a harmless 0, not a crash (a genuinely
    // *unknown* #Gadget is a real, fatal debugger error in real PB
    // itself - this project's own established "not worth modeling every
    // misuse precisely" stance, see placeGadget's own doc comment).
    CHECK(pbGetGadgetAttribute(10, 99) == 0);
    CHECK(pbGetGadgetAttribute(1, 3) == 0); // not a Splitter at all
    CHECK(pbGetGadgetAttribute(9999, 3) == 0); // unknown gadget
    CHECK(pbSetGadgetAttribute(10, 99, 1) == 0);
    CHECK(pbSetGadgetAttribute(1, 1, 1) == 0);

    pbCloseWindow(604);
}

TEST_CASE("SetGadgetAttribute's FirstGadget/SecondGadget swap a pane's own child without "
          "destroying the old one - it's reparented back onto the window instead",
          "[runtime][guilib]") {
    // Oracle-verified directly: replacing a pane's own gadget doesn't
    // free it - IsGadget stays true, and it's put back onto the window
    // that contains the Splitter.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(605, 10, 10, 400, 400, PBString("Test"));
    pbButtonGadget(1, 0, 0, 0, 0, PBString("B1"));
    pbButtonGadget(2, 0, 0, 0, 0, PBString("B2"));
    pbSplitterGadget(10, 10, 10, 300, 200, 1, 2);

    pbButtonGadget(3, 0, 0, 0, 0, PBString("B3"));
    CHECK(pbSetGadgetAttribute(10, 3 /* #PB_Splitter_FirstGadget */, 3) == 1);
    CHECK(pbGetGadgetAttribute(10, 3) == 3);

    CHECK(pbIsGadget(1) == 1); // the old gadget is not destroyed
    CHECK(pbIsGadget(3) == 1);
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(1)) == detail::windowFixedTable().at(605));
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(3)) == detail::gadgetTable().at(10));

    // Replacing with an unknown #Gadget is a harmless failure.
    CHECK(pbSetGadgetAttribute(10, 3, 9999) == 0);

    pbCloseWindow(605);
}

TEST_CASE("ScrollAreaGadget automatically captures subsequently created gadgets immediately, "
          "like Container not Panel",
          "[runtime][guilib]") {
    // Oracle-verified (ScrollAreaGadget.html's own "Remarques"): "Une fois
    // créé, tous les gadgets suivants seront placés dans ce gadget" - no
    // AddGadgetItem-style separate step needed, unlike PanelGadget.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(700, 10, 10, 400, 400, PBString("Test"));

    CHECK(pbScrollAreaGadget(10, 10, 10, 200, 150, 500, 500, 20) == 1);
    CHECK(pbButtonGadget(1, 5, 5, 60, 20, PBString("InScroll")) == 1);
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(1)) == detail::containerFixedTable().at(10));

    CHECK(pbCloseGadgetList() == 1);
    CHECK(pbButtonGadget(2, 5, 300, 60, 20, PBString("WindowLevel")) == 1);
    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(2)) == detail::windowFixedTable().at(700));

    pbCloseWindow(700);
}

TEST_CASE("GetGadgetAttribute/SetGadgetAttribute round-trip a ScrollArea's own inner size, scroll "
          "position, and scroll step",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(701, 10, 10, 400, 400, PBString("Test"));
    pbScrollAreaGadget(10, 10, 10, 200, 150, 500, 500, 20);
    pbCloseGadgetList();

    CHECK(pbGetGadgetAttribute(10, 1 /* #PB_ScrollArea_InnerWidth */) == 500);
    CHECK(pbGetGadgetAttribute(10, 2 /* #PB_ScrollArea_InnerHeight */) == 500);
    CHECK(pbGetGadgetAttribute(10, 3 /* #PB_ScrollArea_X */) == 0);
    CHECK(pbGetGadgetAttribute(10, 4 /* #PB_ScrollArea_Y */) == 0);
    CHECK(pbGetGadgetAttribute(10, 5 /* #PB_ScrollArea_ScrollStep */) == 20);

    CHECK(pbSetGadgetAttribute(10, 1, 800) == 1);
    CHECK(pbGetGadgetAttribute(10, 1) == 800);
    CHECK(pbSetGadgetAttribute(10, 2, 600) == 1);
    CHECK(pbGetGadgetAttribute(10, 2) == 600);
    CHECK(pbSetGadgetAttribute(10, 5, 99) == 1);
    CHECK(pbGetGadgetAttribute(10, 5) == 99);

    // Harmless failures: an unsupported attribute, a non-ScrollArea
    // gadget, and an unknown gadget - consistent with Splitter's own
    // established precedent.
    CHECK(pbGetGadgetAttribute(10, 99) == 0);
    pbButtonGadget(1, 0, 0, 0, 0, PBString("B"));
    CHECK(pbGetGadgetAttribute(1, 1) == 0);
    CHECK(pbGetGadgetAttribute(9999, 1) == 0);
    CHECK(pbSetGadgetAttribute(1, 1, 1) == 0);

    pbCloseWindow(701);
}

TEST_CASE("ScrollAreaGadget maps its own border flags onto GtkScrolledWindow's shadow type, "
          "independently of ContainerGadget's own bit values",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(702, 10, 10, 400, 400, PBString("Test"));

    pbScrollAreaGadget(10, 10, 10, 100, 100, 200, 200, 0, 4 /* #PB_ScrollArea_Single */);
    pbCloseGadgetList();
    CHECK(gtk_scrolled_window_get_shadow_type(GTK_SCROLLED_WINDOW(detail::gadgetTable().at(10))) ==
          GTK_SHADOW_IN);

    pbScrollAreaGadget(11, 120, 10, 100, 100, 200, 200, 0, 2 /* #PB_ScrollArea_Raised */);
    pbCloseGadgetList();
    CHECK(gtk_scrolled_window_get_shadow_type(GTK_SCROLLED_WINDOW(detail::gadgetTable().at(11))) ==
          GTK_SHADOW_OUT);

    pbScrollAreaGadget(12, 230, 10, 100, 100, 200, 200); // no flags at all
    pbCloseGadgetList();
    CHECK(gtk_scrolled_window_get_shadow_type(GTK_SCROLLED_WINDOW(detail::gadgetTable().at(12))) ==
          GTK_SHADOW_NONE);

    pbCloseWindow(702);
}

TEST_CASE("A real scrollbar value change queues #PB_Event_Gadget for ScrollAreaGadget",
          "[runtime][guilib]") {
    // gtk_adjustment_set_value fires the exact "value-changed" signal a
    // real scrollbar drag does - the GtkAdjustment equivalent of
    // gtk_button_clicked in this project's own existing click tests.
    // The exact EventType() value isn't independently oracle-verified
    // (see onScrollAreaValueChanged's own doc comment) - only that an
    // event is queued at all, oracle-verified via ScrollAreaGadget.html's
    // own remarks.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(703, 10, 10, 400, 400, PBString("Test"));
    pbScrollAreaGadget(10, 10, 10, 100, 100, 500, 500, 20);
    pbCloseGadgetList();
    drainEvents();

    auto* scrolled = GTK_SCROLLED_WINDOW(detail::gadgetTable().at(10));
    gtk_adjustment_set_value(gtk_scrolled_window_get_hadjustment(scrolled), 42);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 10);

    pbCloseWindow(703);
}

TEST_CASE("Freeing a ScrollAreaGadget recursively frees every gadget nested inside it",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(704, 10, 10, 400, 400, PBString("Test"));
    pbScrollAreaGadget(10, 10, 10, 200, 150, 500, 500);
    pbButtonGadget(1, 5, 5, 60, 20, PBString("A"));
    pbContainerGadget(2, 5, 30, 150, 100); // a nested container, deeper still
    pbButtonGadget(3, 5, 5, 60, 20, PBString("B"));
    pbCloseGadgetList(); // closes container 2
    pbCloseGadgetList(); // closes the ScrollArea

    CHECK(pbFreeGadget(10) == 1);
    CHECK(pbIsGadget(10) == 0);
    CHECK(pbIsGadget(1) == 0);
    CHECK(pbIsGadget(2) == 0);
    CHECK(pbIsGadget(3) == 0);

    pbCloseWindow(704);
}

TEST_CASE("OpenGadgetList reopens a ScrollAreaGadget so more gadgets can be added to it "
          "dynamically, reusing Container's own existing mechanism unchanged",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(705, 10, 10, 400, 400, PBString("Test"));
    pbScrollAreaGadget(10, 10, 10, 200, 150, 500, 500);
    pbCloseGadgetList();

    CHECK(pbOpenGadgetList(10) == 1);
    CHECK(pbButtonGadget(1, 5, 5, 60, 20, PBString("Added")) == 1);
    CHECK(pbCloseGadgetList() == 1);

    CHECK(gtk_widget_get_parent(detail::gadgetTable().at(1)) == detail::containerFixedTable().at(10));

    pbCloseWindow(705);
}

TEST_CASE("ListViewGadget round-trips items, selection state, selected text, and per-item data",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(800, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbListViewGadget(1, 10, 10, 250, 120) == 1);
    for (int i = 1; i <= 5; ++i) {
        CHECK(pbAddGadgetItem(1, -1, PBString("Item " + std::to_string(i))) == 1);
    }
    CHECK(pbCountGadgetItems(1) == 5);

    CHECK(pbGetGadgetState(1) == -1); // nothing selected yet
    CHECK(pbSetGadgetState(1, 2) == 1);
    CHECK(pbGetGadgetState(1) == 2);
    CHECK(pbGetGadgetText(1).bytes() == "Item 3");

    // Oracle-verified: SetGadgetText selects whichever item's own text
    // exactly matches, or *clears* the current selection entirely if
    // none does (confirmed directly via a dedicated, isolated probe -
    // not simply "leaves it unchanged", the initially assumed and
    // incorrect behavior this project's own golden e2e comparison
    // caught disagreeing with the real oracle).
    CHECK(pbSetGadgetText(1, PBString("Item 4")) == 1);
    CHECK(pbGetGadgetState(1) == 3);
    CHECK(pbSetGadgetText(1, PBString("No such item")) == 1);
    CHECK(pbGetGadgetState(1) == -1); // cleared, not left at 3

    CHECK(pbSetGadgetState(1, 3) == 1); // reselect for the state checks below
    CHECK(pbGetGadgetItemState(1, 3) == 1);
    CHECK(pbGetGadgetItemState(1, 1) == 0);

    // Oracle-verified: single-select mode already enforces exclusivity -
    // selecting a different item deselects the previous one automatically.
    CHECK(pbSetGadgetItemState(1, 1, 1) == 1);
    CHECK(pbGetGadgetItemState(1, 1) == 1);
    CHECK(pbGetGadgetItemState(1, 3) == 0);
    CHECK(pbGetGadgetState(1) == 1);

    CHECK(pbSetGadgetState(1, -1) == 1); // deselect all
    CHECK(pbGetGadgetState(1) == -1);
    CHECK(pbGetGadgetText(1).bytes().empty());

    CHECK(pbSetGadgetItemData(1, 0, 777) == 1);
    CHECK(pbGetGadgetItemData(1, 0) == 777);
    CHECK(pbGetGadgetItemData(1, 1) == 0); // untouched, defaults to 0

    CHECK(pbRemoveGadgetItem(1, 0) == 1);
    CHECK(pbCountGadgetItems(1) == 4);
    CHECK(pbGetGadgetItemText(1, 0).bytes() == "Item 2"); // re-indexed

    CHECK(pbClearGadgetItems(1) == 1);
    CHECK(pbCountGadgetItems(1) == 0);

    pbCloseWindow(800);
}

TEST_CASE("ListViewGadget's own Multiselect flag allows more than one simultaneous selection",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(801, 10, 10, 300, 300, PBString("Test"));
    pbListViewGadget(1, 10, 10, 250, 120, 1 /* #PB_ListView_Multiselect */);
    for (int i = 1; i <= 3; ++i) {
        pbAddGadgetItem(1, -1, PBString("Item " + std::to_string(i)));
    }

    CHECK(pbSetGadgetItemState(1, 0, 1) == 1);
    CHECK(pbSetGadgetItemState(1, 2, 1) == 1);
    CHECK(pbGetGadgetItemState(1, 0) == 1); // both stay selected at once
    CHECK(pbGetGadgetItemState(1, 2) == 1);
    CHECK(pbGetGadgetItemState(1, 1) == 0);

    pbCloseWindow(801);
}

TEST_CASE("A real ListView selection change queues #PB_Event_Gadget with #PB_EventType_LeftClick",
          "[runtime][guilib]") {
    // gtk_tree_selection_select_path, called directly (bypassing
    // pbSetGadgetItemState's own signal-blocking), is the GtkTreeView
    // equivalent of gtk_button_clicked in this project's own existing
    // click tests.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(802, 10, 10, 300, 300, PBString("Test"));
    pbListViewGadget(1, 10, 10, 250, 120);
    pbAddGadgetItem(1, -1, PBString("Item 1"));
    pbAddGadgetItem(1, -1, PBString("Item 2"));
    drainEvents();

    GtkWidget* treeView = detail::listViewTreeView(detail::gadgetTable().at(1));
    REQUIRE(treeView != nullptr);
    GtkTreePath* path = gtk_tree_path_new_from_indices(1, -1);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView)), path);
    gtk_tree_path_free(path);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbCloseWindow(802);
}

TEST_CASE("ComboBoxGadget round-trips items, selection state, and selected text",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(803, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbComboBoxGadget(1, 10, 10, 250, 21) == 1);
    for (int i = 1; i <= 5; ++i) {
        CHECK(pbAddGadgetItem(1, -1, PBString("Item " + std::to_string(i))) == 1);
    }
    CHECK(pbCountGadgetItems(1) == 5);

    CHECK(pbGetGadgetState(1) == -1);
    CHECK(pbGetGadgetText(1).bytes().empty());
    CHECK(pbSetGadgetState(1, 2) == 1);
    CHECK(pbGetGadgetState(1) == 2);
    CHECK(pbGetGadgetText(1).bytes() == "Item 3");

    CHECK(pbSetGadgetText(1, PBString("Item 4")) == 1);
    CHECK(pbGetGadgetState(1) == 3);
    // Oracle-verified (same finding as ListViewGadget's own, confirmed
    // for ComboBoxGadget too): a non-matching text clears the selection.
    CHECK(pbSetGadgetText(1, PBString("No such item")) == 1);
    CHECK(pbGetGadgetState(1) == -1);

    CHECK(pbRemoveGadgetItem(1, 0) == 1);
    CHECK(pbCountGadgetItems(1) == 4);
    CHECK(pbGetGadgetItemText(1, 0).bytes() == "Item 2");

    CHECK(pbClearGadgetItems(1) == 1);
    CHECK(pbCountGadgetItems(1) == 0);

    pbCloseWindow(803);
}

TEST_CASE("An editable ComboBoxGadget accepts arbitrary text that matches no item at all",
          "[runtime][guilib]") {
    // Oracle-verified directly: an editable combo's own GetGadgetText
    // reads back whatever was set via SetGadgetText verbatim, and
    // GetGadgetState stays -1 since nothing in the list matches it.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(804, 10, 10, 300, 300, PBString("Test"));
    pbComboBoxGadget(1, 10, 10, 250, 21, 1 /* #PB_ComboBox_Editable */);
    pbAddGadgetItem(1, -1, PBString("Editable item"));

    CHECK(pbSetGadgetText(1, PBString("Something typed")) == 1);
    CHECK(pbGetGadgetText(1).bytes() == "Something typed");
    CHECK(pbGetGadgetState(1) == -1);

    pbCloseWindow(804);
}

TEST_CASE("A real ComboBox selection change queues #PB_Event_Gadget with #PB_EventType_Change",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(805, 10, 10, 300, 300, PBString("Test"));
    pbComboBoxGadget(1, 10, 10, 250, 21);
    pbAddGadgetItem(1, -1, PBString("Item 1"));
    pbAddGadgetItem(1, -1, PBString("Item 2"));
    drainEvents();

    gtk_combo_box_set_active(GTK_COMBO_BOX(detail::gadgetTable().at(1)), 1);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 768); // #PB_EventType_Change

    pbCloseWindow(805);
}

TEST_CASE("ListView/ComboBox-only functions are a harmless failure on an unrelated gadget type",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(806, 10, 10, 300, 300, PBString("Test"));
    pbButtonGadget(1, 10, 10, 60, 20, PBString("B"));

    CHECK(pbGetGadgetItemState(1, 0) == 0);
    CHECK(pbSetGadgetItemState(1, 0, 1) == 0);
    CHECK(pbGetGadgetItemData(1, 0) == 0);
    CHECK(pbSetGadgetItemData(1, 0, 1) == 0);
    CHECK(pbAddGadgetItem(1, -1, PBString("x")) == 0);
    CHECK(pbCountGadgetItems(1) == 0);

    pbCloseWindow(806);
}

TEST_CASE("ListIconGadget round-trips multi-column items, selection state, text, and per-item data",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(900, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbListIconGadget(1, 10, 10, 250, 120, PBString("Name"), 100) == 1);
    pbAddGadgetColumn(1, 1, PBString("Age"), 60);
    for (int i = 1; i <= 3; ++i) {
        CHECK(pbAddGadgetItem(1, -1, PBString("Person " + std::to_string(i) + "\n" + std::to_string(20 + i))) == 1);
    }
    CHECK(pbCountGadgetItems(1) == 3);
    CHECK(pbGetGadgetItemText(1, 0, 0).bytes() == "Person 1");
    CHECK(pbGetGadgetItemText(1, 0, 1).bytes() == "21");
    CHECK(pbGetGadgetItemText(1, -1, 0).bytes() == "Name"); // column header
    CHECK(pbGetGadgetItemText(1, -1, 1).bytes() == "Age");

    CHECK(pbSetGadgetItemText(1, 1, PBString("Changed"), 0) == 1);
    CHECK(pbGetGadgetItemText(1, 1, 0).bytes() == "Changed");
    CHECK(pbSetGadgetItemText(1, -1, PBString("NameHdr"), 0) == 1);
    CHECK(pbGetGadgetItemText(1, -1, 0).bytes() == "NameHdr");

    CHECK(pbGetGadgetState(1) == -1);
    CHECK(pbSetGadgetState(1, 1) == 1);
    CHECK(pbGetGadgetState(1) == 1);
    CHECK(pbGetGadgetText(1).bytes() == "Changed"); // first column of the selected row

    CHECK(pbSetGadgetItemData(1, 0, 555) == 1);
    CHECK(pbGetGadgetItemData(1, 0) == 555);
    CHECK(pbGetGadgetItemData(1, 1) == 0); // untouched, defaults to 0

    CHECK(pbRemoveGadgetItem(1, 0) == 1);
    CHECK(pbCountGadgetItems(1) == 2);
    CHECK(pbClearGadgetItems(1) == 1);
    CHECK(pbCountGadgetItems(1) == 0);

    pbCloseWindow(900);
}

TEST_CASE("AddGadgetColumn/RemoveGadgetColumn manage a ListIcon's own columns, round-tripping ColumnWidth too",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(901, 10, 10, 300, 300, PBString("Test"));
    pbListIconGadget(1, 10, 10, 250, 120, PBString("Col0"), 100);
    CHECK(pbGetGadgetAttribute(1, 3) == 1); // #PB_ListIcon_ColumnCount

    CHECK(pbAddGadgetColumn(1, 1, PBString("Col1"), 60) == 1);
    CHECK(pbAddGadgetColumn(1, -1, PBString("Col2"), 70) == 1); // appended at the end
    CHECK(pbGetGadgetAttribute(1, 3) == 3);
    CHECK(pbGetGadgetItemAttribute(1, 0, 1, 1) == 60); // #PB_ListIcon_ColumnWidth of Col1

    CHECK(pbSetGadgetItemAttribute(1, 0, 1, 99, 1) == 1);
    CHECK(pbGetGadgetItemAttribute(1, 0, 1, 1) == 99);

    CHECK(pbRemoveGadgetColumn(1, 1) == 1); // removes "Col1"
    CHECK(pbGetGadgetAttribute(1, 3) == 2);
    CHECK(pbGetGadgetItemText(1, -1, 1).bytes() == "Col2"); // Col2 shifted into position 1

    CHECK(pbRemoveGadgetColumn(1, -1) == 1); // #PB_All
    CHECK(pbGetGadgetAttribute(1, 3) == 0);

    pbCloseWindow(901);
}

TEST_CASE("GetGadgetItemState/SetGadgetItemState round-trip a ListIcon's own Selected/Checked bits",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(902, 10, 10, 300, 300, PBString("Test"));
    pbListIconGadget(1, 10, 10, 250, 120, PBString("Name"), 100, 1 /* #PB_ListIcon_CheckBoxes */);
    pbAddGadgetItem(1, -1, PBString("Item 1"));
    pbAddGadgetItem(1, -1, PBString("Item 2"));

    CHECK(pbGetGadgetItemState(1, 0) == 0);
    CHECK(pbSetGadgetItemState(1, 0, 1) == 1); // #PB_ListIcon_Selected
    CHECK(pbGetGadgetItemState(1, 0) == 1);
    CHECK(pbSetGadgetItemState(1, 0, 2) == 1); // #PB_ListIcon_Checked (also clears Selected)
    CHECK(pbGetGadgetItemState(1, 0) == 2);
    CHECK(pbSetGadgetItemState(1, 0, 3) == 1); // both at once
    CHECK(pbGetGadgetItemState(1, 0) == 3);
    CHECK(pbGetGadgetItemState(1, 1) == 0); // untouched

    pbCloseWindow(902);
}

TEST_CASE("A real ListIcon selection change/row activation/right-click/column-header click each queue their own "
          "#PB_EventType",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(903, 10, 10, 300, 300, PBString("Test"));
    pbListIconGadget(1, 10, 10, 250, 120, PBString("Name"), 100);
    pbAddGadgetItem(1, -1, PBString("Item 1"));
    pbAddGadgetItem(1, -1, PBString("Item 2"));
    drainEvents();

    GtkWidget* treeView = detail::listIconTreeView(detail::gadgetTable().at(1));
    REQUIRE(treeView != nullptr);
    GtkTreePath* path = gtk_tree_path_new_from_indices(1, -1);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView)), path);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick
    // The same selection-changed signal also queues #PB_EventType_Change.
    CHECK(pbWindowEvent() == 3);
    CHECK(pbEventType() == 768);

    GtkTreeViewColumn* column0 = gtk_tree_view_get_column(GTK_TREE_VIEW(treeView), 0);
    gtk_tree_view_row_activated(GTK_TREE_VIEW(treeView), path, column0);
    CHECK(pbWindowEvent() == 3);
    CHECK(pbEventType() == 2); // #PB_EventType_LeftDoubleClick
    gtk_tree_path_free(path);

    GdkEvent* rightClick = gdk_event_new(GDK_BUTTON_PRESS);
    rightClick->button.button = 3;
    gboolean handled = GDK_EVENT_PROPAGATE;
    g_signal_emit_by_name(treeView, "button-press-event", rightClick, &handled);
    gdk_event_free(rightClick);
    CHECK(pbWindowEvent() == 3);
    CHECK(pbEventType() == 1); // #PB_EventType_RightClick

    g_signal_emit_by_name(column0, "clicked");
    CHECK(pbWindowEvent() == 3);
    CHECK(pbEventType() == 8); // #PB_EventType_ColumnClick
    CHECK(pbGetGadgetAttribute(1, 4) == 0); // #PB_ListIcon_ClickedColumn

    pbCloseWindow(903);
}

TEST_CASE("A real ListIcon checkbox toggle flips the Checked bit and queues LeftClick/Change",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(904, 10, 10, 300, 300, PBString("Test"));
    pbListIconGadget(1, 10, 10, 250, 120, PBString("Name"), 100, 1 /* #PB_ListIcon_CheckBoxes */);
    pbAddGadgetItem(1, -1, PBString("Item 1"));
    drainEvents();

    GtkWidget* treeView = detail::listIconTreeView(detail::gadgetTable().at(1));
    REQUIRE(treeView != nullptr);
    GtkTreeViewColumn* column0 = gtk_tree_view_get_column(GTK_TREE_VIEW(treeView), 0);
    GList* cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(column0));
    REQUIRE(cells != nullptr);
    auto* toggleRenderer = GTK_CELL_RENDERER_TOGGLE(cells->data);
    g_list_free(cells);

    CHECK(pbGetGadgetItemState(1, 0) == 0);
    g_signal_emit_by_name(toggleRenderer, "toggled", "0");
    CHECK(pbGetGadgetItemState(1, 0) == 2); // #PB_ListIcon_Checked

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbCloseWindow(904);
}

TEST_CASE("TreeGadget round-trips hierarchical items via flat depth-first Element addressing",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(905, 10, 10, 300, 300, PBString("Test"));
    CHECK(pbTreeGadget(1, 10, 10, 250, 200) == 1);

    CHECK(pbAddGadgetItem(1, -1, PBString("Root0"), 0, 0) == 1);
    CHECK(pbAddGadgetItem(1, -1, PBString("Root1"), 0, 0) == 1);
    CHECK(pbAddGadgetItem(1, -1, PBString("Child1.0"), 0, 1) == 1);
    CHECK(pbAddGadgetItem(1, -1, PBString("Child1.1"), 0, 1) == 1);
    CHECK(pbAddGadgetItem(1, -1, PBString("Root2"), 0, 0) == 1);

    CHECK(pbCountGadgetItems(1) == 5); // every depth, not just root-level
    CHECK(pbGetGadgetItemText(1, 2).bytes() == "Child1.0");
    CHECK(pbGetGadgetItemAttribute(1, 2, 1) == 1); // #PB_Tree_SubLevel
    CHECK(pbGetGadgetItemAttribute(1, 0, 1) == 0);

    CHECK(pbSetGadgetItemData(1, 2, 42) == 1);
    CHECK(pbGetGadgetItemData(1, 2) == 42);
    CHECK(pbGetGadgetItemData(1, 0) == 0); // untouched

    pbCloseWindow(905);
}

TEST_CASE("TreeGadget's own SetGadgetState/SetGadgetItemState select a child under a still-collapsed ancestor",
          "[runtime][guilib]") {
    // Oracle-verified: real PB lets a level-1+ child be selected directly,
    // with no prior expansion needed - GtkTreeSelection's own API won't
    // do that on its own (silently does nothing for a row under a
    // collapsed ancestor), so this is exercising the gtk_tree_view_
    // expand_to_path fix-up pbSetGadgetState/pbSetGadgetItemState both
    // apply for TreeGadget specifically.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(906, 10, 10, 300, 300, PBString("Test"));
    pbTreeGadget(1, 10, 10, 250, 200);
    pbAddGadgetItem(1, -1, PBString("Root0"), 0, 0);
    pbAddGadgetItem(1, -1, PBString("Root1"), 0, 0);
    pbAddGadgetItem(1, -1, PBString("Child1.0"), 0, 1);

    CHECK(pbSetGadgetState(1, 2) == 1);
    CHECK(pbGetGadgetState(1) == 2);
    CHECK(pbGetGadgetText(1).bytes() == "Child1.0");

    CHECK(pbSetGadgetState(1, -1) == 1); // deselect all
    CHECK(pbGetGadgetState(1) == -1);

    CHECK(pbSetGadgetItemState(1, 2, 1) == 1); // #PB_Tree_Selected
    CHECK(pbGetGadgetItemState(1, 2) & 1);

    pbCloseWindow(906);
}

TEST_CASE("TreeGadget's own SetGadgetText renames the currently selected element rather than searching by text",
          "[runtime][guilib]") {
    // Oracle-verified directly (not assumed, and genuinely different from
    // ListView/ComboBox's own "find and select by text" behavior): see
    // pbSetGadgetText's own doc comment.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(907, 10, 10, 300, 300, PBString("Test"));
    pbTreeGadget(1, 10, 10, 250, 200);
    pbAddGadgetItem(1, -1, PBString("Root0"), 0, 0);
    pbAddGadgetItem(1, -1, PBString("Root1"), 0, 0);

    // Nothing selected: a harmless no-op.
    CHECK(pbSetGadgetText(1, PBString("Root1")) == 1);
    CHECK(pbGetGadgetState(1) == -1);
    CHECK(pbGetGadgetItemText(1, 1).bytes() == "Root1");

    pbSetGadgetState(1, 0);
    CHECK(pbSetGadgetText(1, PBString("Renamed")) == 1);
    CHECK(pbGetGadgetItemText(1, 0).bytes() == "Renamed");
    CHECK(pbGetGadgetItemText(1, 1).bytes() == "Root1"); // untouched, even though it wasn't the match target

    pbCloseWindow(907);
}

TEST_CASE("RemoveGadgetItem on a TreeGadget removes the item and every descendant",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(908, 10, 10, 300, 300, PBString("Test"));
    pbTreeGadget(1, 10, 10, 250, 200);
    pbAddGadgetItem(1, -1, PBString("Root0"), 0, 0);
    pbAddGadgetItem(1, -1, PBString("Root1"), 0, 0);
    pbAddGadgetItem(1, -1, PBString("Child1.0"), 0, 1);
    pbAddGadgetItem(1, -1, PBString("Child1.1"), 0, 1);
    pbAddGadgetItem(1, -1, PBString("Root2"), 0, 0);
    CHECK(pbCountGadgetItems(1) == 5);

    CHECK(pbRemoveGadgetItem(1, 1) == 1); // Root1, with both of its children
    CHECK(pbCountGadgetItems(1) == 2);
    CHECK(pbGetGadgetItemText(1, 0).bytes() == "Root0");
    CHECK(pbGetGadgetItemText(1, 1).bytes() == "Root2");

    CHECK(pbClearGadgetItems(1) == 1);
    CHECK(pbCountGadgetItems(1) == 0);

    pbCloseWindow(908);
}

TEST_CASE("ListIcon/Tree-only functions are a harmless failure on an unrelated gadget type",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(909, 10, 10, 300, 300, PBString("Test"));
    pbButtonGadget(1, 10, 10, 60, 20, PBString("B"));

    CHECK(pbAddGadgetColumn(1, 0, PBString("x"), 10) == 0);
    CHECK(pbRemoveGadgetColumn(1, 0) == 0);
    CHECK(pbGetGadgetItemAttribute(1, 0, 1) == 0);
    CHECK(pbSetGadgetItemAttribute(1, 0, 1, 1) == 0);

    pbCloseWindow(909);
}

TEST_CASE("OptionGadget groups consecutive calls into one mutually-exclusive radio group, the first "
          "defaulting to selected",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(910, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbOptionGadget(0, 10, 10, 100, 20, PBString("Option 1")) == 1);
    CHECK(pbOptionGadget(1, 10, 40, 100, 20, PBString("Option 2")) == 1);
    CHECK(pbOptionGadget(2, 10, 70, 100, 20, PBString("Option 3")) == 1);

    // Oracle-verified: the first option in a fresh group starts selected.
    CHECK(pbGetGadgetState(0) == 1);
    CHECK(pbGetGadgetState(1) == 0);
    CHECK(pbGetGadgetState(2) == 0);

    CHECK(pbSetGadgetState(1, 1) == 1);
    CHECK(pbGetGadgetState(0) == 0); // automatically deselected
    CHECK(pbGetGadgetState(1) == 1);
    CHECK(pbGetGadgetState(2) == 0);
    CHECK(pbGetGadgetText(1).bytes() == "Option 2");

    pbCloseWindow(910);
}

TEST_CASE("A different gadget type between OptionGadget calls starts a brand new, independent radio group",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(911, 10, 10, 300, 300, PBString("Test"));

    pbOptionGadget(0, 10, 10, 100, 20, PBString("Option 1"));
    pbOptionGadget(1, 10, 40, 100, 20, PBString("Option 2"));
    pbSetGadgetState(1, 1);

    pbTextGadget(10, 10, 100, 100, 20, PBString("sep")); // breaks the group

    CHECK(pbOptionGadget(2, 10, 130, 100, 20, PBString("Option A")) == 1);
    CHECK(pbOptionGadget(3, 10, 160, 100, 20, PBString("Option B")) == 1);
    CHECK(pbGetGadgetState(2) == 1); // first-in-new-group defaults selected
    CHECK(pbGetGadgetState(3) == 0);

    CHECK(pbSetGadgetState(3, 1) == 1);
    CHECK(pbGetGadgetState(2) == 0);
    CHECK(pbGetGadgetState(3) == 1);
    CHECK(pbGetGadgetState(1) == 1); // first group's own state is untouched

    pbCloseWindow(911);
}

TEST_CASE("OptionGadget's own SetGadgetState(id, 0) is a no-op - a radio button can't be deselected directly",
          "[runtime][guilib]") {
    // Oracle-verified directly: unlike CheckBoxGadget's own plain on/off
    // toggle, an already-selected radio button stays selected when told
    // to deselect itself - only selecting a *different* group member
    // actually changes anything.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(912, 10, 10, 300, 300, PBString("Test"));
    pbOptionGadget(0, 10, 10, 100, 20, PBString("Option 1"));
    pbOptionGadget(1, 10, 40, 100, 20, PBString("Option 2"));
    pbSetGadgetState(1, 1);

    CHECK(pbSetGadgetState(1, 0) == 1);
    CHECK(pbGetGadgetState(1) == 1); // unchanged

    pbCloseWindow(912);
}

TEST_CASE("A real OptionGadget toggle queues #PB_Event_Gadget with #PB_EventType_LeftClick",
          "[runtime][guilib]") {
    // Activating one radio button in a group also deactivates whichever
    // sibling was previously active - both widgets' own "toggled" signal
    // genuinely fires, so this queues *two* #PB_Event_Gadget events (the
    // outgoing one first, then the incoming one), not just one the way a
    // standalone CheckBoxGadget's own toggle does.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(913, 10, 10, 300, 300, PBString("Test"));
    pbOptionGadget(0, 10, 10, 100, 20, PBString("Option 1")); // defaults selected
    pbOptionGadget(1, 10, 40, 100, 20, PBString("Option 2"));
    drainEvents();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detail::gadgetTable().at(1)), TRUE);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 0); // Option 1 losing selection
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1); // Option 2 gaining selection
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbCloseWindow(913);
}

TEST_CASE("ProgressBarGadget round-trips its own value, clamped to [Minimum, Maximum], plus the "
          "#PB_ProgressBar_Unknown sentinel",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(914, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbProgressBarGadget(0, 10, 10, 250, 30, 0, 200) == 1);
    CHECK(pbGetGadgetState(0) == 0);

    CHECK(pbSetGadgetState(0, 50) == 1);
    CHECK(pbGetGadgetState(0) == 50);

    CHECK(pbSetGadgetState(0, 1000) == 1); // clamped to Maximum
    CHECK(pbGetGadgetState(0) == 200);
    CHECK(pbSetGadgetState(0, -50) == 1); // clamped to Minimum
    CHECK(pbGetGadgetState(0) == 0);

    CHECK(pbSetGadgetState(0, -1) == 1); // #PB_ProgressBar_Unknown
    CHECK(pbGetGadgetState(0) == -1);

    pbCloseWindow(914);
}

TEST_CASE("GetGadgetAttribute/SetGadgetAttribute round-trip a ProgressBar's own Minimum/Maximum without "
          "retroactively re-clamping the current value",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(915, 10, 10, 300, 300, PBString("Test"));
    pbProgressBarGadget(0, 10, 10, 250, 30, 0, 200);
    pbSetGadgetState(0, 50);

    CHECK(pbGetGadgetAttribute(0, 1) == 0); // #PB_ProgressBar_Minimum
    CHECK(pbGetGadgetAttribute(0, 2) == 200); // #PB_ProgressBar_Maximum

    CHECK(pbSetGadgetAttribute(0, 2, 400) == 1);
    CHECK(pbGetGadgetAttribute(0, 2) == 400);
    CHECK(pbGetGadgetState(0) == 50); // untouched

    pbCloseWindow(915);
}

TEST_CASE("ProgressBar's own Minimum/Maximum attribute, and SetGadgetState's own radio-no-op rule, are a "
          "harmless no-op/failure on an unrelated gadget type",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(916, 10, 10, 300, 300, PBString("Test"));
    pbButtonGadget(1, 10, 10, 60, 20, PBString("B"));

    CHECK(pbGetGadgetAttribute(1, 1) == 0);
    CHECK(pbSetGadgetAttribute(1, 1, 1) == 0);

    pbCloseWindow(916);
}

TEST_CASE("SpinGadget's own displayed text is genuinely independent of its internal state unless "
          "#PB_Spin_Numeric was given at creation time",
          "[runtime][guilib]") {
    // Oracle-verified directly: a freshly-created, non-#PB_Spin_Numeric
    // SpinGadget reports back an *empty* GetGadgetText() (despite an
    // initial value of 0), and SetGadgetState leaves the displayed text
    // completely untouched; SetGadgetText itself is still settable to
    // arbitrary, non-numeric text - confirmed with a dedicated, isolated
    // probe after this slice's own e2e case caught plain
    // gtk_entry_set_text silently getting rejected by GtkSpinButton's
    // own internal numeric-input validation.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(917, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbSpinGadget(0, 10, 10, 100, 20, 0, 100) == 1);
    CHECK(pbGetGadgetState(0) == 0);
    CHECK(pbGetGadgetText(0).bytes().empty());

    CHECK(pbSetGadgetState(0, 42) == 1);
    CHECK(pbGetGadgetState(0) == 42);
    CHECK(pbGetGadgetText(0).bytes().empty()); // still untouched

    CHECK(pbSetGadgetText(0, PBString("hi")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "hi");

    pbCloseWindow(917);
}

TEST_CASE("#PB_Spin_Numeric auto-syncs SpinGadget's own displayed text to SetGadgetState", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(918, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbSpinGadget(0, 10, 10, 100, 20, 5, 100, 8) == 1); // #PB_Spin_Numeric
    CHECK(pbGetGadgetText(0).bytes() == "5"); // formatted from the initial value

    CHECK(pbSetGadgetState(0, 55) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "55");

    pbCloseWindow(918);
}

TEST_CASE("GetGadgetAttribute/SetGadgetAttribute round-trip SpinGadget/TrackBarGadget's own Minimum/Maximum, "
          "and SetGadgetState clamps into the (possibly changed) range",
          "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(919, 10, 10, 300, 300, PBString("Test"));
    pbSpinGadget(0, 10, 10, 100, 20, 0, 100);
    pbTrackBarGadget(1, 10, 40, 200, 30, 0, 50);

    CHECK(pbGetGadgetAttribute(0, 1) == 0); // #PB_Spin_Minimum
    CHECK(pbGetGadgetAttribute(0, 2) == 100); // #PB_Spin_Maximum
    CHECK(pbSetGadgetAttribute(0, 2, 200) == 1);
    CHECK(pbGetGadgetAttribute(0, 2) == 200);
    CHECK(pbSetGadgetState(0, 1000) == 1);
    CHECK(pbGetGadgetState(0) == 200); // clamped to the new Maximum

    CHECK(pbGetGadgetAttribute(1, 1) == 0); // #PB_TrackBar_Minimum
    CHECK(pbGetGadgetAttribute(1, 2) == 50); // #PB_TrackBar_Maximum
    CHECK(pbSetGadgetAttribute(1, 2, 80) == 1);
    CHECK(pbGetGadgetAttribute(1, 2) == 80);
    CHECK(pbSetGadgetState(1, 1000) == 1);
    CHECK(pbGetGadgetState(1) == 80); // clamped to the new Maximum

    pbCloseWindow(919);
}

TEST_CASE("ScrollBarGadget's own PageLength narrows the effective maximum selectable position to "
          "Maximum - PageLength + 1, but #PB_ScrollBar_Maximum itself reads back the plain Maximum",
          "[runtime][guilib]") {
    // Oracle-verified directly: Minimum=0/Maximum=100/PageLength=30
    // clamps SetGadgetState to 71, not 100 or 70.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(920, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbScrollBarGadget(0, 10, 10, 200, 20, 0, 100, 30) == 1);
    CHECK(pbGetGadgetState(0) == 0);
    CHECK(pbGetGadgetAttribute(0, 1) == 0); // #PB_ScrollBar_Minimum
    CHECK(pbGetGadgetAttribute(0, 2) == 100); // #PB_ScrollBar_Maximum
    CHECK(pbGetGadgetAttribute(0, 3) == 30); // #PB_ScrollBar_PageLength

    CHECK(pbSetGadgetState(0, 1000) == 1);
    CHECK(pbGetGadgetState(0) == 71); // clamped to Maximum - PageLength + 1

    pbCloseWindow(920);
}

TEST_CASE("Spin/TrackBar attributes are a harmless failure on an unrelated gadget type", "[runtime][guilib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(921, 10, 10, 300, 300, PBString("Test"));
    pbButtonGadget(1, 10, 10, 60, 20, PBString("B"));

    CHECK(pbGetGadgetAttribute(1, 1) == 0); // #PB_Spin_Minimum
    CHECK(pbSetGadgetAttribute(1, 1, 1) == 0); // #PB_TrackBar_Minimum

    pbCloseWindow(921);
}

TEST_CASE("ImageGadget's own state is an ImageID-style handle, not a boolean - GetGadgetState reads "
          "back whichever handle was most recently set",
          "[runtime][guilib]") {
    // Oracle-verified directly: SetGadgetState(id, ImageID(other)) changes
    // which image is displayed, and GetGadgetState reads back whichever
    // ImageID-style handle was set most recently (at creation or via
    // SetGadgetState) - not a plain boolean the way most other gadgets'
    // own state is.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(922, 10, 10, 300, 300, PBString("Test"));
    pbCreateImage(460, 64, 32);
    pbCreateImage(461, 20, 20);

    CHECK(pbImageGadget(0, 10, 10, 0, 0, pbImageID(460)) == 1);
    CHECK(pbGetGadgetState(0) == pbImageID(460));

    CHECK(pbSetGadgetState(0, pbImageID(461)) == 1);
    CHECK(pbGetGadgetState(0) == pbImageID(461));

    // A harmless, oracle-verified-successful creation with no image at
    // all - leaves the GtkImage empty rather than erroring.
    CHECK(pbImageGadget(1, 10, 50, 50, 20, 0) == 1);
    CHECK(pbGetGadgetState(1) == 0);

    pbFreeImage(460);
    pbFreeImage(461);
    pbCloseWindow(922);
}

TEST_CASE("Neither ImageGadget nor ButtonImageGadget has any text capability - SetGadgetText/"
          "GetGadgetText are both genuine no-ops for each",
          "[runtime][guilib]") {
    // Oracle-verified directly: SetGadgetText(id, "caption") then
    // GetGadgetText(id) stays empty for both - unlike a plain
    // ButtonGadget. Confirmed with a dedicated, isolated probe after
    // this slice's own e2e case caught the naive approach (leaning on
    // the generic GTK_IS_BUTTON dispatch) actually destroying the
    // displayed image via gtk_button_set_label.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(923, 10, 10, 300, 300, PBString("Test"));
    pbCreateImage(462, 64, 32);

    pbImageGadget(0, 10, 10, 0, 0, pbImageID(462));
    CHECK(pbGetGadgetText(0).bytes().empty());
    CHECK(pbSetGadgetText(0, PBString("hello")) == 1); // simplified handle-ish return, see doc comment
    CHECK(pbGetGadgetText(0).bytes().empty());

    pbButtonImageGadget(1, 10, 60, 100, 30, pbImageID(462));
    CHECK(pbGetGadgetText(1).bytes().empty());
    CHECK(pbSetGadgetText(1, PBString("caption")) == 1);
    CHECK(pbGetGadgetText(1).bytes().empty());

    pbFreeImage(462);
    pbCloseWindow(923);
}

TEST_CASE("ButtonImageGadget's own GetGadgetState is a plain boolean, always 0 unless "
          "#PB_Button_Toggle was given at creation",
          "[runtime][guilib]") {
    // Oracle-verified directly: without #PB_Button_Toggle, GetGadgetState
    // always reads back 0, even after SetGadgetState(id, 1) - already
    // exactly what the pre-existing generic GTK_IS_TOGGLE_BUTTON fallback
    // gives for free for a plain (non-toggle) GtkButton. With it, state
    // behaves like CheckBoxGadget's own plain on/off toggle.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(924, 10, 10, 300, 300, PBString("Test"));
    pbCreateImage(463, 64, 32);

    pbButtonImageGadget(0, 10, 10, 100, 30, pbImageID(463));
    CHECK(pbGetGadgetState(0) == 0);
    CHECK(pbSetGadgetState(0, 1) == 0); // harmless no-op: not a toggle button
    CHECK(pbGetGadgetState(0) == 0);

    pbButtonImageGadget(1, 10, 60, 100, 30, pbImageID(463), 4); // #PB_Button_Toggle
    CHECK(pbGetGadgetState(1) == 0);
    CHECK(pbSetGadgetState(1, 1) == 1);
    CHECK(pbGetGadgetState(1) == 1);

    pbFreeImage(463);
    pbCloseWindow(924);
}

TEST_CASE("A real click (gtk_button_clicked) queues #PB_Event_Gadget with #PB_EventType_LeftClick "
          "for a ButtonImageGadget",
          "[runtime][guilib]") {
    // Not independently oracle-verified (this project's own methodology
    // deliberately avoids simulated-click probing against the real
    // oracle) - a reasonable inference by analogy with every other
    // GtkButton-based gadget, which all reuse this same onGadgetClicked
    // handler on "clicked".
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(925, 10, 10, 300, 300, PBString("Test"));
    pbCreateImage(464, 64, 32);
    pbButtonImageGadget(1, 10, 10, 100, 30, pbImageID(464));
    drainEvents();

    GtkWidget* button = detail::gadgetTable().at(1);
    gtk_button_clicked(GTK_BUTTON(button));

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbFreeImage(464);
    pbCloseWindow(925);
}

TEST_CASE("CalendarGadget round-trips a real Date() value, at midnight, through GetGadgetState/"
          "SetGadgetState",
          "[runtime][guilib]") {
    // Oracle-verified directly: GetGadgetState truncates to midnight
    // (Hour/Minute/Second all read back 0) and GetGadgetText is always
    // empty - no text representation at all. Deliberately checks only
    // round-tripped Year/Month/Day/Hour components, never a raw epoch
    // value - the same discipline the M4 Date library's own tests
    // already use (see pbCalendarGadget's own doc comment on why: real
    // PB's own GTK backend is oracle-verified UTC-based on this specific
    // machine, while this project's own Date library is local-time-based
    // by established convention - a separate, tracked discrepancy, not
    // fixed here).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(926, 10, 10, 300, 300, PBString("Test"));

    std::int64_t initial = pbDate(2020, 6, 15, 0, 0, 0);
    CHECK(pbCalendarGadget(0, 10, 10, 200, 150, initial) == 1);
    std::int64_t state = pbGetGadgetState(0);
    CHECK(pbYear(state) == 2020);
    CHECK(pbMonth(state) == 6);
    CHECK(pbDay(state) == 15);
    CHECK(pbHour(state) == 0);
    CHECK(pbGetGadgetText(0).bytes().empty());

    std::int64_t updated = pbDate(2021, 3, 9, 0, 0, 0);
    CHECK(pbSetGadgetState(0, updated) == 1);
    state = pbGetGadgetState(0);
    CHECK(pbYear(state) == 2021);
    CHECK(pbMonth(state) == 3);
    CHECK(pbDay(state) == 9);

    pbCloseWindow(926);
}

TEST_CASE("HyperLinkGadget's text round-trips; its own state is always 0", "[runtime][guilib]") {
    // Oracle-verified directly: a plain, clickable, styled-as-a-link
    // label - not a stateful toggle (always GetGadgetState 0, unlike
    // CheckBoxGadget's own plain on/off toggle).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(927, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbHyperLinkGadget(0, 10, 10, 200, 20, PBString("Click me"), pbRGB(0, 0, 255)) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "Click me");
    CHECK(pbGetGadgetState(0) == 0);

    CHECK(pbSetGadgetText(0, PBString("Renamed")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "Renamed");
    CHECK(pbGetGadgetState(0) == 0);

    pbCloseWindow(927);
}

TEST_CASE("A real click (gtk_button_clicked) queues #PB_Event_Gadget with #PB_EventType_LeftClick "
          "for a HyperLinkGadget",
          "[runtime][guilib]") {
    // Not independently oracle-verified (this project's own methodology
    // deliberately avoids simulated-click probing against the real
    // oracle) - a reasonable inference by analogy, the same as
    // ButtonImageGadget's own equivalent test before it.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(928, 10, 10, 300, 300, PBString("Test"));
    pbHyperLinkGadget(1, 10, 10, 200, 20, PBString("Click me"), pbRGB(0, 0, 255));
    drainEvents();

    GtkWidget* link = detail::gadgetTable().at(1);
    gtk_button_clicked(GTK_BUTTON(link));

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 1);
    CHECK(pbEventType() == 0); // #PB_EventType_LeftClick

    pbCloseWindow(928);
}

TEST_CASE("EditorGadget's text round-trips multi-line content; state is always 0; Flags act for real",
          "[runtime][guilib]") {
    // Oracle-verified: GetGadgetText/SetGadgetText address the buffer's
    // own full, Chr(10)-joined text; GetGadgetState is always 0 (not a
    // stateful gadget, same as HyperLinkGadget's own). ReadOnly/WordWrap
    // are acted on for real via GtkTextView's own editable/wrap-mode
    // properties.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(929, 10, 10, 300, 300, PBString("Test"));

    CHECK(pbEditorGadget(0, 10, 10, 200, 150) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "");
    CHECK(pbGetGadgetState(0) == 0);

    CHECK(pbSetGadgetText(0, PBString("Hello\nWorld")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "Hello\nWorld");
    CHECK(pbGetGadgetState(0) == 0);
    pbSetGadgetState(0, 5);
    CHECK(pbGetGadgetState(0) == 0);

    CHECK(pbEditorGadget(1, 10, 170, 200, 20, 1) == 1); // #PB_Editor_ReadOnly
    GtkWidget* readOnlyScrolled = detail::gadgetTable().at(1);
    GtkWidget* readOnlyView = detail::editorTextView(readOnlyScrolled);
    REQUIRE(readOnlyView != nullptr);
    CHECK(gtk_text_view_get_editable(GTK_TEXT_VIEW(readOnlyView)) == FALSE);

    CHECK(pbEditorGadget(2, 10, 200, 200, 20, 2) == 1); // #PB_Editor_WordWrap
    GtkWidget* wrapScrolled = detail::gadgetTable().at(2);
    GtkWidget* wrapView = detail::editorTextView(wrapScrolled);
    REQUIRE(wrapView != nullptr);
    CHECK(gtk_text_view_get_wrap_mode(GTK_TEXT_VIEW(wrapView)) == GTK_WRAP_WORD);

    pbCloseWindow(929);
}

TEST_CASE("EditorGadget's Add/Count/Clear GadgetItem family treats the buffer as a list of lines",
          "[runtime][guilib]") {
    // Oracle-verified directly, including the one edge case that doesn't
    // match a naive Chr(10)-split: a genuinely empty buffer reports
    // CountGadgetItems 0, not 1 - see detail::editorLines's own doc
    // comment.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(930, 10, 10, 300, 300, PBString("Test"));
    pbEditorGadget(0, 10, 10, 200, 150);

    CHECK(pbCountGadgetItems(0) == 0);
    CHECK(pbGetGadgetItemText(0, 0).bytes() == "");

    CHECK(pbAddGadgetItem(0, -1, PBString("Line 1")) == 1);
    CHECK(pbAddGadgetItem(0, -1, PBString("Line 2")) == 1);
    CHECK(pbAddGadgetItem(0, 0, PBString("Line 0")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "Line 0\nLine 1\nLine 2");
    CHECK(pbCountGadgetItems(0) == 3);
    CHECK(pbGetGadgetItemText(0, 0).bytes() == "Line 0");
    CHECK(pbGetGadgetItemText(0, 1).bytes() == "Line 1");
    CHECK(pbGetGadgetItemText(0, 2).bytes() == "Line 2");
    CHECK(pbGetGadgetItemText(0, 3).bytes() == "");

    pbSetGadgetText(0, PBString("A\nB\nC"));
    CHECK(pbAddGadgetItem(0, 1, PBString("Inserted")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "A\nInserted\nB\nC");

    CHECK(pbClearGadgetItems(0) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "");
    CHECK(pbCountGadgetItems(0) == 0);

    pbCloseWindow(930);
}

TEST_CASE("Real text entry (gtk_text_buffer_insert_at_cursor) queues #PB_Event_Gadget with "
          "#PB_EventType_Change for an EditorGadget; SetGadgetText stays silent",
          "[runtime][guilib]") {
    // Not independently oracle-verified which #PB_EventType_* a real
    // EditorGadget keystroke fires (this project's own methodology
    // deliberately avoids simulated-typing probing against the real
    // oracle) - reuses #PB_EventType_Change (768) by analogy with
    // StringGadget's own identical "every keystroke" test above.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(931, 10, 10, 300, 300, PBString("Test"));
    pbEditorGadget(0, 10, 10, 200, 150);
    drainEvents();

    pbSetGadgetText(0, PBString("no event"));
    drainEvents();
    CHECK(pbWindowEvent() == 0);

    GtkWidget* scrolled = detail::gadgetTable().at(0);
    GtkWidget* textView = detail::editorTextView(scrolled);
    REQUIRE(textView != nullptr);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, "typed", -1);

    CHECK(pbWindowEvent() == 3); // #PB_Event_Gadget
    CHECK(pbEventGadget() == 0);
    CHECK(pbEventType() == 768); // #PB_EventType_Change

    pbCloseWindow(931);
}

TEST_CASE("DateGadget's state keeps only the date/time components the Mask actually mentions",
          "[runtime][guilib]") {
    // Oracle-verified directly, across every combination tested: a
    // date-only mask truncates GetGadgetState to midnight (matching
    // CalendarGadget's own truncation); a mask that also mentions %hh
    // keeps the hour but still zeroes minute/second; a mask that
    // mentions %ii instead keeps the minute but zeroes hour/second -
    // every component not named by the mask resets to the Unix epoch's
    // own value for that field (1970-01-01 00:00:00), not to whatever
    // the originally-given Date happened to hold.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(932, 10, 10, 300, 300, PBString("Test"));
    std::int64_t d = pbDate(2020, 6, 15, 10, 30, 45);

    CHECK(pbDateGadget(0, 10, 10, 200, 25, PBString("%dd/%mm/%yyyy"), d) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "15/06/2020");
    CHECK(pbGetGadgetState(0) == pbDate(2020, 6, 15, 0, 0, 0));

    CHECK(pbDateGadget(1, 10, 50, 200, 25, PBString("%dd/%mm/%yyyy %hh"), d) == 1);
    CHECK(pbGetGadgetState(1) == pbDate(2020, 6, 15, 10, 0, 0));

    CHECK(pbDateGadget(2, 10, 90, 200, 25, PBString("%dd/%mm/%yyyy %ii"), d) == 1);
    CHECK(pbGetGadgetState(2) == pbDate(2020, 6, 15, 0, 30, 0));

    CHECK(pbDateGadget(3, 10, 130, 200, 25, PBString("%hh:%ii:%ss"), d) == 1);
    CHECK(pbGetGadgetState(3) == pbDate(1970, 1, 1, 10, 30, 45));

    CHECK(pbDateGadget(4, 10, 170, 200, 25, PBString("%yyyy"), d) == 1);
    CHECK(pbGetGadgetState(4) == pbDate(2020, 1, 1, 0, 0, 0));

    pbCloseWindow(932);
}

TEST_CASE("DateGadget's SetGadgetState re-filters through the same Mask; SetGadgetText is a "
          "literal-text-and-state-reset escape hatch",
          "[runtime][guilib]") {
    // Oracle-verified: SetGadgetState re-applies the exact same
    // component-filtering rule creation uses (confirmed by setting a
    // date-only-mask gadget's state to a value carrying a non-zero time
    // component and finding it truncated away again). SetGadgetText,
    // unlike every other gadget here, is a genuine escape hatch: it
    // writes the literal text given with no re-formatting, and
    // independently resets GetGadgetState to 0 regardless of what text
    // is given (confirmed with both a non-numeric and a purely numeric
    // string - state never reparses the digits as a timestamp).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(933, 10, 10, 300, 300, PBString("Test"));
    pbDateGadget(0, 10, 10, 200, 25, PBString("%dd/%mm/%yyyy"), pbDate(2020, 6, 15, 0, 0, 0));

    std::int64_t d2 = pbDate(2021, 3, 9, 14, 22, 33);
    CHECK(pbSetGadgetState(0, d2) == 1);
    CHECK(pbGetGadgetState(0) == pbDate(2021, 3, 9, 0, 0, 0));
    CHECK(pbGetGadgetText(0).bytes() == "09/03/2021");

    CHECK(pbSetGadgetText(0, PBString("12345")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "12345");
    CHECK(pbGetGadgetState(0) == 0);

    pbCloseWindow(933);
}

TEST_CASE("IPAddressGadget's SetGadgetText parses text with the real character-by-character "
          "field-entry rule, not a simple split",
          "[runtime][guilib]") {
    // Oracle-verified directly, across every combination tested: a
    // digit that would push the active field over 255 or past 3
    // characters is dropped, not the whole field; a '.' only advances
    // to the next field if the current one already has a digit (so
    // "1.abc.3.4" becomes "1.3.4", not "1..3.4" - the '.' right after
    // the still-empty second field is a no-op); any other junk
    // character (sign, letter, space, 5th '.') is simply ignored; the
    // populated fields are always a contiguous prefix.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(934, 10, 10, 300, 300, PBString("Test"));
    CHECK(pbIPAddressGadget(0, 10, 10, 200, 25) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "");
    CHECK(pbGetGadgetState(0) == 0);

    pbSetGadgetText(0, PBString("192.168.1.42"));
    CHECK(pbGetGadgetText(0).bytes() == "192.168.1.42");
    CHECK(pbGetGadgetState(0) == 704751808);

    pbSetGadgetText(0, PBString("1.2.3.4.5"));
    CHECK(pbGetGadgetText(0).bytes() == "1.2.3.45");

    pbSetGadgetText(0, PBString("1.300.3.4"));
    CHECK(pbGetGadgetText(0).bytes() == "1.30.3.4");

    pbSetGadgetText(0, PBString("1.abc.3.4"));
    CHECK(pbGetGadgetText(0).bytes() == "1.3.4");
    CHECK(pbGetGadgetState(0) == 262913);

    pbSetGadgetText(0, PBString("-1.2.3.4"));
    CHECK(pbGetGadgetText(0).bytes() == "1.2.3.4");

    pbSetGadgetText(0, PBString("1.2.3"));
    CHECK(pbGetGadgetText(0).bytes() == "1.2.3");
    CHECK(pbGetGadgetState(0) == 197121);

    pbSetGadgetText(0, PBString("..."));
    CHECK(pbGetGadgetText(0).bytes() == "");
    CHECK(pbGetGadgetState(0) == 0);

    pbCloseWindow(934);
}

TEST_CASE("IPAddressGadget's SetGadgetState packs/unpacks all 4 octets as a sign-extended, "
          "little-endian 32-bit value",
          "[runtime][guilib]") {
    // Oracle-verified: SetGadgetState always shows all 4 octets (even
    // 0.0.0.0, unlike a partially-typed SetGadgetText), and -1 round-
    // trips through "255.255.255.255" back to -1, not 4294967295 - a
    // genuinely signed 32-bit value, not unsigned.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(935, 10, 10, 300, 300, PBString("Test"));
    pbIPAddressGadget(0, 10, 10, 200, 25);

    CHECK(pbSetGadgetState(0, 0) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "0.0.0.0");
    CHECK(pbGetGadgetState(0) == 0);

    CHECK(pbSetGadgetState(0, 16777226) == 1); // 10.0.0.1
    CHECK(pbGetGadgetText(0).bytes() == "10.0.0.1");

    CHECK(pbSetGadgetState(0, -1) == 1);
    CHECK(pbGetGadgetText(0).bytes() == "255.255.255.255");
    CHECK(pbGetGadgetState(0) == -1);

    pbCloseWindow(935);
}

TEST_CASE("CanvasGadget creates cleanly, defaults every attribute to 0, and auto-opens its own "
          "gadget list only when #PB_Canvas_Container is set",
          "[runtime][guilib]") {
    // Oracle-verified: every `#PB_Canvas_Xxx` attribute reads 0 before any
    // interaction, and `SetGadgetAttribute` has no effect on any of them
    // (all seven are get-only). `#PB_Canvas_Container`'s own auto-open
    // behavior is oracle-verified too (an un-flagged Canvas's own
    // `CloseGadgetList()` raises the usual "can only be called after..."
    // fatal debugger error in real PB - not replicated here, consistent
    // with this project's own established "unsupported combination
    // harmlessly returns 0" stance elsewhere, so this only checks the
    // *positive*, flagged case).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(936, 10, 10, 300, 300, PBString("Test"));
    CHECK(pbCanvasGadget(0, 10, 10, 200, 150) == 1);
    CHECK(pbGetGadgetAttribute(0, 2) == 0); // #PB_Canvas_MouseX
    CHECK(pbGetGadgetAttribute(0, 3) == 0); // #PB_Canvas_MouseY
    CHECK(pbGetGadgetAttribute(0, 4) == 0); // #PB_Canvas_Buttons
    CHECK(pbGetGadgetAttribute(0, 5) == 0); // #PB_Canvas_Key
    CHECK(pbGetGadgetAttribute(0, 6) == 0); // #PB_Canvas_Modifiers
    CHECK(pbGetGadgetAttribute(0, 8) == 0); // #PB_Canvas_WheelDelta
    CHECK(pbSetGadgetAttribute(0, 2, 50) == 0);
    CHECK(pbGetGadgetAttribute(0, 2) == 0);
    CHECK(pbGetGadgetAttribute(999, 2) == 0); // non-existent gadget

    CHECK(pbCanvasGadget(1, 10, 170, 200, 100, 32) == 1); // #PB_Canvas_Container
    CHECK(pbCloseGadgetList() == 1);
    CHECK(pbButtonGadget(2, 10, 10, 80, 25, PBString("inside?")) == 1);
    CHECK(pbIsGadget(2) == 1);

    pbCloseWindow(936);
}

TEST_CASE("The 2D Drawing library (CanvasOutput target): Box/Circle/Plot/Point round-trip through "
          "FrontColor, and BackColor has no effect on any of them",
          "[runtime][guilib]") {
    // Oracle-verified: a freshly-created Canvas reads back RGB(255,255,255)
    // (white) everywhere via Point() before anything is drawn on it;
    // Box() defaults to FrontColor when its own trailing Color is omitted;
    // Circle() is a filled disc (a point one pixel inside its own top
    // edge still reads the fill color, not the background); BackColor
    // has no effect on any of the three drawing commands, even when set
    // immediately before a call that itself omits Color entirely.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(937, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    std::int64_t output = pbCanvasOutput(0);

    CHECK(pbStartDrawing(output) == 1);
    CHECK(pbPoint(0, 0) == 16777215); // RGB(255, 255, 255)

    pbFrontColor(pbRGB(0, 255, 0));
    pbBox(10, 10, 20, 20);
    CHECK(pbPoint(15, 15) == pbRGB(0, 255, 0));

    pbCircle(100, 75, 15, pbRGB(0, 0, 255));
    CHECK(pbPoint(100, 75) == pbRGB(0, 0, 255)); // center
    CHECK(pbPoint(100, 61) == pbRGB(0, 0, 255)); // one pixel inside the top edge - still filled
    CHECK(pbPoint(100, 50) == 16777215); // well outside - untouched

    pbBackColor(pbRGB(123, 45, 67));
    pbBox(150, 10, 10, 10, pbRGB(9, 9, 9));
    CHECK(pbPoint(155, 15) == pbRGB(9, 9, 9)); // BackColor had no effect

    pbPlot(5, 5, pbRGB(1, 2, 3));
    CHECK(pbPoint(5, 5) == pbRGB(1, 2, 3));

    pbStopDrawing();
    pbCloseWindow(937);
}

TEST_CASE("The 2D Drawing library's real fatal-error cases (nested StartDrawing, a drawing "
          "command with no active StartDrawing, an out-of-bounds Plot/Point) are harmless no-ops "
          "here instead",
          "[runtime][guilib]") {
    // Oracle-verified fatal debugger errors, none replicated here,
    // consistent with this project's established stance elsewhere:
    // a *nested* StartDrawing() (before the matching StopDrawing());
    // Box/Circle/Plot/Point/FrontColor/BackColor called with no drawing
    // active at all; Plot/Point (unlike Box/Circle, which clip silently
    // even when entirely off-canvas) given an out-of-bounds coordinate.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(938, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    std::int64_t output = pbCanvasOutput(0);

    CHECK(pbBox(0, 0, 10, 10) == 0);
    CHECK(pbFrontColor(0) == 0);
    CHECK(pbPoint(0, 0) == 0);

    CHECK(pbStartDrawing(output) == 1);
    CHECK(pbStartDrawing(output) == 0); // nested

    CHECK(pbPlot(-5, -5) == 0);
    CHECK(pbPlot(1000, 1000) == 0);
    CHECK(pbPoint(-5, -5) == 0);
    CHECK(pbPoint(1000, 1000) == 0);

    // Box/Circle, by contrast, clip silently even when entirely off-canvas.
    CHECK(pbBox(1000, 1000, 30, 30, pbRGB(10, 20, 30)) == 0);
    CHECK(pbCircle(1000, 1000, 15, pbRGB(1, 2, 3)) == 0);

    pbStopDrawing();
    CHECK(pbStopDrawing() == 0); // already stopped

    pbCloseWindow(938);
}

TEST_CASE("ShortcutGadget creates cleanly, matching every real GTK/gadget-table convention - "
          "its own shortcut activation is not implemented (see pbShortcutGadget's own doc "
          "comment for why)",
          "[runtime][guilib]") {
    // Oracle-verified: creation itself, and IsGadget/FreeGadget via the
    // same shared gadgetTable() every other gadget already uses, both
    // work. Every #PB_Shortcut_*/#PB_GadgetType_Shortcut constant is
    // also oracle-verified (checked separately, in sema's own constant
    // table tests) - this case is about the gadget itself.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(939, 10, 10, 320, 320, PBString("Test"));
    CHECK(pbShortcutGadget(0, 50, 60, 70, 80, 65470 /* #PB_Shortcut_F1 */) == 1);
    CHECK(pbIsGadget(0) == 1);
    pbFreeGadget(0);
    CHECK(pbIsGadget(0) == 0);

    pbCloseWindow(939);
}

TEST_CASE("Line/LineXY: Line is delta-based and draws nothing at all for a zero Width or Height; "
          "LineXY uses two absolute endpoints and has no such quirk",
          "[runtime][guilib]") {
    // Oracle-verified directly: a Line call with Width or Height exactly
    // 0 draws nothing anywhere in a wide neighborhood of the attempted
    // line - a real PB quirk, reproduced as-is. LineXY has no such
    // quirk - a horizontal/vertical LineXY (which would also have a
    // zero "delta" if it worked the same way) draws correctly,
    // including its own far endpoint pixel. Both default their own
    // trailing Color to the ambient FrontColor when omitted, and clip
    // silently off-canvas (confirmed separately below).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(940, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    pbStartDrawing(pbCanvasOutput(0));

    pbLine(10, 10, 20, 0, pbRGB(1, 2, 3));
    CHECK(pbPoint(15, 10) == 16777215); // zero Height: nothing drawn at all
    pbLine(10, 30, 0, 20, pbRGB(1, 2, 3));
    CHECK(pbPoint(10, 35) == 16777215); // zero Width: nothing drawn at all

    pbFrontColor(pbRGB(9, 99, 199));
    pbLine(50, 50, 30, 30);
    CHECK(pbPoint(50, 50) == pbRGB(9, 99, 199)); // defaults to ambient FrontColor

    pbLineXY(100, 100, 150, 100, pbRGB(5, 15, 25));
    CHECK(pbPoint(125, 100) == pbRGB(5, 15, 25));
    CHECK(pbPoint(150, 100) == pbRGB(5, 15, 25)); // far endpoint is included too

    CHECK(pbLine(190, 140, 50, 50, pbRGB(1, 1, 1)) == 0);
    CHECK(pbLineXY(0, 149, -1000, 1000, pbRGB(2, 2, 2)) == 0);
    CHECK(pbPoint(195, 145) == pbRGB(1, 1, 1)); // Line clipped silently, not a crash
    CHECK(pbPoint(0, 149) == pbRGB(2, 2, 2)); // LineXY clipped silently too

    pbStopDrawing();
    pbCloseWindow(940);
}

TEST_CASE("DrawText paints an opaque BackColor background (defaulting to black, not the ambient "
          "one) behind FrontColor text (also defaulting to black); TextWidth/TextHeight return "
          "real Double measurements",
          "[runtime][guilib]") {
    // Oracle-verified directly: FrontColor/BackColor passed to DrawText
    // are independent of the ambient FrontColor/BackColor commands -
    // both default to plain black when omitted, not whatever the
    // ambient one currently is (confirmed: setting the ambient
    // FrontColor first, then calling DrawText with neither argument,
    // still reads back solid black). The background fill is opaque,
    // covering the full TextWidth x TextHeight box (confirmed: a
    // distinct color drawn underneath first is fully overwritten).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(941, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    pbStartDrawing(pbCanvasOutput(0));

    CHECK(pbTextWidth(PBString("")) == 0.0);
    CHECK(pbTextHeight(PBString("")) == 0.0);
    double w = pbTextWidth(PBString("A"));
    double h = pbTextHeight(PBString("A"));
    CHECK(w > 0.0);
    CHECK(h > 0.0);
    CHECK(pbTextWidth(PBString("AB")) > w); // a second character widens it

    pbBox(50, 50, 40, 40, pbRGB(200, 200, 200));
    pbDrawText(50, 50, PBString("A"));
    CHECK(pbPoint(50, 50) == 0); // opaque black background, not the gray box underneath

    pbFrontColor(pbRGB(111, 112, 113));
    pbDrawText(100, 50, PBString("A"));
    CHECK(pbPoint(100, 50) == 0); // still black - FrontColor doesn't inherit the ambient one

    pbDrawText(150, 50, PBString("A"), pbRGB(255, 0, 0), pbRGB(0, 0, 255));
    CHECK(pbPoint(150, 50) == pbRGB(0, 0, 255)); // explicit BackColor used instead

    pbStopDrawing();
    pbCloseWindow(941);
}

TEST_CASE("Ellipse/RoundBox: filled curves with a crisp, oracle-matched boundary at the confirmed "
          "axis extent, both defaulting their own trailing Color to the ambient FrontColor",
          "[runtime][guilib]") {
    // Oracle-verified directly, along both axes independently: Ellipse's
    // own exact boundary pixel at x +/- xRadius and y +/- yRadius is
    // itself still filled, one pixel further out is not - confirmed
    // with a perfectly crisp edge (no blended pixel at all, unlike an
    // earlier Cairo-arc-based attempt at this). RoundBox cuts its own 4
    // corners away by a quarter-ellipse of the given corner dimensions
    // - confirmed a corner pixel reads back as untouched background,
    // while the flat portions of every edge (including the degenerate
    // case where a corner dimension is exactly half the box's own
    // width/height, where an earlier version of this had a real 1-pixel
    // seam bug) are filled solidly, the same as a plain Box.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(946, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    pbStartDrawing(pbCanvasOutput(0));

    pbEllipse(50, 50, 30, 20, pbRGB(1, 2, 3));
    CHECK(pbPoint(50, 50) == pbRGB(1, 2, 3)); // center
    CHECK(pbPoint(80, 50) == pbRGB(1, 2, 3)); // exact right-edge boundary - still filled
    CHECK(pbPoint(81, 50) == 16777215); // one pixel further out - not filled
    CHECK(pbPoint(50, 70) == pbRGB(1, 2, 3)); // exact bottom-edge boundary - still filled
    CHECK(pbPoint(50, 71) == 16777215); // one pixel further out - not filled
    CHECK(pbPoint(10, 10) == 16777215); // well outside - untouched

    pbFrontColor(pbRGB(11, 12, 13));
    pbEllipse(150, 50, 20, 20);
    CHECK(pbPoint(150, 50) == pbRGB(11, 12, 13)); // defaults to ambient FrontColor

    CHECK(pbEllipse(1000, 1000, 10, 10, pbRGB(2, 2, 2)) == 0); // clips silently off-canvas

    // A degenerate RoundBox where CornerHeight is exactly half the
    // box's own Height - the exact case the earlier 1-pixel seam bug
    // needed to tile its own corner/flat bands without a gap.
    pbRoundBox(10, 10, 40, 20, 10, 10, pbRGB(4, 5, 6));
    CHECK(pbPoint(30, 20) == pbRGB(4, 5, 6)); // center
    CHECK(pbPoint(10, 10) == 16777215); // top-left corner, cut away
    CHECK(pbPoint(49, 29) == 16777215); // bottom-right corner, cut away
    CHECK(pbPoint(25, 10) == pbRGB(4, 5, 6)); // top edge, flat region
    CHECK(pbPoint(10, 20) == pbRGB(4, 5, 6)); // left edge, right at the degenerate seam
    CHECK(pbPoint(50, 10) == 16777215); // just outside the box entirely

    pbFrontColor(pbRGB(21, 22, 23));
    pbRoundBox(70, 10, 40, 20, 10, 10);
    CHECK(pbPoint(90, 20) == pbRGB(21, 22, 23)); // defaults to ambient FrontColor

    CHECK(pbRoundBox(1000, 1000, 10, 10, 2, 2, pbRGB(3, 3, 3)) == 0); // clips silently off-canvas

    pbStopDrawing();
    pbCloseWindow(946);
}

TEST_CASE("FillArea is a raster-level boundary fill (replacing every reachable pixel regardless "
          "of its own original color), not a classic start-color-matching flood fill, stopping "
          "only at a pixel that already matches BorderColor",
          "[runtime][guilib]") {
    // Oracle-verified directly, with the argument order confirmed the
    // hard way: a first attempt at this with the two trailing color
    // arguments the other way around (FillColor before BorderColor)
    // flooded the *entire* reachable canvas with the "FillColor"
    // argument, since nothing on the canvas ever matched that
    // argument's own real role (the border/stop color) - the real
    // signature is FillArea(x, y, BorderColor [, FillColor]), FillColor
    // defaulting to the ambient FrontColor when omitted (BorderColor
    // has no default - a 2-argument call is a real compile error).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(947, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    pbStartDrawing(pbCanvasOutput(0));

    pbBox(10, 10, 50, 50, pbRGB(0, 0, 0)); // black outer box acts as the boundary
    pbBox(20, 20, 20, 20, pbRGB(255, 255, 255)); // white inner region to fill
    CHECK(pbFillArea(25, 25, pbRGB(0, 0, 0), pbRGB(9, 8, 7)) == 0);
    CHECK(pbPoint(25, 25) == pbRGB(9, 8, 7)); // filled
    CHECK(pbPoint(39, 39) == pbRGB(9, 8, 7)); // filled, near the inner region's own edge
    CHECK(pbPoint(15, 15) == 0); // the black boundary itself - untouched
    CHECK(pbPoint(100, 100) == 16777215); // far outside everything - untouched background

    pbFrontColor(pbRGB(31, 32, 33));
    pbFillArea(25, 25, pbRGB(0, 0, 0)); // FillColor omitted - defaults to ambient FrontColor
    CHECK(pbPoint(25, 25) == pbRGB(31, 32, 33));

    pbStopDrawing();
    pbCloseWindow(947);
}

TEST_CASE("DrawingMode is an ambient setting, like FrontColor/BackColor, changing how every "
          "other drawing command composites its own color against the canvas - Outlined draws "
          "only a shape's own boundary, XOr bitwise-XORs against the destination, AlphaBlend "
          "does a standard alpha composite using the color's own top byte, and Transparent "
          "skips DrawText's own background fill specifically",
          "[runtime][guilib]") {
    // Oracle-verified directly, each independently - see pbDrawingMode's
    // own doc comment in guilib.hpp for the full set of findings.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    pbOpenWindow(948, 10, 10, 320, 320, PBString("Test"));
    pbCanvasGadget(0, 10, 10, 200, 150);
    pbStartDrawing(pbCanvasOutput(0));

    // Outlined: Box draws only its own boundary, not the filled interior.
    CHECK(pbDrawingMode(4) == 0); // #PB_2DDrawing_Outlined
    pbBox(10, 10, 20, 20, pbRGB(1, 2, 3));
    CHECK(pbPoint(10, 10) == pbRGB(1, 2, 3)); // corner - on the boundary
    CHECK(pbPoint(15, 15) == 16777215); // center - interior, skipped

    // Outlined has no effect on Plot (a single pixel has no interior).
    pbPlot(50, 50, pbRGB(9, 9, 9));
    CHECK(pbPoint(50, 50) == pbRGB(9, 9, 9));

    // DrawingMode is ambient - persists across calls with no new DrawingMode() in between.
    pbBox(50, 10, 20, 20, pbRGB(4, 5, 6));
    CHECK(pbPoint(55, 15) == 16777215); // still outline-only

    // XOr: a plain per-channel bitwise XOR against the existing destination pixel.
    pbDrawingMode(0); // #PB_2DDrawing_Default
    pbBox(10, 50, 5, 5, pbRGB(255, 0, 0));
    pbDrawingMode(2); // #PB_2DDrawing_XOr
    pbBox(10, 50, 5, 5, pbRGB(255, 255, 255));
    CHECK(pbPoint(12, 52) == pbRGB(0, 255, 255)); // (255,0,0) XOR (255,255,255) = cyan

    // AlphaBlend: standard "source over destination" using the color's own top byte as alpha -
    // RGB() always packs alpha 0 (fully transparent), RGBA() sets it explicitly.
    pbDrawingMode(0);
    pbBox(50, 50, 20, 20, pbRGB(200, 0, 0));
    pbDrawingMode(16); // #PB_2DDrawing_AlphaBlend
    pbBox(50, 50, 20, 20, pbRGB(0, 200, 0)); // alpha 0 - fully invisible
    CHECK(pbPoint(55, 55) == pbRGB(200, 0, 0));
    pbBox(50, 50, 20, 20, pbRGBA(0, 200, 0, 255)); // alpha 255 - fully opaque
    CHECK(pbPoint(55, 55) == pbRGB(0, 200, 0));
    pbBox(90, 50, 20, 20, pbRGB(200, 0, 0));
    pbBox(90, 50, 20, 20, pbRGBA(50, 60, 70, 100)); // partial alpha - truncating-integer blend
    std::int64_t blended = pbPoint(95, 55);
    CHECK(pbRed(blended) == 174);
    CHECK(pbGreen(blended) == 178);
    CHECK(pbBlue(blended) == 182);

    // Transparent: no effect on Box - still opaque, same as Default.
    pbDrawingMode(1); // #PB_2DDrawing_Transparent
    pbBox(130, 50, 20, 20, pbRGB(7, 7, 7));
    CHECK(pbPoint(135, 55) == pbRGB(7, 7, 7));

    // Transparent's real effect: skips DrawText's own background fill entirely.
    pbDrawingMode(0);
    pbBox(10, 90, 40, 40, pbRGB(50, 60, 70));
    pbDrawingMode(1);
    pbDrawText(15, 95, PBString("A"), pbRGB(0, 0, 0), pbRGB(9, 9, 9));
    CHECK(pbPoint(15, 95) == pbRGB(50, 60, 70)); // background fill skipped - gray box still visible

    // A new StartDrawing resets DrawingMode back to Default, like FrontColor/BackColor.
    pbStopDrawing();
    pbStartDrawing(pbCanvasOutput(0));
    pbBox(10, 10, 20, 20, pbRGB(8, 8, 8));
    CHECK(pbPoint(15, 15) == pbRGB(8, 8, 8)); // filled, not outline-only

    pbStopDrawing();
    pbCloseWindow(948);
}

TEST_CASE("ExplorerListGadget lists a directory's own entries - directories always first, "
          "each group in raw directory-entry order, prefixed by a fixed '..' - with "
          "GetGadgetItemText/GetGadgetItemState (a plain file/directory type code, not a "
          "selection bitmask) and no real selection querying at all",
          "[runtime][guilib]") {
    // Oracle-verified directly, across several independently-decoupled
    // probes: "sort order" isn't a deliberate sort by name or
    // modification time at all - it's whatever raw order the OS's own
    // directory enumeration happens to return, grouped directories-
    // first. GetGadgetText/GetGadgetState/SetGadgetState are all red
    // herrings for selection (GetGadgetText always reports the browsed
    // directory path itself; GetGadgetState stays a fixed -1 regardless
    // of SetGadgetState). GetGadgetItemState's own "2"/"1" values are
    // #PB_Explorer_Directory/#PB_Explorer_File - a plain, fixed type
    // indicator, not a ListIconGadget-style Selected/Checked bitmask
    // (despite the coincidentally-matching "2").
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "pbcxx_explorerlist_test";
    fs::remove_all(dir);
    fs::create_directories(dir / "zzz_dir");
    fs::create_directories(dir / "aaa_dir");
    { std::ofstream(dir / "mmm_file.txt"); }
    { std::ofstream(dir / "bbb_file.txt"); }
    { std::ofstream(dir / ".hidden_file.txt"); }

    pbOpenWindow(942, 10, 10, 320, 320, PBString("Test"));
    CHECK(pbExplorerListGadget(0, 10, 10, 200, 150, PBString(dir.string() + "/")) == 1);

    std::int64_t n = pbCountGadgetItems(0);
    REQUIRE(n == 5); // ".." + 2 real dirs + 2 real (non-hidden) files
    CHECK(pbGetGadgetItemText(0, 0).bytes() == "..");
    CHECK(pbGetGadgetItemState(0, 0) == 2); // #PB_Explorer_Directory

    // Directories always come before files - confirmed not to be a
    // deliberate sort by name or modification time at all, just
    // whatever raw order the OS's own directory enumeration happens to
    // return (confirmed directly: the real oracle itself gives a
    // *different* intra-group order on this project's own ext4-backed
    // dev directory than it does right here, under /tmp's tmpfs - this
    // test only asserts the one filesystem-independent invariant, not a
    // specific intra-group order that would be a coincidence of
    // whichever filesystem happens to back the test runner's own /tmp).
    std::vector<std::string> names;
    std::vector<std::int64_t> states;
    for (std::int64_t i = 0; i < n; ++i) {
        names.push_back(pbGetGadgetItemText(0, i).bytes());
        states.push_back(pbGetGadgetItemState(0, i));
    }
    CHECK(names[0] == "..");
    CHECK(states == std::vector<std::int64_t>{2, 2, 2, 1, 1}); // both dirs, then both files
    std::vector<std::string> dirNames(names.begin() + 1, names.begin() + 3);
    std::vector<std::string> fileNames(names.begin() + 3, names.end());
    std::sort(dirNames.begin(), dirNames.end());
    std::sort(fileNames.begin(), fileNames.end());
    CHECK(dirNames == std::vector<std::string>{"aaa_dir", "zzz_dir"});
    CHECK(fileNames == std::vector<std::string>{"bbb_file.txt", "mmm_file.txt"});

    CHECK(pbGetGadgetText(0).bytes() == dir.string() + "/");
    CHECK(pbGetGadgetState(0) == -1);
    CHECK(pbSetGadgetState(0, 0) == 0);
    CHECK(pbGetGadgetState(0) == -1); // still a dead end afterward
    CHECK(pbSetGadgetItemState(0, 1, 99) == 0);
    CHECK(pbGetGadgetItemState(0, 1) == 2); // unaffected

    // #PB_Explorer_HiddenFiles (262144): the dotfile now shows up too.
    CHECK(pbExplorerListGadget(1, 10, 170, 200, 100, PBString(dir.string() + "/"), 262144) == 1);
    CHECK(pbCountGadgetItems(1) == 6);

    // #PB_Explorer_NoFiles (2) / NoFolders (128) / NoParentFolder (64).
    CHECK(pbExplorerListGadget(2, 10, 170, 200, 100, PBString(dir.string() + "/"), 2) == 1);
    CHECK(pbCountGadgetItems(2) == 3); // ".." + 2 dirs, no files
    CHECK(pbExplorerListGadget(3, 10, 170, 200, 100, PBString(dir.string() + "/"), 128) == 1);
    CHECK(pbCountGadgetItems(3) == 3); // ".." + 2 files, no dirs
    CHECK(pbExplorerListGadget(4, 10, 170, 200, 100, PBString(dir.string() + "/"), 64) == 1);
    CHECK(pbCountGadgetItems(4) == 4); // no "..", 2 dirs + 2 files

    // SetGadgetText re-browses to a new directory.
    fs::path dir2 = fs::temp_directory_path() / "pbcxx_explorerlist_test2";
    fs::remove_all(dir2);
    fs::create_directories(dir2);
    { std::ofstream(dir2 / "only_file.txt"); }
    pbSetGadgetText(0, PBString(dir2.string() + "/"));
    CHECK(pbGetGadgetText(0).bytes() == dir2.string() + "/");
    CHECK(pbCountGadgetItems(0) == 2); // ".." + only_file.txt

    pbCloseWindow(942);
    fs::remove_all(dir);
    fs::remove_all(dir2);
}

TEST_CASE("ExplorerListGadget's own trailing-slash sensitivity: without one, the parent is "
          "listed with the last path component filtering only the file half, never directories",
          "[runtime][guilib]") {
    // Oracle-verified directly (three decoupled cases): a name matching
    // a real directory, one matching nothing at all, and one matching a
    // real file - every subdirectory of the parent is listed in full
    // regardless, while files only ever show up if the given name
    // matches one of them exactly.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "pbcxx_explorerlist_slash_test";
    fs::remove_all(dir);
    fs::create_directories(dir / "subdir");
    { std::ofstream(dir / "real_file.txt"); }
    { std::ofstream(dir / "other_file.txt"); }

    pbOpenWindow(943, 10, 10, 320, 320, PBString("Test"));

    // No trailing slash, last component names a real directory.
    CHECK(pbExplorerListGadget(0, 10, 10, 200, 150, PBString((dir / "subdir").string())) == 1);
    CHECK(pbCountGadgetItems(0) == 2); // ".." + "subdir" - no files at all

    // No trailing slash, last component matches nothing.
    CHECK(pbExplorerListGadget(1, 10, 170, 200, 100, PBString((dir / "nope").string())) == 1);
    CHECK(pbCountGadgetItems(1) == 2); // ".." + "subdir" - still no files

    // No trailing slash, last component matches a real file exactly.
    CHECK(pbExplorerListGadget(2, 10, 170, 200, 100, PBString((dir / "real_file.txt").string())) == 1);
    CHECK(pbCountGadgetItems(2) == 3); // ".." + "subdir" + "real_file.txt" only

    pbCloseWindow(943);
    fs::remove_all(dir);
}

TEST_CASE("ExplorerComboGadget is a plain text display of the browsed directory - no "
          "PB-queryable item list at all, and its own GetGadgetState default differs from "
          "ExplorerListGadget's own",
          "[runtime][guilib]") {
    // Oracle-verified directly: CountGadgetItems/GetGadgetItemText/
    // GetGadgetItemState all stay 0/empty/0 - real PB's own API can't
    // query ExplorerComboGadget's own dropdown contents either.
    // GetGadgetState is a fixed 0 - a *different* fixed value than
    // ExplorerListGadget's own -1.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "pbcxx_explorercombo_test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    pbOpenWindow(944, 10, 10, 320, 320, PBString("Test"));
    CHECK(pbExplorerComboGadget(0, 10, 10, 200, 25, PBString(dir.string() + "/")) == 1);
    CHECK(pbGetGadgetText(0).bytes() == dir.string() + "/");
    CHECK(pbCountGadgetItems(0) == 0);
    CHECK(pbGetGadgetItemText(0, 0).bytes() == "");
    CHECK(pbGetGadgetItemState(0, 0) == 0);
    CHECK(pbGetGadgetState(0) == 0);

    fs::path dir2 = fs::temp_directory_path() / "pbcxx_explorercombo_test2";
    fs::remove_all(dir2);
    fs::create_directories(dir2);
    pbSetGadgetText(0, PBString(dir2.string() + "/"));
    CHECK(pbGetGadgetText(0).bytes() == dir2.string() + "/");

    pbCloseWindow(944);
    fs::remove_all(dir);
    fs::remove_all(dir2);
}

TEST_CASE("ExplorerTreeGadget's own PB-level API surface is almost entirely a dead end - "
          "CountGadgetItems/GetGadgetItemText/GetGadgetItemState stay 0/empty/0 regardless of "
          "real content, and GetGadgetState is a fixed 2, distinct from its two siblings' own",
          "[runtime][guilib]") {
    // Oracle-verified directly, against several different real,
    // populated directories, not just an empty one. GetGadgetText/
    // SetGadgetText have their own simpler normalization rule than
    // ExplorerListGadget's own: always ensure a trailing slash, and
    // fall back to "/" (not the given path's own parent) if the
    // resulting path isn't a real, existing directory.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "pbcxx_explorertree_test";
    fs::remove_all(dir);
    fs::create_directories(dir / "subdir_a");
    fs::create_directories(dir / "subdir_b");
    { std::ofstream(dir / "a_file.txt"); }

    pbOpenWindow(945, 10, 10, 320, 320, PBString("Test"));
    CHECK(pbExplorerTreeGadget(0, 10, 10, 200, 150, PBString(dir.string())) == 1);

    // Trailing slash added even though it was omitted above.
    CHECK(pbGetGadgetText(0).bytes() == dir.string() + "/");
    CHECK(pbGetGadgetState(0) == 2);
    CHECK(pbSetGadgetState(0, 0) == 0);
    CHECK(pbGetGadgetState(0) == 2); // unaffected

    // A real, populated directory still reports no items via the
    // generic item-access API at all.
    CHECK(pbCountGadgetItems(0) == 0);
    CHECK(pbGetGadgetItemText(0, 0).bytes() == "");
    CHECK(pbGetGadgetItemState(0, 0) == 0);

    // A nonexistent directory falls back to "/", not its own parent.
    CHECK(pbExplorerTreeGadget(1, 10, 170, 200, 100, PBString("/nonexistent/path/at/all/")) == 1);
    CHECK(pbGetGadgetText(1).bytes() == "/");

    // SetGadgetText re-browses through the same normalization rule.
    fs::path dir2 = fs::temp_directory_path() / "pbcxx_explorertree_test2";
    fs::remove_all(dir2);
    fs::create_directories(dir2);
    pbSetGadgetText(0, PBString(dir2.string()));
    CHECK(pbGetGadgetText(0).bytes() == dir2.string() + "/");
    pbSetGadgetText(0, PBString("/nonexistent/again/"));
    CHECK(pbGetGadgetText(0).bytes() == "/");

    pbCloseWindow(945);
    fs::remove_all(dir);
    fs::remove_all(dir2);
}
