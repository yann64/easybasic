; Exercises M7b's sixth GUI slice: ToolBar (CreateToolBar/ToolBarImageButton/
; ToolBarSeparator/IsToolBar/FreeToolBar/DisableToolBarButton/
; Get-SetToolBarButtonState/ToolBarButtonText/ToolBarToolTip/ToolBarHeight/
; ToolBarID). test.bmp (shared with gui_image's own test - see
; run_gui_case.sh's "extra fixture files" copy step) stands in for a real
; PB example toolbar icon.
;
; Deliberately avoids anything requiring a simulated click (xdotool) - a
; real toolbar button click is covered by the Catch2 unit tests instead
; (driving the "clicked" signal directly), the same split every other GUI
; slice's own test already uses. Only return-value and accessor round-trips
; here, so this stays deterministic.
OpenWindow(1, 10, 10, 220, 120, "Test")
CreateMenu(1, WindowID(1)) ; So the toolbar-below-menu ordering is also exercised here, not just in the unit tests.

LoadImage(0, "test.bmp")

Debug IsToolBar(1)
Debug CreateToolBar(1, WindowID(1), #PB_ToolBar_Text)
Debug IsToolBar(1)
Debug IsToolBar(999)

ToolBarImageButton(10, ImageID(0), #PB_ToolBar_Normal, "Open")
ToolBarSeparator()
ToolBarImageButton(11, ImageID(0), #PB_ToolBar_Toggle)

Debug GetToolBarButtonState(1, 11)
SetToolBarButtonState(1, 11, 1)
Debug GetToolBarButtonState(1, 11)
SetToolBarButtonState(1, 11, 0)
Debug GetToolBarButtonState(1, 11)

DisableToolBarButton(1, 10, 1)
DisableToolBarButton(1, 10, 0)
ToolBarButtonText(1, 10, "Renamed")
ToolBarToolTip(1, 10, "A tip")

If ToolBarHeight(1) > 0
  Debug "ToolBarHeight positive"
EndIf

If ToolBarID(1) <> 0
  Debug "ToolBarID nonzero"
EndIf

Debug FreeToolBar(1)
Debug IsToolBar(1)

CloseWindow(1)
Debug IsToolBar(999)
