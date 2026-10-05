#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <unistd.h> // getpid() - POSIX-only, the same acceptance guilib.hpp's own driver-side
                    // pkg-config helper already has (see main.cpp's own doc comment); AppIndicator/
                    // the StatusNotifierItem protocol it speaks is itself a Linux-desktop-only
                    // concept with no Windows equivalent, so this doesn't narrow anything further.
#include <unordered_map>

#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>

#include "guilib.hpp" // CreatePopupMenu/CreatePopupImageMenu reuse guilib.hpp's own Menu machinery
                       // directly (detail::menuTable()/menuBuildStack()/etc.) - a popup menu is just
                       // a bare GtkMenu rather than a GtkMenuBar, but every other menu-building
                       // function (MenuTitle/MenuItem/MenuBar/OpenSubMenu/CloseSubMenu/IsMenu/
                       // FreeMenu/...) already operates generically on whatever GtkWidget* is in
                       // menuTable(), so none of that needs to know popup menus exist at all.
#include "pbstring.hpp"

namespace easybasic::runtime {

namespace detail {

/// M7b's seventh GUI slice: `AddSysTrayIcon`/etc. `AppIndicator` (the
/// modern StatusNotifierItem-protocol library real PB's own Linux backend
/// itself uses internally too - confirmed directly, not assumed: an
/// oracle probe misusing `SysTrayIconMenu` surfaced a real
/// `libayatana-appindicator (CRITICAL): app_indicator_set_menu: assertion
/// 'GTK_IS_MENU (menu)' failed` warning) has no in-memory-pixbuf icon
/// support at all, unlike every other image-consuming function in this
/// library (`ToolBarImageButton`, `MenuItem`'s own `ImageID`) - only an
/// icon *name* resolved through an icon theme search path. Each tray
/// icon therefore owns a small scratch directory it writes its own
/// current icon to as a plain PNG file, named uniquely per icon set
/// (`iconCounter`) rather than overwritten in place, so `ChangeSysTrayIcon`
/// can't be served a stale, theme-cached image under an unchanged name.
struct SysTrayHandle {
    AppIndicator* indicator = nullptr;
    std::filesystem::path iconDir;
    int iconCounter = 0;
};

inline std::unordered_map<std::int64_t, SysTrayHandle>& sysTrayTable() {
    static std::unordered_map<std::int64_t, SysTrayHandle> table;
    return table;
}

inline bool writePixbufPng(GdkPixbuf* pixbuf, const std::filesystem::path& path) {
    GError* error = nullptr;
    gboolean ok = gdk_pixbuf_save(pixbuf, path.string().c_str(), "png", &error, nullptr);
    if (error != nullptr) {
        g_error_free(error);
    }
    return ok == TRUE;
}

inline void destroySysTrayHandle(SysTrayHandle& handle) {
    if (handle.indicator != nullptr) {
        g_object_unref(handle.indicator);
    }
    std::error_code ec;
    std::filesystem::remove_all(handle.iconDir, ec); // Best-effort - a failure here isn't worth surfacing.
}

} // namespace detail

/// Oracle-verified functionally identical to `CreateMenu` apart from having
/// no owning window at all: a bare `GtkMenu` rather than a `GtkMenuBar`,
/// with no vbox to pack into. Unlike `CreateMenu`, a popup menu's own
/// build stack is pre-populated with the menu itself (rather than left
/// empty, waiting for `MenuTitle` to populate it) - oracle-verified real
/// usage (`SysTray.pb`'s own idiom, and `CreatePopupImageMenu`'s own docs:
/// "Décommenter [MenuTitle] pour voir la différence") calls `MenuItem`/
/// `MenuBar` directly after `CreatePopupMenu`, with no `MenuTitle` at all -
/// confirmed directly that attempting the same on a *regular* `CreateMenu`
/// (whose own build stack *is* left empty) is a real, oracle-reproduced
/// GTK-level crash, not just an arbitrary design choice being avoided here
/// defensively.
///
/// Tagged with window id `-1` (never a legitimate PB window id) rather
/// than left untagged (which would read back as `0`, a very common real
/// first-window id) - `destroyWindow`'s own per-window menu cleanup scan
/// must never mistake an ownerless popup menu for one belonging to
/// whichever window happens to close with id `0`.
inline std::int64_t pbCreatePopupMenu(std::int64_t menuId) {
    detail::ensureGtkInit();
    GtkWidget* menu = gtk_menu_new();
    auto old = detail::menuTable().find(menuId);
    if (old != detail::menuTable().end()) {
        gtk_widget_destroy(old->second);
    }
    detail::menuTable()[menuId] = menu;
    detail::menuTitleWidgets()[menuId].clear();
    detail::menuItemWidgets()[menuId].clear();
    detail::menuBuildStack()[menuId] = {menu};
    detail::activeMenuId() = menuId;
    g_object_set_data(G_OBJECT(menu), detail::menuWindowIdKey(), reinterpret_cast<gpointer>(static_cast<std::int64_t>(-1)));
    return 1;
}

/// `Options` (`#PB_Menu_NativeImageSize`) doesn't even exist as a constant
/// on this oracle's own Linux build ("Constant not found") - accepted but
/// not acted on, consistent with the same treatment `CreateImageMenu`'s
/// own Windows-only `Options` bit already gets.
inline std::int64_t pbCreatePopupImageMenu(std::int64_t menuId, std::int64_t /*options*/ = 0) {
    return pbCreatePopupMenu(menuId);
}

/// Oracle-verified: non-zero on success - real PB's own return here is a
/// native-handle-ish value too, simplified to a clean `1`/`0` the same way
/// every other creation function in this library already is.
///
/// `WindowID` is accepted (real PB's own documented signature requires it)
/// but entirely unused - `AppIndicator` has no association with any
/// window at all, confirmed directly (the misuse probe above crashed
/// inside `app_indicator_set_menu` with no window-related state involved
/// anywhere). `ImageID` can't be handed to `AppIndicator` directly (see
/// `detail::SysTrayHandle`'s own doc comment) - written out as this
/// icon's own first PNG file instead, with `app_indicator_set_icon_theme_path`
/// pointed at its one-icon-per-tray scratch directory.
inline std::int64_t pbAddSysTrayIcon(std::int64_t trayId, std::int64_t /*windowHandle*/, std::int64_t imageId) {
    detail::ensureGtkInit();
    // `imageId` is `ImageID()`'s own return value - the real `GdkPixbuf*`
    // pointer itself, not a plain `#Image` number to look up in
    // `imageTable()` - see `pbToolBarImageButton`'s own doc comment for
    // the real bug this same confusion caused there, caught by this
    // function failing identically until fixed the same way.
    if (imageId == 0) {
        return 0;
    }
    auto* pixbuf = reinterpret_cast<GdkPixbuf*>(imageId);
    detail::SysTrayHandle handle;
    handle.iconDir = std::filesystem::temp_directory_path() /
                      ("pbcxx_systray_" + std::to_string(::getpid()) + "_" + std::to_string(trayId));
    std::error_code ec;
    std::filesystem::create_directories(handle.iconDir, ec);
    std::filesystem::path iconPath = handle.iconDir / "icon0.png";
    if (!detail::writePixbufPng(pixbuf, iconPath)) {
        return 0;
    }
    // app_indicator_new_with_path's own constructor is deprecated with no
    // replacement offered in this header at all (unlike set_icon/
    // set_attention_icon, which point to their own _full variants) -
    // still the only way to construct one, so suppressed rather than
    // worked around with nothing to work around it with.
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    handle.indicator = app_indicator_new_with_path(("pbcxx-systray-" + std::to_string(trayId)).c_str(), "icon0",
                                                     APP_INDICATOR_CATEGORY_APPLICATION_STATUS,
                                                     handle.iconDir.string().c_str());
    G_GNUC_END_IGNORE_DEPRECATIONS
    // A freshly constructed indicator starts PASSIVE (invisible) - its own
    // header doc comment says so explicitly.
    app_indicator_set_status(handle.indicator, APP_INDICATOR_STATUS_ACTIVE);
    auto old = detail::sysTrayTable().find(trayId);
    if (old != detail::sysTrayTable().end()) {
        detail::destroySysTrayHandle(old->second);
    }
    detail::sysTrayTable()[trayId] = std::move(handle);
    return 1;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`, the
/// same established convention as `pbKillThread`'s own (see its doc
/// comment). A fresh PNG file under a *new* name each call (see
/// `detail::SysTrayHandle`'s own doc comment) - `app_indicator_set_icon_full`
/// is called with that new name, and the previous file is removed only
/// *after* the new one is live, avoiding even a brief window where the
/// icon name on disk doesn't exist.
inline std::int64_t pbChangeSysTrayIcon(std::int64_t trayId, std::int64_t imageId) {
    auto trayIt = detail::sysTrayTable().find(trayId);
    if (trayIt == detail::sysTrayTable().end() || imageId == 0) {
        return 0;
    }
    auto* pixbuf = reinterpret_cast<GdkPixbuf*>(imageId); // See pbAddSysTrayIcon's own doc comment.
    detail::SysTrayHandle& handle = trayIt->second;
    int newIndex = ++handle.iconCounter;
    std::string newName = "icon" + std::to_string(newIndex);
    if (!detail::writePixbufPng(pixbuf, handle.iconDir / (newName + ".png"))) {
        return 0;
    }
    app_indicator_set_icon_full(handle.indicator, newName.c_str(), "");
    std::error_code ec;
    std::filesystem::remove(handle.iconDir / ("icon" + std::to_string(newIndex - 1) + ".png"), ec);
    return 0;
}

/// Oracle-verified: deliberately crash-proof for any argument - real PB's
/// own docs say so explicitly, the same "IsX created so a bad handle can
/// never crash" family `IsImage`/`IsWindow`/`IsToolBar` already belong to.
inline std::int64_t pbIsSysTrayIcon(std::int64_t trayId) { return detail::sysTrayTable().contains(trayId) ? 1 : 0; }

/// `#PB_All` (`-1`) frees every remaining tray icon at once - the same
/// established convention as `pbFreeMenu`/`pbFreeToolBar`/`pbFreeImage`.
/// Oracle-verified no return value ("Aucune") - always returns `0`.
inline std::int64_t pbRemoveSysTrayIcon(std::int64_t trayId) {
    if (trayId == -1) {
        for (auto& [id, handle] : detail::sysTrayTable()) {
            detail::destroySysTrayHandle(handle);
        }
        detail::sysTrayTable().clear();
        return 0;
    }
    auto it = detail::sysTrayTable().find(trayId);
    if (it != detail::sysTrayTable().end()) {
        detail::destroySysTrayHandle(it->second);
        detail::sysTrayTable().erase(it);
    }
    return 0;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`.
/// `MenuID` is the real `GtkWidget*` `pbMenuID`/`pbCreatePopupMenu` already
/// established (the handle already *is* the real pointer) - reinterpreted
/// straight back and handed to `app_indicator_set_menu`, which is why
/// real PB's own docs require that menu to have been built via
/// `CreatePopupMenu`/`CreatePopupImageMenu` (a bare `GtkMenu`) rather than
/// `CreateMenu` (a `GtkMenuBar`, not a `GtkMenu` at all, confirmed
/// directly to fail `app_indicator_set_menu`'s own `GTK_IS_MENU`
/// assertion if attempted anyway).
inline std::int64_t pbSysTrayIconMenu(std::int64_t trayId, std::int64_t menuHandle) {
    auto it = detail::sysTrayTable().find(trayId);
    if (it == detail::sysTrayTable().end()) {
        return 0;
    }
    app_indicator_set_menu(it->second.indicator, GTK_MENU(reinterpret_cast<GtkWidget*>(menuHandle)));
    return 0;
}

/// Oracle-verified no return value ("Aucune") - always returns `0`.
/// `AppIndicator` has no hover-tooltip concept at all (confirmed via its
/// own GIR-embedded documentation comment: `app_indicator_set_title`'s own
/// text describes it as "how [the indicator] should be referred in a
/// human readable form", shown in places like Unity's HUD, not as a
/// mouse-hover bubble) - used anyway as the closest available
/// approximation, a deliberate, documented divergence rather than a
/// silent no-op, since it's the one piece of descriptive text `AppIndicator`
/// does expose per-icon.
inline std::int64_t pbSysTrayIconToolTip(std::int64_t trayId, const PBString& text) {
    auto it = detail::sysTrayTable().find(trayId);
    if (it == detail::sysTrayTable().end()) {
        return 0;
    }
    app_indicator_set_title(it->second.indicator, text.bytes().c_str());
    return 0;
}

} // namespace easybasic::runtime
