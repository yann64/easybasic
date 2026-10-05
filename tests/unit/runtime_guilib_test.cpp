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
    CHECK_FALSE(result.bytes().empty());
    CHECK(result.bytes().back() == '/');
}
