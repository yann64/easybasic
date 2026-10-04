#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <thread>
#include <unordered_map>

#include <gtk/gtk.h>

#include "pbstring.hpp"

namespace easybasic::runtime {

/// M7b's first GUI slice: `OpenWindow`/`CloseWindow`/`IsWindow`/
/// `ResizeWindow`/`HideWindow`, `WindowEvent`/`WaitWindowEvent`,
/// `EventWindow`/`EventGadget`/`EventType`. Calls GTK3 directly from
/// generated C++ - no PureBasic subsystem ABI, no `.pbl` packaging, just
/// plain library calls, per this project's own stated design stance (see
/// the M7 roadmap notes). GTK3 itself is lazily initialized on first real
/// use (`ensureGtkInit`), so a program that never calls a GUI function
/// never touches GTK at runtime or needs a display at all - `main.cpp`'s
/// own driver mirrors this at the build level, only adding GTK3's
/// compile/link flags when `Sema::usesGuiLibrary()` says the program
/// actually needs them.
///
/// **The central design problem this header solves**: PB's own GUI API is
/// *poll-based* (`WaitWindowEvent()` blocks and returns what happened),
/// but GTK3's is *callback-based* (a signal fires asynchronously while
/// pumping the main loop). Bridged here with a plain FIFO queue
/// (`detail::eventQueue()`): every GTK signal handler this header
/// registers just pushes a `PBWindowEvent` onto the queue and returns,
/// and `pbWindowEvent`/`pbWaitWindowEvent` are the only things that ever
/// pump GTK's own main loop (`gtk_main_iteration()`) and pop from it.
namespace detail {

struct PBWindowEvent {
    std::int64_t type = 0;
    std::int64_t windowId = 0;
    std::int64_t gadgetId = 0;
    std::int64_t eventType = 0;
};

inline std::deque<PBWindowEvent>& eventQueue() {
    static std::deque<PBWindowEvent> queue;
    return queue;
}

inline PBWindowEvent& currentEvent() {
    static PBWindowEvent current;
    return current;
}

inline std::unordered_map<std::int64_t, GtkWidget*>& windowTable() {
    static std::unordered_map<std::int64_t, GtkWidget*> table;
    return table;
}

/// One `GtkFixed` per window, holding every gadget placed into it. `GtkFixed`
/// is the natural fit for PB's own gadget model: gadgets are positioned by
/// absolute window-relative pixel coordinates, exactly what `gtk_fixed_put`
/// provides (unlike GTK's usual box/grid layout managers, which don't take
/// explicit positions at all).
inline std::unordered_map<std::int64_t, GtkWidget*>& windowFixedTable() {
    static std::unordered_map<std::int64_t, GtkWidget*> table;
    return table;
}

inline std::unordered_map<std::int64_t, GtkWidget*>& gadgetTable() {
    static std::unordered_map<std::int64_t, GtkWidget*> table;
    return table;
}

/// Oracle-verified: gadget-creation functions (`ButtonGadget`, etc.) take
/// *no* window parameter at all - PB places a new gadget into whichever
/// window was *most recently opened* (confirmed directly: with two windows
/// open, a gadget created after both lands in the second one, not the
/// first). `UseGadgetList` (for explicitly retargeting this, e.g. to add a
/// gadget to an earlier window) is real PB's own escape hatch for this -
/// not implemented in this first gadget slice, a deliberately deferred gap
/// like `#PB_Window_*` flags were for the first GUI slice.
inline std::int64_t& activeWindowId() {
    static std::int64_t id = 0;
    return id;
}

inline void ensureGtkInit() {
    // A plain bool guarded by a function-local static's own thread-safe
    // initialization (C++11 "magic statics") - gtk_init itself must only
    // ever run once per process.
    static bool initialized = [] {
        int argc = 0;
        char** argv = nullptr;
        gtk_init(&argc, &argv);
        return true;
    }();
    (void)initialized;
}

// Tags for g_object_set_data - lets a GTK signal handler recover which PB
// window ID (and this project's own move/resize-tracking state) a given
// GtkWidget* belongs to, without a second id->widget-reversed lookup table.
inline const char* windowIdKey() { return "pbcxx-window-id"; }
inline const char* windowStateKey() { return "pbcxx-window-state"; }
// Same idea, for gadgets: a gadget's own PB ID, and the ID of the window it
// was placed into (needed so a gadget event can also set `EventWindow()` -
// oracle-verified that it does, not just `EventGadget()`).
inline const char* gadgetIdKey() { return "pbcxx-gadget-id"; }
inline const char* gadgetWindowIdKey() { return "pbcxx-gadget-window-id"; }

/// The last known position/size for one window - `configure-event` fires
/// for *any* geometry change without saying which part changed, so this is
/// diffed against on every event to decide whether to queue
/// `#PB_Event_MoveWindow`, `#PB_Event_SizeWindow`, or (if both actually
/// changed at once) both.
struct WindowState {
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t width = 0;
    std::int64_t height = 0;
};

inline gboolean onDeleteEvent(GtkWidget* widget, GdkEvent*, gpointer) {
    auto id = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(widget), windowIdKey()));
    PBWindowEvent ev;
    ev.type = 2; // #PB_Event_CloseWindow
    ev.windowId = id;
    eventQueue().push_back(ev);
    // Oracle-verified: the window stays open (and IsWindow() stays truthy)
    // until the PB program's own CloseWindow() call - returning TRUE here
    // tells GTK not to destroy the widget itself in response to the user's
    // own close-button click.
    return TRUE;
}

inline gboolean onConfigureEvent(GtkWidget* widget, const GdkEventConfigure* event, gpointer) {
    auto id = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(widget), windowIdKey()));
    auto* state = static_cast<WindowState*>(g_object_get_data(G_OBJECT(widget), windowStateKey()));
    if (state == nullptr) {
        return FALSE;
    }
    bool moved = event->x != state->x || event->y != state->y;
    bool resized = event->width != state->width || event->height != state->height;
    state->x = event->x;
    state->y = event->y;
    state->width = event->width;
    state->height = event->height;
    if (moved) {
        PBWindowEvent ev;
        ev.type = 5; // #PB_Event_MoveWindow
        ev.windowId = id;
        eventQueue().push_back(ev);
    }
    if (resized) {
        PBWindowEvent ev;
        ev.type = 6; // #PB_Event_SizeWindow
        ev.windowId = id;
        eventQueue().push_back(ev);
    }
    return FALSE; // Let GTK's own default configure handling still run too.
}

inline gboolean onDraw(GtkWidget* widget, cairo_t*, gpointer) {
    auto id = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(widget), windowIdKey()));
    PBWindowEvent ev;
    ev.type = 4; // #PB_Event_Repaint
    ev.windowId = id;
    eventQueue().push_back(ev);
    return FALSE; // Let GTK still actually paint the window.
}

/// Pushes a `#PB_Event_Gadget` event for `widget`, reading its own gadget/
/// window IDs back from `g_object_set_data` (set in `placeGadget`) - shared
/// by every gadget signal handler below, which differ only in which real
/// GTK signal they're connected to and which `eventType` they report.
inline void queueGadgetEvent(GtkWidget* widget, std::int64_t eventType) {
    auto gadgetId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(widget), gadgetIdKey()));
    auto windowId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(widget), gadgetWindowIdKey()));
    PBWindowEvent ev;
    ev.type = 3; // #PB_Event_Gadget
    ev.windowId = windowId;
    ev.gadgetId = gadgetId;
    ev.eventType = eventType;
    eventQueue().push_back(ev);
}

/// `GtkButton::clicked` and `GtkToggleButton::toggled` are both plain `void`
/// signals (no return value to give GTK back) - oracle-verified that a
/// real button click *and* a real checkbox toggle both report
/// `#PB_EventType_LeftClick` (`0`), not `#PB_EventType_Change`, as their
/// `EventType()`.
inline void onGadgetClicked(GtkWidget* widget, gpointer) { queueGadgetEvent(widget, 0); }

/// `GtkEditable::changed` (used by `GtkEntry`, i.e. `StringGadget`) fires on
/// every content change - oracle-verified each keystroke queues its own
/// `#PB_Event_Gadget` with `#PB_EventType_Change` (`768`).
inline void onGadgetChanged(GtkWidget* widget, gpointer) { queueGadgetEvent(widget, 768); }

/// Shared by `pbCloseWindow` and `pbOpenWindow`'s own "re-using an already-
/// open ID destroys the old window" branch. Oracle-verified: closing a
/// window implicitly frees all of its gadgets too (`IsGadget` on a gadget
/// that belonged to a just-closed window returns `0`) - `gtk_widget_destroy`
/// on the window already recursively destroys the `GtkFixed` and every
/// gadget GTK-side, but `gadgetTable()` itself would otherwise be left
/// holding dangling pointers, so every gadget actually owned by this window
/// is scanned for and erased first.
inline void destroyWindow(std::int64_t windowId, GtkWidget* window) {
    auto* state = static_cast<WindowState*>(g_object_get_data(G_OBJECT(window), windowStateKey()));
    delete state;
    for (auto it = gadgetTable().begin(); it != gadgetTable().end();) {
        auto owner = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(it->second), gadgetWindowIdKey()));
        it = (owner == windowId) ? gadgetTable().erase(it) : std::next(it);
    }
    gtk_widget_destroy(window);
}

/// Shared by every gadget-creation function: places `widget` into
/// `windowId`'s own `GtkFixed` at the given position/size, tags it with its
/// PB gadget ID and owning window ID (read back by `queueGadgetEvent` and
/// `destroyWindow`), and records it in `gadgetTable()`. Returns `false`
/// (and destroys `widget` again without placing it) if `windowId` isn't a
/// currently open window - a real PB program is very unlikely to reference
/// one deliberately, and this project's own established stance only
/// replicates real PB's debugger-fatal-error behavior for cases already
/// found worth the trouble (see M7a's `KillThread` notes).
inline bool placeGadget(std::int64_t windowId, std::int64_t gadgetId, std::int64_t x, std::int64_t y,
                         std::int64_t width, std::int64_t height, GtkWidget* widget) {
    auto fixedIt = windowFixedTable().find(windowId);
    if (fixedIt == windowFixedTable().end()) {
        gtk_widget_destroy(widget);
        return false;
    }
    // Oracle-verified elsewhere (OpenWindow): re-using an already-live ID
    // replaces the old widget rather than erroring - applied the same way
    // here for consistency.
    auto old = gadgetTable().find(gadgetId);
    if (old != gadgetTable().end()) {
        gtk_widget_destroy(old->second);
    }
    g_object_set_data(G_OBJECT(widget), gadgetIdKey(), reinterpret_cast<gpointer>(gadgetId));
    g_object_set_data(G_OBJECT(widget), gadgetWindowIdKey(), reinterpret_cast<gpointer>(windowId));
    gtk_fixed_put(GTK_FIXED(fixedIt->second), widget, static_cast<int>(x), static_cast<int>(y));
    gtk_widget_set_size_request(widget, static_cast<int>(width), static_cast<int>(height));
    gtk_widget_show(widget);
    gadgetTable()[gadgetId] = widget;
    return true;
}

} // namespace detail

/// Oracle-verified: `OpenWindow`'s own return value is some nonzero,
/// native-handle-ish Integer on success (not a clean `1`) - a real PB
/// program only ever uses it for truthiness (`If OpenWindow(...)`), never
/// compares it to a specific value, so this returns a plain `1` rather
/// than trying to replicate an arbitrary internal pointer value no public
/// contract documents. `Flags` (`#PB_Window_*`) is accepted but not yet
/// acted on - window chrome/behavior flags are a deliberately deferred gap
/// for this first GUI slice (every window currently gets the GTK default
/// titlebar/resize/close chrome regardless of what's requested).
inline std::int64_t pbOpenWindow(std::int64_t windowId, std::int64_t x, std::int64_t y, std::int64_t width,
                                  std::int64_t height, const PBString& title, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title.bytes().c_str());
    gtk_window_move(GTK_WINDOW(window), static_cast<int>(x), static_cast<int>(y));
    gtk_window_set_default_size(GTK_WINDOW(window), static_cast<int>(width), static_cast<int>(height));

    g_object_set_data(G_OBJECT(window), detail::windowIdKey(), reinterpret_cast<gpointer>(windowId));
    auto* state = new detail::WindowState{x, y, width, height};
    g_object_set_data(G_OBJECT(window), detail::windowStateKey(), state);

    g_signal_connect(window, "delete-event", G_CALLBACK(detail::onDeleteEvent), nullptr);
    g_signal_connect(window, "configure-event", G_CALLBACK(detail::onConfigureEvent), nullptr);
    g_signal_connect(window, "draw", G_CALLBACK(detail::onDraw), nullptr);

    // A GtkFixed child holds this window's gadgets (M7b's second GUI
    // slice) - see its own doc comment on `detail::windowFixedTable()`.
    GtkWidget* fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(window), fixed);

    gtk_widget_show_all(window);

    // Oracle-verified: re-using an already-open window ID is a harmless
    // no-op that just hands back the existing window rather than creating
    // a second one - not separately replicated here (a genuinely new
    // GtkWidget is always created), a documented, narrow divergence for an
    // edge case real programs are unlikely to rely on deliberately.
    auto old = detail::windowTable().find(windowId);
    if (old != detail::windowTable().end()) {
        detail::destroyWindow(windowId, old->second);
    }
    detail::windowTable()[windowId] = window;
    detail::windowFixedTable()[windowId] = fixed;
    detail::activeWindowId() = windowId;
    return 1;
}

inline std::int64_t pbCloseWindow(std::int64_t windowId) {
    auto it = detail::windowTable().find(windowId);
    if (it == detail::windowTable().end()) {
        return 0;
    }
    detail::destroyWindow(windowId, it->second);
    detail::windowTable().erase(it);
    detail::windowFixedTable().erase(windowId);
    return 1;
}

/// Oracle-verified: real PB's own `IsWindow` also returns a native-handle-
/// ish nonzero value, not a clean `1` - simplified the same way
/// `pbOpenWindow`'s own return value is (see its own notes).
inline std::int64_t pbIsWindow(std::int64_t windowId) { return detail::windowTable().contains(windowId) ? 1 : 0; }

inline std::int64_t pbResizeWindow(std::int64_t windowId, std::int64_t x, std::int64_t y, std::int64_t width,
                                    std::int64_t height) {
    auto it = detail::windowTable().find(windowId);
    if (it == detail::windowTable().end()) {
        return 0;
    }
    gtk_window_move(GTK_WINDOW(it->second), static_cast<int>(x), static_cast<int>(y));
    gtk_window_resize(GTK_WINDOW(it->second), static_cast<int>(width), static_cast<int>(height));
    return 1;
}

inline std::int64_t pbHideWindow(std::int64_t windowId, std::int64_t state) {
    auto it = detail::windowTable().find(windowId);
    if (it == detail::windowTable().end()) {
        return 0;
    }
    if (state != 0) {
        gtk_widget_hide(it->second);
    } else {
        gtk_widget_show(it->second);
    }
    return 1;
}

/// Non-blocking poll - oracle-verified to return `0` immediately when no
/// event is pending, pumping only whatever GTK events are *already*
/// queued (`gtk_events_pending()`/`gtk_main_iteration()`), never blocking
/// to wait for a new one (that's `pbWaitWindowEvent`'s own job).
inline std::int64_t pbWindowEvent() {
    detail::ensureGtkInit();
    while (detail::eventQueue().empty() && gtk_events_pending() != 0) {
        gtk_main_iteration();
    }
    if (detail::eventQueue().empty()) {
        return 0;
    }
    detail::currentEvent() = detail::eventQueue().front();
    detail::eventQueue().pop_front();
    return detail::currentEvent().type;
}

/// Oracle-verified: blocks until an event occurs, or - if `timeoutMs` is
/// given - until that many milliseconds pass with nothing happening, in
/// which case this returns `0` (confirmed via a real ~300ms-timeout test
/// that genuinely waited that long before returning 0 - a value this
/// project initially, incorrectly suspected might be `-1` instead, before
/// realizing that `-1` was actually a real, repeating event this specific
/// headless-Xvfb test environment generates, unrelated to timeout
/// behavior at all - drained away by waiting long enough first). No
/// timeout argument (`timeoutMs < 0`, matching the default parameter
/// below) blocks indefinitely. Implemented as a plain poll-with-short-
/// sleep loop rather than a GTK timeout-source-based wakeup - simpler,
/// at the cost of up to ~5ms of extra latency recognizing a timeout
/// expiring, acceptable for this first GUI milestone.
inline std::int64_t pbWaitWindowEvent(std::int64_t timeoutMs = -1) {
    detail::ensureGtkInit();
    auto start = std::chrono::steady_clock::now();
    while (detail::eventQueue().empty()) {
        if (gtk_events_pending() != 0) {
            gtk_main_iteration();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (timeoutMs >= 0) {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                                     start)
                                 .count();
            if (elapsedMs >= timeoutMs) {
                return 0;
            }
        }
    }
    detail::currentEvent() = detail::eventQueue().front();
    detail::eventQueue().pop_front();
    return detail::currentEvent().type;
}

inline std::int64_t pbEventWindow() { return detail::currentEvent().windowId; }
inline std::int64_t pbEventGadget() { return detail::currentEvent().gadgetId; }
inline std::int64_t pbEventType() { return detail::currentEvent().eventType; }

/// M7b's second GUI slice: basic gadgets. Oracle-verified return-value
/// simplification, same as `pbOpenWindow`'s own (see its notes): real PB's
/// `ButtonGadget`/`TextGadget`/`StringGadget`/`CheckBoxGadget`/`FrameGadget`
/// all return a native-handle-ish nonzero Integer, not a clean `1` - these
/// return a plain `1`/`0` instead (`0` if there's no currently open window
/// to place the gadget into - see `activeWindowId()`'s own doc comment).
/// `Flags` is accepted but not yet acted on, the same deliberately deferred
/// gap as `OpenWindow`'s own `Flags`.
inline std::int64_t pbButtonGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                    std::int64_t height, const PBString& text, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_button_new_with_label(text.bytes().c_str());
    g_signal_connect(widget, "clicked", G_CALLBACK(detail::onGadgetClicked), nullptr);
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

inline std::int64_t pbTextGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                  std::int64_t height, const PBString& text, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_label_new(text.bytes().c_str());
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

inline std::int64_t pbStringGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                    std::int64_t height, const PBString& content, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widget), content.bytes().c_str());
    g_signal_connect(widget, "changed", G_CALLBACK(detail::onGadgetChanged), nullptr);
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

/// Oracle-verified: a checkbox's own toggle reports `#PB_EventType_LeftClick`
/// as its `EventType()`, *not* `#PB_EventType_Change` - see
/// `detail::onGadgetClicked`'s own doc comment.
inline std::int64_t pbCheckBoxGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                      std::int64_t height, const PBString& text, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_check_button_new_with_label(text.bytes().c_str());
    g_signal_connect(widget, "toggled", G_CALLBACK(detail::onGadgetClicked), nullptr);
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

inline std::int64_t pbFrameGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                   std::int64_t height, const PBString& text, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_frame_new(text.bytes().c_str());
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

/// Oracle-verified: real PB's own `IsGadget` also returns a native-handle-
/// ish nonzero value, not a clean `1` - simplified the same way
/// `pbIsWindow`'s own is.
inline std::int64_t pbIsGadget(std::int64_t gadgetId) { return detail::gadgetTable().contains(gadgetId) ? 1 : 0; }

inline std::int64_t pbFreeGadget(std::int64_t gadgetId) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    gtk_widget_destroy(it->second);
    detail::gadgetTable().erase(it);
    return 1;
}

inline std::int64_t pbResizeGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                    std::int64_t height) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    GtkWidget* parent = gtk_widget_get_parent(it->second);
    gtk_fixed_move(GTK_FIXED(parent), it->second, static_cast<int>(x), static_cast<int>(y));
    gtk_widget_set_size_request(it->second, static_cast<int>(width), static_cast<int>(height));
    return 1;
}

inline std::int64_t pbHideGadget(std::int64_t gadgetId, std::int64_t state) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (state != 0) {
        gtk_widget_hide(it->second);
    } else {
        gtk_widget_show(it->second);
    }
    return 1;
}

inline std::int64_t pbDisableGadget(std::int64_t gadgetId, std::int64_t state) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    gtk_widget_set_sensitive(it->second, state == 0 ? TRUE : FALSE);
    return 1;
}

/// Dispatches on the gadget's own real GTK widget type rather than needing
/// a separate accessor per gadget type - `GtkToggleButton` (CheckBoxGadget)
/// is itself a `GtkButton` subclass, so `GTK_IS_BUTTON` alone already
/// covers both ButtonGadget and CheckBoxGadget's own label text (oracle-
/// verified both are readable/writable this way).
inline PBString pbGetGadgetText(std::int64_t gadgetId) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return PBString();
    }
    GtkWidget* widget = it->second;
    // Each of these GTK getters can technically return nullptr (e.g. a
    // GtkFrame with no label widget at all) - none of this code's own
    // creation functions leave a gadget in that state (always constructed
    // with at least an empty-string label), but PBString(nullptr) would be
    // undefined behavior if one ever did, so guard defensively anyway.
    auto safe = [](const char* text) { return PBString(text != nullptr ? text : ""); };
    if (GTK_IS_ENTRY(widget) != 0) {
        return safe(gtk_entry_get_text(GTK_ENTRY(widget)));
    }
    if (GTK_IS_BUTTON(widget) != 0) {
        return safe(gtk_button_get_label(GTK_BUTTON(widget)));
    }
    if (GTK_IS_LABEL(widget) != 0) {
        return safe(gtk_label_get_text(GTK_LABEL(widget)));
    }
    if (GTK_IS_FRAME(widget) != 0) {
        return safe(gtk_frame_get_label(GTK_FRAME(widget)));
    }
    return PBString();
}

/// Oracle-verified: `SetGadgetText` on a `StringGadget` does *not* queue a
/// spurious `#PB_Event_Gadget` the way real typing does - same false-event
/// risk and fix as `pbSetGadgetState`'s own (see its doc comment);
/// `gtk_entry_set_text` fires `GtkEntry`'s `"changed"` signal just as much
/// for a programmatic change as a keystroke.
inline std::int64_t pbSetGadgetText(std::int64_t gadgetId, const PBString& text) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    GtkWidget* widget = it->second;
    if (GTK_IS_ENTRY(widget) != 0) {
        g_signal_handlers_block_by_func(widget, reinterpret_cast<gpointer>(detail::onGadgetChanged), nullptr);
        gtk_entry_set_text(GTK_ENTRY(widget), text.bytes().c_str());
        g_signal_handlers_unblock_by_func(widget, reinterpret_cast<gpointer>(detail::onGadgetChanged), nullptr);
    } else if (GTK_IS_BUTTON(widget) != 0) {
        gtk_button_set_label(GTK_BUTTON(widget), text.bytes().c_str());
    } else if (GTK_IS_LABEL(widget) != 0) {
        gtk_label_set_text(GTK_LABEL(widget), text.bytes().c_str());
    } else if (GTK_IS_FRAME(widget) != 0) {
        gtk_frame_set_label(GTK_FRAME(widget), text.bytes().c_str());
    } else {
        return 0;
    }
    return 1;
}

/// Only `CheckBoxGadget` (a `GtkToggleButton`) has a meaningful checked
/// state - every other gadget type harmlessly returns/ignores `0`, the same
/// kind of narrow, documented simplification as `GetGadgetState`'s own
/// untested-by-the-oracle behavior for non-stateful gadgets.
inline std::int64_t pbGetGadgetState(std::int64_t gadgetId) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end() || GTK_IS_TOGGLE_BUTTON(it->second) == 0) {
        return 0;
    }
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(it->second)) != 0 ? 1 : 0;
}

/// Oracle-verified: `SetGadgetState` does *not* queue a spurious
/// `#PB_Event_Gadget` the way a real user click/toggle does - confirmed
/// directly (a `SetGadgetState` immediately followed by a drained event
/// poll loop reports zero gadget events). `gtk_toggle_button_set_active`
/// would otherwise fire the same `"toggled"` signal `pbCheckBoxGadget`
/// connects for real clicks, so the handler is blocked around this one
/// programmatic change specifically.
inline std::int64_t pbSetGadgetState(std::int64_t gadgetId, std::int64_t state) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end() || GTK_IS_TOGGLE_BUTTON(it->second) == 0) {
        return 0;
    }
    g_signal_handlers_block_by_func(it->second, reinterpret_cast<gpointer>(detail::onGadgetClicked), nullptr);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(it->second), state != 0 ? TRUE : FALSE);
    g_signal_handlers_unblock_by_func(it->second, reinterpret_cast<gpointer>(detail::onGadgetClicked), nullptr);
    return 1;
}

} // namespace easybasic::runtime
