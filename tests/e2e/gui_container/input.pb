; Exercises M7b's ninth GUI slice: ContainerGadget, the first of PB's
; "gadget-list nesting" family (OpenGadgetList/CloseGadgetList - shared
; machinery PanelGadget/ScrollAreaGadget will also need later, once either
; is implemented).
;
; Deliberately avoids anything requiring a simulated click (xdotool) - this
; slice has no click-driven behavior at all (a plain layout container, not
; an interactive gadget), so only creation/nesting/IsGadget/FreeGadget
; return-value round-trips are exercised here, the same deterministic split
; every other GUI slice's own e2e case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug IsGadget(10)
Debug ContainerGadget(10, 10, 10, 250, 250, #PB_Container_Raised)
Debug IsGadget(10)

; Nested container - its own gadgets are relative to it, not the window
; or the outer container (oracle-verified separately; not observable from
; plain Debug output here, since this project has no GadgetX of its own).
ContainerGadget(11, 20, 20, 150, 150, #PB_Container_Single)
ButtonGadget(12, 5, 5, 60, 20, "Deep")
CloseGadgetList() ; back to container 10
ButtonGadget(13, 5, 180, 60, 20, "OuterLevel")
CloseGadgetList() ; back to the window
ButtonGadget(14, 5, 260, 60, 20, "WindowLevel")

Debug IsGadget(11)
Debug IsGadget(12)
Debug IsGadget(13)
Debug IsGadget(14)

; OpenGadgetList reopens a previously-closed container dynamically.
Debug OpenGadgetList(10)
ButtonGadget(15, 100, 100, 60, 20, "Reopened")
CloseGadgetList()
Debug IsGadget(15)

; A non-container (or unknown) #Gadget is a harmless failure.
Debug OpenGadgetList(14)
Debug OpenGadgetList(9999)

; Freeing the outer container recursively frees every gadget nested
; inside it, at any depth - oracle-verified directly.
Debug FreeGadget(10)
Debug IsGadget(10)
Debug IsGadget(11)
Debug IsGadget(12)
Debug IsGadget(13)
Debug IsGadget(15)
Debug IsGadget(14) ; window-level, unaffected

; CloseGadgetList with nothing open is a harmless no-op in this project
; (real PB's own debugger raises a fatal error here instead - a
; deliberate simplification, see pbCloseGadgetList's own doc comment).
Debug CloseGadgetList()
Debug IsGadget(14)
