; Exercises M7b's twelfth GUI slice: ScrollAreaGadget - back to the
; gadget-list nesting family (ContainerGadget/PanelGadget's own ninth/
; tenth slices), auto-capturing subsequently created gadgets immediately
; on creation, like Container (not Panel, which needs AddGadgetItem
; first). Also exercises GetGadgetAttribute/SetGadgetAttribute's own
; five ScrollArea attributes (InnerWidth/InnerHeight/X/Y/ScrollStep),
; reusing the same two generic, dispatch-based functions the Splitter
; slice introduced.
;
; Deliberately avoids anything requiring a simulated click (xdotool) -
; the scroll event this gadget documents firing on a real scrollbar drag
; is covered by a Catch2 unit test instead (driving GtkAdjustment's own
; "value-changed" signal directly), the same deterministic split every
; other GUI slice's own e2e case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug IsGadget(10)
Debug ScrollAreaGadget(10, 10, 10, 200, 150, 500, 500, 20)
ButtonGadget(11, 5, 5, 60, 20, "InScroll")

; A nested Container inside the ScrollArea - its own gadgets are placed
; into it, not directly into the ScrollArea.
ContainerGadget(12, 5, 30, 150, 100)
ButtonGadget(13, 5, 5, 60, 20, "Deep")
CloseGadgetList() ; closes container 12
ButtonGadget(14, 5, 140, 60, 20, "BackInScroll")
CloseGadgetList() ; closes the ScrollArea
ButtonGadget(15, 5, 300, 60, 20, "WindowLevel")

Debug IsGadget(11)
Debug IsGadget(12)
Debug IsGadget(13)
Debug IsGadget(14)
Debug IsGadget(15)

Debug GetGadgetAttribute(10, #PB_ScrollArea_InnerWidth)
Debug GetGadgetAttribute(10, #PB_ScrollArea_InnerHeight)
Debug GetGadgetAttribute(10, #PB_ScrollArea_X)
Debug GetGadgetAttribute(10, #PB_ScrollArea_Y)
Debug GetGadgetAttribute(10, #PB_ScrollArea_ScrollStep)

SetGadgetAttribute(10, #PB_ScrollArea_InnerWidth, 800)
Debug GetGadgetAttribute(10, #PB_ScrollArea_InnerWidth)
SetGadgetAttribute(10, #PB_ScrollArea_InnerHeight, 600)
Debug GetGadgetAttribute(10, #PB_ScrollArea_InnerHeight)
SetGadgetAttribute(10, #PB_ScrollArea_ScrollStep, 99)
Debug GetGadgetAttribute(10, #PB_ScrollArea_ScrollStep)

; OpenGadgetList reopens a ScrollArea dynamically, reusing the exact
; same mechanism Container's own ninth slice already established.
OpenGadgetList(10)
ButtonGadget(16, 100, 100, 60, 20, "Reopened")
CloseGadgetList()
Debug IsGadget(16)

; Harmless failures: an unsupported attribute, a non-ScrollArea gadget.
; A genuinely unknown #Gadget is instead a real, fatal debugger error in
; real PB itself (not replicated by this project - see
; pbGetGadgetAttribute's own doc comment, shared with the Splitter
; slice - and left out of this golden case for the same reason: it's
; fatal in the oracle).
Debug GetGadgetAttribute(10, 99)
Debug GetGadgetAttribute(11, #PB_ScrollArea_InnerWidth)

; Freeing the ScrollArea recursively frees every gadget nested inside
; it, at any depth - the window-level button is unaffected.
Debug FreeGadget(10)
Debug IsGadget(10)
Debug IsGadget(11)
Debug IsGadget(12)
Debug IsGadget(13)
Debug IsGadget(14)
Debug IsGadget(16)
Debug IsGadget(15)
