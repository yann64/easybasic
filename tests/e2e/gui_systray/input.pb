; Exercises M7b's seventh GUI slice: SysTrayIcon (AddSysTrayIcon/
; ChangeSysTrayIcon/IsSysTrayIcon/RemoveSysTrayIcon/SysTrayIconMenu/
; SysTrayIconToolTip) plus its own hard prerequisite, CreatePopupMenu/
; CreatePopupImageMenu (oracle-verified SysTrayIconMenu requires a popup
; menu, not a plain CreateMenu one). test.bmp is shared with gui_image's
; own test (see run_gui_case.sh's "extra fixture files" copy step).
;
; Run as a golden fixture test (fixed expected.stdout), not an oracle diff,
; the same split every GUI slice since the fourth one already uses - real
; AddSysTrayIcon/CreatePopupMenu return a native-handle-ish value (not a
; clean 1), simplified here the same way every other creation function in
; this library already is, so a literal stdout diff would fail even on
; fully correct behavior.
OpenWindow(0, 0, 0, 10, 10, "", #PB_Window_Invisible)
LoadImage(0, "test.bmp")

Debug IsSysTrayIcon(0)
Debug AddSysTrayIcon(0, WindowID(0), ImageID(0))
Debug IsSysTrayIcon(0)
Debug IsSysTrayIcon(999)

Debug IsMenu(1)
Debug CreatePopupMenu(1)
Debug IsMenu(1)
; Oracle-verified: a popup menu's own MenuItem/MenuBar work immediately,
; with no MenuTitle needed first (unlike a regular CreateMenu'd menu bar).
MenuItem(10, "About")
MenuBar()
MenuItem(11, "Quit")
Debug GetMenuItemText(1, 10)

Debug SysTrayIconMenu(0, MenuID(1))
Debug "menu associated"

Debug SysTrayIconToolTip(0, "Hello")
Debug "tooltip set"

Debug ChangeSysTrayIcon(0, ImageID(0))
Debug "icon changed"

Debug RemoveSysTrayIcon(0)
Debug IsSysTrayIcon(0)

FreeMenu(1)
CloseWindow(0)
