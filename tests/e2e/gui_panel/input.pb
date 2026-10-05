; Exercises M7b's tenth GUI slice: PanelGadget, the second of PB's
; "gadget-list nesting" family (ContainerGadget, the ninth slice, was the
; first) - AddGadgetItem's own "replace, not push" semantics on the same
; Panel, a nested Panel inside another Panel's own tab (replicating real
; PureBasic's own flagship PanelGadget.html example structure), item
; management (CountGadgetItems/RemoveGadgetItem/ClearGadgetItems/
; GetGadgetItemText/SetGadgetItemText), and GetGadgetState/SetGadgetState
; as the active-tab accessor.
;
; Deliberately avoids anything requiring a simulated click (xdotool) -
; #PB_EventType_Change is covered by the Catch2 unit tests instead
; (driving GtkNotebook's own "switch-page" signal directly), the same
; split every other GUI slice's own e2e case already uses. Only return-
; value and accessor round-trips here, so this stays deterministic.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug IsGadget(10)
Debug PanelGadget(10, 10, 10, 280, 280)
Debug CountGadgetItems(10)

Debug AddGadgetItem(10, -1, "Tab1")
Debug CountGadgetItems(10)
ButtonGadget(11, 5, 5, 60, 20, "InTab1")

; A nested Panel inside Tab1 - its own AddGadgetItem pushes a genuinely
; new frame (a *different* #Gadget), unlike the same-Panel case below.
PanelGadget(12, 5, 30, 260, 150)
AddGadgetItem(12, -1, "Inner1")
ButtonGadget(13, 5, 5, 60, 20, "Deep")
CloseGadgetList() ; closes the inner Panel 12

; A second AddGadgetItem on the *same* outer Panel 10, no CloseGadgetList
; since the first one - replaces, not pushes, the current frame.
Debug AddGadgetItem(10, -1, "Tab2")
Debug CountGadgetItems(10)
ButtonGadget(14, 5, 5, 60, 20, "InTab2")

CloseGadgetList() ; one call fully exits Panel 10, back to window level
ButtonGadget(15, 5, 290, 60, 20, "WindowLevel")

Debug GetGadgetState(10)
SetGadgetState(10, 1)
Debug GetGadgetState(10)

Debug GetGadgetItemText(10, 0)
Debug GetGadgetItemText(10, 1)
SetGadgetItemText(10, 1, "Tab2Renamed")
Debug GetGadgetItemText(10, 1)

; OpenGadgetList reopens an existing tab by its own Element index, rather
; than creating a new one.
OpenGadgetList(10, 0)
ButtonGadget(16, 100, 100, 60, 20, "ReopenedInTab1")
CloseGadgetList()
Debug CountGadgetItems(10)

Debug IsGadget(11)
Debug IsGadget(12)
Debug IsGadget(13)
Debug IsGadget(14)
Debug IsGadget(16)

; RemoveGadgetItem frees every gadget nested in that one tab, at any
; depth, and re-indexes the rest.
RemoveGadgetItem(10, 0)
Debug CountGadgetItems(10)
Debug IsGadget(11)
Debug IsGadget(12)
Debug IsGadget(13)
Debug IsGadget(16)
Debug IsGadget(14) ; the surviving tab's own gadget, unaffected
Debug GetGadgetItemText(10, 0)

; ClearGadgetItems frees every remaining gadget across every tab.
ClearGadgetItems(10)
Debug CountGadgetItems(10)
Debug IsGadget(14)

; Freeing the Panel itself is a harmless no-op by this point (already
; empty) - IsGadget(15), the window-level button, is unaffected either
; way.
Debug FreeGadget(10)
Debug IsGadget(10)
Debug IsGadget(15)
