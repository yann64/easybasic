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

    gtk_widget_show_all(window);

    // Oracle-verified: re-using an already-open window ID is a harmless
    // no-op that just hands back the existing window rather than creating
    // a second one - not separately replicated here (a genuinely new
    // GtkWidget is always created), a documented, narrow divergence for an
    // edge case real programs are unlikely to rely on deliberately.
    auto old = detail::windowTable().find(windowId);
    if (old != detail::windowTable().end()) {
        gtk_widget_destroy(old->second);
    }
    detail::windowTable()[windowId] = window;
    return 1;
}

inline std::int64_t pbCloseWindow(std::int64_t windowId) {
    auto it = detail::windowTable().find(windowId);
    if (it == detail::windowTable().end()) {
        return 0;
    }
    auto* state = static_cast<detail::WindowState*>(g_object_get_data(G_OBJECT(it->second), detail::windowStateKey()));
    delete state;
    gtk_widget_destroy(it->second);
    detail::windowTable().erase(it);
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

} // namespace easybasic::runtime
