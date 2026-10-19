; Exercises SetGadgetColor/GetGadgetColor/GadgetToolTip/
; DisableGadgetRedraw. Oracle-verified entirely via plain Debug probes
; - no synthetic input needed. See pbSetGadgetColor's/pbGetGadgetColor's
; own doc comments in guilib.hpp for the full set of findings this is
; based on: support is genuinely gadget-type-specific, not universal -
; a "fixed" gadget's own SetGadgetColor silently succeeds (no error)
; but has no effect at all, GetGadgetColor always reporting the same
; fixed default; "settable" gadgets (TextGadget/ContainerGadget/
; SpinGadget/ListView family/ExplorerList/ExplorerTree) give a real,
; persisting round-trip; CanvasGadget only supports BackColor, not
; FrontColor. ScrollAreaGadget's own real GetGadgetColor is genuinely
; non-deterministic garbage in real PB itself (a different huge number
; every run) and HyperLinkGadget's own real FrontColor round-trip is
; off by one (254, not the 255 given) - neither is included here, to
; keep this a byte-exact golden comparison (both are covered, and the
; divergence explained, in the unit test instead).
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug "--- fixed gadgets: Set silently succeeds, Get never reflects it ---"
ButtonGadget(1, 10, 10, 50, 20, "B")
Debug GetGadgetColor(1, #PB_Gadget_FrontColor)
Debug SetGadgetColor(1, #PB_Gadget_FrontColor, RGB(255, 0, 0))
Debug GetGadgetColor(1, #PB_Gadget_FrontColor)

StringGadget(2, 10, 40, 50, 20, "")
Debug GetGadgetColor(2, #PB_Gadget_FrontColor)
Debug GetGadgetColor(2, #PB_Gadget_BackColor)
SetGadgetColor(2, #PB_Gadget_FrontColor, RGB(255, 0, 0))
SetGadgetColor(2, #PB_Gadget_BackColor, RGB(0, 0, 255))
Debug GetGadgetColor(2, #PB_Gadget_FrontColor)
Debug GetGadgetColor(2, #PB_Gadget_BackColor)

Debug "--- settable gadgets: a real, persisting round-trip ---"
TextGadget(3, 10, 70, 50, 20, "T")
Debug GetGadgetColor(3, #PB_Gadget_FrontColor)
SetGadgetColor(3, #PB_Gadget_FrontColor, RGB(255, 0, 0))
Debug GetGadgetColor(3, #PB_Gadget_FrontColor)
Debug GetGadgetColor(3, #PB_Gadget_BackColor)
SetGadgetColor(3, #PB_Gadget_BackColor, RGB(0, 0, 255))
Debug GetGadgetColor(3, #PB_Gadget_BackColor)

ListIconGadget(4, 10, 100, 50, 20, "", 50)
Debug GetGadgetColor(4, #PB_Gadget_FrontColor)
Debug GetGadgetColor(4, #PB_Gadget_BackColor)
SetGadgetColor(4, #PB_Gadget_FrontColor, RGB(255, 0, 0))
SetGadgetColor(4, #PB_Gadget_BackColor, RGB(0, 0, 255))
Debug GetGadgetColor(4, #PB_Gadget_FrontColor)
Debug GetGadgetColor(4, #PB_Gadget_BackColor)

Debug "--- CanvasGadget: BackColor only ---"
CanvasGadget(5, 10, 130, 50, 20)
SetGadgetColor(5, #PB_Gadget_FrontColor, RGB(255, 0, 0))
Debug GetGadgetColor(5, #PB_Gadget_FrontColor)
SetGadgetColor(5, #PB_Gadget_BackColor, RGB(0, 0, 255))
Debug GetGadgetColor(5, #PB_Gadget_BackColor)

Debug "--- GadgetToolTip/DisableGadgetRedraw ---"
Debug GadgetToolTip(1, "hello")
Debug DisableGadgetRedraw(1, 1)
Debug DisableGadgetRedraw(1, 0)

Debug "done"
