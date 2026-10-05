; Exercises M7b's fourteenth GUI slice: ListIconGadget/TreeGadget.
;
; ListIconGadget extends the "universal item" family (AddGadgetItem/
; CountGadgetItems/RemoveGadgetItem/ClearGadgetItems/GetGadgetItemText/
; SetGadgetItemText/GetGadgetItemState/SetGadgetItemState/GetGadgetItemData/
; SetGadgetItemData) ListViewGadget/ComboBoxGadget already joined, plus its
; own multi-column mechanics (AddGadgetColumn/RemoveGadgetColumn,
; Chr(10)-separated AddGadgetItem text, Element=-1 column-header
; addressing) and GetGadgetAttribute's own ColumnCount/ClickedColumn.
;
; TreeGadget reuses the same item-function family, translated through its
; own flat, depth-first Element addressing - AddGadgetItem's own Options
; parameter (the new item's own level) is always required for it - plus
; the brand new GetGadgetItemAttribute/SetGadgetItemAttribute pair, scoped
; here to TreeGadget's own SubLevel.
;
; Deliberately avoids anything requiring a simulated click (xdotool) -
; LeftClick/LeftDoubleClick/RightClick/ColumnClick/checkbox-toggle events
; are covered by Catch2 unit tests instead (driving the real GTK signals
; directly), the same deterministic split every other GUI slice's own e2e
; case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug ListIconGadget(0, 10, 10, 250, 120, "Name", 100)
AddGadgetColumn(0, 1, "Age", 60)
For a = 1 To 3
  AddGadgetItem(0, -1, "Person " + Str(a) + Chr(10) + Str(20 + a))
Next
Debug CountGadgetItems(0)
Debug GetGadgetItemText(0, 0, 0)
Debug GetGadgetItemText(0, 0, 1)
Debug GetGadgetItemText(0, -1, 0) ; column header title
Debug GetGadgetItemText(0, -1, 1)

SetGadgetItemText(0, 1, "Changed", 0)
Debug GetGadgetItemText(0, 1, 0)
SetGadgetItemText(0, -1, "NameHdr", 0) ; rename header
Debug GetGadgetItemText(0, -1, 0)

Debug GetGadgetState(0) ; nothing selected yet
SetGadgetState(0, 1)
Debug GetGadgetState(0)
Debug GetGadgetText(0) ; first column of the selected row

Debug GetGadgetAttribute(0, #PB_ListIcon_ColumnCount)

SetGadgetItemData(0, 0, 555)
Debug GetGadgetItemData(0, 0)
Debug GetGadgetItemData(0, 1) ; untouched, defaults to 0

Debug GetGadgetItemState(0, 1) ; #PB_ListIcon_Selected
SetGadgetItemState(0, 1, 0) ; deselect
Debug GetGadgetItemState(0, 1)

RemoveGadgetColumn(0, 1)
Debug GetGadgetAttribute(0, #PB_ListIcon_ColumnCount)

RemoveGadgetItem(0, 0)
Debug CountGadgetItems(0)
ClearGadgetItems(0)
Debug CountGadgetItems(0)

Debug TreeGadget(1, 10, 140, 250, 120)
AddGadgetItem(1, -1, "Root0", 0, 0)
AddGadgetItem(1, -1, "Root1", 0, 0)
AddGadgetItem(1, -1, "Child1.0", 0, 1)
AddGadgetItem(1, -1, "Child1.1", 0, 1)
AddGadgetItem(1, -1, "Root2", 0, 0)
Debug CountGadgetItems(1) ; every depth, not just root-level
Debug GetGadgetItemText(1, 2) ; flat depth-first Element addressing
Debug GetGadgetItemAttribute(1, 2, #PB_Tree_SubLevel)
Debug GetGadgetItemAttribute(1, 0, #PB_Tree_SubLevel)

SetGadgetState(1, 2) ; a level-1 child, under a collapsed parent
Debug GetGadgetState(1)
Debug GetGadgetText(1)

; Oracle-verified: unlike ListView/ComboBox, SetGadgetText on a Tree
; renames whichever element is *currently selected* - it doesn't search
; for a matching item at all.
SetGadgetText(1, "RenamedRoot")
Debug GetGadgetState(1)
Debug GetGadgetItemText(1, 2)

Debug GetGadgetItemState(1, 0)
SetGadgetItemState(1, 0, #PB_Tree_Selected)
Debug GetGadgetItemState(1, 0)

SetGadgetItemData(1, 0, 42)
Debug GetGadgetItemData(1, 0)

RemoveGadgetItem(1, 2) ; removes Child1.0 (and any descendants)
Debug CountGadgetItems(1)

ClearGadgetItems(1)
Debug CountGadgetItems(1)

; Harmless failures: ListIcon/Tree-only functions on an unrelated gadget
; type.
ButtonGadget(2, 10, 270, 60, 20, "B")
Debug AddGadgetColumn(2, 0, "x", 10)
Debug RemoveGadgetColumn(2, 0)
Debug GetGadgetItemAttribute(2, 0, #PB_Tree_SubLevel)
Debug SetGadgetItemAttribute(2, 0, 1, 1)
