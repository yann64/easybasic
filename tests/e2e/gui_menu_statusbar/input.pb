; Exercises M7b's fourth GUI slice: Menu (CreateMenu/MenuTitle/MenuItem/
; MenuBar/OpenSubMenu/CloseSubMenu + generic menu management - IsMenu/
; FreeMenu/HideMenu/DisableMenuItem - and the state/text accessors -
; GetMenuItemState/SetMenuItemState/GetMenuItemText/SetMenuItemText/
; GetMenuTitleText/SetMenuTitleText/MenuHeight/MenuID/WindowID) and
; StatusBar (CreateStatusBar/AddStatusBarField/StatusBarText + IsStatusBar/
; FreeStatusBar/StatusBarHeight/StatusBarID). Deliberately avoids anything
; requiring a simulated click (xdotool) - that's covered by the Catch2 unit
; tests instead, via direct GTK signal emission/driving (gtk_menu_item_
; activate), the same split the gui_basic_gadgets case already uses for
; gadget clicks. Only return-value and accessor round-trips here, so this
; stays deterministic. Run via run_gui_case.sh (not the plain run_case.sh) -
; see its own comments for why.
OpenWindow(1, 10, 10, 220, 150, "Test")

CreateMenu(1, WindowID(1))
MenuTitle("File")
MenuItem(10, "Open")
MenuBar()
OpenSubMenu("Recent")
  MenuItem(11, "A")
CloseSubMenu()
MenuItem(12, "Quit")
MenuTitle("Edit")
MenuItem(13, "Cut")

Debug IsMenu(1)
Debug IsMenu(999)

Debug GetMenuItemText(1, 10)
SetMenuItemText(1, 10, "Renamed")
Debug GetMenuItemText(1, 10)

Debug GetMenuTitleText(1, 0)
Debug GetMenuTitleText(1, 1)
SetMenuTitleText(1, 0, "Fichier")
Debug GetMenuTitleText(1, 0)

Debug GetMenuItemState(1, 10)
SetMenuItemState(1, 10, 1)
Debug GetMenuItemState(1, 10)
SetMenuItemState(1, 10, 0)
Debug GetMenuItemState(1, 10)

Debug DisableMenuItem(1, 10, 1)
Debug DisableMenuItem(1, 10, 0)
Debug HideMenu(1, 1)
Debug HideMenu(1, 0)

If MenuHeight() > 0
  Debug "MenuHeight positive"
EndIf

If MenuID(1) <> 0
  Debug "MenuID nonzero"
EndIf

CreateStatusBar(1, WindowID(1))
AddStatusBarField(100)
AddStatusBarField(#PB_Ignore)

Debug IsStatusBar(1)
Debug IsStatusBar(999)
Debug StatusBarText(1, 0, "Area 1")
Debug StatusBarText(1, 1, "Area 2", #PB_StatusBar_Right)
Debug StatusBarText(1, 2, "No such field")

If StatusBarHeight(1) > 0
  Debug "StatusBarHeight positive"
EndIf

If StatusBarID(1) <> 0
  Debug "StatusBarID nonzero"
EndIf

Debug FreeStatusBar(1)
Debug IsStatusBar(1)

Debug FreeMenu(1)
Debug IsMenu(1)

CloseWindow(1)
Debug IsMenu(999)
