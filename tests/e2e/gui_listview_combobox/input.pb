; Exercises M7b's thirteenth GUI slice: ListViewGadget/ComboBoxGadget -
; the first "universal item" gadget types besides PanelGadget itself,
; sharing AddGadgetItem/CountGadgetItems/RemoveGadgetItem/ClearGadgetItems/
; GetGadgetItemText/SetGadgetItemText with it, plus two brand new
; item-level functions this slice introduces (GetGadgetItemState/
; SetGadgetItemState, GetGadgetItemData/SetGadgetItemData).
;
; Deliberately avoids anything requiring a simulated click (xdotool) -
; the LeftClick/LeftDoubleClick/RightClick (ListView) and Change/Focus/
; LostFocus (ComboBox) events are covered by Catch2 unit tests instead
; (driving the real GTK signals directly), the same deterministic split
; every other GUI slice's own e2e case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug ListViewGadget(0, 10, 10, 250, 120)
For a = 1 To 5
  AddGadgetItem(0, -1, "Item " + Str(a))
Next
Debug CountGadgetItems(0)
Debug GetGadgetState(0) ; nothing selected yet

SetGadgetState(0, 2)
Debug GetGadgetState(0)
Debug GetGadgetText(0)

; Oracle-verified: SetGadgetText selects whichever item's own text
; exactly matches, or does nothing at all if none does.
SetGadgetText(0, "Item 4")
Debug GetGadgetState(0)
SetGadgetText(0, "No such item")
Debug GetGadgetState(0) ; unchanged

Debug GetGadgetItemState(0, 3)
; Oracle-verified: single-select mode already enforces exclusivity -
; selecting a different item deselects the previous one automatically.
SetGadgetItemState(0, 1, 1)
Debug GetGadgetItemState(0, 1)
Debug GetGadgetItemState(0, 3)

SetGadgetState(0, -1) ; deselect all
Debug GetGadgetState(0)

SetGadgetItemData(0, 0, 777)
Debug GetGadgetItemData(0, 0)
Debug GetGadgetItemData(0, 1) ; untouched, defaults to 0

RemoveGadgetItem(0, 0)
Debug CountGadgetItems(0)
Debug GetGadgetItemText(0, 0) ; re-indexed

ClearGadgetItems(0)
Debug CountGadgetItems(0)

Debug ComboBoxGadget(1, 10, 140, 250, 21)
For a = 1 To 5
  AddGadgetItem(1, -1, "Item " + Str(a))
Next
Debug CountGadgetItems(1)
Debug GetGadgetState(1)
Debug GetGadgetText(1)

SetGadgetState(1, 2)
Debug GetGadgetState(1)
Debug GetGadgetText(1)

SetGadgetText(1, "Item 4")
Debug GetGadgetState(1)
SetGadgetText(1, "No such item")
Debug GetGadgetState(1) ; unchanged

RemoveGadgetItem(1, 0)
Debug CountGadgetItems(1)
Debug GetGadgetItemText(1, 0)
ClearGadgetItems(1)
Debug CountGadgetItems(1)

; An editable combo accepts arbitrary text that matches no item at all.
ComboBoxGadget(2, 10, 170, 250, 21, #PB_ComboBox_Editable)
AddGadgetItem(2, -1, "Editable item")
SetGadgetText(2, "Something typed")
Debug GetGadgetText(2)
Debug GetGadgetState(2)

; Harmless failures: ListView/ComboBox-only functions on an unrelated
; gadget type.
ButtonGadget(3, 10, 200, 60, 20, "B")
Debug GetGadgetItemState(3, 0)
Debug SetGadgetItemState(3, 0, 1)
Debug GetGadgetItemData(3, 0)
Debug AddGadgetItem(3, -1, "x")
Debug CountGadgetItems(3)
