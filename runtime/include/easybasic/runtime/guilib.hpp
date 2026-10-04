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
    std::int64_t menuElementId = 0; ///< M7b's fourth GUI slice: `EventMenu()`'s own value.
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

/// M7b's fourth GUI slice: `Menu`/`StatusBar`. `#Menu` -> its `GtkMenuBar`
/// (the direct child of a window's own vbox, reordered to position 0 so it
/// always renders above the gadget area regardless of creation order
/// relative to a status bar).
inline std::unordered_map<std::int64_t, GtkWidget*>& menuTable() {
    static std::unordered_map<std::int64_t, GtkWidget*> table;
    return table;
}

/// `#Menu` -> every `MenuTitle()`'s own top-level `GtkMenuItem*`, in creation
/// order - what `GetMenuTitleText`/`SetMenuTitleText`'s own 0-based `Titre`
/// index addresses.
inline std::unordered_map<std::int64_t, std::vector<GtkWidget*>>& menuTitleWidgets() {
    static std::unordered_map<std::int64_t, std::vector<GtkWidget*>> table;
    return table;
}

/// `#Menu` -> (`ElementID` -> its own `GtkCheckMenuItem*`) - every leaf
/// `MenuItem()`, addressed by its own PB-level element ID (not creation
/// order, unlike `menuTitleWidgets()`).
inline std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, GtkWidget*>>& menuItemWidgets() {
    static std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, GtkWidget*>> table;
    return table;
}

/// `#Menu` -> a stack of `GtkMenu*`, the menu-building equivalent of
/// `activeWindowId()`/`activeMenuId()` themselves: `MenuItem`/`MenuBar()`
/// (the separator) always append to the *top* of this stack, `MenuTitle`
/// resets it to a single fresh top-level submenu, and `OpenSubMenu`/
/// `CloseSubMenu` push/pop a nested one - oracle-verified that all of these
/// operate on whichever menu was most recently `CreateMenu`'d, exactly the
/// same "current context" pattern gadgets use for their own active window.
inline std::unordered_map<std::int64_t, std::vector<GtkWidget*>>& menuBuildStack() {
    static std::unordered_map<std::int64_t, std::vector<GtkWidget*>> table;
    return table;
}

inline std::int64_t& activeMenuId() {
    static std::int64_t id = 0;
    return id;
}

/// `#StatusBar` -> its own horizontal `GtkBox`, packed at the *end* of the
/// window's vbox (so it renders below the gadget area).
inline std::unordered_map<std::int64_t, GtkWidget*>& statusBarTable() {
    static std::unordered_map<std::int64_t, GtkWidget*> table;
    return table;
}

/// `#StatusBar` -> every `AddStatusBarField()`'s own `GtkFrame*` (wrapping a
/// `GtkLabel`, for a visible field border matching a native status bar's
/// look), in creation order - what `StatusBarText`'s own 0-based `Champ`
/// index addresses.
inline std::unordered_map<std::int64_t, std::vector<GtkWidget*>>& statusBarFieldFrames() {
    static std::unordered_map<std::int64_t, std::vector<GtkWidget*>> table;
    return table;
}

inline std::int64_t& activeStatusBarId() {
    static std::int64_t id = 0;
    return id;
}

/// M7b's fifth GUI slice: the Image library. `#Image` -> its own
/// `GdkPixbuf*` - a bitmap that exists independently of any window,
/// exactly like real PB's own Image objects. `ImageID()` (see its own doc
/// comment) returns this same pointer directly, the same "the handle
/// already *is* the real pointer" convention `pbWindowID`/`pbMenuID`
/// established.
inline std::unordered_map<std::int64_t, GdkPixbuf*>& imageTable() {
    static std::unordered_map<std::int64_t, GdkPixbuf*> table;
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
// Same idea again, for M7b's fourth GUI slice: a leaf menu item's own
// element ID and owning window ID (read back by `onMenuItemActivate`), and
// the owning window ID tagged directly on a `#Menu`'s `GtkMenuBar`/a
// `#StatusBar`'s own `GtkBox` (read back by `destroyWindow`, the same way
// it already prunes `gadgetTable()`).
inline const char* menuElementIdKey() { return "pbcxx-menu-element-id"; }
inline const char* menuWindowIdKey() { return "pbcxx-menu-window-id"; }
inline const char* menuCheckedKey() { return "pbcxx-menu-checked"; }
inline const char* statusBarWindowIdKey() { return "pbcxx-statusbar-window-id"; }

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

/// Queues a `#PB_Event_Menu` for a leaf `MenuItem()`'s own `"activate"`
/// signal, reading its element/window IDs back from `g_object_set_data`
/// (set in `pbMenuItem`) - the menu equivalent of `queueGadgetEvent`.
inline void onMenuItemActivate(GtkWidget* item, gpointer) {
    auto elementId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(item), menuElementIdKey()));
    auto windowId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(item), menuWindowIdKey()));
    PBWindowEvent ev;
    ev.type = 1; // #PB_Event_Menu
    ev.windowId = windowId;
    ev.menuElementId = elementId;
    eventQueue().push_back(ev);
}

/// Every leaf `MenuItem()` is a `GtkCheckMenuItem` under the hood (needed so
/// `SetMenuItemState`'s own checkmark has somewhere to render), but
/// oracle-verified a real click does **not** change `GetMenuItemState()`'s
/// own value (unlike a `CheckBoxGadget` toggle) - only an explicit
/// `SetMenuItemState()` call does. GTK's own default `"activate"` handler
/// unconditionally flips a `GtkCheckMenuItem`'s active state first, though,
/// so this handler (connected to `"toggled"`, fired as a side effect of
/// that flip) immediately reverts any change the *stored* `menuCheckedKey()`
/// intent (updated only by `pbSetMenuItemState`) doesn't agree with -
/// blocking itself around the corrective call the same way
/// `pbSetGadgetState` already blocks around its own programmatic change, to
/// avoid recursing into itself.
inline void onMenuItemToggled(GtkWidget* item, gpointer) {
    bool intended = reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(item), menuCheckedKey())) != 0;
    bool actual = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)) != 0;
    if (actual == intended) {
        return;
    }
    g_signal_handlers_block_by_func(item, reinterpret_cast<gpointer>(onMenuItemToggled), nullptr);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), intended ? TRUE : FALSE);
    g_signal_handlers_unblock_by_func(item, reinterpret_cast<gpointer>(onMenuItemToggled), nullptr);
}

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
    // Same idea again for M7b's fourth GUI slice's own two per-window
    // owned-widget tables - GTK will recursively destroy the actual
    // GtkMenuBar/GtkBox widgets as children of `window` below, but
    // `menuTable()`/`statusBarTable()` (and their own side tables) would
    // otherwise be left holding dangling pointers, same risk `gadgetTable()`
    // already had.
    for (auto it = menuTable().begin(); it != menuTable().end();) {
        auto owner = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(it->second), menuWindowIdKey()));
        if (owner == windowId) {
            menuTitleWidgets().erase(it->first);
            menuItemWidgets().erase(it->first);
            menuBuildStack().erase(it->first);
            it = menuTable().erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = statusBarTable().begin(); it != statusBarTable().end();) {
        auto owner = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(it->second), statusBarWindowIdKey()));
        if (owner == windowId) {
            statusBarFieldFrames().erase(it->first);
            it = statusBarTable().erase(it);
        } else {
            ++it;
        }
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

/// Finds the real `GtkLabel` for a menu item's own text, whether it's
/// still the item's direct child (no image attached) or nested inside the
/// `GtkBox` `attachMenuItemImage` built (see its own doc comment) - shared
/// by `pbGetMenuItemText`/`pbSetMenuItemText` so neither needs to know
/// which case applies.
inline GtkWidget* menuItemLabel(GtkWidget* item) {
    GtkWidget* child = gtk_bin_get_child(GTK_BIN(item));
    if (child == nullptr) {
        return nullptr;
    }
    if (GTK_IS_LABEL(child)) {
        return child;
    }
    if (GTK_IS_CONTAINER(child)) {
        GList* children = gtk_container_get_children(GTK_CONTAINER(child));
        GtkWidget* label = nullptr;
        for (GList* l = children; l != nullptr; l = l->next) {
            if (GTK_IS_LABEL(l->data)) {
                label = GTK_WIDGET(l->data);
                break;
            }
        }
        g_list_free(children);
        return label;
    }
    return nullptr;
}

/// Shared by `pbMenuItem`/`pbOpenSubMenu` (both take an optional `ImageID`
/// - oracle-verified real PB's own docs for each). A plain `GtkMenuItem`/
/// `GtkCheckMenuItem` is a `GtkBin` with a single `GtkLabel` child; this
/// swaps that child for a small `GtkBox` holding the image (scaled to
/// 16x16 - oracle-verified real PB's own `MenuItem` docs: "Les dimensions
/// des images sont de 16x16 pixels") next to a fresh label with the same
/// text (found via `menuItemLabel`, since `GetMenuItemText`/
/// `SetMenuItemText` need to keep finding it correctly afterward too - see
/// their own doc comments). Deliberately *not* gated on whether the menu
/// was built with `CreateMenu` vs `CreateImageMenu` (real PB's own docs
/// require the latter) - GTK itself has no such distinction to enforce,
/// so requiring it here would be an arbitrary, unenforceable-for-a-reason
/// restriction rather than a real one.
inline void attachMenuItemImage(GtkWidget* item, std::int64_t imageId) {
    if (imageId == 0) {
        return;
    }
    auto* pixbuf = reinterpret_cast<GdkPixbuf*>(imageId);
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, 16, 16, GDK_INTERP_BILINEAR);
    if (scaled == nullptr) {
        return;
    }
    GtkWidget* image = gtk_image_new_from_pixbuf(scaled);
    g_object_unref(scaled);
    GtkWidget* oldChild = gtk_bin_get_child(GTK_BIN(item));
    std::string text = oldChild != nullptr ? gtk_label_get_text(GTK_LABEL(oldChild)) : "";
    if (oldChild != nullptr) {
        gtk_container_remove(GTK_CONTAINER(item), oldChild);
    }
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    GtkWidget* label = gtk_label_new(text.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(item), box);
    gtk_widget_show_all(box);
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
    // It's wrapped in a vertical GtkBox (M7b's fourth GUI slice) rather than
    // added to the window directly, so a later `CreateMenu`/`CreateStatusBar`
    // on this same window has somewhere to pack a menu bar above it / a
    // status bar below it without disturbing the fixed's own gadget
    // coordinates - `pbCreateMenu`/`pbCreateStatusBar` find this vbox back
    // via `gtk_bin_get_child` on the window handle they're given (`WindowID()`'s
    // own return value), rather than needing a third id-keyed table here.
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    GtkWidget* fixed = gtk_fixed_new();
    gtk_box_pack_start(GTK_BOX(vbox), fixed, TRUE, TRUE, 0);

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

/// M7b's fourth GUI slice needs a real native window handle for the first
/// time: `CreateMenu`/`CreateStatusBar`'s own second argument is documented
/// (and oracle-verified) to be real PB's own `WindowID(#Window)` - not the
/// PB-level window ID `pbOpenWindow` already simplifies to a clean `1`/`0`
/// for everywhere else. So, unlike every other GUI handle this project has
/// simplified so far, `WindowID()`'s return value genuinely is load-bearing:
/// `pbCreateMenu`/`pbCreateStatusBar` below `reinterpret_cast` it straight
/// back into the real `GtkWidget*` it is, with no separate lookup table
/// needed at all.
inline std::int64_t pbWindowID(std::int64_t windowId) {
    auto it = detail::windowTable().find(windowId);
    return it != detail::windowTable().end() ? reinterpret_cast<std::int64_t>(it->second) : 0;
}

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
inline std::int64_t pbEventMenu() { return detail::currentEvent().menuElementId; }

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

/// M7b's third GUI slice: `MessageRequester`, the simplest modal dialog.
/// Unlike every other GUI builtin so far, this one is genuinely
/// *synchronous* from PB's own point of view - it blocks the calling
/// "thread" until a button is clicked and returns which one, with no
/// `WaitWindowEvent`/event-queue involvement at all. `gtk_dialog_run`
/// (GTK3's own nested-main-loop "block until response" API) is the exact
/// right fit for this, despite being deprecated in favor of an async,
/// signal-based pattern in newer GTK - that pattern doesn't exist for a
/// good reason here, since PB's own `MessageRequester` call site is
/// genuinely blocking.
///
/// `Flags`' bits 0-1 select the button set (`0`=Ok, `1`=YesNo,
/// `2`=YesNoCancel - oracle-verified via `#PB_MessageRequester_Ok`/
/// `YesNo`/`YesNoCancel`), and bits 2-4 independently select an icon
/// (`#PB_MessageRequester_Info`=4/`Error`=8/`Warning`=16) - no icon at all
/// if none of those three are set. GTK has no built-in "Yes/No/Cancel"
/// button-set enum, so that one combination is built by hand
/// (`GTK_BUTTONS_NONE` + three `gtk_dialog_add_button` calls) rather than
/// using `GtkButtonsType` for it.
///
/// **Oracle-verified finding that shapes the whole return-value mapping**:
/// clicking the *only* button on a plain Ok-type dialog returns `6`
/// (`#PB_MessageRequester_Yes`'s own value), **not** `0`
/// (`#PB_MessageRequester_Ok`) - confirmed directly, repeatedly, by
/// clicking that exact button under Xvfb and reading back the Debug
/// output. This looks like a genuine real-PB implementation quirk (its
/// GTK3 backend likely routes a single-button dialog's response through
/// the same code path as a "Yes" response), not a documented public
/// contract - but unlike `OpenWindow`/`IsWindow`'s own native-handle-value
/// simplification (an arbitrary internal pointer no reasonable program
/// would compare against), `#PB_MessageRequester_Ok`/`Yes`/`No`/`Cancel`
/// are meaningful constants a real program might reasonably check equality
/// against, so this one *is* replicated exactly rather than "cleaned up"
/// to the more intuitive `0`.
inline std::int64_t pbMessageRequester(const PBString& title, const PBString& text, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    std::int64_t buttonSet = flags & 3;
    GtkMessageType messageType = GTK_MESSAGE_OTHER;
    if ((flags & 4) != 0) {
        messageType = GTK_MESSAGE_INFO;
    } else if ((flags & 8) != 0) {
        messageType = GTK_MESSAGE_ERROR;
    } else if ((flags & 16) != 0) {
        messageType = GTK_MESSAGE_WARNING;
    }

    GtkButtonsType buttons = GTK_BUTTONS_OK;
    if (buttonSet == 1) {
        buttons = GTK_BUTTONS_YES_NO;
    } else if (buttonSet == 2) {
        buttons = GTK_BUTTONS_NONE;
    }

    GtkWidget* dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, messageType, buttons, "%s",
                                                text.bytes().c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), title.bytes().c_str());
    if (buttonSet == 2) {
        // Matches real PB's own left-to-right button order (confirmed via
        // screenshot under Xvfb: No, Yes, Cancel).
        gtk_dialog_add_button(GTK_DIALOG(dialog), "No", GTK_RESPONSE_NO);
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Yes", GTK_RESPONSE_YES);
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_CANCEL);
    }

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch (response) {
    case GTK_RESPONSE_NO:
        return 7; // #PB_MessageRequester_No
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
        return 2; // #PB_MessageRequester_Cancel
    case GTK_RESPONSE_YES:
    case GTK_RESPONSE_OK:
    default:
        return 6; // #PB_MessageRequester_Yes - see this function's own doc comment.
    }
}

/// M7b's fourth GUI slice: `Menu`. `GtkMenuBar` is the natural GTK3 fit for
/// real PB's own menu bar - oracle-verified `CreateMenu`'s second argument
/// must be a real `WindowID()` handle (see `pbWindowID`'s own doc comment),
/// reinterpret_cast straight back into the `GtkWidget*` it already is. Like
/// `pbOpenWindow`'s own return value, real `CreateMenu` returns a native-
/// handle-ish nonzero Integer, simplified here to a clean `1`/`0`.
///
/// `ToolBar` was **deliberately out of scope for this slice** - oracle-
/// verified (via `ToolBarImageButton`'s own docs) that real PB has no way to
/// create a toolbar button without an `ImageID` at all (`ToolBarButtonText`
/// only *relabels* an already-`ToolBarImageButton`-created button) - and
/// this project's Image library (`LoadImage`/`CreateImage`/`ImageID`) didn't
/// exist yet. The Image library landed in M7b's fifth slice (see
/// `pbCreateImage`'s own doc comment); `ToolBar` itself is still a
/// follow-up, not part of that slice either.
inline std::int64_t pbCreateMenu(std::int64_t menuId, std::int64_t windowHandle) {
    detail::ensureGtkInit();
    auto* window = reinterpret_cast<GtkWidget*>(windowHandle);
    GtkWidget* vbox = gtk_bin_get_child(GTK_BIN(window));
    GtkWidget* menuBar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menuBar, FALSE, FALSE, 0);
    // Always force it back to the very top - `fixed` (and, now, possibly an
    // already-existing status bar) may already have been packed into `vbox`
    // before this call.
    gtk_box_reorder_child(GTK_BOX(vbox), menuBar, 0);
    gtk_widget_show(menuBar);

    auto windowId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(window), detail::windowIdKey()));
    g_object_set_data(G_OBJECT(menuBar), detail::menuWindowIdKey(), reinterpret_cast<gpointer>(windowId));

    // Oracle-verified: re-using an already-live #Menu ID replaces the old
    // one, the same convention as `pbOpenWindow`/`placeGadget`.
    auto old = detail::menuTable().find(menuId);
    if (old != detail::menuTable().end()) {
        gtk_widget_destroy(old->second);
    }
    detail::menuTable()[menuId] = menuBar;
    detail::menuTitleWidgets()[menuId].clear();
    detail::menuItemWidgets()[menuId].clear();
    detail::menuBuildStack()[menuId].clear();
    detail::activeMenuId() = menuId;
    return 1;
}

/// Oracle-verified functionally identical to `CreateMenu` on this backend -
/// real PB's own distinction is that only a menu created this way accepts
/// an `ImageID` on its own `MenuItem`/`OpenSubMenu` calls, a restriction
/// GTK itself has no equivalent for (see `detail::attachMenuItemImage`'s
/// own doc comment on why that restriction isn't enforced here either).
/// `Options` (`#PB_Menu_NativeImageSize`) is Windows-only - accepted but
/// not acted on.
inline std::int64_t pbCreateImageMenu(std::int64_t menuId, std::int64_t windowHandle, std::int64_t /*options*/ = 0) {
    return pbCreateMenu(menuId, windowHandle);
}

/// Oracle-verified: `MenuTitle`/`MenuItem`/`MenuBar`/`OpenSubMenu`/
/// `CloseSubMenu` all operate on whichever `#Menu` was most recently
/// `CreateMenu`'d - no explicit `#Menu` argument exists for any of them,
/// exactly the "current context" pattern M7b's second GUI slice already
/// established for gadgets and `activeWindowId()`. `MenuTitle` resets this
/// menu's own build stack to a single fresh top-level `GtkMenu`, which
/// `MenuItem`/`MenuBar()` (the separator)/`OpenSubMenu` all then append
/// into.
inline std::int64_t pbMenuTitle(const PBString& text) {
    auto menuId = detail::activeMenuId();
    auto it = detail::menuTable().find(menuId);
    if (it == detail::menuTable().end()) {
        return 0;
    }
    GtkWidget* titleItem = gtk_menu_item_new_with_label(text.bytes().c_str());
    GtkWidget* submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(titleItem), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(it->second), titleItem);
    gtk_widget_show(titleItem);
    detail::menuTitleWidgets()[menuId].push_back(titleItem);
    detail::menuBuildStack()[menuId] = {submenu};
    return 1;
}

/// `ImageID` (M7b's fifth GUI slice) - see `detail::attachMenuItemImage`'s
/// own doc comment for how it's attached. Every leaf item is a
/// `GtkCheckMenuItem` - see `detail::onMenuItemToggled`'s own doc comment
/// for why.
inline std::int64_t pbMenuItem(std::int64_t elementId, const PBString& text, std::int64_t imageId = 0) {
    auto menuId = detail::activeMenuId();
    auto stackIt = detail::menuBuildStack().find(menuId);
    if (stackIt == detail::menuBuildStack().end() || stackIt->second.empty()) {
        return 0;
    }
    GtkWidget* item = gtk_check_menu_item_new_with_label(text.bytes().c_str());
    detail::attachMenuItemImage(item, imageId);
    auto windowId = reinterpret_cast<std::int64_t>(
        g_object_get_data(G_OBJECT(detail::menuTable()[menuId]), detail::menuWindowIdKey()));
    g_object_set_data(G_OBJECT(item), detail::menuElementIdKey(), reinterpret_cast<gpointer>(elementId));
    g_object_set_data(G_OBJECT(item), detail::menuWindowIdKey(), reinterpret_cast<gpointer>(windowId));
    g_object_set_data(G_OBJECT(item), detail::menuCheckedKey(), reinterpret_cast<gpointer>(std::intptr_t{0}));
    g_signal_connect(item, "activate", G_CALLBACK(detail::onMenuItemActivate), nullptr);
    g_signal_connect(item, "toggled", G_CALLBACK(detail::onMenuItemToggled), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(stackIt->second.back()), item);
    gtk_widget_show(item);
    detail::menuItemWidgets()[menuId][elementId] = item;
    return 1;
}

inline std::int64_t pbMenuBar() {
    auto menuId = detail::activeMenuId();
    auto stackIt = detail::menuBuildStack().find(menuId);
    if (stackIt == detail::menuBuildStack().end() || stackIt->second.empty()) {
        return 0;
    }
    GtkWidget* sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(stackIt->second.back()), sep);
    gtk_widget_show(sep);
    return 1;
}

inline std::int64_t pbOpenSubMenu(const PBString& text, std::int64_t imageId = 0) {
    auto menuId = detail::activeMenuId();
    auto stackIt = detail::menuBuildStack().find(menuId);
    if (stackIt == detail::menuBuildStack().end() || stackIt->second.empty()) {
        return 0;
    }
    GtkWidget* subItem = gtk_menu_item_new_with_label(text.bytes().c_str());
    detail::attachMenuItemImage(subItem, imageId);
    GtkWidget* submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(subItem), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(stackIt->second.back()), subItem);
    gtk_widget_show(subItem);
    stackIt->second.push_back(submenu);
    return 1;
}

inline std::int64_t pbCloseSubMenu() {
    auto stackIt = detail::menuBuildStack().find(detail::activeMenuId());
    if (stackIt == detail::menuBuildStack().end() || stackIt->second.size() <= 1) {
        return 0;
    }
    stackIt->second.pop_back();
    return 1;
}

/// Oracle-verified: real PB's own `IsMenu` also returns a native-handle-ish
/// nonzero value - simplified the same way `pbIsWindow`'s own is.
inline std::int64_t pbIsMenu(std::int64_t menuId) { return detail::menuTable().contains(menuId) ? 1 : 0; }

/// `#PB_All` (`-1`) frees every remaining menu at once - oracle-verified via
/// `FreeMenu`'s own docs.
inline std::int64_t pbFreeMenu(std::int64_t menuId) {
    if (menuId == -1) {
        for (auto& [id, widget] : detail::menuTable()) {
            gtk_widget_destroy(widget);
        }
        detail::menuTable().clear();
        detail::menuTitleWidgets().clear();
        detail::menuItemWidgets().clear();
        detail::menuBuildStack().clear();
        return 1;
    }
    auto it = detail::menuTable().find(menuId);
    if (it == detail::menuTable().end()) {
        return 0;
    }
    gtk_widget_destroy(it->second);
    detail::menuTable().erase(it);
    detail::menuTitleWidgets().erase(menuId);
    detail::menuItemWidgets().erase(menuId);
    detail::menuBuildStack().erase(menuId);
    return 1;
}

inline std::int64_t pbHideMenu(std::int64_t menuId, std::int64_t state) {
    auto it = detail::menuTable().find(menuId);
    if (it == detail::menuTable().end()) {
        return 0;
    }
    if (state != 0) {
        gtk_widget_hide(it->second);
    } else {
        gtk_widget_show(it->second);
    }
    return 1;
}

inline std::int64_t pbDisableMenuItem(std::int64_t menuId, std::int64_t element, std::int64_t state) {
    auto menuIt = detail::menuItemWidgets().find(menuId);
    if (menuIt == detail::menuItemWidgets().end()) {
        return 0;
    }
    auto itemIt = menuIt->second.find(element);
    if (itemIt == menuIt->second.end()) {
        return 0;
    }
    gtk_widget_set_sensitive(itemIt->second, state == 0 ? TRUE : FALSE);
    return 1;
}

inline std::int64_t pbGetMenuItemState(std::int64_t menuId, std::int64_t element) {
    auto menuIt = detail::menuItemWidgets().find(menuId);
    if (menuIt == detail::menuItemWidgets().end()) {
        return 0;
    }
    auto itemIt = menuIt->second.find(element);
    if (itemIt == menuIt->second.end()) {
        return 0;
    }
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(itemIt->second)) != 0 ? 1 : 0;
}

/// Updates `menuCheckedKey()`'s own stored intent too (not just the real GTK
/// state) - see `detail::onMenuItemToggled`'s own doc comment for why a
/// later real click needs to read that back.
inline std::int64_t pbSetMenuItemState(std::int64_t menuId, std::int64_t element, std::int64_t state) {
    auto menuIt = detail::menuItemWidgets().find(menuId);
    if (menuIt == detail::menuItemWidgets().end()) {
        return 0;
    }
    auto itemIt = menuIt->second.find(element);
    if (itemIt == menuIt->second.end()) {
        return 0;
    }
    GtkWidget* item = itemIt->second;
    g_object_set_data(G_OBJECT(item), detail::menuCheckedKey(),
                       reinterpret_cast<gpointer>(static_cast<std::intptr_t>(state != 0 ? 1 : 0)));
    g_signal_handlers_block_by_func(item, reinterpret_cast<gpointer>(detail::onMenuItemToggled), nullptr);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), state != 0 ? TRUE : FALSE);
    g_signal_handlers_unblock_by_func(item, reinterpret_cast<gpointer>(detail::onMenuItemToggled), nullptr);
    return 1;
}

/// Uses `detail::menuItemLabel` rather than `gtk_menu_item_get_label`
/// directly - see `pbSetMenuItemText`'s own doc comment for why (the same
/// reason applies here).
inline PBString pbGetMenuItemText(std::int64_t menuId, std::int64_t element) {
    auto menuIt = detail::menuItemWidgets().find(menuId);
    if (menuIt == detail::menuItemWidgets().end()) {
        return PBString();
    }
    auto itemIt = menuIt->second.find(element);
    if (itemIt == menuIt->second.end()) {
        return PBString();
    }
    GtkWidget* label = detail::menuItemLabel(itemIt->second);
    const char* text = label != nullptr ? gtk_label_get_text(GTK_LABEL(label)) : nullptr;
    return PBString(text != nullptr ? text : "");
}

/// Uses `detail::menuItemLabel` rather than `gtk_menu_item_set_label`
/// directly - the latter assumes the item's own direct child is the label,
/// which stops being true once `attachMenuItemImage` has nested it inside
/// a `GtkBox` instead (see that function's own doc comment).
inline std::int64_t pbSetMenuItemText(std::int64_t menuId, std::int64_t element, const PBString& text) {
    auto menuIt = detail::menuItemWidgets().find(menuId);
    if (menuIt == detail::menuItemWidgets().end()) {
        return 0;
    }
    auto itemIt = menuIt->second.find(element);
    if (itemIt == menuIt->second.end()) {
        return 0;
    }
    GtkWidget* label = detail::menuItemLabel(itemIt->second);
    if (label != nullptr) {
        gtk_label_set_text(GTK_LABEL(label), text.bytes().c_str());
    }
    return 1;
}

inline PBString pbGetMenuTitleText(std::int64_t menuId, std::int64_t titleIndex) {
    auto it = detail::menuTitleWidgets().find(menuId);
    if (it == detail::menuTitleWidgets().end() || titleIndex < 0 ||
        static_cast<std::size_t>(titleIndex) >= it->second.size()) {
        return PBString();
    }
    const char* text = gtk_menu_item_get_label(GTK_MENU_ITEM(it->second[static_cast<std::size_t>(titleIndex)]));
    return PBString(text != nullptr ? text : "");
}

inline std::int64_t pbSetMenuTitleText(std::int64_t menuId, std::int64_t titleIndex, const PBString& text) {
    auto it = detail::menuTitleWidgets().find(menuId);
    if (it == detail::menuTitleWidgets().end() || titleIndex < 0 ||
        static_cast<std::size_t>(titleIndex) >= it->second.size()) {
        return 0;
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(it->second[static_cast<std::size_t>(titleIndex)]), text.bytes().c_str());
    return 1;
}

/// Takes no `#Menu` argument at all, like every other menu-building function
/// - operates on `activeMenuId()`. **A real, oracle-driven correction to the
/// official docs mid-implementation**: the French help's own `MenuHeight()`
/// page claims Linux (and MacOS) always return `0`, since "the menu bar
/// isn't part of the window". Directly tested against this PB 6.50 beta 1 /
/// GTK3 install instead of trusting that: a plain `Debug MenuHeight()` right
/// after building a one-title, one-item menu read `27`, a real, nonzero
/// rendered height - GTK3's own menu bar genuinely is embedded in the
/// window's own content area (unlike an older native-X11-WM-chrome model
/// the docs may predate), so `pbMenuHeight` returns the real allocated
/// height, not a hardcoded `0`. Pending GTK events are drained first since
/// a freshly built/shown menu bar may not have gone through a size-allocate
/// pass yet (the oracle's own immediate, pre-event-loop read already
/// reflected a settled size, so this draining is a defensive, not merely
/// cosmetic, correctness step for `pbcxx`'s own single-process cycle).
inline std::int64_t pbMenuHeight() {
    auto it = detail::menuTable().find(detail::activeMenuId());
    if (it == detail::menuTable().end()) {
        return 0;
    }
    while (gtk_events_pending() != 0) {
        gtk_main_iteration();
    }
    return gtk_widget_get_allocated_height(it->second);
}

/// Oracle-verified: real PB's own `MenuID` also returns a native-handle-ish
/// value - unlike most of this project's own such simplifications, this one
/// is replicated exactly (the real `GtkWidget*`), the same way `WindowID`'s
/// own return value has to be (see its own doc comment) - `MenuID()` has no
/// known real-PB consumer like `CreateMenu`'s own `WindowID` argument, but
/// there is no more "obvious" simplified value to pick instead, and a raw
/// pointer is no less meaningful here than real PB's own is.
inline std::int64_t pbMenuID(std::int64_t menuId) {
    auto it = detail::menuTable().find(menuId);
    return it != detail::menuTable().end() ? reinterpret_cast<std::int64_t>(it->second) : 0;
}

/// M7b's fourth GUI slice: `StatusBar`. A plain `GtkStatusbar` doesn't model
/// real PB's own multi-field, independently-widthed/aligned/bordered field
/// list at all (it's a single text line with a context-ID message stack) -
/// a horizontal `GtkBox` of one `GtkFrame`-wrapped `GtkLabel` per field is
/// the natural fit instead, the same kind of "model PB's own shape, not
/// whichever same-named GTK widget happens to exist" choice `GtkFixed` was
/// for gadgets.
inline std::int64_t pbCreateStatusBar(std::int64_t barId, std::int64_t windowHandle) {
    detail::ensureGtkInit();
    auto* window = reinterpret_cast<GtkWidget*>(windowHandle);
    GtkWidget* vbox = gtk_bin_get_child(GTK_BIN(window));
    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_box_pack_end(GTK_BOX(vbox), bar, FALSE, FALSE, 0);
    gtk_widget_show(bar);

    auto windowId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(window), detail::windowIdKey()));
    g_object_set_data(G_OBJECT(bar), detail::statusBarWindowIdKey(), reinterpret_cast<gpointer>(windowId));

    auto old = detail::statusBarTable().find(barId);
    if (old != detail::statusBarTable().end()) {
        gtk_widget_destroy(old->second);
    }
    detail::statusBarTable()[barId] = bar;
    detail::statusBarFieldFrames()[barId].clear();
    detail::activeStatusBarId() = barId;
    return 1;
}

/// Takes no `#StatusBar` argument - operates on `activeStatusBarId()`, the
/// same "current context" pattern `MenuTitle`/`MenuItem` above use.
/// `Width = #PB_Ignore` (`-65535`, oracle-verified) auto-sizes the field to
/// share whatever space is left, via `GtkBox`'s own `expand`/`fill` instead
/// of an explicit size request.
inline std::int64_t pbAddStatusBarField(std::int64_t width) {
    auto barId = detail::activeStatusBarId();
    auto it = detail::statusBarTable().find(barId);
    if (it == detail::statusBarTable().end()) {
        return 0;
    }
    GtkWidget* frame = gtk_frame_new(nullptr);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    GtkWidget* label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
    gtk_container_add(GTK_CONTAINER(frame), label);
    bool autoSize = width == -65535; // #PB_Ignore
    if (!autoSize) {
        gtk_widget_set_size_request(frame, static_cast<int>(width), -1);
    }
    gtk_box_pack_start(GTK_BOX(it->second), frame, autoSize ? TRUE : FALSE, autoSize ? TRUE : FALSE, 0);
    gtk_widget_show_all(frame);
    detail::statusBarFieldFrames()[barId].push_back(frame);
    return 1;
}

/// `Apparence`'s bits (`#PB_StatusBar_Raised`=1/`BorderLess`=2/`Center`=4/
/// `Right`=8, oracle-verified) are independent, unlike `MessageRequester`'s
/// own grouped flags - `Raised`/`BorderLess` both pick the field's own
/// `GtkFrame` shadow style (mutually exclusive in practice; `BorderLess`
/// wins if a caller somehow sets both) and `Center`/`Right` the label's own
/// horizontal alignment (likewise mutually exclusive, `Right` winning).
inline std::int64_t pbStatusBarText(std::int64_t barId, std::int64_t field, const PBString& text,
                                     std::int64_t flags = 0) {
    auto it = detail::statusBarFieldFrames().find(barId);
    if (it == detail::statusBarFieldFrames().end() || field < 0 ||
        static_cast<std::size_t>(field) >= it->second.size()) {
        return 0;
    }
    GtkWidget* frame = it->second[static_cast<std::size_t>(field)];
    GtkWidget* label = gtk_bin_get_child(GTK_BIN(frame));
    gtk_label_set_text(GTK_LABEL(label), text.bytes().c_str());

    float xalign = 0.0F;
    if ((flags & 8) != 0) {
        xalign = 1.0F; // #PB_StatusBar_Right
    } else if ((flags & 4) != 0) {
        xalign = 0.5F; // #PB_StatusBar_Center
    }
    gtk_label_set_xalign(GTK_LABEL(label), xalign);

    GtkShadowType shadow = GTK_SHADOW_IN;
    if ((flags & 2) != 0) {
        shadow = GTK_SHADOW_NONE; // #PB_StatusBar_BorderLess
    } else if ((flags & 1) != 0) {
        shadow = GTK_SHADOW_OUT; // #PB_StatusBar_Raised
    }
    gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow);
    return 1;
}

/// Oracle-verified: real PB's own `IsStatusBar` also returns a native-
/// handle-ish nonzero value - simplified the same way `pbIsWindow`'s own is.
inline std::int64_t pbIsStatusBar(std::int64_t barId) { return detail::statusBarTable().contains(barId) ? 1 : 0; }

/// `#PB_All` (`-1`) frees every remaining status bar at once, the same
/// convention as `pbFreeMenu`'s own.
inline std::int64_t pbFreeStatusBar(std::int64_t barId) {
    if (barId == -1) {
        for (auto& [id, widget] : detail::statusBarTable()) {
            gtk_widget_destroy(widget);
        }
        detail::statusBarTable().clear();
        detail::statusBarFieldFrames().clear();
        return 1;
    }
    auto it = detail::statusBarTable().find(barId);
    if (it == detail::statusBarTable().end()) {
        return 0;
    }
    gtk_widget_destroy(it->second);
    detail::statusBarTable().erase(it);
    detail::statusBarFieldFrames().erase(barId);
    return 1;
}

/// See `pbMenuHeight`'s own doc comment for why pending events are drained
/// first - the same freshly-built-widget-not-yet-size-allocated concern
/// applies here too.
inline std::int64_t pbStatusBarHeight(std::int64_t barId) {
    auto it = detail::statusBarTable().find(barId);
    if (it == detail::statusBarTable().end()) {
        return 0;
    }
    while (gtk_events_pending() != 0) {
        gtk_main_iteration();
    }
    return gtk_widget_get_allocated_height(it->second);
}

/// Oracle-verified native-handle-ish value, not replicated-for-a-reason the
/// same way `pbMenuID`'s own is (see its doc comment).
inline std::int64_t pbStatusBarID(std::int64_t barId) {
    auto it = detail::statusBarTable().find(barId);
    return it != detail::statusBarTable().end() ? reinterpret_cast<std::int64_t>(it->second) : 0;
}

/// M7b's fifth GUI slice: the Image library, scoped to just enough to
/// unblock `CreateImageMenu`/`MenuItem`/`OpenSubMenu`'s own `ImageID`
/// argument (landed alongside this) and (a follow-up) `ToolBar`, which has
/// no way to create a button without one either - see this project's own
/// M7b fourth-slice notes on that blocker. `GdkPixbuf` is the natural GTK3
/// fit for a real PB Image: a bitmap independent of any window, exactly
/// what `CreateImage`/`LoadImage` need.
///
/// Depth/BackgroundColor (this function's own optional 4th/5th real-PB
/// arguments) aren't supported yet - deliberately: `RGB()`/`RGBA()` don't
/// exist anywhere in this project yet either, so there's no way to
/// construct a meaningful color argument for them in the first place.
/// Every image created here is 24-bit-equivalent with a black background,
/// matching real PB's own documented default for when they're omitted.
///
/// Oracle-verified: real PB's own "non-zero on success" return value for
/// this function is actually the real image handle itself (confirmed
/// directly: identical to what `ImageID()` then returns) - simplified
/// here to a clean `1`/`0` anyway, the same established convention as
/// `pbOpenWindow`'s own return (real programs needing the actual handle
/// call `ImageID()` explicitly, exactly like this function's own official
/// example does).
inline std::int64_t pbCreateImage(std::int64_t imageId, std::int64_t width, std::int64_t height) {
    if (width <= 0 || height <= 0) {
        return 0;
    }
    GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, static_cast<int>(width), static_cast<int>(height));
    if (pixbuf == nullptr) {
        return 0;
    }
    gdk_pixbuf_fill(pixbuf, 0x000000ff); // Black - real PB's own documented default background.
    auto old = detail::imageTable().find(imageId);
    if (old != detail::imageTable().end()) {
        g_object_unref(old->second);
    }
    detail::imageTable()[imageId] = pixbuf;
    return 1;
}

/// Oracle-verified: a bad/nonexistent file path is a harmless failure
/// (`0`), not a fatal debugger error - confirmed directly, unlike most
/// other invalid-handle cases this library's own functions hit (see
/// `pbImageWidth`'s own doc comment). `GdkPixbuf` auto-detects BMP/PNG/
/// JPEG/GIF/TIFF/ICO from file content - a deliberate, documented
/// divergence from real PB, which requires an explicit `UsePNGImageDecoder`/
/// etc. call first for anything beyond BMP (none of which exist in this
/// project yet); not replicated as a *restriction* since there's no
/// reason to turn a working load into a failure for projects that
/// otherwise pass a PNG/JPEG straight through.
inline std::int64_t pbLoadImage(std::int64_t imageId, const PBString& filename) {
    GError* error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(filename.bytes().c_str(), &error);
    if (pixbuf == nullptr) {
        if (error != nullptr) {
            g_error_free(error);
        }
        return 0;
    }
    auto old = detail::imageTable().find(imageId);
    if (old != detail::imageTable().end()) {
        g_object_unref(old->second);
    }
    detail::imageTable()[imageId] = pixbuf;
    return 1;
}

/// Oracle-verified: deliberately crash-proof for any argument, including
/// one that was never created at all - real PB's own docs say so
/// explicitly ("fonction... créée pour pouvoir passer n'importe quelle
/// valeur en paramètre sans qu'il ne puisse y avoir de plantage"), unlike
/// `ImageWidth`/`ImageHeight`/`ImageID` themselves (see their own doc
/// comments). Oracle-verified real PB's own "non-zero" here is also a
/// native-handle-ish value, not a clean `1` - simplified the same way
/// `pbIsWindow`'s own is.
inline std::int64_t pbIsImage(std::int64_t imageId) { return detail::imageTable().contains(imageId) ? 1 : 0; }

/// `#PB_All` (`-1`) frees every remaining image at once - the same
/// established convention as `pbFreeMenu`/`pbFreeStatusBar`. Oracle-
/// verified no return value ("Aucune") - always returns `0`, the same
/// established convention as `pbKillThread`'s own (see its doc comment).
inline std::int64_t pbFreeImage(std::int64_t imageId) {
    if (imageId == -1) {
        for (auto& [id, pixbuf] : detail::imageTable()) {
            g_object_unref(pixbuf);
        }
        detail::imageTable().clear();
        return 0;
    }
    auto it = detail::imageTable().find(imageId);
    if (it != detail::imageTable().end()) {
        g_object_unref(it->second);
        detail::imageTable().erase(it);
    }
    return 0;
}

/// Oracle-verified: an unknown/never-created `#Image` is a **fatal
/// debugger error** in real PB ("The specified #Image is not
/// initialised."), unlike `IsImage`'s own deliberately crash-proof
/// contract (see its doc comment) - pbcxx deliberately diverges there,
/// the same established precedent as `pbKillThread`'s own doc comment
/// describes, by just returning a harmless `0` instead.
inline std::int64_t pbImageID(std::int64_t imageId) {
    auto it = detail::imageTable().find(imageId);
    return it != detail::imageTable().end() ? reinterpret_cast<std::int64_t>(it->second) : 0;
}

/// See `pbImageID`'s own doc comment on the fatal-debugger-error divergence
/// - applies here too.
inline std::int64_t pbImageWidth(std::int64_t imageId) {
    auto it = detail::imageTable().find(imageId);
    return it != detail::imageTable().end() ? gdk_pixbuf_get_width(it->second) : 0;
}

/// See `pbImageID`'s own doc comment on the fatal-debugger-error divergence
/// - applies here too.
inline std::int64_t pbImageHeight(std::int64_t imageId) {
    auto it = detail::imageTable().find(imageId);
    return it != detail::imageTable().end() ? gdk_pixbuf_get_height(it->second) : 0;
}

} // namespace easybasic::runtime
