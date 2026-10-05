#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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

/// M7b's ninth GUI slice: `ContainerGadget`'s own "gadget-list stack" -
/// oracle-verified (`ContainerGadget.html`'s own "Remarques"): once a
/// container gadget is created, every gadget created afterward becomes
/// part of it, until a matching `CloseGadgetList()` returns to whatever
/// was current before - containers can nest (confirmed directly: an
/// inner container's own `GadgetX()` is relative to its *immediate*
/// parent container, not the outermost window), so this is a real stack,
/// not a single global - designed to be shared, unchanged, by
/// `PanelGadget`/`ScrollAreaGadget` whenever either of those is
/// implemented, the same shared-infrastructure shape the Qt6 sibling
/// project's own equivalent design settled on. `fixed` is the container's
/// own inner `GtkFixed` - every other gadget-creation function keeps
/// calling `placeGadget` exactly as it already does; the stack is
/// consulted entirely inside `placeGadget` itself, so none of them need
/// to know this exists at all.
struct GadgetListFrame {
    std::int64_t containerId = 0;
    GtkWidget* fixed = nullptr;
};

inline std::vector<GadgetListFrame>& gadgetListStack() {
    static std::vector<GadgetListFrame> stack;
    return stack;
}

/// `#Gadget` (a container) -> its own inner `GtkFixed` - what
/// `OpenGadgetList` re-pushes onto `gadgetListStack()` to resume adding
/// gadgets to a container after an earlier `CloseGadgetList()`.
inline std::unordered_map<std::int64_t, GtkWidget*>& containerFixedTable() {
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

/// M7b's sixth GUI slice: `ToolBar`. `#ToolBar` -> its own `GtkToolbar`.
inline std::unordered_map<std::int64_t, GtkWidget*>& toolBarTable() {
    static std::unordered_map<std::int64_t, GtkWidget*> table;
    return table;
}

/// `#ToolBar` -> (`Button` -> its own `GtkToolButton*`/`GtkToggleToolButton*`)
/// - what `DisableToolBarButton`/`Get`/`SetToolBarButtonState`/
/// `ToolBarButtonText`/`ToolBarToolTip` all address, the same shape
/// `menuItemWidgets()` already has for leaf `MenuItem()`s.
inline std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, GtkWidget*>>& toolBarButtonWidgets() {
    static std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, GtkWidget*>> table;
    return table;
}

/// `#ToolBar` -> the pixel size `ToolBarImageButton` should scale its own
/// `ImageID` to - oracle-verified `CreateToolBar`'s own `#PB_ToolBar_Small`
/// (16, the default) vs `#PB_ToolBar_Large` (24) `Options` bit, decided once
/// at creation time rather than threaded through every button call.
inline std::unordered_map<std::int64_t, int>& toolBarIconPixelSize() {
    static std::unordered_map<std::int64_t, int> table;
    return table;
}

inline std::int64_t& activeToolBarId() {
    static std::int64_t id = 0;
    return id;
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

/// Shared by every Requester function taking an optional `ParentID` -
/// `0` (never a legitimate `WindowID()` value) means "no parent", the
/// same convention every other optional-handle parameter in this library
/// already uses. A real connection to `WindowID()`'s own return value
/// (the real `GtkWidget*` pointer `pbWindowID` already established) -
/// unlike the reference `qt6_subsystem` project's own documented gap here
/// ("`ParentID` is accepted on every overload... but not connected to
/// actual window parenting"), this project's own existing "the handle
/// already is the real pointer" convention makes wiring it up free.
inline GtkWindow* parentWindowFromId(std::int64_t parentId) {
    return parentId != 0 ? GTK_WINDOW(reinterpret_cast<GtkWidget*>(parentId)) : nullptr;
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
inline const char* toolBarWindowIdKey() { return "pbcxx-toolbar-window-id"; }
// M7b's ninth GUI slice: `ContainerGadget`. The immediately-enclosing
// container's own PB gadget ID (0 if placed directly into a window) -
// read back by `pruneContainerGadgets` so freeing a container also frees
// every gadget nested inside it, at any depth, the same way GTK itself
// already recursively destroys their *widgets*.
inline const char* gadgetContainerIdKey() { return "pbcxx-gadget-container-id"; }
// M7b's eleventh GUI slice: `SplitterGadget`. The minimum size (in
// pixels) each of its own two panes should keep, tagged directly on the
// `GtkPaned` itself (not its current child - `SetGadgetAttribute`'s own
// `#PB_Splitter_FirstGadget`/`SecondGadget` can swap a pane's child out
// later, and a previously-set minimum size still applies to whatever
// replaces it, oracle-verified real PB's own docs never say it resets).
inline const char* splitterFirstMinSizeKey() { return "pbcxx-splitter-first-minsize"; }
inline const char* splitterSecondMinSizeKey() { return "pbcxx-splitter-second-minsize"; }
// `#PB_Splitter_FirstFixed`/`SecondFixed`'s own resolved `gtk_paned_pack1`/
// `pack2` "resize" bool, tagged the same way - `SetGadgetAttribute`'s own
// `FirstGadget`/`SecondGadget` replacement needs to re-pack the new child
// with the *same* resize behavior the Splitter was created with, and
// GtkPaned has no public getter for a child's own already-packed
// "resize" property to read it back from instead.
inline const char* splitterFirstResizeKey() { return "pbcxx-splitter-first-resize"; }
inline const char* splitterSecondResizeKey() { return "pbcxx-splitter-second-resize"; }

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

/// `GtkNotebook::switch-page` (M7b's tenth GUI slice: `PanelGadget`) -
/// oracle-verified a real tab switch reports `#PB_EventType_Change`
/// (`768`) as `EventType()`, the same value `StringGadget`'s own edits
/// use. `switch-page`'s own signal shape (`notebook, page, page_num,
/// data`) is GTK-specific to `GtkNotebook`, unlike every other gadget
/// signal this header connects, which is why this isn't just another
/// call to `onGadgetChanged` directly - GTK's signal marshaling requires
/// the handler's own parameter list to match.
inline void onPanelSwitchPage(GtkNotebook* notebook, GtkWidget*, guint, gpointer) {
    queueGadgetEvent(GTK_WIDGET(notebook), 768);
}

/// `GtkAdjustment::value-changed` (M7b's twelfth GUI slice:
/// `ScrollAreaGadget`), connected to both the horizontal and vertical
/// adjustment - oracle-verified real PB's own docs: "Un évènement est
/// généré lorsque l'utilisateur déplace les ascenseurs du gadget", but
/// its own `EventType()` isn't independently confirmed (simulating a
/// real scrollbar drag against the oracle needs interactive automation
/// this project's own test methodology deliberately avoids - every GUI
/// slice's own e2e case says so explicitly); `0` is used as the same
/// "nothing distinctive" default `ButtonGadget`/`CheckBoxGadget`'s own
/// clicks already use, a reasonable choice rather than a confirmed one.
/// `userData` carries the real `GtkScrolledWindow*` itself (not derivable
/// from the adjustment), tagged with this gadget's own PB id the normal
/// way.
inline void onScrollAreaValueChanged(GtkAdjustment*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 0);
}

/// `GtkTreeSelection::changed` (M7b's thirteenth GUI slice:
/// `ListViewGadget`) - oracle-verified via `ListViewGadget.html`'s own
/// remarks: `#PB_EventType_LeftClick` ("également déclenché lors d'un
/// changement de sélection") is the real event a selection change
/// reports, not a dedicated `Change` the way `PanelGadget`'s own tab
/// switch does. `userData` carries the real `GtkScrolledWindow*` wrapper
/// itself (the gadget's own tagged widget, not the inner `GtkTreeView`
/// the selection belongs to) - blocked around `SetGadgetState`/
/// `SetGadgetItemState`'s own programmatic changes, the same established
/// pattern every other gadget's own state-setter already uses.
inline void onListViewSelectionChanged(GtkTreeSelection*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 0);
}

/// `GtkTreeView::row-activated` (a real double-click, or Enter/Return on
/// the selected row) - oracle-verified `#PB_EventType_LeftDoubleClick`
/// (`2`) via `ListViewGadget.html`'s own remarks.
inline void onListViewRowActivated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 2);
}

/// `GtkTreeView::button-press-event` - the only way to distinguish a
/// right-click from `GtkTreeSelection::changed` alone (which fires for
/// *any* button, or none at all for keyboard navigation) - oracle-
/// verified `#PB_EventType_RightClick` (`1`) via `ListViewGadget.html`'s
/// own remarks. Returns `FALSE` (GTK's own "didn't handle it, keep
/// propagating") unconditionally - this only ever *observes* the click,
/// never needs to stop GTK's own default handling (e.g. showing a
/// context-appropriate selection) from running too.
inline gboolean onListViewButtonPress(GtkWidget*, const GdkEventButton* event, gpointer userData) {
    if (event->button == 3) {
        queueGadgetEvent(GTK_WIDGET(userData), 1);
    }
    return FALSE;
}

/// `GtkComboBox::changed` (M7b's thirteenth GUI slice: `ComboBoxGadget`)
/// - oracle-verified `#PB_EventType_Change` (`768`, the same value
/// `StringGadget`'s own edits and `PanelGadget`'s own tab switch use) via
/// `ComboBoxGadget.html`'s own remarks. Fires for *either* a new item
/// becoming active or (for an editable combo) the entry's own text
/// changing - blocked around `SetGadgetState`/`SetGadgetText`'s own
/// programmatic changes.
inline void onComboBoxChanged(GtkWidget* combo, gpointer) { queueGadgetEvent(combo, 768); }

/// `GtkEntry::focus-in-event`/`focus-out-event`, on an editable combo's
/// own internal entry widget only - oracle-verified `#PB_EventType_Focus`
/// (`256`)/`LostFocus` (`512`) via `ComboBoxGadget.html`'s own remarks
/// ("ComboBox modifiable uniquement"). `userData` carries the combo
/// itself (the entry has no gadget ID of its own tagged on it - it was
/// never placed via `placeGadget`, just packed inside the combo).
/// Returns `FALSE` unconditionally, the same reason `onListViewButtonPress`
/// already does.
inline gboolean onComboBoxFocusIn(GtkWidget*, GdkEvent*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 256);
    return FALSE;
}
inline gboolean onComboBoxFocusOut(GtkWidget*, GdkEvent*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 512);
    return FALSE;
}

/// `ListIconGadget`'s own `CHECKED` store column index and `TreeGadget`'s
/// own - needed by the toggle handlers below, defined this early (rather
/// than alongside the rest of each gadget's own column-layout constants
/// further down) purely so those handlers can see them; see
/// `listIconMaxColumns`'s own doc comment for the full column-layout
/// rationale and `treeTextColumn`'s own for `TreeGadget`'s own model
/// shape.
inline int listIconCheckedColumn() { return 17; }
inline int treeCheckedColumn() { return 2; }

/// M7b's fourteenth GUI slice: `ListIconGadget`/`TreeGadget` share the
/// same event wiring shape `ListViewGadget`'s own thirteenth slice
/// already established (`GtkTreeSelection::changed`/`row-activated`/
/// `button-press-event`), extended for the two additional event types
/// real PB's own docs list for both that `ListViewGadget` doesn't have:
/// `#PB_EventType_RightDoubleClick` (`3`) and `#PB_EventType_Change`
/// (`768`) - the latter fired from the exact same selection-change signal
/// `LeftClick` already is, a reasonable simplification given real PB's
/// own precise distinction between the two (a plain click vs. the
/// selection specifically changing) isn't independently verifiable
/// without the interactive click automation this project's own test
/// methodology deliberately avoids.
inline void onListIconSelectionChanged(GtkTreeSelection*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 0);
    queueGadgetEvent(GTK_WIDGET(userData), 768);
}
inline void onListIconRowActivated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer userData) {
    queueGadgetEvent(GTK_WIDGET(userData), 2);
}
inline gboolean onListIconButtonPress(GtkWidget*, const GdkEventButton* event, gpointer userData) {
    if (event->button == 3) {
        bool isDouble = event->type == GDK_2BUTTON_PRESS;
        queueGadgetEvent(GTK_WIDGET(userData), isDouble ? 3 : 1);
    }
    return FALSE;
}

/// `GtkCellRendererToggle::toggled` - unlike a plain `GtkTreeSelection`,
/// GTK doesn't auto-update the bound model column on a checkbox click;
/// this flips `listIconCheckedColumn()`'s own value for the clicked row
/// manually, then queues the same `LeftClick`/`Change` pair a selection
/// change does - oracle-verified `ListIconGadget.html`'s own remarks:
/// "#PB_EventType_LeftClick : ... ou une case à cocher a été
/// cochée/décochée".
inline void onListIconToggle(GtkCellRendererToggle* renderer, gchar* pathStr, gpointer userData) {
    GtkWidget* treeView = gtk_bin_get_child(GTK_BIN(static_cast<GtkWidget*>(userData)));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
    GtkTreeIter iter;
    GtkTreePath* path = gtk_tree_path_new_from_string(pathStr);
    if (gtk_tree_model_get_iter(model, &iter, path) != 0) {
        gboolean current = gtk_cell_renderer_toggle_get_active(renderer);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, listIconCheckedColumn(), current == 0 ? TRUE : FALSE, -1);
    }
    gtk_tree_path_free(path);
    queueGadgetEvent(static_cast<GtkWidget*>(userData), 0);
    queueGadgetEvent(static_cast<GtkWidget*>(userData), 768);
}

/// `TreeGadget`'s own sibling of `onListIconToggle` - the same idea, but
/// against its own `GtkTreeStore` (`gtk_tree_store_set`, not
/// `gtk_list_store_set` - different C functions even though both
/// ultimately write through the same `GtkTreeModel` interface) and its
/// own `treeCheckedColumn()`.
inline void onTreeToggle(GtkCellRendererToggle* renderer, gchar* pathStr, gpointer userData) {
    GtkWidget* treeView = gtk_bin_get_child(GTK_BIN(static_cast<GtkWidget*>(userData)));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
    GtkTreeIter iter;
    GtkTreePath* path = gtk_tree_path_new_from_string(pathStr);
    if (gtk_tree_model_get_iter(model, &iter, path) != 0) {
        gboolean current = gtk_cell_renderer_toggle_get_active(renderer);
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, treeCheckedColumn(), current == 0 ? TRUE : FALSE, -1);
    }
    gtk_tree_path_free(path);
    queueGadgetEvent(static_cast<GtkWidget*>(userData), 0);
    queueGadgetEvent(static_cast<GtkWidget*>(userData), 768);
}

/// `GtkTreeViewColumn::clicked` (a header click) - oracle-verified
/// `#PB_EventType_ColumnClick` (`8`); the clicked column's own visual
/// position (what `#PB_ListIcon_ClickedColumn` reports back via
/// `GetGadgetAttribute`) is recorded on the `GtkTreeView` itself, read
/// back by `pbGetGadgetAttribute`.
inline const char* listIconClickedColumnKey() { return "pbcxx-listicon-clicked-column"; }
inline void onListIconColumnClicked(GtkTreeViewColumn* column, gpointer userData) {
    auto* scrolled = static_cast<GtkWidget*>(userData);
    GtkWidget* treeView = gtk_bin_get_child(GTK_BIN(scrolled));
    GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(treeView));
    int position = g_list_index(columns, column);
    g_list_free(columns);
    g_object_set_data(G_OBJECT(treeView), listIconClickedColumnKey(), reinterpret_cast<gpointer>(static_cast<std::intptr_t>(position)));
    queueGadgetEvent(scrolled, 8);
}

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
        if (owner == windowId) {
            // M7b's ninth GUI slice: a container gadget owned by this
            // window needs its own `containerFixedTable()` entry cleared
            // too, same risk `gadgetTable()` itself already had - a plain
            // erase (not `pruneContainerGadgets`, which is for a single
            // live container being freed on its own) since this loop is
            // already visiting every gadget this window owns, nested or
            // not.
            containerFixedTable().erase(it->first);
            it = gadgetTable().erase(it);
        } else {
            ++it;
        }
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
    // Same idea again for M7b's sixth GUI slice's own owned-widget table.
    for (auto it = toolBarTable().begin(); it != toolBarTable().end();) {
        auto owner = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(it->second), toolBarWindowIdKey()));
        if (owner == windowId) {
            toolBarButtonWidgets().erase(it->first);
            toolBarIconPixelSize().erase(it->first);
            it = toolBarTable().erase(it);
        } else {
            ++it;
        }
    }
    gtk_widget_destroy(window);
}

/// M7b's fifteenth GUI slice: `OptionGadget`'s own radio-button grouping -
/// oracle-verified directly ("Au premier appel de cette fonction, un
/// groupe de cases à options est créé, et tous les appels suivants
/// ajouteront une nouvelle case à options au groupe. Pour terminer le
/// groupe, il suffit d'appeler un autre type de gadget."): the *last*
/// `OptionGadget` successfully placed, so long as nothing else has been
/// placed since - `placeGadget`'s own end clears this back to `nullptr`
/// whenever the widget it just placed *isn't* a `GtkRadioButton`, the
/// literal "calling another gadget type ends the group" rule, needing no
/// special-casing in any other gadget's own creation function at all. A
/// single global rather than scoped per-window - real PB programs
/// grouping radio buttons across two different top-level windows at once
/// would be a bizarre thing to rely on, and this isn't independently
/// oracle-verified for that specific cross-window case.
inline GtkWidget*& optionGroupAnchor() {
    static GtkWidget* anchor = nullptr;
    return anchor;
}

/// Shared by every gadget-creation function: places `widget` into the
/// *current* `GtkFixed` - `windowId`'s own top-level one, unless
/// `gadgetListStack()` is non-empty, in which case its own top frame's
/// inner `GtkFixed` (a container gadget's own, M7b's ninth GUI slice) -
/// at the given position/size, tags it with its PB gadget ID, owning
/// window ID (read back by `queueGadgetEvent` and `destroyWindow`), and
/// immediately-enclosing container ID (`0` if none - read back by
/// `pruneContainerGadgets`), and records it in `gadgetTable()`. Returns
/// `false` (and destroys `widget` again without placing it) if `windowId`
/// isn't a currently open window - a real PB program is very unlikely to
/// reference one deliberately, and this project's own established stance
/// only replicates real PB's debugger-fatal-error behavior for cases
/// already found worth the trouble (see M7a's `KillThread` notes). Every
/// existing gadget-creation function keeps calling this exactly as before
/// (passing `windowId` for bookkeeping, never the target `GtkFixed`
/// itself) - container-awareness lives entirely here, nowhere else.
inline bool placeGadget(std::int64_t windowId, std::int64_t gadgetId, std::int64_t x, std::int64_t y,
                         std::int64_t width, std::int64_t height, GtkWidget* widget) {
    GtkWidget* fixed = nullptr;
    std::int64_t containerId = 0;
    if (!gadgetListStack().empty()) {
        fixed = gadgetListStack().back().fixed;
        containerId = gadgetListStack().back().containerId;
    } else {
        auto fixedIt = windowFixedTable().find(windowId);
        fixed = fixedIt == windowFixedTable().end() ? nullptr : fixedIt->second;
    }
    if (fixed == nullptr) {
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
    g_object_set_data(G_OBJECT(widget), gadgetContainerIdKey(), reinterpret_cast<gpointer>(containerId));
    gtk_fixed_put(GTK_FIXED(fixed), widget, static_cast<int>(x), static_cast<int>(y));
    gtk_widget_set_size_request(widget, static_cast<int>(width), static_cast<int>(height));
    gtk_widget_show(widget);
    gadgetTable()[gadgetId] = widget;
    optionGroupAnchor() = GTK_IS_RADIO_BUTTON(widget) != 0 ? widget : nullptr;
    return true;
}

/// `pbFreeGadget`'s own recursive half: erases every gadget nested inside
/// `containerId`, at any depth, from `gadgetTable()`/`containerFixedTable()`
/// - oracle-verified directly (freeing an outer container makes `IsGadget`
/// false for *every* gadget nested inside it, not just its own immediate
/// children) - GTK itself already recursively destroys the actual
/// *widgets* as children of the container being destroyed; this just keeps
/// this project's own id-keyed bookkeeping from being left holding
/// dangling pointers, the same risk every other owned-widget table in this
/// header already has. Safe to call before the container's own widget is
/// destroyed: erasing a *different* key from `gadgetTable()` never
/// invalidates `it` from `pbFreeGadget`'s own, separate lookup.
inline void pruneContainerGadgets(std::int64_t containerId) {
    for (auto it = gadgetTable().begin(); it != gadgetTable().end();) {
        auto owner = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(it->second), gadgetContainerIdKey()));
        if (owner == containerId) {
            auto childId = it->first;
            pruneContainerGadgets(childId);
            containerFixedTable().erase(childId);
            it = gadgetTable().erase(it);
        } else {
            ++it;
        }
    }
}

/// `pbRemoveGadgetItem`/`pbClearGadgetItems`'s own recursive half (M7b's
/// tenth GUI slice: `PanelGadget`) - a sibling to `pruneContainerGadgets`
/// above, needed because that one prunes by a *whole gadget's* own
/// `gadgetContainerIdKey()` (every tab of a Panel shares the same one,
/// the Panel's own `#Gadget` ID - correct for freeing the *entire* Panel,
/// wrong for removing just *one* of its tabs). This instead walks
/// `gadgetTable()` once and keeps whatever GTK's own `gtk_widget_is_
/// ancestor` already knows how to answer - is this gadget's own widget a
/// descendant of `root` (one tab's own inner `GtkFixed`) - which finds
/// every nested gadget at any depth, including a container/another Panel
/// nested inside this tab, without needing its own recursion at all.
inline void pruneGadgetsUnderWidget(GtkWidget* root) {
    for (auto it = gadgetTable().begin(); it != gadgetTable().end();) {
        if (gtk_widget_is_ancestor(it->second, root) != 0) {
            containerFixedTable().erase(it->first);
            it = gadgetTable().erase(it);
        } else {
            ++it;
        }
    }
}

/// `AddGadgetItem`'s own tab-label widget (M7b's tenth GUI slice:
/// `PanelGadget`) - a plain `GtkLabel` with no `ImageID`, otherwise a
/// `GtkBox` pairing a 16x16 icon with one (oracle-verified: "Les
/// dimensions des images sont de 16x16 pixels", the exact wording
/// `attachMenuItemImage`'s own doc comment already cites for `MenuItem`'s
/// identical `ImageID` convention - not duplicated here since a tab label
/// isn't a `GtkBin` being retrofitted, it's built fresh for a brand new
/// page, so swapping a single child in place doesn't apply).
inline GtkWidget* buildTabLabel(const std::string& text, std::int64_t imageId) {
    GtkWidget* label = gtk_label_new(text.c_str());
    if (imageId == 0) {
        return label;
    }
    auto* pixbuf = reinterpret_cast<GdkPixbuf*>(imageId);
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, 16, 16, GDK_INTERP_BILINEAR);
    if (scaled == nullptr) {
        return label;
    }
    GtkWidget* image = gtk_image_new_from_pixbuf(scaled);
    g_object_unref(scaled);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(image);
    gtk_widget_show(label);
    return box;
}

/// Finds the real `GtkLabel` inside a tab's own label widget, whether
/// it's the widget itself (no `ImageID`) or nested inside `buildTabLabel`'s
/// own `GtkBox` - the `PanelGadget` equivalent of `menuItemLabel` below,
/// shared by `pbGetGadgetItemText`/`pbSetGadgetItemText`.
inline GtkWidget* findLabelInTabWidget(GtkWidget* tabLabelWidget) {
    if (tabLabelWidget == nullptr) {
        return nullptr;
    }
    if (GTK_IS_LABEL(tabLabelWidget) != 0) {
        return tabLabelWidget;
    }
    if (GTK_IS_CONTAINER(tabLabelWidget) != 0) {
        GList* children = gtk_container_get_children(GTK_CONTAINER(tabLabelWidget));
        GtkWidget* label = nullptr;
        for (GList* l = children; l != nullptr; l = l->next) {
            if (GTK_IS_LABEL(l->data) != 0) {
                label = GTK_WIDGET(l->data);
                break;
            }
        }
        g_list_free(children);
        return label;
    }
    return nullptr;
}

/// M7b's thirteenth GUI slice: `ListViewGadget`/`ComboBoxGadget` share
/// the exact same two-column model (`TEXT`, `DATA` - see
/// `itemListColumn()` below) and so share every "universal item" function
/// (`AddGadgetItem`/`CountGadgetItems`/etc.) uniformly too, dispatched
/// through this one helper rather than duplicating the model-manipulation
/// logic per widget type. Returns `nullptr` for anything else (falling
/// through to `PanelGadget`'s own, separate `GtkNotebook`-based handling
/// in each caller) - a `ListViewGadget` is a `GtkScrolledWindow` wrapping
/// a `GtkTreeView` directly (unlike `ScrollAreaGadget`'s own
/// `GtkScrolledWindow`, whose child is an auto-created `GtkViewport`
/// wrapping a plain `GtkFixed` - `GtkTreeView` implements `GtkScrollable`
/// natively, so no such auto-wrapping happens for it, letting this
/// distinguish the two cases just by checking the immediate child's own
/// type), while `ComboBoxGadget` is a plain `GtkComboBox` stored directly,
/// no wrapper at all.
/// M7b's fourteenth GUI slice (`ListIconGadget`/`TreeGadget`) adds two
/// more `GtkTreeView`-based gadget types, both also wrapped in a
/// `GtkScrolledWindow` the exact same way `ListViewGadget`'s own already
/// is - `itemListStoreFor`/`listViewTreeView` below need to tell a bare
/// `ListViewGadget` apart from the other two now, since their own column
/// layouts are completely different (`ListIconGadget`'s own has many text
/// columns plus a checkbox one; `TreeGadget`'s own model is hierarchical,
/// a `GtkTreeStore`, not a flat `GtkListStore` at all) - tagged directly
/// on the `GtkTreeView` itself at creation time (`ListViewGadget`'s own
/// stays untagged, so existing behavior for it needs no changes at all).
inline const char* gadgetKindKey() { return "pbcxx-gadget-kind"; }
inline const char* gadgetKindListIcon() { return "listicon"; }
inline const char* gadgetKindTree() { return "tree"; }

inline GtkListStore* itemListStoreFor(GtkWidget* stored) {
    if (GTK_IS_SCROLLED_WINDOW(stored) != 0) {
        GtkWidget* inner = gtk_bin_get_child(GTK_BIN(stored));
        if (inner != nullptr && GTK_IS_TREE_VIEW(inner) != 0 &&
            g_object_get_data(G_OBJECT(inner), gadgetKindKey()) == nullptr) {
            return GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(inner)));
        }
        return nullptr;
    }
    if (GTK_IS_COMBO_BOX(stored) != 0) {
        return GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(stored)));
    }
    return nullptr;
}

/// The real `GtkTreeView` inside a `ListViewGadget`'s own `GtkScrolledWindow`
/// wrapper, or `nullptr` for anything else - needed (unlike every other
/// "universal item" function) by `GetGadgetItemState`/`SetGadgetItemState`,
/// since selection is a `GtkTreeSelection` concept tied to the *view*, not
/// the model `itemListStoreFor` already extracts - not applicable to
/// `ComboBoxGadget` at all (real PB's own docs don't list either function
/// for it).
inline GtkWidget* listViewTreeView(GtkWidget* stored) {
    if (GTK_IS_SCROLLED_WINDOW(stored) == 0) {
        return nullptr;
    }
    GtkWidget* inner = gtk_bin_get_child(GTK_BIN(stored));
    if (inner == nullptr || GTK_IS_TREE_VIEW(inner) == 0 ||
        g_object_get_data(G_OBJECT(inner), gadgetKindKey()) != nullptr) {
        return nullptr;
    }
    return inner;
}

/// The real `GtkTreeView` inside a `ListIconGadget`'s own `GtkScrolledWindow`
/// wrapper (tagged with `gadgetKindListIcon()` at creation), or `nullptr`
/// for anything else.
inline GtkWidget* listIconTreeView(GtkWidget* stored) {
    if (GTK_IS_SCROLLED_WINDOW(stored) == 0) {
        return nullptr;
    }
    GtkWidget* inner = gtk_bin_get_child(GTK_BIN(stored));
    if (inner == nullptr || GTK_IS_TREE_VIEW(inner) == 0) {
        return nullptr;
    }
    auto* kind = static_cast<const char*>(g_object_get_data(G_OBJECT(inner), gadgetKindKey()));
    return (kind != nullptr && g_strcmp0(kind, gadgetKindListIcon()) == 0) ? inner : nullptr;
}

/// The real `GtkTreeView` inside a `TreeGadget`'s own `GtkScrolledWindow`
/// wrapper (tagged with `gadgetKindTree()` at creation), or `nullptr` for
/// anything else.
inline GtkWidget* treeGadgetTreeView(GtkWidget* stored) {
    if (GTK_IS_SCROLLED_WINDOW(stored) == 0) {
        return nullptr;
    }
    GtkWidget* inner = gtk_bin_get_child(GTK_BIN(stored));
    if (inner == nullptr || GTK_IS_TREE_VIEW(inner) == 0) {
        return nullptr;
    }
    auto* kind = static_cast<const char*>(g_object_get_data(G_OBJECT(inner), gadgetKindKey()));
    return (kind != nullptr && g_strcmp0(kind, gadgetKindTree()) == 0) ? inner : nullptr;
}

/// Column indices for `itemListStoreFor`'s own two-column
/// `gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT64)` model, shared by
/// `ListViewGadget`/`ComboBoxGadget` alike.
inline int itemTextColumn() { return 0; }
inline int itemDataColumn() { return 1; }

/// `ListIconGadget`'s own `GtkListStore` layout: a fixed, generously-sized
/// number of text columns (GTK's own `GtkListStore` can't gain/lose
/// columns once created, unlike real PB's own dynamically growable column
/// list - `AddGadgetColumn`/`RemoveGadgetColumn` only ever add/remove the
/// *view*'s own `GtkTreeViewColumn`s, each tagged with which of these
/// pre-allocated store slots it's bound to via `listIconColumnSlotKey()`,
/// never reassigning or reclaiming one - 16 is far more than any real PB
/// program is likely to need, the same kind of generous-but-finite limit
/// real PB's own "65536 items" cap on `AddGadgetItem` already is), plus
/// one `DATA` column (`GetGadgetItemData`/`SetGadgetItemData`, shared
/// layout idea with `itemDataColumn()` above but not the same index) and
/// one `CHECKED` column (`#PB_ListIcon_CheckBoxes`), always present in
/// the model regardless of whether that flag was actually requested, to
/// keep every `ListIconGadget`'s own model shape uniform.
inline int listIconMaxColumns() { return 16; }
inline int listIconDataColumn() { return listIconMaxColumns(); }
// listIconCheckedColumn() - defined earlier, see its own doc comment.
inline int listIconTotalModelColumns() { return listIconMaxColumns() + 2; }

/// Tag on each `GtkTreeViewColumn` recording which of `ListIconGadget`'s
/// own pre-allocated store slots it displays - `gtk_tree_view_get_columns`
/// returns columns in their current *visual* order (what `AddGadgetColumn`'s
/// own `Position` and `GetGadgetItemText`/`SetGadgetItemText`'s own
/// `Column` both address), which is independent of - and, once a column is
/// inserted anywhere but the end, no longer the same sequence as - the
/// underlying store slot each one is bound to.
inline const char* listIconColumnSlotKey() { return "pbcxx-listicon-column-slot"; }
/// The next not-yet-used store slot for this `ListIconGadget`'s own next
/// `AddGadgetColumn` call, tagged on the `GtkTreeView` itself (starts at
/// `1` - slot `0` is always the column `ListIconGadget` itself creates).
inline const char* listIconNextSlotKey() { return "pbcxx-listicon-next-slot"; }

/// The real `GtkTreeViewColumn` at visual position `position` (what a
/// `ListIconGadget`'s own `AddGadgetColumn`/`GetGadgetItemText` etc. all
/// address), or `nullptr` if out of range.
inline GtkTreeViewColumn* listIconColumnAtPosition(GtkTreeView* treeView, int position) {
    if (position < 0) {
        return nullptr;
    }
    GList* columns = gtk_tree_view_get_columns(treeView);
    GList* nth = g_list_nth(columns, static_cast<guint>(position));
    auto* column = nth != nullptr ? GTK_TREE_VIEW_COLUMN(nth->data) : nullptr;
    g_list_free(columns);
    return column;
}

/// The underlying store slot a visual column position is bound to, or
/// `-1` if that position doesn't exist.
inline int listIconStoreSlotAtPosition(GtkTreeView* treeView, int position) {
    GtkTreeViewColumn* column = listIconColumnAtPosition(treeView, position);
    if (column == nullptr) {
        return -1;
    }
    return static_cast<int>(reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(column), listIconColumnSlotKey())));
}

/// `AddGadgetItem`/`SetGadgetItemText`'s own `Chr(10)`-separated
/// multi-column text, oracle-verified via `ListIconGadget.html`'s own
/// remarks: "premiÃ¨re colonne"+Chr(10)+"deuxiÃ¨me colonne".
inline std::vector<std::string> splitByNewline(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t pos = text.find('\n', start);
        parts.push_back(text.substr(start, pos == std::string::npos ? std::string::npos : pos - start));
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    return parts;
}

/// `TreeGadget`'s own model: a two-column `GtkTreeStore` (`TEXT`, `DATA` -
/// the same column *indices* `itemTextColumn()`/`itemDataColumn()` already
/// use, just a hierarchical store instead of a flat one, so the same
/// constants are reused rather than duplicated).
inline int treeTextColumn() { return itemTextColumn(); }
inline int treeDataColumn() { return itemDataColumn(); }
// treeCheckedColumn() - defined earlier, see its own doc comment.

/// `AddGadgetItem`'s own required `Options` (the new item's own level) -
/// oracle-verified directly: real PB tracks an implicit "last item
/// inserted at each depth" per Tree, used to find a new item's own
/// parent (depth `L`'s own parent is whatever's currently at depth
/// `L - 1`) - an invalid (too deep) level clamps to one deeper than
/// the *previous* insertion's own level, not simply the requested one,
/// confirmed with a dedicated probe (jumping from level 0 straight to
/// level 5 lands at level 1, not 5 or an error). Lives as a real
/// `std::vector<GtkTreeIter>`, attached to the `GtkTreeView` itself via
/// `g_object_set_data_full` so it's automatically freed alongside the
/// widget - GTK's own `g_object_data` can only hold a raw `gpointer`,
/// unlike a `GtkTreeIter` which isn't safely representable as one on its
/// own.
inline const char* treeLevelStackKey() { return "pbcxx-tree-level-stack"; }
inline std::vector<GtkTreeIter>& treeLevelStack(GtkWidget* treeView) {
    auto* stack = static_cast<std::vector<GtkTreeIter>*>(g_object_get_data(G_OBJECT(treeView), treeLevelStackKey()));
    if (stack == nullptr) {
        stack = new std::vector<GtkTreeIter>();
        g_object_set_data_full(G_OBJECT(treeView), treeLevelStackKey(), stack,
                                [](gpointer p) { delete static_cast<std::vector<GtkTreeIter>*>(p); });
    }
    return *stack;
}

/// Finds the `GtkTreeIter` at `targetIndex`'s own flat, depth-first,
/// pre-order position across the whole tree - oracle-verified directly to
/// be exactly how real PB's own `Element` addresses a `TreeGadget`'s
/// items (confirmed with a dedicated probe: inserting two root items,
/// two children of the second, then a third root item reports back
/// `Element` 0-4 in exactly that visitation order, not e.g. grouped by
/// level).
inline bool treeIterAtFlatIndexRecursive(GtkTreeModel* model, GtkTreeIter* parent, int& remaining,
                                          GtkTreeIter* out) {
    GtkTreeIter iter;
    if (gtk_tree_model_iter_children(model, &iter, parent) == 0) {
        return false;
    }
    do {
        if (remaining == 0) {
            *out = iter;
            return true;
        }
        --remaining;
        if (treeIterAtFlatIndexRecursive(model, &iter, remaining, out)) {
            return true;
        }
    } while (gtk_tree_model_iter_next(model, &iter) != 0);
    return false;
}
inline bool treeIterAtFlatIndex(GtkTreeModel* model, int targetIndex, GtkTreeIter* out) {
    if (targetIndex < 0) {
        return false;
    }
    int remaining = targetIndex;
    return treeIterAtFlatIndexRecursive(model, nullptr, remaining, out);
}

/// The reverse direction - a given iter's own flat, depth-first index,
/// needed by `GetGadgetState`'s own "which element is selected" query.
/// Two iterators aren't reliably comparable for equality directly, so
/// each candidate is compared by converting both to a `GtkTreePath`
/// instead (`gtk_tree_path_compare`'s own well-defined `0`-means-equal
/// contract).
inline bool treeFlatIndexRecursive(GtkTreeModel* model, GtkTreeIter* parent, GtkTreePath* targetPath, int& counter,
                                    int& result) {
    GtkTreeIter iter;
    if (gtk_tree_model_iter_children(model, &iter, parent) == 0) {
        return false;
    }
    do {
        GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
        bool isMatch = gtk_tree_path_compare(path, targetPath) == 0;
        gtk_tree_path_free(path);
        if (isMatch) {
            result = counter;
            return true;
        }
        ++counter;
        if (treeFlatIndexRecursive(model, &iter, targetPath, counter, result)) {
            return true;
        }
    } while (gtk_tree_model_iter_next(model, &iter) != 0);
    return false;
}
inline int treeFlatIndexOfIter(GtkTreeModel* model, GtkTreeIter* target) {
    GtkTreePath* targetPath = gtk_tree_model_get_path(model, target);
    int counter = 0;
    int result = -1;
    treeFlatIndexRecursive(model, nullptr, targetPath, counter, result);
    gtk_tree_path_free(targetPath);
    return result;
}

/// `CountGadgetItems`'s own total node count across every depth - "Nombre
/// d'éléments actuellement contenus dans le gadget" counts the *whole*
/// tree, not just root-level items.
inline int treeCountAll(GtkTreeModel* model, GtkTreeIter* parent) {
    GtkTreeIter iter;
    if (gtk_tree_model_iter_children(model, &iter, parent) == 0) {
        return 0;
    }
    int count = 0;
    do {
        ++count;
        count += treeCountAll(model, &iter);
    } while (gtk_tree_model_iter_next(model, &iter) != 0);
    return count;
}

/// Shared by `pbSetGadgetText`'s own `ListViewGadget`/non-editable-
/// `ComboBoxGadget` case - oracle-verified `SetGadgetText` selects
/// whichever item's own text *exactly* matches the given string, or does
/// nothing at all if none does (confirmed directly: the selection/active
/// item is left unchanged, not cleared). Returns a real, found iterator
/// via `found`, or `false` if nothing matched.
inline bool findRowByText(GtkTreeModel* model, const std::string& text, GtkTreeIter* found) {
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter) == 0) {
        return false;
    }
    do {
        gchar* rowText = nullptr;
        gtk_tree_model_get(model, &iter, itemTextColumn(), &rowText, -1);
        bool matches = rowText != nullptr && text == rowText;
        g_free(rowText);
        if (matches) {
            *found = iter;
            return true;
        }
    } while (gtk_tree_model_iter_next(model, &iter) != 0);
    return false;
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

/// `FontRequester`'s own last-selected-font state, read back afterward by
/// `SelectedFontName`/`SelectedFontSize`/`SelectedFontStyle`/
/// `SelectedFontColor` - the same "separate accessor functions, not a
/// return-value struct" shape real PB's own API has, so a single, plain
/// struct (not a table keyed by anything) is all that's needed: there's
/// only ever one "most recent" FontRequester result to ask about, exactly
/// like there's only one "most recent" `EventWindow()`/`EventGadget()`.
struct SelectedFontState {
    std::string name;
    std::int64_t size = 0;
    std::int64_t style = 0;
    std::int64_t color = 0;
};
inline SelectedFontState& selectedFont() {
    static SelectedFontState state;
    return state;
}

/// `OpenFileRequester`'s own `#PB_Requester_MultiSelection` state -
/// `NextSelectedFileName()`'s own cursor into whatever the most recent
/// multi-selection call returned (the *first* selected file is
/// `OpenFileRequester`'s own return value directly; the rest queue up
/// here, oracle-verified via `NextSelectedFileName`'s own docs: "renvoie
/// le fichier sélectionné suivant" - the *next* one, after the first).
inline std::vector<std::string>& multiSelectedFiles() {
    static std::vector<std::string> files;
    return files;
}
inline std::size_t& multiSelectedFilesCursor() {
    static std::size_t cursor = 0;
    return cursor;
}
/// `SelectedFilePattern`'s own state - the 0-based index (within the
/// pattern string's own `|`-separated groups) of whichever filter was
/// active when `OpenFileRequester`/`SaveFileRequester` closed, `-1` if
/// the dialog was cancelled (oracle-verified via `SelectedFilePattern`'s
/// own docs example: `If Index > -1`).
inline std::int64_t& selectedFilePatternIndex() {
    static std::int64_t index = -1;
    return index;
}

/// Splits real PB's own file-pattern syntax (`"Label|*.ext;*.ext2|Label2|
/// *.ext3"` - alternating label/glob-list pairs joined by `|`, oracle-
/// verified via `OpenFileRequester`'s own docs) into `(label, globs)`
/// pairs, each glob-list itself `;`-separated. Shared by
/// `OpenFileRequester`/`SaveFileRequester`, the only two functions with a
/// `Pattern$` argument at all.
inline std::vector<std::pair<std::string, std::vector<std::string>>> parseFilterPattern(const std::string& pattern) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t pos = pattern.find('|', start);
        parts.push_back(pattern.substr(start, pos == std::string::npos ? std::string::npos : pos - start));
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    std::vector<std::pair<std::string, std::vector<std::string>>> result;
    for (std::size_t i = 0; i + 1 < parts.size(); i += 2) {
        std::vector<std::string> globs;
        std::size_t globStart = 0;
        const std::string& globStr = parts[i + 1];
        while (true) {
            std::size_t globPos = globStr.find(';', globStart);
            globs.push_back(globStr.substr(globStart, globPos == std::string::npos ? std::string::npos
                                                                                     : globPos - globStart));
            if (globPos == std::string::npos) {
                break;
            }
            globStart = globPos + 1;
        }
        result.emplace_back(parts[i], std::move(globs));
    }
    return result;
}

/// Builds one `GtkFileFilter` per `(label, globs)` pair from
/// `parseFilterPattern`, adds each to `chooser`, and selects whichever one
/// is at `patternPosition` (0-based, oracle-verified via
/// `OpenFileRequester`'s own docs) - shared by `OpenFileRequester`/
/// `SaveFileRequester`. Returns the full ordered filter list, so the
/// caller can look its own active filter's index back up afterward for
/// `SelectedFilePattern()` (`GtkFileChooser` only exposes the filter
/// *object* itself, not its position).
inline std::vector<GtkFileFilter*> applyFileFilters(GtkFileChooser* chooser, const std::string& pattern,
                                                      std::int64_t patternPosition) {
    std::vector<GtkFileFilter*> filters;
    for (auto& [label, globs] : parseFilterPattern(pattern)) {
        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, label.c_str());
        for (auto& glob : globs) {
            if (!glob.empty()) {
                gtk_file_filter_add_pattern(filter, glob.c_str());
            }
        }
        gtk_file_chooser_add_filter(chooser, filter);
        filters.push_back(filter);
    }
    if (patternPosition >= 0 && static_cast<std::size_t>(patternPosition) < filters.size()) {
        gtk_file_chooser_set_filter(chooser, filters[static_cast<std::size_t>(patternPosition)]);
    }
    return filters;
}

/// M7b's fifteenth GUI slice: `ProgressBarGadget` - a `GtkProgressBar`,
/// whose own `gtk_progress_bar_set_fraction` only ever takes a `[0.0,
/// 1.0]` double, with no inherent idea of a PB-style `[Minimum, Maximum]`
/// integer range at all. The real range/current-value state lives in
/// three `g_object_data` tags instead (`Minimum`/`Maximum`/`Value`),
/// `progressBarUpdateFraction` mapping them onto the widget's own
/// fraction whenever any of the three changes. Oracle-verified directly:
/// `SetGadgetState` clamps to `[Minimum, Maximum]`, *except* for the
/// literal sentinel `#PB_ProgressBar_Unknown` (`-1`), which is stored
/// and read back verbatim instead of being clamped into range - mapped
/// onto a flat `0.0` fraction here (GTK has no built-in true
/// "indeterminate" bar the way a real OS widget might; an animated pulse
/// would need its own timer, out of scope for what a static fraction
/// read-back can verify anyway). Changing `Minimum`/`Maximum` via
/// `SetGadgetAttribute` does *not* retroactively re-clamp an existing
/// value into the new range - oracle-verified directly (raising
/// `Maximum` left an existing, already-in-range value unchanged) - not
/// independently re-verified for the narrower case of *shrinking* a
/// range out from under an existing value, since real PB's own docs
/// don't describe that case either.
inline const char* progressBarMinKey() { return "pbcxx-progressbar-min"; }
inline const char* progressBarMaxKey() { return "pbcxx-progressbar-max"; }
inline const char* progressBarValueKey() { return "pbcxx-progressbar-value"; }

inline void progressBarUpdateFraction(GtkWidget* widget) {
    auto value = static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(widget), progressBarValueKey())));
    auto minimum = static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(widget), progressBarMinKey())));
    auto maximum = static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(widget), progressBarMaxKey())));
    if (value == -1 || maximum <= minimum) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget), 0.0);
        return;
    }
    double fraction = static_cast<double>(value - minimum) / static_cast<double>(maximum - minimum);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget), fraction);
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

/// M7b's fifteenth GUI slice: `OptionGadget` - a `GtkRadioButton`, joined
/// to `optionGroupAnchor()`'s own group when non-null (see its own doc
/// comment for the full grouping rule), otherwise starting a fresh one.
/// `GtkButton`/`GtkToggleButton`'s own generic `GetGadgetText`/
/// `SetGadgetText`/`GetGadgetState`/`SetGadgetState` dispatch (already in
/// place since `CheckBoxGadget`'s own second GUI slice) covers a
/// `GtkRadioButton` for free, no new branch needed there at all - this is
/// purely the creation function plus the grouping mechanism.
/// Oracle-verified directly: the *first* button in a new group starts out
/// selected (`#PB_ProgressBar`-style "no selection" isn't a valid radio-
/// group state), set here explicitly (signal-blocked, the same reason
/// every other stateful gadget's own creation function avoids a spurious
/// creation-time event) rather than assumed from whatever GTK's own
/// default happens to be.
inline std::int64_t pbOptionGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                    std::int64_t height, const PBString& text) {
    detail::ensureGtkInit();
    GtkWidget* anchor = detail::optionGroupAnchor();
    GtkWidget* widget = anchor != nullptr
                             ? gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(anchor), text.bytes().c_str())
                             : gtk_radio_button_new_with_label(nullptr, text.bytes().c_str());
    g_signal_connect(widget, "toggled", G_CALLBACK(detail::onGadgetClicked), nullptr);
    bool placed = detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget);
    if (placed && anchor == nullptr) {
        g_signal_handlers_block_by_func(widget, reinterpret_cast<gpointer>(detail::onGadgetClicked), nullptr);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
        g_signal_handlers_unblock_by_func(widget, reinterpret_cast<gpointer>(detail::onGadgetClicked), nullptr);
    }
    return placed ? 1 : 0;
}

/// `#PB_ProgressBar_Vertical` maps directly onto `GtkOrientable`'s own
/// orientation; `#PB_ProgressBar_Smooth` (oracle-verified value `0`) is
/// registered for name-compiling completeness but can't be a meaningful
/// bit flag at all (`flags & 0` is always `0`) - matching real PB's own
/// documented "n'a aucun effet" note for it on several platforms anyway.
/// `GetGadgetColor`/`SetGadgetColor` support is out of scope here, the
/// same broad, many-gadget-type feature already deferred for
/// `ScrollAreaGadget`'s own twelfth slice.
inline std::int64_t pbProgressBarGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                         std::int64_t height, std::int64_t minimum, std::int64_t maximum,
                                         std::int64_t flags = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_progress_bar_new();
    if ((flags & 1) != 0) { // #PB_ProgressBar_Vertical
        gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
    }
    g_object_set_data(G_OBJECT(widget), detail::progressBarMinKey(), reinterpret_cast<gpointer>(minimum));
    g_object_set_data(G_OBJECT(widget), detail::progressBarMaxKey(), reinterpret_cast<gpointer>(maximum));
    g_object_set_data(G_OBJECT(widget), detail::progressBarValueKey(), reinterpret_cast<gpointer>(minimum));
    detail::progressBarUpdateFraction(widget);
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

inline std::int64_t pbFrameGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                   std::int64_t height, const PBString& text, std::int64_t /*flags*/ = 0) {
    detail::ensureGtkInit();
    GtkWidget* widget = gtk_frame_new(text.bytes().c_str());
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, widget) ? 1 : 0;
}

/// M7b's ninth GUI slice: `ContainerGadget` - a plain panel meant to hold
/// other gadgets, the first of PB's "gadget-list nesting" family
/// (`PanelGadget`/`ScrollAreaGadget` share the same `OpenGadgetList`/
/// `CloseGadgetList` mechanism, oracle-verified, but aren't implemented
/// yet). A `GtkFrame` (for `Flags`' own border styling) wrapping an inner
/// `GtkFixed` (the actual placement target gadgets created afterward land
/// in) - `gtk_fixed_put`'s own coordinates are relative to *that* `GtkFixed`
/// for free, exactly matching real PB's own oracle-verified behavior
/// (`GadgetX()` on a gadget nested in a container is relative to the
/// container's own top-left, not the window's - confirmed directly, both
/// one level deep and nested two containers deep). Successfully creating
/// one pushes it onto `gadgetListStack()` immediately - oracle-verified
/// (`ContainerGadget.html`'s own "Remarques"): no separate "open" call is
/// needed, unlike reopening one later via `OpenGadgetList`.
inline std::int64_t pbContainerGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                       std::int64_t height, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    GtkWidget* frame = gtk_frame_new(nullptr);
    // Oracle-verified bit values (`#PB_Container_BorderLess`=0 (default),
    // `Flat`=1, `Raised`=2, `Single`=4, `Double`=8) mapped onto GtkFrame's
    // own shadow-type vocabulary by name, not pixel-matched against a real
    // screenshot - no accessor exists yet to query a gadget's own visual
    // chrome from PB code, so there's nothing to verify that precisely
    // against.
    GtkShadowType shadow = GTK_SHADOW_NONE;
    if (flags & 8) {
        shadow = GTK_SHADOW_ETCHED_IN; // #PB_Container_Double
    } else if (flags & 4) {
        shadow = GTK_SHADOW_IN; // #PB_Container_Single
    } else if (flags & 2) {
        shadow = GTK_SHADOW_OUT; // #PB_Container_Raised
    } else if (flags & 1) {
        shadow = GTK_SHADOW_ETCHED_OUT; // #PB_Container_Flat
    }
    gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow);
    if (!detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, frame)) {
        return 0;
    }
    GtkWidget* inner = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(frame), inner);
    gtk_widget_show(inner);
    detail::containerFixedTable()[gadgetId] = inner;
    detail::gadgetListStack().push_back({gadgetId, inner});
    return 1;
}

/// M7b's tenth GUI slice: `PanelGadget`, a tabbed box - `GtkNotebook` is
/// the natural fit. Unlike `ContainerGadget`, creating one does *not*
/// push anything onto `gadgetListStack()` by itself - oracle-verified
/// directly (`PanelGadget.html`'s own "Remarques"): a freshly created
/// Panel's own item list is empty, and at least one tab must exist
/// (via `AddGadgetItem`, the only thing that actually pushes a frame)
/// before any gadget can be placed into it at all. `Flags` doesn't exist
/// for this gadget at all in real PB's own current docs (unlike
/// `ContainerGadget`'s own) - confirmed by this project's own methodology
/// (read the real docs first), not merely absent from the signature by
/// omission.
inline std::int64_t pbPanelGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                   std::int64_t height) {
    detail::ensureGtkInit();
    GtkWidget* notebook = gtk_notebook_new();
    g_signal_connect(notebook, "switch-page", G_CALLBACK(detail::onPanelSwitchPage), nullptr);
    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, notebook) ? 1 : 0;
}

/// M7b's twelfth GUI slice: `ScrollAreaGadget` - back to the gadget-list
/// nesting family (`ContainerGadget`'s own ninth slice), auto-capturing
/// subsequently created gadgets immediately on creation, like Container
/// (not `PanelGadget`, which needs `AddGadgetItem` first) - oracle-
/// verified directly via `ScrollAreaGadget.html`'s own "Remarques": "Une
/// fois créé, tous les gadgets suivants seront placés dans ce gadget."
/// `GtkScrolledWindow` is the natural match, with a plain `GtkFixed` -
/// sized to `InnerWidth`/`InnerHeight`, exactly like `ContainerGadget`'s
/// own inner one - as its content; GTK already auto-hides a scrollbar
/// when the inner area doesn't exceed the outer one in that axis
/// (`GTK_POLICY_AUTOMATIC`), the same behavior real PB's own docs
/// describe ("si sa taille est plus petite..., les barres de défilement
/// seront masquées"). `#PB_ScrollArea_Center` (centering a smaller inner
/// area) is accepted but not acted on - GTK has no direct equivalent to
/// hook into here, the same kind of narrow, deliberate gap
/// `#PB_Splitter_Separator`'s own "3D pattern" flag already is. Unlike
/// `ContainerGadget`'s own border flags, this gadget's own bit values
/// are *not* assumed to match - confirmed independently via a direct
/// `Debug #PB_ScrollArea_Xxx` probe.
inline std::int64_t pbScrollAreaGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                        std::int64_t height, std::int64_t innerWidth, std::int64_t innerHeight,
                                        std::int64_t scrollStep = 0, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkShadowType shadow = GTK_SHADOW_NONE; // #PB_ScrollArea_BorderLess (8), or no flags at all
    if ((flags & 4) != 0) {
        shadow = GTK_SHADOW_IN; // #PB_ScrollArea_Single
    } else if ((flags & 2) != 0) {
        shadow = GTK_SHADOW_OUT; // #PB_ScrollArea_Raised
    } else if ((flags & 1) != 0) {
        shadow = GTK_SHADOW_ETCHED_OUT; // #PB_ScrollArea_Flat
    }
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), shadow);

    GtkWidget* inner = gtk_fixed_new();
    gtk_widget_set_size_request(inner, static_cast<int>(innerWidth), static_cast<int>(innerHeight));
    gtk_container_add(GTK_CONTAINER(scrolled), inner);
    gtk_widget_show(inner);

    GtkAdjustment* hAdjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled));
    GtkAdjustment* vAdjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
    if (scrollStep > 0) {
        gtk_adjustment_set_step_increment(hAdjust, static_cast<double>(scrollStep));
        gtk_adjustment_set_step_increment(vAdjust, static_cast<double>(scrollStep));
    }

    if (!detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, scrolled)) {
        return 0;
    }
    // Connected *after* `placeGadget` so the gadget/window IDs `queueGadgetEvent`
    // (via `onScrollAreaValueChanged`'s own `userData`) reads back are
    // already tagged on `scrolled` itself.
    g_signal_connect(hAdjust, "value-changed", G_CALLBACK(detail::onScrollAreaValueChanged), scrolled);
    g_signal_connect(vAdjust, "value-changed", G_CALLBACK(detail::onScrollAreaValueChanged), scrolled);
    detail::containerFixedTable()[gadgetId] = inner;
    detail::gadgetListStack().push_back({gadgetId, inner});
    return 1;
}

/// M7b's thirteenth GUI slice: `ListViewGadget`/`ComboBoxGadget` - the
/// first "universal item" gadget types besides `PanelGadget` itself,
/// sharing a plain two-column `GtkListStore` (`TEXT`/`DATA` -
/// `itemTextColumn`/`itemDataColumn`) with it via `itemListStoreFor`,
/// rather than duplicating `AddGadgetItem`'s own family per widget type.
/// `#PB_ComboBox_Image`/`LowerCase`/`UpperCase` are deliberately out of
/// scope for this slice (accepted but not acted on) - `Image` needs a
/// genuinely different `GtkCellRendererPixbuf` setup alongside the text
/// one, and `LowerCase`/`UpperCase` need their own live text-transforming
/// `"changed"` handler on the entry - both real, separate pieces of work
/// deferred the same deliberate way `#PB_Splitter_Separator`'s own "3D
/// pattern" already is, not implemented partially/incorrectly instead.
///
/// `ListViewGadget` wraps a headerless, single-column `GtkTreeView` in a
/// `GtkScrolledWindow` (needed for real scrolling through many items,
/// the gadget's whole reason to exist) - oracle-verified `#PB_ListView_
/// Multiselect`/`ClickSelect` both map onto `GTK_SELECTION_MULTIPLE`
/// (GTK doesn't distinguish "consecutive range" from "individual toggle"
/// selection as two separate modes the way PB's own docs describe them,
/// but `GTK_SELECTION_MULTIPLE` already supports *both* interactions at
/// once, so nothing meaningful is lost); no flags at all is
/// `GTK_SELECTION_SINGLE`, confirmed to already enforce the same
/// deselect-the-previous-one exclusivity oracle-verified directly for
/// `SetGadgetItemState` below, with no extra code needed for that.
inline std::int64_t pbListViewGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                      std::int64_t height, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    GtkListStore* store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT64);
    GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* column =
        gtk_tree_view_column_new_with_attributes("", renderer, "text", detail::itemTextColumn(), nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
    gtk_tree_selection_set_mode(selection, (flags & 3) != 0 ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), treeView);
    gtk_widget_show(treeView);

    if (!detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, scrolled)) {
        return 0;
    }
    g_signal_connect(selection, "changed", G_CALLBACK(detail::onListViewSelectionChanged), scrolled);
    g_signal_connect(treeView, "row-activated", G_CALLBACK(detail::onListViewRowActivated), scrolled);
    g_signal_connect(treeView, "button-press-event", G_CALLBACK(detail::onListViewButtonPress), scrolled);
    return 1;
}

/// `ComboBoxGadget` - a plain `GtkComboBox` (not the simpler
/// `GtkComboBoxText` convenience widget, despite not needing images or
/// per-row data *yet* - building on the full model/cell-renderer API from
/// the start means `#PB_ComboBox_Image` and `GetGadgetItemData`/
/// `SetGadgetItemData` both have somewhere real to go later with no
/// rework, not just a hypothetical future need) sharing the identical
/// `TEXT`/`DATA` model `ListViewGadget` already uses. `#PB_ComboBox_
/// Editable`'s own `"has-entry"` is a real `GObject` construct property;
/// `gtk_combo_box_set_entry_text_column` is what makes the entry's own
/// text track the active row's own `TEXT` column, oracle-verified
/// `SetGadgetText` on an editable combo accepting arbitrary text that
/// matches no item at all (so this is only wired up when editable - a
/// non-editable combo has no entry to sync at all).
inline std::int64_t pbComboBoxGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                      std::int64_t height, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    GtkListStore* store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT64);
    bool editable = (flags & 1) != 0;
    GtkWidget* combo = GTK_WIDGET(g_object_new(GTK_TYPE_COMBO_BOX, "model", store, "has-entry",
                                                editable ? TRUE : FALSE, nullptr));
    g_object_unref(store);
    if (editable) {
        gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combo), detail::itemTextColumn());
    } else {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", detail::itemTextColumn(), nullptr);
    }

    if (!detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, combo)) {
        return 0;
    }
    g_signal_connect(combo, "changed", G_CALLBACK(detail::onComboBoxChanged), nullptr);
    if (editable) {
        GtkWidget* entry = gtk_bin_get_child(GTK_BIN(combo));
        if (entry != nullptr) {
            g_signal_connect(entry, "focus-in-event", G_CALLBACK(detail::onComboBoxFocusIn), combo);
            g_signal_connect(entry, "focus-out-event", G_CALLBACK(detail::onComboBoxFocusOut), combo);
        }
    }
    return 1;
}

/// M7b's fourteenth GUI slice: `ListIconGadget` - a multi-column
/// `GtkTreeView` (headers visible by default; `#PB_ListIcon_NoHeaders`
/// hides them, `#PB_ListIcon_GridLines` maps directly onto
/// `gtk_tree_view_set_grid_lines`), created with one initial column -
/// `#PB_ListIcon_CheckBoxes` packs a `GtkCellRendererToggle` alongside
/// that column's own text renderer (oracle-verified: "Affiche une case à
/// cocher dans la première colonne"), bound to `listIconCheckedColumn()`'s
/// own always-present model slot regardless of whether the flag was
/// actually requested (see `listIconMaxColumns()`'s own doc comment for
/// why the whole model shape is fixed and uniform this way).
/// `#PB_ListIcon_MultiSelect` maps onto `GTK_SELECTION_MULTIPLE`, the
/// same as `ListViewGadget`'s own `Multiselect`/`ClickSelect` flags
/// already do. `#PB_ListIcon_FullRowSelect`/`AlwaysShowSelection`/
/// `HeaderDragDrop`/`ThreeState` are accepted but not acted on - the
/// first three are documented Windows-only in real PB itself, and
/// `ThreeState`'s own "indeterminate" checkbox state has no
/// `GtkCellRendererToggle` equivalent to map onto without a materially
/// bigger custom-renderer investment, the same kind of deliberate,
/// narrow gap `#PB_ComboBox_Image`'s own deferred flag already is.
inline std::int64_t pbListIconGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                      std::int64_t height, const PBString& firstColumnTitle,
                                      std::int64_t firstColumnWidth, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    std::vector<GType> types(static_cast<std::size_t>(detail::listIconTotalModelColumns()));
    for (int i = 0; i < detail::listIconMaxColumns(); ++i) {
        types[static_cast<std::size_t>(i)] = G_TYPE_STRING;
    }
    types[static_cast<std::size_t>(detail::listIconDataColumn())] = G_TYPE_INT64;
    types[static_cast<std::size_t>(detail::listIconCheckedColumn())] = G_TYPE_BOOLEAN;
    GtkListStore* store = gtk_list_store_newv(static_cast<gint>(types.size()), types.data());
    GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    g_object_set_data(G_OBJECT(treeView), detail::gadgetKindKey(),
                       const_cast<char*>(detail::gadgetKindListIcon()));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), (flags & 128) == 0 ? TRUE : FALSE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeView), (flags & 4) != 0 ? GTK_TREE_VIEW_GRID_LINES_BOTH
                                                                            : GTK_TREE_VIEW_GRID_LINES_NONE);

    GtkTreeViewColumn* firstColumn = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(firstColumn, firstColumnTitle.bytes().c_str());
    gtk_tree_view_column_set_sizing(firstColumn, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(firstColumn, static_cast<int>(firstColumnWidth));
    GtkCellRenderer* toggleRenderer = nullptr;
    if ((flags & 1) != 0) { // #PB_ListIcon_CheckBoxes
        toggleRenderer = gtk_cell_renderer_toggle_new();
        gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(toggleRenderer), TRUE);
        gtk_tree_view_column_pack_start(firstColumn, toggleRenderer, FALSE);
        gtk_tree_view_column_add_attribute(firstColumn, toggleRenderer, "active", detail::listIconCheckedColumn());
    }
    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(firstColumn, textRenderer, TRUE);
    gtk_tree_view_column_add_attribute(firstColumn, textRenderer, "text", 0);
    gtk_tree_view_column_set_clickable(firstColumn, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), firstColumn);
    g_object_set_data(G_OBJECT(firstColumn), detail::listIconColumnSlotKey(),
                       reinterpret_cast<gpointer>(std::intptr_t{0}));
    g_object_set_data(G_OBJECT(treeView), detail::listIconNextSlotKey(), reinterpret_cast<gpointer>(std::intptr_t{1}));

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
    gtk_tree_selection_set_mode(selection, (flags & 2) != 0 ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), treeView);
    gtk_widget_show(treeView);

    if (!detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, scrolled)) {
        return 0;
    }
    g_signal_connect(selection, "changed", G_CALLBACK(detail::onListIconSelectionChanged), scrolled);
    g_signal_connect(treeView, "row-activated", G_CALLBACK(detail::onListIconRowActivated), scrolled);
    g_signal_connect(treeView, "button-press-event", G_CALLBACK(detail::onListIconButtonPress), scrolled);
    g_signal_connect(firstColumn, "clicked", G_CALLBACK(detail::onListIconColumnClicked), scrolled);
    if (toggleRenderer != nullptr) {
        g_signal_connect(toggleRenderer, "toggled", G_CALLBACK(detail::onListIconToggle), scrolled);
    }
    return 1;
}

/// `AddGadgetColumn(#Gadget, Position, Title$, Width)` - `Position`'s own
/// `-1`-means-"append" convention matches `gtk_tree_view_insert_column`'s
/// own directly. Returns `0` (a harmless failure, this project's own
/// established simplification for the function's real "no documented
/// return value" contract) once `listIconMaxColumns()`'s own pre-allocated
/// slots are exhausted - unreachable in practice for any real PB program.
inline std::int64_t pbAddGadgetColumn(std::int64_t gadgetId, std::int64_t position, const PBString& title,
                                       std::int64_t width) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    GtkWidget* treeView = detail::listIconTreeView(it->second);
    if (treeView == nullptr) {
        return 0;
    }
    auto nextSlot =
        reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(treeView), detail::listIconNextSlotKey()));
    if (nextSlot >= detail::listIconMaxColumns()) {
        return 0;
    }
    GtkTreeViewColumn* column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, title.bytes().c_str());
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, static_cast<int>(width));
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", static_cast<int>(nextSlot));
    gtk_tree_view_column_set_clickable(column, TRUE);
    int insertAt = position < 0 ? -1 : static_cast<int>(position);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(treeView), column, insertAt);
    g_object_set_data(G_OBJECT(column), detail::listIconColumnSlotKey(), reinterpret_cast<gpointer>(nextSlot));
    g_object_set_data(G_OBJECT(treeView), detail::listIconNextSlotKey(), reinterpret_cast<gpointer>(nextSlot + 1));
    g_signal_connect(column, "clicked", G_CALLBACK(detail::onListIconColumnClicked), it->second);
    return 1;
}

/// `RemoveGadgetColumn(#Gadget, Column)` - `#PB_All` (`-1`) removes every
/// column. The underlying store slot a removed column was bound to is
/// never reclaimed (see `listIconMaxColumns()`'s own doc comment) - its
/// own old data just becomes permanently unreachable, which is
/// functionally equivalent to "removed" from the PB program's own point
/// of view, matching the documented contract ("supprime une colonne
/// (ainsi que ses données)") without needing to actually clear it.
inline std::int64_t pbRemoveGadgetColumn(std::int64_t gadgetId, std::int64_t column) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    GtkWidget* treeView = detail::listIconTreeView(it->second);
    if (treeView == nullptr) {
        return 0;
    }
    if (column < 0) { // #PB_All
        GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(treeView));
        for (GList* l = columns; l != nullptr; l = l->next) {
            gtk_tree_view_remove_column(GTK_TREE_VIEW(treeView), GTK_TREE_VIEW_COLUMN(l->data));
        }
        g_list_free(columns);
        return 1;
    }
    GtkTreeViewColumn* col = detail::listIconColumnAtPosition(GTK_TREE_VIEW(treeView), static_cast<int>(column));
    if (col == nullptr) {
        return 0;
    }
    gtk_tree_view_remove_column(GTK_TREE_VIEW(treeView), col);
    return 1;
}

/// M7b's fourteenth GUI slice: `TreeGadget` - a headerless, single-column
/// `GtkTreeView` over a hierarchical `GtkTreeStore` (`TEXT`/`DATA`/
/// `CHECKED` - `treeTextColumn`/`treeDataColumn`/`treeCheckedColumn`),
/// wrapped in a `GtkScrolledWindow` the same way every other `GtkTreeView`-
/// based gadget already is. `#PB_Tree_NoLines`/`NoButtons` map directly
/// onto `gtk_tree_view_set_enable_tree_lines`/`gtk_tree_view_set_show_
/// expanders`; `#PB_Tree_CheckBoxes` packs a `GtkCellRendererToggle` the
/// same way `ListIconGadget`'s own does. `#PB_Tree_AlwaysShowSelection`/
/// `ThreeState` are accepted but not acted on, the same deliberate,
/// narrow gaps `ListIconGadget`'s own equivalent flags already are.
inline std::int64_t pbTreeGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                  std::int64_t height, std::int64_t flags = 0) {
    detail::ensureGtkInit();
    GtkTreeStore* store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_INT64, G_TYPE_BOOLEAN);
    GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    g_object_set_data(G_OBJECT(treeView), detail::gadgetKindKey(), const_cast<char*>(detail::gadgetKindTree()));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(treeView), (flags & 1) == 0 ? TRUE : FALSE); // #PB_Tree_NoLines
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(treeView), (flags & 2) == 0 ? TRUE : FALSE); // #PB_Tree_NoButtons

    GtkTreeViewColumn* column = gtk_tree_view_column_new();
    GtkCellRenderer* toggleRenderer = nullptr;
    if ((flags & 4) != 0) { // #PB_Tree_CheckBoxes
        toggleRenderer = gtk_cell_renderer_toggle_new();
        gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(toggleRenderer), TRUE);
        gtk_tree_view_column_pack_start(column, toggleRenderer, FALSE);
        gtk_tree_view_column_add_attribute(column, toggleRenderer, "active", detail::treeCheckedColumn());
    }
    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, textRenderer, TRUE);
    gtk_tree_view_column_add_attribute(column, textRenderer, "text", detail::treeTextColumn());
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), treeView);
    gtk_widget_show(treeView);

    if (!detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, scrolled)) {
        return 0;
    }
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
    g_signal_connect(selection, "changed", G_CALLBACK(detail::onListIconSelectionChanged), scrolled);
    g_signal_connect(treeView, "row-activated", G_CALLBACK(detail::onListIconRowActivated), scrolled);
    g_signal_connect(treeView, "button-press-event", G_CALLBACK(detail::onListIconButtonPress), scrolled);
    if (toggleRenderer != nullptr) {
        g_signal_connect(toggleRenderer, "toggled", G_CALLBACK(detail::onTreeToggle), scrolled);
    }
    return 1;
}

/// `AddGadgetItem(#Gadget, Position, Text$ [, ImageID [, Options]])` -
/// scoped to `PanelGadget` only for now (the one gadget type this project
/// actually has that supports it; real PB's own docs also list
/// `ComboBoxGadget`/`EditorGadget`/`ListViewGadget`/`ListIconGadget`/
/// `MDIGadget`/`TreeGadget`, none implemented yet - dispatching on the
/// gadget's own real GTK widget type, same as `pbGetGadgetText`, so
/// adding any of those later is a new branch here, not a rewrite).
/// `Options` is `TreeGadget`/`MDIGadget`-specific (sub-level / window
/// flags respectively) - accepted but ignored for Panel, oracle-verified
/// via `AddGadgetItem.html`'s own remarks. `Position`'s `-1` ("append")/
/// non-negative ("insert at this index") split already matches
/// `gtk_notebook_insert_page`'s own convention exactly, needing no
/// translation.
///
/// **The "replace, not push" nesting rule** - oracle-verified directly,
/// matching the exact real `PanelGadget.html` example sequence (two
/// `AddGadgetItem` calls on the *same* Panel, no `CloseGadgetList`
/// between them, then exactly *one* `CloseGadgetList()` at the end,
/// confirmed to return all the way back to window level - a second,
/// immediately following `CloseGadgetList()` then correctly hits the
/// real "nothing open" fatal error): a second `AddGadgetItem` call
/// targeting the Panel that's *already* the current top of
/// `gadgetListStack()` (because an earlier `AddGadgetItem` on it put it
/// there) retargets that same frame to the new tab instead of pushing an
/// additional one - the same design the Qt6 sibling project's own
/// `docs/42-gadget-container-nesting.md` reached independently for the
/// identical reason, confirmed here too rather than assumed from that
/// precedent alone. A *different* Panel (including one nested inside the
/// first, as the real `PanelGadget.html` example's own "Sous-onglet"
/// case exercises) still pushes a genuinely new frame, since its own
/// `containerId` won't match whatever's currently on top.
inline std::int64_t pbAddGadgetItem(std::int64_t gadgetId, std::int64_t position, const PBString& text,
                                     std::int64_t imageId = 0, std::int64_t options = 0) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    // M7b's thirteenth GUI slice: ListViewGadget/ComboBoxGadget - `ImageID`
    // isn't supported for either yet (`#PB_ComboBox_Image`'s own deferred
    // gap - see pbComboBoxGadget's own doc comment), only for Panel.
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        GtkTreeIter iter;
        int insertAt = position < 0 ? -1 : static_cast<int>(position);
        gtk_list_store_insert(store, &iter, insertAt);
        gtk_list_store_set(store, &iter, detail::itemTextColumn(), text.bytes().c_str(), -1);
        return 1;
    }
    // M7b's fourteenth GUI slice: ListIconGadget - `Chr(10)`-separated
    // multi-column text, written to each visual column's own underlying
    // store slot (`ImageID` is deferred here too, the same reason).
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        auto* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
        GtkTreeIter iter;
        int insertAt = position < 0 ? -1 : static_cast<int>(position);
        gtk_list_store_insert(store, &iter, insertAt);
        auto parts = detail::splitByNewline(text.bytes());
        for (std::size_t visualPos = 0; visualPos < parts.size(); ++visualPos) {
            int slot = detail::listIconStoreSlotAtPosition(GTK_TREE_VIEW(treeView), static_cast<int>(visualPos));
            if (slot < 0) {
                break;
            }
            gtk_list_store_set(store, &iter, slot, parts[visualPos].c_str(), -1);
        }
        return 1;
    }
    // M7b's fourteenth GUI slice: TreeGadget - `Options` is the new
    // item's own level, required (not optional in practice, though still
    // accepted with a default like every other builtin here) - see
    // `treeLevelStack`'s own doc comment for the parent-finding/level-
    // clamping algorithm, oracle-verified directly.
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
        auto& stack = detail::treeLevelStack(treeView);
        int requestedLevel = static_cast<int>(options);
        int effectiveLevel = std::min(requestedLevel < 0 ? 0 : requestedLevel, static_cast<int>(stack.size()));
        GtkTreeIter* parent = effectiveLevel == 0 ? nullptr : &stack[static_cast<std::size_t>(effectiveLevel - 1)];
        GtkTreeIter iter;
        int insertAt = position < 0 ? -1 : static_cast<int>(position);
        gtk_tree_store_insert(store, &iter, parent, insertAt);
        gtk_tree_store_set(store, &iter, detail::treeTextColumn(), text.bytes().c_str(), -1);
        stack.resize(static_cast<std::size_t>(effectiveLevel) + 1);
        stack[static_cast<std::size_t>(effectiveLevel)] = iter;
        return 1;
    }
    if (GTK_IS_NOTEBOOK(it->second) == 0) {
        return 0;
    }
    auto* notebook = GTK_NOTEBOOK(it->second);
    GtkWidget* page = gtk_fixed_new();
    GtkWidget* label = detail::buildTabLabel(text.bytes(), imageId);
    int insertAt = position < 0 ? -1 : static_cast<int>(position);
    gtk_notebook_insert_page(notebook, page, label, insertAt);
    gtk_widget_show(page);
    gtk_widget_show(label);

    if (!detail::gadgetListStack().empty() && detail::gadgetListStack().back().containerId == gadgetId) {
        detail::gadgetListStack().back().fixed = page;
    } else {
        detail::gadgetListStack().push_back({gadgetId, page});
    }
    return 1;
}

inline std::int64_t pbCountGadgetItems(std::int64_t gadgetId) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        return gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), nullptr);
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        return gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)), nullptr);
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        return detail::treeCountAll(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)), nullptr);
    }
    if (GTK_IS_NOTEBOOK(it->second) == 0) {
        return 0;
    }
    return gtk_notebook_get_n_pages(GTK_NOTEBOOK(it->second));
}

/// Oracle-verified (both this and `ClearGadgetItems` below): removing a
/// tab also frees every gadget that was nested inside it, at any depth -
/// via `pruneGadgetsUnderWidget`, *before* `gtk_notebook_remove_page`
/// destroys the page's own widget tree GTK-side, the same ordering
/// `pbFreeGadget`'s own `pruneContainerGadgets` call already uses and for
/// the same reason.
inline std::int64_t pbRemoveGadgetItem(std::int64_t gadgetId, std::int64_t position) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, nullptr, static_cast<int>(position)) == 0) {
            return 0;
        }
        gtk_list_store_remove(store, &iter);
        return 1;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        auto* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView)));
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, nullptr, static_cast<int>(position)) == 0) {
            return 0;
        }
        gtk_list_store_remove(store, &iter);
        return 1;
    }
    // `gtk_tree_store_remove` already recursively removes every
    // descendant row GTK-side, matching the documented "et ses sous-
    // éléments" contract with no extra code needed. `treeLevelStack` is
    // cleared afterward - removing a node could have invalidated whatever
    // it was tracking as "the last item at level N", and real PB's own
    // precise behavior for a subsequent AddGadgetItem *after* a removal
    // isn't independently oracle-verified; clamping back to level 0 is a
    // safe, correctness-preserving simplification (never a dangling
    // GtkTreeIter) rather than a confirmed-faithful one.
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(position), &iter)) {
            return 0;
        }
        gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
        detail::treeLevelStack(treeView).clear();
        return 1;
    }
    if (GTK_IS_NOTEBOOK(it->second) == 0) {
        return 0;
    }
    auto* notebook = GTK_NOTEBOOK(it->second);
    GtkWidget* page = gtk_notebook_get_nth_page(notebook, static_cast<int>(position));
    if (page == nullptr) {
        return 0;
    }
    detail::pruneGadgetsUnderWidget(page);
    gtk_notebook_remove_page(notebook, static_cast<int>(position));
    return 1;
}

inline std::int64_t pbClearGadgetItems(std::int64_t gadgetId) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        gtk_list_store_clear(store);
        return 1;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView))));
        return 1;
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        gtk_tree_store_clear(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeView))));
        detail::treeLevelStack(treeView).clear();
        return 1;
    }
    if (GTK_IS_NOTEBOOK(it->second) == 0) {
        return 0;
    }
    auto* notebook = GTK_NOTEBOOK(it->second);
    for (int n = gtk_notebook_get_n_pages(notebook) - 1; n >= 0; --n) {
        GtkWidget* page = gtk_notebook_get_nth_page(notebook, n);
        if (page != nullptr) {
            detail::pruneGadgetsUnderWidget(page);
        }
        gtk_notebook_remove_page(notebook, n);
    }
    return 1;
}

/// `Column` is `ListIconGadget`/`ExplorerListGadget`-specific - accepted
/// but ignored for `PanelGadget`, oracle-verified via each function's own
/// remarks ("'Colonne' est ignorée" for Panel specifically).
inline PBString pbGetGadgetItemText(std::int64_t gadgetId, std::int64_t element, std::int64_t column = 0) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return PBString();
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, nullptr, static_cast<int>(element)) == 0) {
            return PBString();
        }
        gchar* text = nullptr;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, detail::itemTextColumn(), &text, -1);
        PBString result(text != nullptr ? text : "");
        g_free(text);
        return result;
    }
    // `Element = -1` addresses the column header's own title instead of
    // any row's cell - oracle-verified directly via `ListIconGadget.html`.
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        GtkTreeView* view = GTK_TREE_VIEW(treeView);
        if (element == -1) {
            GtkTreeViewColumn* col = detail::listIconColumnAtPosition(view, static_cast<int>(column));
            if (col == nullptr) {
                return PBString();
            }
            const char* title = gtk_tree_view_column_get_title(col);
            return PBString(title != nullptr ? title : "");
        }
        int slot = detail::listIconStoreSlotAtPosition(view, static_cast<int>(column));
        if (slot < 0) {
            return PBString();
        }
        GtkTreeIter iter;
        auto* model = gtk_tree_view_get_model(view);
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, static_cast<int>(element)) == 0) {
            return PBString();
        }
        gchar* text = nullptr;
        gtk_tree_model_get(model, &iter, slot, &text, -1);
        PBString result(text != nullptr ? text : "");
        g_free(text);
        return result;
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
            return PBString();
        }
        gchar* text = nullptr;
        gtk_tree_model_get(model, &iter, detail::treeTextColumn(), &text, -1);
        PBString result(text != nullptr ? text : "");
        g_free(text);
        return result;
    }
    if (GTK_IS_NOTEBOOK(it->second) == 0) {
        return PBString();
    }
    auto* notebook = GTK_NOTEBOOK(it->second);
    GtkWidget* page = gtk_notebook_get_nth_page(notebook, static_cast<int>(element));
    if (page == nullptr) {
        return PBString();
    }
    GtkWidget* label = detail::findLabelInTabWidget(gtk_notebook_get_tab_label(notebook, page));
    if (label == nullptr) {
        return PBString();
    }
    const char* text = gtk_label_get_text(GTK_LABEL(label));
    return PBString(text != nullptr ? text : "");
}

inline std::int64_t pbSetGadgetItemText(std::int64_t gadgetId, std::int64_t element, const PBString& text,
                                         std::int64_t column = 0) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        gtk_list_store_set(store, &iter, detail::itemTextColumn(), text.bytes().c_str(), -1);
        return 1;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        GtkTreeView* view = GTK_TREE_VIEW(treeView);
        if (element == -1) {
            GtkTreeViewColumn* col = detail::listIconColumnAtPosition(view, static_cast<int>(column));
            if (col == nullptr) {
                return 0;
            }
            gtk_tree_view_column_set_title(col, text.bytes().c_str());
            return 1;
        }
        int slot = detail::listIconStoreSlotAtPosition(view, static_cast<int>(column));
        if (slot < 0) {
            return 0;
        }
        GtkTreeIter iter;
        auto* model = gtk_tree_view_get_model(view);
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, slot, text.bytes().c_str(), -1);
        return 1;
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
            return 0;
        }
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, detail::treeTextColumn(), text.bytes().c_str(), -1);
        return 1;
    }
    if (GTK_IS_NOTEBOOK(it->second) == 0) {
        return 0;
    }
    auto* notebook = GTK_NOTEBOOK(it->second);
    GtkWidget* page = gtk_notebook_get_nth_page(notebook, static_cast<int>(element));
    if (page == nullptr) {
        return 0;
    }
    GtkWidget* label = detail::findLabelInTabWidget(gtk_notebook_get_tab_label(notebook, page));
    if (label == nullptr) {
        return 0;
    }
    gtk_label_set_text(GTK_LABEL(label), text.bytes().c_str());
    return 1;
}

/// `GetGadgetItemState(#Gadget, Element)` - scoped to `ListViewGadget`
/// only (real PB's own docs don't list `SetGadgetAttribute`'s sibling for
/// `ComboBoxGadget` at all), since selection is a `GtkTreeSelection`
/// concept tied to the *view*, not the model `itemListStoreFor` shares
/// with `ComboBoxGadget` - hence `listViewTreeView` here, not that one.
inline std::int64_t pbGetGadgetItemState(std::int64_t gadgetId, std::int64_t element) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    // `ListIconGadget`'s own bitmask - oracle-verified values
    // `#PB_ListIcon_Selected`=1, `Checked`=2, `Inbetween`=4 (the latter
    // unreachable here - `#PB_ListIcon_ThreeState` is a deliberately
    // deferred gap, see `pbListIconGadget`'s own doc comment).
    if (GtkWidget* iconView = detail::listIconTreeView(it->second)) {
        GtkTreeView* view = GTK_TREE_VIEW(iconView);
        auto* model = gtk_tree_view_get_model(view);
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
        gboolean selected = gtk_tree_selection_path_is_selected(gtk_tree_view_get_selection(view), path);
        gtk_tree_path_free(path);
        gboolean checked = FALSE;
        gtk_tree_model_get(model, &iter, detail::listIconCheckedColumn(), &checked, -1);
        std::int64_t result = 0;
        if (selected != 0) {
            result |= 1;
        }
        if (checked != 0) {
            result |= 2;
        }
        return result;
    }
    // `TreeGadget`'s own bitmask - oracle-verified values
    // `#PB_Tree_Selected`=1, `Expanded`=2, `Checked`=4, `Collapsed`=8,
    // `Inbetween`=16 (the last unreachable, the same deliberate-gap
    // reason `ListIconGadget`'s own `Inbetween` is). `Expanded`/
    // `Collapsed` are mutually exclusive, set from
    // `gtk_tree_view_row_expanded`'s own boolean - a leaf (no children)
    // reports `Collapsed`, not independently oracle-verified for that
    // specific case but the only sensible reading of a strictly binary
    // expanded/collapsed state.
    if (GtkWidget* treeGadgetView = detail::treeGadgetTreeView(it->second)) {
        GtkTreeView* view = GTK_TREE_VIEW(treeGadgetView);
        auto* model = gtk_tree_view_get_model(view);
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
            return 0;
        }
        GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
        gboolean selected = gtk_tree_selection_path_is_selected(gtk_tree_view_get_selection(view), path);
        gboolean expanded = gtk_tree_view_row_expanded(view, path);
        gtk_tree_path_free(path);
        gboolean checked = FALSE;
        gtk_tree_model_get(model, &iter, detail::treeCheckedColumn(), &checked, -1);
        std::int64_t result = 0;
        if (selected != 0) {
            result |= 1;
        }
        result |= expanded != 0 ? 2 : 8;
        if (checked != 0) {
            result |= 4;
        }
        return result;
    }
    GtkWidget* treeView = detail::listViewTreeView(it->second);
    if (treeView == nullptr) {
        return 0;
    }
    GtkTreePath* path = gtk_tree_path_new_from_indices(static_cast<int>(element), -1);
    gboolean selected = gtk_tree_selection_path_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView)), path);
    gtk_tree_path_free(path);
    return selected != 0 ? 1 : 0;
}

/// `SetGadgetItemState(#Gadget, Element, State)` - oracle-verified
/// directly: selecting a *different* row while in single-select mode
/// already deselects the previous one automatically, `GtkTreeSelection`'s
/// own built-in exclusivity needing no extra code to enforce it here.
/// Blocked around `GtkTreeSelection::changed` the same established reason
/// every other gadget's own state-setter already is.
inline std::int64_t pbSetGadgetItemState(std::int64_t gadgetId, std::int64_t element, std::int64_t state) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkWidget* iconView = detail::listIconTreeView(it->second)) {
        GtkTreeView* view = GTK_TREE_VIEW(iconView);
        auto* model = gtk_tree_view_get_model(view);
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        GtkTreeSelection* selection = gtk_tree_view_get_selection(view);
        GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
        g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                         it->second);
        if ((state & 1) != 0) {
            gtk_tree_selection_select_path(selection, path);
        } else {
            gtk_tree_selection_unselect_path(selection, path);
        }
        g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                           it->second);
        gtk_tree_path_free(path);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, detail::listIconCheckedColumn(),
                            (state & 2) != 0 ? TRUE : FALSE, -1);
        return 1;
    }
    if (GtkWidget* treeGadgetView = detail::treeGadgetTreeView(it->second)) {
        GtkTreeView* view = GTK_TREE_VIEW(treeGadgetView);
        auto* model = gtk_tree_view_get_model(view);
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
            return 0;
        }
        GtkTreeSelection* selection = gtk_tree_view_get_selection(view);
        GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
        g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                         it->second);
        if ((state & 1) != 0) {
            // See `pbSetGadgetState`'s own matching doc comment -
            // `GtkTreeSelection` won't select a row under a collapsed
            // ancestor on its own.
            gtk_tree_view_expand_to_path(view, path);
            gtk_tree_selection_select_path(selection, path);
        } else {
            gtk_tree_selection_unselect_path(selection, path);
        }
        g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                           it->second);
        if ((state & 2) != 0) {
            gtk_tree_view_expand_row(view, path, FALSE);
        } else if ((state & 8) != 0) {
            gtk_tree_view_collapse_row(view, path);
        }
        gtk_tree_path_free(path);
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, detail::treeCheckedColumn(),
                            (state & 4) != 0 ? TRUE : FALSE, -1);
        return 1;
    }
    GtkWidget* treeView = detail::listViewTreeView(it->second);
    if (treeView == nullptr) {
        return 0;
    }
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
    GtkTreePath* path = gtk_tree_path_new_from_indices(static_cast<int>(element), -1);
    g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListViewSelectionChanged),
                                     it->second);
    if (state != 0) {
        gtk_tree_selection_select_path(selection, path);
    } else {
        gtk_tree_selection_unselect_path(selection, path);
    }
    g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListViewSelectionChanged),
                                       it->second);
    gtk_tree_path_free(path);
    return 1;
}

/// `GetGadgetItemData`/`SetGadgetItemData(#Gadget, Element [, Value])` -
/// an arbitrary per-row integer tag, not displayed - the `DATA` column
/// `itemListStoreFor`'s own model already has alongside `TEXT`, shared by
/// `ListViewGadget`/`ComboBoxGadget` alike (real PB's own docs list
/// `PanelGadget` too, but it isn't model-backed the same way - a
/// `GtkNotebook`'s own tabs have no equivalent "extra column" to reuse -
/// so this is scoped to the two model-backed types for now).
inline std::int64_t pbGetGadgetItemData(std::int64_t gadgetId, std::int64_t element) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        gint64 data = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, detail::itemDataColumn(), &data, -1);
        return data;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        gint64 data = 0;
        gtk_tree_model_get(model, &iter, detail::listIconDataColumn(), &data, -1);
        return data;
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
            return 0;
        }
        gint64 data = 0;
        gtk_tree_model_get(model, &iter, detail::treeDataColumn(), &data, -1);
        return data;
    }
    return 0;
}

inline std::int64_t pbSetGadgetItemData(std::int64_t gadgetId, std::int64_t element, std::int64_t value) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkListStore* store = detail::itemListStoreFor(it->second)) {
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        gtk_list_store_set(store, &iter, detail::itemDataColumn(), static_cast<gint64>(value), -1);
        return 1;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, static_cast<int>(element)) == 0) {
            return 0;
        }
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, detail::listIconDataColumn(), static_cast<gint64>(value),
                            -1);
        return 1;
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
            return 0;
        }
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, detail::treeDataColumn(), static_cast<gint64>(value), -1);
        return 1;
    }
    return 0;
}

/// `GetGadgetItemAttribute`/`SetGadgetItemAttribute(#Gadget, Element,
/// Attribute [, Value], [Column])` - a brand new, generic function pair
/// (the same dispatch shape `GetGadgetAttribute`'s own already has, at
/// item level instead of gadget level). Real PB's own docs, oracle-
/// verified, scope this to two gadget/attribute pairs: `TreeGadget`'s
/// own `#PB_Tree_SubLevel`=1 (`Element`-driven, `Column` ignored) and
/// `ListIconGadget`'s own `#PB_ListIcon_ColumnWidth`=1 (`Column`-driven,
/// `Element`/`Value`'s own... wait, `Element` is ignored, not `Value` -
/// "Le paramètre 'Element' est ignoré" for both Get and Set).
/// `SubLevel` - a given element's own depth, read directly off its
/// `GtkTreePath`'s own `gtk_tree_path_get_depth` (`1` for a root item,
/// matching `#PB_Tree_SubLevel`'s own `0`-based convention once offset
/// by one) - is get-only, real PB's own docs listing no settable
/// counterpart for it at all.
inline std::int64_t pbGetGadgetItemAttribute(std::int64_t gadgetId, std::int64_t element, std::int64_t attribute,
                                              std::int64_t column = 0) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        if (attribute != 1) { // #PB_ListIcon_ColumnWidth
            return 0;
        }
        GtkTreeViewColumn* col = detail::listIconColumnAtPosition(GTK_TREE_VIEW(treeView), static_cast<int>(column));
        return col == nullptr ? 0 : gtk_tree_view_column_get_fixed_width(col);
    }
    GtkWidget* treeView = detail::treeGadgetTreeView(it->second);
    if (treeView == nullptr || attribute != 1) { // #PB_Tree_SubLevel
        return 0;
    }
    auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
    GtkTreeIter iter;
    if (!detail::treeIterAtFlatIndex(model, static_cast<int>(element), &iter)) {
        return 0;
    }
    GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
    std::int64_t level = gtk_tree_path_get_depth(path) - 1;
    gtk_tree_path_free(path);
    return level;
}

inline std::int64_t pbSetGadgetItemAttribute(std::int64_t gadgetId, std::int64_t /*element*/,
                                              std::int64_t attribute, std::int64_t value, std::int64_t column = 0) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    GtkWidget* treeView = detail::listIconTreeView(it->second);
    if (treeView == nullptr || attribute != 1) { // #PB_ListIcon_ColumnWidth
        return 0;
    }
    GtkTreeViewColumn* col = detail::listIconColumnAtPosition(GTK_TREE_VIEW(treeView), static_cast<int>(column));
    if (col == nullptr) {
        return 0;
    }
    gtk_tree_view_column_set_fixed_width(col, static_cast<int>(value));
    return 1;
}

/// M7b's eleventh GUI slice: `SplitterGadget` - a resizable divider
/// between two *already-existing* gadgets, oracle-verified via its own
/// real syntax to be fundamentally different from `ContainerGadget`/
/// `PanelGadget`: it takes `#Gadget1`/`#Gadget2` directly, rather than
/// being a gadget-list nesting target gadgets are created *into* -
/// `GtkPaned` is the exact match. `#Gadget1`/`#Gadget2` must already
/// exist (real PB's own official examples create them with a throwaway
/// `0,0,0,0` position/size first, since the Splitter determines their
/// real position/size itself) - each is removed from its own current
/// parent (whatever `placeGadget` originally put it into) and packed
/// into the new `GtkPaned` instead.
///
/// **Orientation, oracle-verified directly rather than assumed from the
/// flag's own name alone**: `#PB_Splitter_Vertical`'s default divider
/// position (no explicit `SetGadgetState`) is half of the Splitter's own
/// *width*, confirmed against a default (no-flag) Splitter's own default
/// position being half its own *height* instead - meaning `Vertical`
/// means "the divider bar itself is vertical" (panes side by side,
/// `GTK_ORIENTATION_HORIZONTAL` for `GtkPaned`), the opposite naming
/// convention from `ContainerGadget`'s own shadow-type flags, confirmed
/// this way rather than guessed from the name.
///
/// **`FirstFixed`/`SecondFixed`**: oracle-verified directly (a
/// `FirstFixed` Splitter's own divider position, in pixels from the
/// start, stays unchanged after widening it with `ResizeGadget`) to map
/// onto `gtk_paned_pack1`/`pack2`'s own `resize` parameter - the fixed
/// pane doesn't grow/shrink when the Splitter itself is resized, the
/// other one absorbs the difference. `shrink` is unconditionally `TRUE`
/// either way, so `FirstMinimumSize`/`SecondMinimumSize` (set via
/// `SetGadgetAttribute` below) can still take effect - real PB's own
/// docs describe configuring a minimum size at all, which would be
/// meaningless if a pane could never be dragged smaller than its natural
/// size in the first place.
///
/// **`#PB_Splitter_Separator`'s own "3D pattern in the separator bar" is
/// accepted but not acted on** - purely cosmetic (GTK's own theme engine
/// already decides a `GtkPaned` handle's appearance; there's no portable
/// "always render a 3D pattern regardless of theme" knob to hook into),
/// the same kind of deliberate simplification `ContainerGadget`'s own
/// shadow-type flags are a *positive* example of doing properly - this
/// one genuinely has no equivalent to map onto.
///
/// No `EventType()` is documented for this gadget at all (unlike
/// `PanelGadget`'s own `#PB_EventType_Change`/`Resize`) - oracle-verified
/// by its own absence from `SplitterGadget.html`'s own remarks, so no
/// signal handler is connected here at all.
inline std::int64_t pbSplitterGadget(std::int64_t gadgetId, std::int64_t x, std::int64_t y, std::int64_t width,
                                      std::int64_t height, std::int64_t gadget1Id, std::int64_t gadget2Id,
                                      std::int64_t flags = 0) {
    detail::ensureGtkInit();
    auto it1 = detail::gadgetTable().find(gadget1Id);
    auto it2 = detail::gadgetTable().find(gadget2Id);
    if (it1 == detail::gadgetTable().end() || it2 == detail::gadgetTable().end()) {
        return 0;
    }
    GtkWidget* child1 = it1->second;
    GtkWidget* child2 = it2->second;
    GtkWidget* paned = gtk_paned_new((flags & 1) != 0 ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);

    // `gtk_container_remove` drops the *old* parent's own reference - if
    // that was the only one (always true here, nothing else of this
    // project's own ever takes a reference of its own on a plain gadget
    // widget), the child is destroyed on the spot unless a reference of
    // its own is taken first, exactly like `pbSetGadgetAttribute`'s own
    // replacement case below already does.
    GtkWidget* oldParent1 = gtk_widget_get_parent(child1);
    if (oldParent1 != nullptr) {
        g_object_ref(child1);
        gtk_container_remove(GTK_CONTAINER(oldParent1), child1);
    }
    GtkWidget* oldParent2 = gtk_widget_get_parent(child2);
    if (oldParent2 != nullptr) {
        g_object_ref(child2);
        gtk_container_remove(GTK_CONTAINER(oldParent2), child2);
    }
    gboolean resize1 = (flags & 4) == 0 ? TRUE : FALSE;
    gboolean resize2 = (flags & 8) == 0 ? TRUE : FALSE;
    gtk_paned_pack1(GTK_PANED(paned), child1, resize1, TRUE);
    gtk_paned_pack2(GTK_PANED(paned), child2, resize2, TRUE);
    if (oldParent1 != nullptr) {
        g_object_unref(child1);
    }
    if (oldParent2 != nullptr) {
        g_object_unref(child2);
    }
    g_object_set_data(G_OBJECT(paned), detail::splitterFirstResizeKey(),
                       reinterpret_cast<gpointer>(static_cast<std::intptr_t>(resize1)));
    g_object_set_data(G_OBJECT(paned), detail::splitterSecondResizeKey(),
                       reinterpret_cast<gpointer>(static_cast<std::intptr_t>(resize2)));
    // Oracle-verified: a freshly created Splitter's own divider starts at
    // the halfway point of whichever dimension its own orientation
    // splits (width for Vertical/side-by-side, height otherwise) - GTK's
    // own `gtk_paned_new` doesn't pick this on its own (its own natural
    // position depends on a size allocation that hasn't happened yet,
    // this early), so it's set explicitly here instead.
    gtk_paned_set_position(GTK_PANED(paned),
                           static_cast<int>(((flags & 1) != 0 ? width : height) / 2));

    return detail::placeGadget(detail::activeWindowId(), gadgetId, x, y, width, height, paned) ? 1 : 0;
}

/// `GetGadgetAttribute(#Gadget, Attribute)` - a brand new, generic,
/// dispatch-based function (the same shape `AddGadgetItem`'s own family
/// already has), scoped to `SplitterGadget`'s own four attributes and
/// `ScrollAreaGadget`'s own five (M7b's twelfth slice) for now - real PB's
/// own docs list many more gadget types sharing this function, none
/// implemented yet. Oracle-verified: an unsupported attribute (or a
/// `#Gadget` that isn't one of these at all) harmlessly returns `0` - a
/// *nonexistent* `#Gadget` is instead a real, fatal debugger error, not
/// replicated here (this project's own established stance - see
/// `placeGadget`'s own doc comment), so this returns `0` for that case
/// too, the same as an unsupported one.
inline std::int64_t pbGetGadgetAttribute(std::int64_t gadgetId, std::int64_t attribute) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    // `ListViewGadget` (M7b's thirteenth GUI slice) is *also* a
    // `GtkScrolledWindow` (wrapping a natively-scrollable `GtkTreeView`
    // directly, not an auto-created `GtkViewport`) - `containerFixedTable()`
    // only ever holds a *ScrollArea's* own inner `GtkFixed`, so checking
    // it too safely tells the two apart without needing to inspect the
    // wrapped child's own type here as well.
    if (GTK_IS_SCROLLED_WINDOW(it->second) != 0 && detail::containerFixedTable().contains(gadgetId)) {
        auto* scrolled = GTK_SCROLLED_WINDOW(it->second);
        // Not `gtk_bin_get_child(GTK_BIN(scrolled))` - GTK3 auto-wraps a
        // non-`GtkScrollable` child (a plain `GtkFixed`, same as every
        // other gadget-list nesting type's own inner one) in a
        // `GtkViewport` on `gtk_container_add`, so that would find the
        // auto-created viewport, not the real inner `GtkFixed` - already
        // tracked directly in `containerFixedTable()` since creation (the
        // same lookup `OpenGadgetList`'s own fallback case already uses),
        // reused here instead of navigating the widget tree.
        GtkWidget* inner = detail::containerFixedTable().at(gadgetId);
        switch (attribute) {
            case 1: { // #PB_ScrollArea_InnerWidth
                int w = -1;
                int h = -1;
                gtk_widget_get_size_request(inner, &w, &h);
                return w;
            }
            case 2: { // #PB_ScrollArea_InnerHeight
                int w = -1;
                int h = -1;
                gtk_widget_get_size_request(inner, &w, &h);
                return h;
            }
            case 3: // #PB_ScrollArea_X
                return static_cast<std::int64_t>(gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(scrolled)));
            case 4: // #PB_ScrollArea_Y
                return static_cast<std::int64_t>(gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(scrolled)));
            case 5: // #PB_ScrollArea_ScrollStep
                return static_cast<std::int64_t>(
                    gtk_adjustment_get_step_increment(gtk_scrolled_window_get_hadjustment(scrolled)));
            default:
                return 0;
        }
    }
    // M7b's fourteenth GUI slice: `ListIconGadget` - both get-only,
    // oracle-verified attribute values `#PB_ListIcon_ColumnCount`=3/
    // `ClickedColumn`=4 (`DisplayMode`/`ColumnWidth`/`ColumnAlignment`
    // aren't implemented yet - real PB's own docs list them too, but
    // they're a narrower, deferred gap for now).
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        switch (attribute) {
            case 3: { // #PB_ListIcon_ColumnCount
                GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(treeView));
                int count = g_list_length(columns);
                g_list_free(columns);
                return count;
            }
            case 4: // #PB_ListIcon_ClickedColumn
                return reinterpret_cast<std::int64_t>(
                    g_object_get_data(G_OBJECT(treeView), detail::listIconClickedColumnKey()));
            default:
                return 0;
        }
    }
    // M7b's fifteenth GUI slice: `ProgressBarGadget` - both get-only
    // here (`SetGadgetAttribute`'s own matching branch below is the
    // setter half).
    if (GTK_IS_PROGRESS_BAR(it->second) != 0) {
        switch (attribute) {
            case 1: // #PB_ProgressBar_Minimum
                return static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(
                    g_object_get_data(G_OBJECT(it->second), detail::progressBarMinKey())));
            case 2: // #PB_ProgressBar_Maximum
                return static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(
                    g_object_get_data(G_OBJECT(it->second), detail::progressBarMaxKey())));
            default:
                return 0;
        }
    }
    if (GTK_IS_PANED(it->second) == 0) {
        return 0;
    }
    auto* paned = GTK_PANED(it->second);
    switch (attribute) {
        case 1: // #PB_Splitter_FirstMinimumSize
            return reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(paned), detail::splitterFirstMinSizeKey()));
        case 2: // #PB_Splitter_SecondMinimumSize
            return reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(paned), detail::splitterSecondMinSizeKey()));
        case 3: { // #PB_Splitter_FirstGadget
            GtkWidget* child = gtk_paned_get_child1(paned);
            return child == nullptr ? 0
                                     : reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(child), detail::gadgetIdKey()));
        }
        case 4: { // #PB_Splitter_SecondGadget
            GtkWidget* child = gtk_paned_get_child2(paned);
            return child == nullptr ? 0
                                     : reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(child), detail::gadgetIdKey()));
        }
        default:
            return 0;
    }
}

/// `SetGadgetAttribute(#Gadget, Attribute, Value)` - see
/// `pbGetGadgetAttribute`'s own doc comment for the general shape.
/// `FirstMinimumSize`/`SecondMinimumSize` apply via `gtk_widget_set_size_
/// request` on whichever dimension the Splitter's own orientation makes
/// relevant (width for a `Vertical` - side-by-side - Splitter, height
/// otherwise), preserving the *other* dimension's own existing request.
/// `FirstGadget`/`SecondGadget` replace a pane's own child with a
/// different, already-existing gadget - oracle-verified directly: the
/// *old* child is not freed, it's reparented back onto the window that
/// contains the Splitter (read via the Splitter's own `gadgetWindowIdKey()`
/// tag), landing at `(0, 0)` - real PB's own docs don't specify exactly
/// where, only that it isn't destroyed, so an arbitrary but harmless
/// placement is enough; a caller that cares can `ResizeGadget` it
/// afterward, the same way real PB code would reposition it manually too.
inline std::int64_t pbSetGadgetAttribute(std::int64_t gadgetId, std::int64_t attribute, std::int64_t value) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    // See pbGetGadgetAttribute's own note on disambiguating from ListViewGadget.
    if (GTK_IS_SCROLLED_WINDOW(it->second) != 0 && detail::containerFixedTable().contains(gadgetId)) {
        auto* scrolled = GTK_SCROLLED_WINDOW(it->second);
        GtkWidget* inner = detail::containerFixedTable().at(gadgetId);
        switch (attribute) {
            case 1: { // #PB_ScrollArea_InnerWidth
                int w = -1;
                int h = -1;
                gtk_widget_get_size_request(inner, &w, &h);
                gtk_widget_set_size_request(inner, static_cast<int>(value), h);
                return 1;
            }
            case 2: { // #PB_ScrollArea_InnerHeight
                int w = -1;
                int h = -1;
                gtk_widget_get_size_request(inner, &w, &h);
                gtk_widget_set_size_request(inner, w, static_cast<int>(value));
                return 1;
            }
            case 3: // #PB_ScrollArea_X
                gtk_adjustment_set_value(gtk_scrolled_window_get_hadjustment(scrolled), static_cast<double>(value));
                return 1;
            case 4: // #PB_ScrollArea_Y
                gtk_adjustment_set_value(gtk_scrolled_window_get_vadjustment(scrolled), static_cast<double>(value));
                return 1;
            case 5: // #PB_ScrollArea_ScrollStep
                gtk_adjustment_set_step_increment(gtk_scrolled_window_get_hadjustment(scrolled),
                                                   static_cast<double>(value));
                gtk_adjustment_set_step_increment(gtk_scrolled_window_get_vadjustment(scrolled),
                                                   static_cast<double>(value));
                return 1;
            default:
                return 0;
        }
    }
    if (GTK_IS_PROGRESS_BAR(it->second) != 0) {
        switch (attribute) {
            case 1: // #PB_ProgressBar_Minimum
                g_object_set_data(G_OBJECT(it->second), detail::progressBarMinKey(), reinterpret_cast<gpointer>(value));
                detail::progressBarUpdateFraction(it->second);
                return 1;
            case 2: // #PB_ProgressBar_Maximum
                g_object_set_data(G_OBJECT(it->second), detail::progressBarMaxKey(), reinterpret_cast<gpointer>(value));
                detail::progressBarUpdateFraction(it->second);
                return 1;
            default:
                return 0;
        }
    }
    if (GTK_IS_PANED(it->second) == 0) {
        return 0;
    }
    auto* paned = GTK_PANED(it->second);
    GtkWidget* panedWidget = it->second;
    switch (attribute) {
        case 1: // #PB_Splitter_FirstMinimumSize
        case 2: { // #PB_Splitter_SecondMinimumSize
            const char* key =
                attribute == 1 ? detail::splitterFirstMinSizeKey() : detail::splitterSecondMinSizeKey();
            g_object_set_data(G_OBJECT(paned), key, reinterpret_cast<gpointer>(value));
            GtkWidget* child = attribute == 1 ? gtk_paned_get_child1(paned) : gtk_paned_get_child2(paned);
            if (child == nullptr) {
                return 0;
            }
            int curW = -1;
            int curH = -1;
            gtk_widget_get_size_request(child, &curW, &curH);
            if (gtk_orientable_get_orientation(GTK_ORIENTABLE(paned)) == GTK_ORIENTATION_HORIZONTAL) {
                gtk_widget_set_size_request(child, static_cast<int>(value), curH);
            } else {
                gtk_widget_set_size_request(child, curW, static_cast<int>(value));
            }
            return 1;
        }
        case 3: // #PB_Splitter_FirstGadget
        case 4: { // #PB_Splitter_SecondGadget
            auto newIt = detail::gadgetTable().find(value);
            if (newIt == detail::gadgetTable().end()) {
                return 0;
            }
            GtkWidget* oldChild = attribute == 3 ? gtk_paned_get_child1(paned) : gtk_paned_get_child2(paned);
            GtkWidget* newChild = newIt->second;
            GtkWidget* newChildParent = gtk_widget_get_parent(newChild);
            if (newChildParent != nullptr) {
                g_object_ref(newChild);
                gtk_container_remove(GTK_CONTAINER(newChildParent), newChild);
            }
            if (oldChild != nullptr) {
                g_object_ref(oldChild);
                gtk_container_remove(GTK_CONTAINER(panedWidget), oldChild);
                auto windowId =
                    reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(panedWidget), detail::gadgetWindowIdKey()));
                auto fixedIt = detail::windowFixedTable().find(windowId);
                if (fixedIt != detail::windowFixedTable().end()) {
                    gtk_fixed_put(GTK_FIXED(fixedIt->second), oldChild, 0, 0);
                }
                g_object_unref(oldChild);
            }
            const char* resizeKey = attribute == 3 ? detail::splitterFirstResizeKey() : detail::splitterSecondResizeKey();
            auto resize = static_cast<gboolean>(
                reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(panedWidget), resizeKey)));
            if (attribute == 3) {
                gtk_paned_pack1(paned, newChild, resize, TRUE);
            } else {
                gtk_paned_pack2(paned, newChild, resize, TRUE);
            }
            if (newChildParent != nullptr) {
                g_object_unref(newChild);
            }
            return 1;
        }
        default:
            return 0;
    }
}

/// `OpenGadgetList(#Gadget [, Element])` - reopens a previously-created
/// container as the current gadget-list target, so more gadgets can be
/// added to it dynamically after its own matching `CloseGadgetList()`.
/// `Element` is `PanelGadget`-specific (selects which existing tab to add
/// to) - accepted but ignored for a plain `ContainerGadget`, oracle-
/// verified a bare one-argument call against one needs nothing else.
/// Returns `0` (and pushes nothing) for a `#Gadget` that isn't a
/// currently live container/panel - this project's own established "not
/// worth modeling every misuse precisely" stance (see `placeGadget`'s own
/// doc comment).
///
/// M7b's tenth GUI slice (`PanelGadget`) extends this to dispatch on the
/// gadget's own real GTK widget type, the same pattern `pbGetGadgetText`
/// already uses: a `GtkNotebook` reopens `Element`'s own existing tab
/// page (oracle-verified: `Element` genuinely addresses an *existing* tab
/// here - `gtk_notebook_get_nth_page` returns `nullptr`, a harmless
/// failure, for one that doesn't exist), everything else falls back to
/// `containerFixedTable()` exactly as before. Real PB's own docs suggest
/// omitting `Element` entirely dynamically adds a *new* tab for a Panel -
/// oracle-tested directly and found to misbehave on this platform (real,
/// unprompted `Gtk (CRITICAL): gtk_layout_put: assertion 'GTK_IS_LAYOUT
/// (layout)' failed`/`gtk_widget_realize` warnings from the oracle itself,
/// with no new tab actually created) - not replicated; omitting `Element`
/// here always means the same as passing `0` (reopening the first tab),
/// a safer, well-defined choice `AddGadgetItem` remains the correct way
/// to add a genuinely new tab dynamically either way.
inline std::int64_t pbOpenGadgetList(std::int64_t gadgetId, std::int64_t element = 0) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GTK_IS_NOTEBOOK(it->second) != 0) {
        GtkWidget* page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(it->second), static_cast<int>(element));
        if (page == nullptr) {
            return 0;
        }
        detail::gadgetListStack().push_back({gadgetId, page});
        return 1;
    }
    auto containerIt = detail::containerFixedTable().find(gadgetId);
    if (containerIt == detail::containerFixedTable().end()) {
        return 0;
    }
    detail::gadgetListStack().push_back({gadgetId, containerIt->second});
    return 1;
}

/// `CloseGadgetList()` - pops `gadgetListStack()` back to whatever was
/// current before the matching `ContainerGadget`/`OpenGadgetList` call.
/// Oracle-verified: calling this with nothing open is a real, fatal
/// debugger error ("CloseGadgetList(): CloseGadgetList() can only be
/// called after OpenGadgetList() or container gadgets.") - not
/// replicated here, a deliberate simplification (silent no-op instead),
/// since a release (non-`-d`) build was oracle-confirmed to *not* crash
/// either, and this project has no existing mechanism for a plain
/// generic-builtin runtime function to behave differently in debug vs.
/// release mode the way `Read`'s own data-exhaustion check does (that
/// one's debug-only check is injected by codegen itself, around a
/// dedicated `Read` statement - `CloseGadgetList()` is just an ordinary
/// builtin call, nothing dedicated to hook into the same way).
inline std::int64_t pbCloseGadgetList() {
    if (detail::gadgetListStack().empty()) {
        return 0;
    }
    detail::gadgetListStack().pop_back();
    return 1;
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
    // M7b's ninth GUI slice: a no-op for a plain (non-container) gadget -
    // see `pruneContainerGadgets`'s own doc comment for why this needs to
    // happen *before* `gtk_widget_destroy` below, not after.
    detail::pruneContainerGadgets(gadgetId);
    gtk_widget_destroy(it->second);
    detail::gadgetTable().erase(it);
    detail::containerFixedTable().erase(gadgetId);
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
    // M7b's thirteenth GUI slice: ListViewGadget's own selected row's
    // text, or ComboBoxGadget's own entry (editable) / active row's text
    // (not) - oracle-verified via each gadget's own remarks.
    if (GtkWidget* treeView = detail::listViewTreeView(widget)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows == nullptr) {
            return PBString();
        }
        auto* model = GTK_TREE_MODEL(detail::itemListStoreFor(widget));
        GtkTreeIter iter;
        PBString result;
        if (gtk_tree_model_get_iter(model, &iter, static_cast<GtkTreePath*>(rows->data)) != 0) {
            gchar* text = nullptr;
            gtk_tree_model_get(model, &iter, detail::itemTextColumn(), &text, -1);
            result = safe(text);
            g_free(text);
        }
        g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        return result;
    }
    if (GTK_IS_COMBO_BOX(widget) != 0) {
        auto* combo = GTK_COMBO_BOX(widget);
        GtkWidget* entry = gtk_bin_get_child(GTK_BIN(combo));
        if (entry != nullptr && GTK_IS_ENTRY(entry) != 0) {
            return safe(gtk_entry_get_text(GTK_ENTRY(entry)));
        }
        GtkTreeIter iter;
        if (gtk_combo_box_get_active_iter(combo, &iter) == 0) {
            return PBString();
        }
        gchar* text = nullptr;
        gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter, detail::itemTextColumn(), &text, -1);
        PBString result = safe(text);
        g_free(text);
        return result;
    }
    // M7b's fourteenth GUI slice: `ListIconGadget` returns the *first
    // column's* own text of the first selected row (oracle-verified:
    // "Renvoie le texte de la première colonne de l'élément sélectionné"),
    // `TreeGadget` the selected element's own text - both via the same
    // `GtkTreeSelection`-based shape `ListViewGadget`'s own above uses.
    if (GtkWidget* treeView = detail::listIconTreeView(widget)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows == nullptr) {
            return PBString();
        }
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        PBString result;
        if (gtk_tree_model_get_iter(model, &iter, static_cast<GtkTreePath*>(rows->data)) != 0) {
            gchar* text = nullptr;
            gtk_tree_model_get(model, &iter, 0, &text, -1);
            result = safe(text);
            g_free(text);
        }
        g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        return result;
    }
    if (GtkWidget* treeView = detail::treeGadgetTreeView(widget)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows == nullptr) {
            return PBString();
        }
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        PBString result;
        if (gtk_tree_model_get_iter(model, &iter, static_cast<GtkTreePath*>(rows->data)) != 0) {
            gchar* text = nullptr;
            gtk_tree_model_get(model, &iter, detail::treeTextColumn(), &text, -1);
            result = safe(text);
            g_free(text);
        }
        g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        return result;
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
    } else if (GtkWidget* treeView = detail::listViewTreeView(widget)) {
        // Oracle-verified directly (not just assumed): a non-matching
        // text genuinely *clears* the current selection rather than
        // leaving it unchanged - confirmed with a dedicated, isolated
        // probe after this project's own golden e2e comparison caught it
        // disagreeing with the real oracle.
        GtkTreeIter iter;
        auto* model = GTK_TREE_MODEL(detail::itemListStoreFor(widget));
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListViewSelectionChanged),
                                         widget);
        if (detail::findRowByText(model, text.bytes(), &iter)) {
            gtk_tree_selection_select_iter(selection, &iter);
        } else {
            gtk_tree_selection_unselect_all(selection);
        }
        g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListViewSelectionChanged),
                                           widget);
    } else if (GTK_IS_COMBO_BOX(widget) != 0) {
        auto* combo = GTK_COMBO_BOX(widget);
        GtkWidget* entry = gtk_bin_get_child(GTK_BIN(combo));
        if (entry != nullptr && GTK_IS_ENTRY(entry) != 0) {
            // Oracle-verified: an editable combo accepts arbitrary text,
            // whether or not it matches any item - unlike a non-editable
            // one (below), which only ever selects an existing item (or
            // clears the selection entirely, the same real finding as
            // ListViewGadget's own above).
            g_signal_handlers_block_by_func(widget, reinterpret_cast<gpointer>(detail::onComboBoxChanged), nullptr);
            gtk_entry_set_text(GTK_ENTRY(entry), text.bytes().c_str());
            g_signal_handlers_unblock_by_func(widget, reinterpret_cast<gpointer>(detail::onComboBoxChanged), nullptr);
        } else {
            GtkTreeIter iter;
            GtkTreeModel* model = gtk_combo_box_get_model(combo);
            g_signal_handlers_block_by_func(widget, reinterpret_cast<gpointer>(detail::onComboBoxChanged), nullptr);
            if (detail::findRowByText(model, text.bytes(), &iter)) {
                gtk_combo_box_set_active_iter(combo, &iter);
            } else {
                gtk_combo_box_set_active(combo, -1);
            }
            g_signal_handlers_unblock_by_func(widget, reinterpret_cast<gpointer>(detail::onComboBoxChanged), nullptr);
        }
    } else if (GtkWidget* treeView = detail::treeGadgetTreeView(widget)) {
        // `SetGadgetText` is documented for `TreeGadget` but *not* for
        // `ListIconGadget` (oracle-verified: absent from its own
        // "particulièrement utile pour" list, unlike `GetGadgetText`
        // above which does list it) - so there's no `listIconTreeView`
        // branch here at all, falling through to the harmless-failure
        // `return 0` below for it.
        //
        // Oracle-verified directly (not assumed, and genuinely different
        // from ListView/ComboBox's own above): `TreeGadget`'s own
        // `SetGadgetText` does *not* search for a matching item at all -
        // it renames whichever item is *currently selected*, symmetric
        // with `GetGadgetText`'s own "selected element's text" reading
        // above. A dedicated probe confirmed both halves: passing an
        // existing *other* item's own text does nothing to the
        // selection (stays on whatever was already selected, even when
        // that text would have matched a different node), and with
        // nothing selected at all it's a complete no-op.
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows != nullptr) {
            auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(model, &iter, static_cast<GtkTreePath*>(rows->data)) != 0) {
                gtk_tree_store_set(GTK_TREE_STORE(model), &iter, detail::treeTextColumn(), text.bytes().c_str(), -1);
            }
            g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        }
    } else {
        return 0;
    }
    return 1;
}

/// `CheckBoxGadget` (a `GtkToggleButton`) has a meaningful checked state;
/// `PanelGadget` (M7b's tenth GUI slice, a `GtkNotebook`) reports its own
/// currently-displayed tab index instead, oracle-verified via
/// `PanelGadget.html`'s own remarks ("Renvoie le numéro de l'onglet
/// actuellement affiché") - every other gadget type harmlessly returns/
/// ignores `0`, the same kind of narrow, documented simplification as
/// `GetGadgetState`'s own untested-by-the-oracle behavior for every other
/// non-stateful gadget.
inline std::int64_t pbGetGadgetState(std::int64_t gadgetId) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GTK_IS_NOTEBOOK(it->second) != 0) {
        return gtk_notebook_get_current_page(GTK_NOTEBOOK(it->second));
    }
    // `SplitterGadget` (M7b's eleventh GUI slice, a `GtkPaned`) reports
    // its own divider position in pixels - oracle-verified via
    // `SplitterGadget.html`'s own remarks ("Renvoie la position de la
    // barre de séparation, en pixels").
    if (GTK_IS_PANED(it->second) != 0) {
        return gtk_paned_get_position(GTK_PANED(it->second));
    }
    // M7b's thirteenth GUI slice: `ListViewGadget` reports the (0-based)
    // index of its own first selected row, `-1` if none - oracle-verified
    // via `ListViewGadget.html`'s own remarks. `ComboBoxGadget` maps onto
    // `gtk_combo_box_get_active` directly, already using the identical
    // `-1`-means-"none" convention PB's own docs describe, with no
    // translation needed at all.
    if (GtkWidget* treeView = detail::listViewTreeView(it->second)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows == nullptr) {
            return -1;
        }
        auto* path = static_cast<GtkTreePath*>(rows->data);
        std::int64_t index = gtk_tree_path_get_indices(path)[0];
        g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        return index;
    }
    // M7b's fourteenth GUI slice: `ListIconGadget` - oracle-verified
    // "Renvoie le numéro du premier élément sélectionné ou -1" - the same
    // flat row-index convention `ListViewGadget`'s own above already
    // uses, just over its own distinct marker-tagged `GtkTreeView`.
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows == nullptr) {
            return -1;
        }
        auto* path = static_cast<GtkTreePath*>(rows->data);
        std::int64_t index = gtk_tree_path_get_indices(path)[0];
        g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        return index;
    }
    // `TreeGadget` - the same selected-index convention, but translated
    // through `treeFlatIndexOfIter` since the index is the flat, depth-
    // first position across the whole tree, not a sibling index.
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        GList* rows = gtk_tree_selection_get_selected_rows(selection, nullptr);
        if (rows == nullptr) {
            return -1;
        }
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter iter;
        std::int64_t index = -1;
        if (gtk_tree_model_get_iter(model, &iter, static_cast<GtkTreePath*>(rows->data)) != 0) {
            index = detail::treeFlatIndexOfIter(model, &iter);
        }
        g_list_free_full(rows, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
        return index;
    }
    if (GTK_IS_COMBO_BOX(it->second) != 0) {
        return gtk_combo_box_get_active(GTK_COMBO_BOX(it->second));
    }
    // M7b's fifteenth GUI slice: `ProgressBarGadget` reports its own
    // current value verbatim (including the `#PB_ProgressBar_Unknown`
    // sentinel) - `OptionGadget` needs no branch of its own here at all,
    // already covered below by the pre-existing `GTK_IS_TOGGLE_BUTTON`
    // fallback (a `GtkRadioButton` is one).
    if (GTK_IS_PROGRESS_BAR(it->second) != 0) {
        return static_cast<std::int64_t>(
            reinterpret_cast<std::intptr_t>(g_object_get_data(G_OBJECT(it->second), detail::progressBarValueKey())));
    }
    if (GTK_IS_TOGGLE_BUTTON(it->second) == 0) {
        return 0;
    }
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(it->second)) != 0 ? 1 : 0;
}

/// Oracle-verified: `SetGadgetState` does *not* queue a spurious
/// `#PB_Event_Gadget` the way a real user click/toggle (or, for
/// `PanelGadget`, tab click) does - confirmed directly both ways (a
/// `SetGadgetState` immediately followed by a drained event poll loop
/// reports zero gadget events either way). `gtk_toggle_button_set_active`/
/// `gtk_notebook_set_current_page` would otherwise fire the same
/// `"toggled"`/`"switch-page"` signal a real click does, so the handler
/// is blocked around this one programmatic change specifically, same
/// idea either way.
inline std::int64_t pbSetGadgetState(std::int64_t gadgetId, std::int64_t state) {
    auto it = detail::gadgetTable().find(gadgetId);
    if (it == detail::gadgetTable().end()) {
        return 0;
    }
    if (GTK_IS_NOTEBOOK(it->second) != 0) {
        g_signal_handlers_block_by_func(it->second, reinterpret_cast<gpointer>(detail::onPanelSwitchPage), nullptr);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(it->second), static_cast<int>(state));
        g_signal_handlers_unblock_by_func(it->second, reinterpret_cast<gpointer>(detail::onPanelSwitchPage), nullptr);
        return 1;
    }
    // No signal to block here at all - `SplitterGadget` has no
    // documented `EventType()` support of its own (see `pbSplitterGadget`'s
    // own doc comment), so there's nothing a programmatic position change
    // could spuriously fire in the first place.
    if (GTK_IS_PANED(it->second) != 0) {
        gtk_paned_set_position(GTK_PANED(it->second), static_cast<int>(state));
        return 1;
    }
    // Oracle-verified: `-1` deselects every row, for `ListViewGadget`
    // (even in multi-select mode) just like `ComboBoxGadget`'s own
    // identical `-1` convention `gtk_combo_box_set_active` already uses
    // natively.
    if (GtkWidget* treeView = detail::listViewTreeView(it->second)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListViewSelectionChanged),
                                         it->second);
        if (state < 0) {
            gtk_tree_selection_unselect_all(selection);
        } else {
            GtkTreePath* path = gtk_tree_path_new_from_indices(static_cast<int>(state), -1);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
        g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListViewSelectionChanged),
                                           it->second);
        return 1;
    }
    if (GtkWidget* treeView = detail::listIconTreeView(it->second)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                         it->second);
        if (state < 0) {
            gtk_tree_selection_unselect_all(selection);
        } else {
            GtkTreePath* path = gtk_tree_path_new_from_indices(static_cast<int>(state), -1);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
        g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                           it->second);
        return 1;
    }
    // `GtkTreeSelection::select_iter`/`select_path` silently do nothing
    // for a row whose own ancestor chain isn't expanded - oracle-verified
    // real PB's own `SetGadgetState`/`SetGadgetItemState` have no such
    // restriction (a freshly-populated Tree, with every branch collapsed
    // by default, still lets element 2 - a level-1 child - be selected
    // directly), so the target row's own ancestor chain is force-expanded
    // first via `gtk_tree_view_expand_to_path` to match.
    if (GtkWidget* treeView = detail::treeGadgetTreeView(it->second)) {
        auto* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        g_signal_handlers_block_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                         it->second);
        GtkTreeIter iter;
        if (state < 0) {
            gtk_tree_selection_unselect_all(selection);
        } else if (detail::treeIterAtFlatIndex(model, static_cast<int>(state), &iter)) {
            GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
            gtk_tree_view_expand_to_path(GTK_TREE_VIEW(treeView), path);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
        g_signal_handlers_unblock_by_func(selection, reinterpret_cast<gpointer>(detail::onListIconSelectionChanged),
                                           it->second);
        return 1;
    }
    if (GTK_IS_COMBO_BOX(it->second) != 0) {
        g_signal_handlers_block_by_func(it->second, reinterpret_cast<gpointer>(detail::onComboBoxChanged), nullptr);
        gtk_combo_box_set_active(GTK_COMBO_BOX(it->second), static_cast<int>(state));
        g_signal_handlers_unblock_by_func(it->second, reinterpret_cast<gpointer>(detail::onComboBoxChanged), nullptr);
        return 1;
    }
    // `OptionGadget` - oracle-verified directly: `SetGadgetState(…, 0)`
    // on an already-selected radio button does *nothing* at all (stays
    // selected) - a radio group can't be left with nothing selected this
    // way, only by selecting a *different* member of the group, unlike
    // `CheckBoxGadget`'s own plain on/off toggle the generic fallback
    // below already handles correctly. `state != 0` still falls through
    // to that same generic case (selecting it, `GtkRadioButton`'s own
    // grouping automatically deselecting whichever sibling was active).
    if (GTK_IS_RADIO_BUTTON(it->second) != 0 && state == 0) {
        return 1;
    }
    if (GTK_IS_PROGRESS_BAR(it->second) != 0) {
        auto minimum = static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(
            g_object_get_data(G_OBJECT(it->second), detail::progressBarMinKey())));
        auto maximum = static_cast<std::int64_t>(reinterpret_cast<std::intptr_t>(
            g_object_get_data(G_OBJECT(it->second), detail::progressBarMaxKey())));
        std::int64_t clamped = state == -1 ? -1 : std::clamp(state, minimum, maximum);
        g_object_set_data(G_OBJECT(it->second), detail::progressBarValueKey(), reinterpret_cast<gpointer>(clamped));
        detail::progressBarUpdateFraction(it->second);
        return 1;
    }
    if (GTK_IS_TOGGLE_BUTTON(it->second) == 0) {
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

/// M7b's own Requester family, completing it (`MessageRequester` was the
/// third GUI slice already). `GtkColorChooserDialog` is the natural GTK3
/// fit. Oracle-verified return: the selected color, packed the same way
/// `RGB()` itself is (see mathlib.hpp's own doc comment) - `0x00BBGGRR`,
/// not `0x00RRGGBB` - or `-1` on cancel.
inline std::int64_t pbColorRequester(std::int64_t initialColor = -1, std::int64_t parentId = 0) {
    detail::ensureGtkInit();
    GtkWidget* dialog = gtk_color_chooser_dialog_new("Choose Color", detail::parentWindowFromId(parentId));
    if (initialColor != -1) {
        GdkRGBA rgba;
        rgba.red = static_cast<double>(initialColor & 0xff) / 255.0;
        rgba.green = static_cast<double>((initialColor >> 8) & 0xff) / 255.0;
        rgba.blue = static_cast<double>((initialColor >> 16) & 0xff) / 255.0;
        rgba.alpha = 1.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &rgba);
    }
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    std::int64_t result = -1;
    if (response == GTK_RESPONSE_OK) {
        GdkRGBA rgba;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &rgba);
        auto r = static_cast<std::int64_t>(std::lround(rgba.red * 255.0));
        auto g = static_cast<std::int64_t>(std::lround(rgba.green * 255.0));
        auto b = static_cast<std::int64_t>(std::lround(rgba.blue * 255.0));
        result = r | (g << 8) | (b << 16);
    }
    gtk_widget_destroy(dialog);
    return result;
}

/// `GtkFontChooserDialog` is the natural GTK3 fit, built around a
/// `PangoFontDescription` rather than string parsing (`gtk_font_chooser_
/// get_font_desc` returns one directly, *transfer full* per its own GTK
/// docs - freed here once its fields are copied into `detail::selectedFont()`
/// for the separate accessor functions below to read back).
///
/// `Color` is accepted (real PB's own documented signature requires it)
/// but not wired into an actual color swatch - `GtkFontChooser` has no
/// built-in color picker at all, the identical gap the reference
/// `qt6_subsystem` project's own port already found for the same reason
/// (`QFontDialog` doesn't either) - echoed back unchanged by
/// `SelectedFontColor()` rather than silently dropped. `Options`
/// (`#PB_FontRequester_Effects`) is real PB's own Windows-only flag -
/// accepted but not acted on, the same treatment every other OS-specific
/// niceity in this library already gets.
inline std::int64_t pbFontRequester(const PBString& fontName, std::int64_t fontSize, std::int64_t /*options*/,
                                     std::int64_t color = 0, std::int64_t style = 0, std::int64_t parentId = 0) {
    detail::ensureGtkInit();
    GtkWidget* dialog = gtk_font_chooser_dialog_new("Choose Font", detail::parentWindowFromId(parentId));
    PangoFontDescription* initial = pango_font_description_new();
    if (!fontName.bytes().empty()) {
        pango_font_description_set_family(initial, fontName.bytes().c_str());
    }
    if (fontSize > 0) {
        pango_font_description_set_size(initial, static_cast<gint>(fontSize) * PANGO_SCALE);
    }
    if ((style & 1) != 0) { // #PB_Font_Bold
        pango_font_description_set_weight(initial, PANGO_WEIGHT_BOLD);
    }
    if ((style & 2) != 0) { // #PB_Font_Italic
        pango_font_description_set_style(initial, PANGO_STYLE_ITALIC);
    }
    gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(dialog), initial);
    pango_font_description_free(initial);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    std::int64_t result = 0;
    if (response == GTK_RESPONSE_OK) {
        PangoFontDescription* chosen = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));
        if (chosen != nullptr) {
            detail::SelectedFontState& state = detail::selectedFont();
            const char* family = pango_font_description_get_family(chosen);
            state.name = family != nullptr ? family : "";
            state.size = pango_font_description_get_size(chosen) / PANGO_SCALE;
            std::int64_t newStyle = 0;
            if (pango_font_description_get_weight(chosen) >= PANGO_WEIGHT_BOLD) {
                newStyle |= 1; // #PB_Font_Bold
            }
            if (pango_font_description_get_style(chosen) == PANGO_STYLE_ITALIC) {
                newStyle |= 2; // #PB_Font_Italic
            }
            state.style = newStyle;
            state.color = color;
            pango_font_description_free(chosen);
            result = 1;
        }
    }
    gtk_widget_destroy(dialog);
    return result;
}

inline PBString pbSelectedFontName() { return PBString(detail::selectedFont().name); }
inline std::int64_t pbSelectedFontSize() { return detail::selectedFont().size; }
inline std::int64_t pbSelectedFontStyle() { return detail::selectedFont().style; }
/// Oracle-verified "OS Supportés Windows" only - on every other platform
/// this is just `FontRequester`'s own `Color` argument, echoed back
/// unchanged (see `pbFontRequester`'s own doc comment).
inline std::int64_t pbSelectedFontColor() { return detail::selectedFont().color; }

/// No native GTK3 "get a line of text" dialog exists (unlike Qt's own
/// `QInputDialog`) - built from a plain `GtkDialog` + `GtkLabel` +
/// `GtkEntry` instead. `gtk_entry_set_activates_default` + making the OK
/// button the dialog's own default widget is what makes pressing Enter
/// in the entry submit, rather than requiring an explicit button click.
///
/// Oracle-verified: cancelling returns an empty string by default: only
/// with `#PB_InputRequester_HandleCancel` set does it instead return
/// `#PB_InputRequester_Cancel` - itself a *String* constant (`Chr(10) +
/// Chr(9)`, confirmed byte-by-byte against the real oracle), not an
/// Integer the way every other `#PB_*` constant in this project is -
/// Sema's own builtin-constant table is Integer-only, so that one
/// specific constant isn't exposed as a referenceable name yet (a real,
/// deliberately narrow gap - the *behavior* itself, returning that exact
/// two-character string, is still implemented faithfully; a program can
/// still compare against it by writing `Chr(10)+Chr(9)` directly).
inline PBString pbInputRequester(const PBString& title, const PBString& message, const PBString& defaultText,
                                  std::int64_t options = 0, std::int64_t parentId = 0) {
    detail::ensureGtkInit();
    GtkWidget* dialog =
        gtk_dialog_new_with_buttons(title.bytes().c_str(), detail::parentWindowFromId(parentId), GTK_DIALOG_MODAL,
                                     "_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_OK, nullptr);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget* label = gtk_label_new(message.bytes().c_str());
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), defaultText.bytes().c_str());
    if ((options & 1) != 0) { // #PB_InputRequester_Password
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    }
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_container_add(GTK_CONTAINER(box), label);
    gtk_container_add(GTK_CONTAINER(box), entry);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);
    GtkWidget* okButton = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_set_can_default(okButton, TRUE);
    gtk_widget_grab_default(okButton);
    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    PBString result;
    if (response == GTK_RESPONSE_OK) {
        result = PBString(gtk_entry_get_text(GTK_ENTRY(entry)));
    } else if ((options & 2) != 0) { // #PB_InputRequester_HandleCancel
        result = PBString("\n\t"); // #PB_InputRequester_Cancel - see this function's own doc comment.
    }
    gtk_widget_destroy(dialog);
    return result;
}

/// `GtkFileChooserDialog` (action `OPEN`) is the natural GTK3 fit.
/// `#PB_Requester_MultiSelection` is genuinely supported (unlike the
/// reference `qt6_subsystem` project's own port, whose documented gap
/// here was specifically not having this constant's real numeric value
/// available to confirm - this project's own oracle access resolved that
/// directly, see Sema's own builtin-constant table) - `NextSelectedFileName()`
/// queues every selection after the first (this function's own return
/// value) via `detail::multiSelectedFiles()`.
inline PBString pbOpenFileRequester(const PBString& title, const PBString& defaultFile, const PBString& pattern,
                                     std::int64_t patternPosition, std::int64_t options = 0,
                                     std::int64_t parentId = 0) {
    detail::ensureGtkInit();
    GtkWidget* dialog = gtk_file_chooser_dialog_new(title.bytes().c_str(), detail::parentWindowFromId(parentId),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_OK, nullptr);
    auto* chooser = GTK_FILE_CHOOSER(dialog);
    bool multi = (options & 1) != 0; // #PB_Requester_MultiSelection
    gtk_file_chooser_set_select_multiple(chooser, multi ? TRUE : FALSE);
    if (!defaultFile.bytes().empty()) {
        gtk_file_chooser_set_filename(chooser, defaultFile.bytes().c_str());
    }
    std::vector<GtkFileFilter*> filters = detail::applyFileFilters(chooser, pattern.bytes(), patternPosition);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    PBString result;
    detail::multiSelectedFiles().clear();
    detail::multiSelectedFilesCursor() = 0;
    detail::selectedFilePatternIndex() = -1;
    if (response == GTK_RESPONSE_OK) {
        const GtkFileFilter* activeFilter = gtk_file_chooser_get_filter(chooser);
        for (std::size_t idx = 0; idx < filters.size(); ++idx) {
            if (filters[idx] == activeFilter) {
                detail::selectedFilePatternIndex() = static_cast<std::int64_t>(idx);
                break;
            }
        }
        if (multi) {
            GSList* names = gtk_file_chooser_get_filenames(chooser);
            bool first = true;
            for (GSList* n = names; n != nullptr; n = n->next) {
                auto* name = static_cast<char*>(n->data);
                if (first) {
                    result = PBString(name);
                    first = false;
                } else {
                    detail::multiSelectedFiles().emplace_back(name);
                }
                g_free(name);
            }
            g_slist_free(names);
        } else {
            gchar* filename = gtk_file_chooser_get_filename(chooser);
            if (filename != nullptr) {
                result = PBString(filename);
                g_free(filename);
            }
        }
    }
    gtk_widget_destroy(dialog);
    return result;
}

/// Oracle-verified: queues up every file after `OpenFileRequester`'s own
/// first (`#PB_Requester_MultiSelection` only) - returns an empty string
/// once exhausted, or if the last `OpenFileRequester` call wasn't multi-
/// selection (or was cancelled) at all.
inline PBString pbNextSelectedFileName() {
    auto& files = detail::multiSelectedFiles();
    auto& cursor = detail::multiSelectedFilesCursor();
    if (cursor >= files.size()) {
        return PBString();
    }
    return PBString(files[cursor++]);
}

/// Oracle-verified: the 0-based index of whichever filter was active when
/// `OpenFileRequester`/`SaveFileRequester` closed, or `-1` after a
/// cancelled call (see `detail::selectedFilePatternIndex`'s own doc
/// comment).
inline std::int64_t pbSelectedFilePattern() { return detail::selectedFilePatternIndex(); }

/// `GtkFileChooserDialog` (action `SAVE`) - `set_do_overwrite_confirmation`
/// is a reasonable UX default GTK itself provides (real PB's own docs
/// don't say either way whether overwrite confirmation happens, so this
/// isn't replicating a specific oracle-verified behavior, just a sensible
/// choice consistent with what a native "Save As" dialog normally does).
inline PBString pbSaveFileRequester(const PBString& title, const PBString& defaultFile, const PBString& pattern,
                                     std::int64_t patternPosition, std::int64_t parentId = 0) {
    detail::ensureGtkInit();
    GtkWidget* dialog = gtk_file_chooser_dialog_new(title.bytes().c_str(), detail::parentWindowFromId(parentId),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Save", GTK_RESPONSE_OK, nullptr);
    auto* chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    if (!defaultFile.bytes().empty()) {
        gtk_file_chooser_set_filename(chooser, defaultFile.bytes().c_str());
    }
    std::vector<GtkFileFilter*> filters = detail::applyFileFilters(chooser, pattern.bytes(), patternPosition);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    PBString result;
    detail::selectedFilePatternIndex() = -1;
    if (response == GTK_RESPONSE_OK) {
        const GtkFileFilter* activeFilter = gtk_file_chooser_get_filter(chooser);
        for (std::size_t idx = 0; idx < filters.size(); ++idx) {
            if (filters[idx] == activeFilter) {
                detail::selectedFilePatternIndex() = static_cast<std::int64_t>(idx);
                break;
            }
        }
        gchar* filename = gtk_file_chooser_get_filename(chooser);
        if (filename != nullptr) {
            result = PBString(filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
    return result;
}

/// `GtkFileChooserDialog` (action `SELECT_FOLDER`). Oracle-verified: the
/// returned path has a trailing `/` on Linux (a trailing `\` on Windows,
/// not this backend's concern) - `gtk_file_chooser_get_filename` doesn't
/// include one on its own, appended here to match.
inline PBString pbPathRequester(const PBString& title, const PBString& initialPath, std::int64_t parentId = 0) {
    detail::ensureGtkInit();
    GtkWidget* dialog =
        gtk_file_chooser_dialog_new(title.bytes().c_str(), detail::parentWindowFromId(parentId),
                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Cancel", GTK_RESPONSE_CANCEL,
                                     "_Select", GTK_RESPONSE_OK, nullptr);
    if (!initialPath.bytes().empty()) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initialPath.bytes().c_str());
    }
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    PBString result;
    if (response == GTK_RESPONSE_OK) {
        gchar* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (folder != nullptr) {
            std::string path = folder;
            if (!path.empty() && path.back() != '/') {
                path += '/';
            }
            result = PBString(path);
            g_free(folder);
        }
    }
    gtk_widget_destroy(dialog);
    return result;
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

/// M7b's sixth GUI slice: `ToolBar`, unblocked by the Image library (fifth
/// slice) landing first - oracle-verified (`ToolBarImageButton`'s own docs)
/// that real PB has no way to create a toolbar button without an `ImageID`
/// at all. `GtkToolbar` is the natural GTK3 fit. Oracle-verified real
/// `CreateToolBar`'s own "non-zero on success" is a native-handle-ish
/// value, not a clean `1` - simplified the same way every other creation
/// function in this library already is.
///
/// `#PB_ToolBar_Small`/`#PB_ToolBar_Large` decide the pixel size
/// `ToolBarImageButton` scales its own image to (16/24 - stored per-toolbar
/// in `toolBarIconPixelSize()`, consulted there); `#PB_ToolBar_Text` maps to
/// `GTK_TOOLBAR_BOTH` (icon with its own label below, when a button
/// actually supplies one). `#PB_ToolBar_InlineText` is real PB's own
/// Windows-only option (label beside the icon) - not implemented, the same
/// "no equivalent on this backend" treatment `CreateImageMenu`'s own
/// Windows-only `Options` bit already gets.
///
/// Oracle-verified: a real toolbar is positioned right below an existing
/// menu bar, above the gadget area - `gtk_box_reorder_child` alone (the
/// same trick `pbCreateMenu` already uses to always force itself to
/// position 0) isn't quite enough here, since *this* widget needs to land
/// at position 0 *unless* a menu already exists for the same window, in
/// which case it must go to position 1 instead (right after it) - a menu
/// created *afterward* still ends up above the toolbar regardless, since
/// `pbCreateMenu`'s own unconditional "always force myself to 0" already
/// pushes anything already there (including a toolbar) down by one.
inline std::int64_t pbCreateToolBar(std::int64_t toolBarId, std::int64_t windowHandle, std::int64_t options = 1) {
    detail::ensureGtkInit();
    auto* window = reinterpret_cast<GtkWidget*>(windowHandle);
    GtkWidget* vbox = gtk_bin_get_child(GTK_BIN(window));
    GtkWidget* toolbar = gtk_toolbar_new();
    bool hasText = (options & 4) != 0; // #PB_ToolBar_Text
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), hasText ? GTK_TOOLBAR_BOTH : GTK_TOOLBAR_ICONS);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    bool windowHasMenu = false;
    auto windowId = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(window), detail::windowIdKey()));
    for (auto& [id, menuWidget] : detail::menuTable()) {
        auto owner = reinterpret_cast<std::int64_t>(g_object_get_data(G_OBJECT(menuWidget), detail::menuWindowIdKey()));
        if (owner == windowId) {
            windowHasMenu = true;
            break;
        }
    }
    gtk_box_reorder_child(GTK_BOX(vbox), toolbar, windowHasMenu ? 1 : 0);
    gtk_widget_show(toolbar);

    g_object_set_data(G_OBJECT(toolbar), detail::toolBarWindowIdKey(), reinterpret_cast<gpointer>(windowId));

    // Oracle-verified elsewhere (OpenWindow/CreateMenu): re-using an
    // already-live ID replaces the old one, applied the same way here too.
    auto old = detail::toolBarTable().find(toolBarId);
    if (old != detail::toolBarTable().end()) {
        gtk_widget_destroy(old->second);
    }
    detail::toolBarTable()[toolBarId] = toolbar;
    detail::toolBarButtonWidgets()[toolBarId].clear();
    detail::toolBarIconPixelSize()[toolBarId] = (options & 2) != 0 ? 24 : 16; // #PB_ToolBar_Large : #PB_ToolBar_Small
    detail::activeToolBarId() = toolBarId;
    return 1;
}

/// Oracle-verified: no return value ("Aucune") - always returns `0`, the
/// same established convention as `pbKillThread`'s own (see its doc
/// comment). Operates on whichever `#ToolBar` was most recently
/// `CreateToolBar`'d - the same "current context" pattern `MenuItem`'s own
/// already established for `#Menu`, applied here independently (confirmed
/// directly in real PB's own docs: `ToolBarImageButton`'s remarks say only
/// that `CreateToolBar` must have been called first, with no `#ToolBar`
/// argument of its own).
///
/// Click events are oracle-verified (via this function's own docs) to
/// route through the *same* `#PB_Event_Menu`/`EventMenu()` mechanism real
/// menu items use, not `#PB_Event_Gadget` - so this reuses
/// `detail::onMenuItemActivate` directly rather than a separate handler,
/// tagging the button with the same `menuElementIdKey`/`menuWindowIdKey`
/// `MenuItem()`'s own leaf items already use.
inline std::int64_t pbToolBarImageButton(std::int64_t buttonId, std::int64_t imageId, std::int64_t mode = 0,
                                          const PBString& text = PBString()) {
    auto toolBarId = detail::activeToolBarId();
    auto toolBarIt = detail::toolBarTable().find(toolBarId);
    if (toolBarIt == detail::toolBarTable().end()) {
        return 0;
    }
    int iconSize = detail::toolBarIconPixelSize().count(toolBarId) != 0 ? detail::toolBarIconPixelSize()[toolBarId] : 16;
    GtkWidget* iconImage = nullptr;
    // A real bug, caught only by testing end-to-end with pbcxx rather than
    // by this project's own earlier unit/e2e coverage (none of which ever
    // checked that the icon widget was actually populated, only return
    // values/labels): `imageId` here is `ImageID()`'s own return value -
    // the real `GdkPixbuf*` pointer itself (oracle-verified: "'ImageID'
    // peut être facilement obtenu avec ImageID()"), not a plain `#Image`
    // number to look up in `imageTable()` - the same direct-pointer
    // convention `attachMenuItemImage`'s own code already uses correctly.
    if (imageId != 0) {
        auto* pixbuf = reinterpret_cast<GdkPixbuf*>(imageId);
        GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, iconSize, iconSize, GDK_INTERP_BILINEAR);
        if (scaled != nullptr) {
            iconImage = gtk_image_new_from_pixbuf(scaled);
            g_object_unref(scaled);
        }
    }
    bool toggle = mode == 1; // #PB_ToolBar_Toggle
    GtkToolItem* item = toggle ? gtk_toggle_tool_button_new() : gtk_tool_button_new(iconImage, text.bytes().c_str());
    if (toggle) {
        gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), iconImage);
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), text.bytes().c_str());
    }
    auto windowId = reinterpret_cast<std::int64_t>(
        g_object_get_data(G_OBJECT(toolBarIt->second), detail::toolBarWindowIdKey()));
    g_object_set_data(G_OBJECT(item), detail::menuElementIdKey(), reinterpret_cast<gpointer>(buttonId));
    g_object_set_data(G_OBJECT(item), detail::menuWindowIdKey(), reinterpret_cast<gpointer>(windowId));
    g_signal_connect(item, "clicked", G_CALLBACK(detail::onMenuItemActivate), nullptr);
    gtk_toolbar_insert(GTK_TOOLBAR(toolBarIt->second), item, -1);
    gtk_widget_show_all(GTK_WIDGET(item));
    detail::toolBarButtonWidgets()[toolBarId][buttonId] = GTK_WIDGET(item);
    return 0;
}

/// See `pbToolBarImageButton`'s own doc comment on "Aucune"/current-context.
inline std::int64_t pbToolBarSeparator() {
    auto toolBarIt = detail::toolBarTable().find(detail::activeToolBarId());
    if (toolBarIt == detail::toolBarTable().end()) {
        return 0;
    }
    GtkToolItem* sep = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolBarIt->second), sep, -1);
    gtk_widget_show(GTK_WIDGET(sep));
    return 0;
}

/// Oracle-verified: deliberately crash-proof for any argument - real PB's
/// own docs say so explicitly, the same "IsX created so a bad handle can
/// never crash" family `IsImage`/`IsWindow`/`IsMenu` already belong to.
inline std::int64_t pbIsToolBar(std::int64_t toolBarId) { return detail::toolBarTable().contains(toolBarId) ? 1 : 0; }

/// `#PB_All` (`-1`) frees every remaining toolbar at once - the same
/// established convention as `pbFreeMenu`/`pbFreeStatusBar`/`pbFreeImage`.
/// Oracle-verified no return value ("Aucune") - always returns `0`.
inline std::int64_t pbFreeToolBar(std::int64_t toolBarId) {
    if (toolBarId == -1) {
        for (auto& [id, widget] : detail::toolBarTable()) {
            gtk_widget_destroy(widget);
        }
        detail::toolBarTable().clear();
        detail::toolBarButtonWidgets().clear();
        detail::toolBarIconPixelSize().clear();
        return 0;
    }
    auto it = detail::toolBarTable().find(toolBarId);
    if (it != detail::toolBarTable().end()) {
        gtk_widget_destroy(it->second);
        detail::toolBarTable().erase(it);
        detail::toolBarButtonWidgets().erase(toolBarId);
        detail::toolBarIconPixelSize().erase(toolBarId);
    }
    return 0;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`.
inline std::int64_t pbDisableToolBarButton(std::int64_t toolBarId, std::int64_t button, std::int64_t state) {
    auto barIt = detail::toolBarButtonWidgets().find(toolBarId);
    if (barIt != detail::toolBarButtonWidgets().end()) {
        auto buttonIt = barIt->second.find(button);
        if (buttonIt != barIt->second.end()) {
            gtk_widget_set_sensitive(buttonIt->second, state == 0 ? TRUE : FALSE);
        }
    }
    return 0;
}

/// Oracle-verified: non-zero while a `#PB_ToolBar_Toggle` button is pressed,
/// zero otherwise - simplified to a clean `1`/`0`, the same treatment every
/// other real "non-zero/zero" return in this library already gets.
inline std::int64_t pbGetToolBarButtonState(std::int64_t toolBarId, std::int64_t button) {
    auto barIt = detail::toolBarButtonWidgets().find(toolBarId);
    if (barIt == detail::toolBarButtonWidgets().end()) {
        return 0;
    }
    auto buttonIt = barIt->second.find(button);
    if (buttonIt == barIt->second.end() || !GTK_IS_TOGGLE_TOOL_BUTTON(buttonIt->second)) {
        return 0;
    }
    return gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(buttonIt->second)) != FALSE ? 1 : 0;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`.
inline std::int64_t pbSetToolBarButtonState(std::int64_t toolBarId, std::int64_t button, std::int64_t state) {
    auto barIt = detail::toolBarButtonWidgets().find(toolBarId);
    if (barIt != detail::toolBarButtonWidgets().end()) {
        auto buttonIt = barIt->second.find(button);
        if (buttonIt != barIt->second.end() && GTK_IS_TOGGLE_TOOL_BUTTON(buttonIt->second)) {
            gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(buttonIt->second), state != 0 ? TRUE : FALSE);
        }
    }
    return 0;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`.
inline std::int64_t pbToolBarButtonText(std::int64_t toolBarId, std::int64_t button, const PBString& text) {
    auto barIt = detail::toolBarButtonWidgets().find(toolBarId);
    if (barIt != detail::toolBarButtonWidgets().end()) {
        auto buttonIt = barIt->second.find(button);
        if (buttonIt != barIt->second.end()) {
            gtk_tool_button_set_label(GTK_TOOL_BUTTON(buttonIt->second), text.bytes().c_str());
        }
    }
    return 0;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`. An
/// empty `Texte$` removes the tooltip - oracle-verified via this function's
/// own docs; `gtk_widget_set_tooltip_text` with an empty string already
/// does exactly that.
inline std::int64_t pbToolBarToolTip(std::int64_t toolBarId, std::int64_t button, const PBString& text) {
    auto barIt = detail::toolBarButtonWidgets().find(toolBarId);
    if (barIt != detail::toolBarButtonWidgets().end()) {
        auto buttonIt = barIt->second.find(button);
        if (buttonIt != barIt->second.end()) {
            gtk_widget_set_tooltip_text(buttonIt->second, text.bytes().empty() ? nullptr : text.bytes().c_str());
        }
    }
    return 0;
}

/// See `pbMenuHeight`'s own doc comment for why pending events are drained
/// first - the same freshly-built-widget-not-yet-size-allocated concern
/// applies here too.
inline std::int64_t pbToolBarHeight(std::int64_t toolBarId) {
    auto it = detail::toolBarTable().find(toolBarId);
    if (it == detail::toolBarTable().end()) {
        return 0;
    }
    while (gtk_events_pending() != 0) {
        gtk_main_iteration();
    }
    return gtk_widget_get_allocated_height(it->second);
}

/// Oracle-verified native-handle-ish value, not replicated-for-a-reason the
/// same way `pbMenuID`'s own is (see its doc comment).
inline std::int64_t pbToolBarID(std::int64_t toolBarId) {
    auto it = detail::toolBarTable().find(toolBarId);
    return it != detail::toolBarTable().end() ? reinterpret_cast<std::int64_t>(it->second) : 0;
}

} // namespace easybasic::runtime
