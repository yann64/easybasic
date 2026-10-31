; Exercises GetGadgetItemColor/SetGadgetItemColor/SetGadgetItemImage.
; Oracle-verified entirely via plain Debug probes - see guilib.hpp's own
; pbGetGadgetItemColor/pbSetGadgetItemColor/pbSetGadgetItemImage doc
; comments for the full set of findings this is based on:
; GetGadgetItemColor/SetGadgetItemColor are scoped to ListIconGadget/
; TreeGadget (ListViewGadget/ComboBoxGadget silently accept the calls
; with no effect at all); colors are real, per-item, and follow the row
; itself across RemoveGadgetItem/ClearGadgetItems rather than staying
; pinned to a numeric position. `Column` is accepted but, in this
; implementation, every column of a row shares that row's own single
; front/back slot (a documented, narrower simplification than real PB's
; own genuine per-column overrides) - this test only exercises the
; always-omitted-Column shape that both share, so it's byte-comparable
; against the oracle either way. SetGadgetItemImage has no Get
; counterpart in real PB at all, so it's checked only for its own
; return value, never for a displayed effect.
Debug "--- ListIconGadget ---"
OpenWindow(0, 10, 10, 320, 320, "Test")
ListIconGadget(1, 10, 10, 200, 100, "Col0", 100)
AddGadgetItem(1, 0, "Row0")
AddGadgetItem(1, 1, "Row1")
Debug GetGadgetItemColor(1, 0, #PB_Gadget_FrontColor)
SetGadgetItemColor(1, 0, #PB_Gadget_FrontColor, RGB(255, 0, 0))
Debug GetGadgetItemColor(1, 0, #PB_Gadget_FrontColor)
Debug GetGadgetItemColor(1, 1, #PB_Gadget_FrontColor)
Debug GetGadgetItemColor(1, 0, #PB_Gadget_BackColor)
SetGadgetItemColor(1, 0, #PB_Gadget_BackColor, RGB(0, 0, 255))
Debug GetGadgetItemColor(1, 0, #PB_Gadget_BackColor)
Debug GetGadgetItemColor(1, 999, #PB_Gadget_FrontColor)

Debug "--- TreeGadget ---"
TreeGadget(2, 10, 120, 200, 100)
AddGadgetItem(2, -1, "A")
AddGadgetItem(2, -1, "B")
SetGadgetItemColor(2, 0, #PB_Gadget_FrontColor, RGB(10, 20, 30))
Debug GetGadgetItemColor(2, 0, #PB_Gadget_FrontColor)
Debug GetGadgetItemColor(2, 1, #PB_Gadget_FrontColor)

Debug "--- ListViewGadget/ComboBoxGadget: no effect ---"
ListViewGadget(3, 10, 230, 200, 30)
AddGadgetItem(3, 0, "X")
SetGadgetItemColor(3, 0, #PB_Gadget_FrontColor, RGB(255, 0, 0))
Debug GetGadgetItemColor(3, 0, #PB_Gadget_FrontColor)

Debug "--- colors follow the row, not a fixed position ---"
SetGadgetItemColor(1, 1, #PB_Gadget_FrontColor, RGB(0, 255, 0))
RemoveGadgetItem(1, 0) ; removes the red Row0; the green Row1 shifts down to position 0
Debug GetGadgetItemColor(1, 0, #PB_Gadget_FrontColor)
ClearGadgetItems(1)
AddGadgetItem(1, 0, "Fresh")
Debug GetGadgetItemColor(1, 0, #PB_Gadget_FrontColor)

Debug "--- SetGadgetItemImage ---"
CreateImage(0, 16, 16)
Debug SetGadgetItemImage(1, 0, ImageID(0))
Debug SetGadgetItemImage(2, 0, ImageID(0))
Debug SetGadgetItemImage(1, 0, 0)

Debug "done"
