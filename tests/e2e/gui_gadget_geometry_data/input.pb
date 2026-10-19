; Exercises the Gadget geometry/data/introspection cluster: GadgetX/
; GadgetY/GadgetWidth/GadgetHeight/GadgetID/GadgetType/SetGadgetData/
; GetGadgetData/GetActiveGadget/SetActiveGadget. Oracle-verified entirely
; via plain Debug probes - no synthetic input needed. See pbGadgetX's/
; pbGadgetType's/pbGetActiveGadget's own doc comments in guilib.hpp for
; the full set of findings this is based on: GadgetX/Y/Width/Height
; default to ContainerCoordinate/ActualSize (exactly what the gadget was
; created/ResizeGadget'd with); #PB_Gadget_WindowCoordinate sums every
; ancestor container's own offset up to the window, but (like
; GetActiveGadget below) only reads back correctly once the window has
; processed at least one event; #PB_Gadget_RequiredSize reports a real,
; different (smaller) natural size, not pixel-matched against the
; oracle's own exact value (a GTK-theme-dependent divergence, not
; checked here beyond "it's real and different"); GadgetID is a real
; native handle, not pixel-matched either; GadgetType is oracle-verified
; for every implemented gadget type, including several that share an
; identical underlying GTK widget (ButtonImage/Button, Date/IPAddress/
; ExplorerCombo/Spin/String, Container/Frame, ScrollArea/ListView,
; Editor/ListView, Canvas/Shortcut) told apart only by this project's
; own marker tags/side tables; SetGadgetData/GetGadgetData round-trip a
; plain per-gadget Integer; GetActiveGadget/SetActiveGadget track real
; keyboard focus, across any open window, not scoped to the "current" one.
OpenWindow(0, 10, 10, 320, 320, "Test")

ButtonGadget(1, 20, 30, 100, 40, "Hi")
Debug GadgetX(1)
Debug GadgetY(1)
Debug GadgetWidth(1)
Debug GadgetHeight(1)
ResizeGadget(1, 25, 35, 110, 45)
Debug GadgetX(1)
Debug GadgetWidth(1)
requiredWidth = GadgetWidth(1, #PB_Gadget_RequiredSize)
If requiredWidth > 0 And requiredWidth <> 110
  Debug "required-size-differs"
EndIf
Debug GadgetWidth(1) ; unaffected by the RequiredSize query above

ContainerGadget(2, 50, 60, 200, 150)
ButtonGadget(3, 20, 30, 50, 20, "Nested")
CloseGadgetList()
For i = 1 To 20
  WindowEvent()
Next
Delay(100)
For i = 1 To 20
  WindowEvent()
Next
Debug GadgetX(2, #PB_Gadget_WindowCoordinate)
Debug GadgetX(3, #PB_Gadget_WindowCoordinate)
Debug GadgetY(3, #PB_Gadget_WindowCoordinate)
Debug GadgetX(3) ; ContainerCoordinate stays relative to its own immediate parent

Debug "--- GadgetID/IsGadget ---"
Debug IsGadget(GadgetID(1))

Debug "--- GadgetType ---"
Debug GadgetType(1)
ButtonImageGadget(4, 10, 200, 50, 20, 0)
Debug GadgetType(4)
StringGadget(5, 70, 200, 50, 20, "")
Debug GadgetType(5)
DateGadget(6, 130, 200, 50, 20, "YYYY-MM-DD", 0)
Debug GadgetType(6)
IPAddressGadget(7, 190, 200, 50, 20)
Debug GadgetType(7)
FrameGadget(8, 10, 230, 50, 20, "F")
Debug GadgetType(8)
ScrollAreaGadget(9, 70, 230, 50, 20, 100, 100, 10)
CloseGadgetList()
Debug GadgetType(9)
EditorGadget(10, 130, 230, 50, 20)
Debug GadgetType(10)
CanvasGadget(11, 190, 230, 50, 20)
Debug GadgetType(11)
ShortcutGadget(12, 10, 260, 50, 20, 0)
Debug GadgetType(12)

Debug "--- SetGadgetData/GetGadgetData ---"
Debug GetGadgetData(1)
SetGadgetData(1, 12345)
Debug GetGadgetData(1)
SetGadgetData(1, -99)
Debug GetGadgetData(1)

Debug "--- GetActiveGadget/SetActiveGadget ---"
Debug GetActiveGadget()
SetActiveGadget(5)
For i = 1 To 20
  WindowEvent()
Next
Debug GetActiveGadget()

Debug "done"
