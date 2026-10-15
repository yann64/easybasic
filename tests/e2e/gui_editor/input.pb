; Exercises M7b's nineteenth GUI slice: EditorGadget.
;
; GetGadgetText/SetGadgetText address the buffer's own full, Chr(10)-joined
; multi-line text (oracle-verified directly); GetGadgetState is always 0
; (not a stateful gadget, same as HyperLinkGadget's own). AddGadgetItem/
; CountGadgetItems/ClearGadgetItems/GetGadgetItemText all operate on the
; same buffer one *line* at a time instead - also oracle-verified,
; including the one edge case that doesn't match a naive Chr(10)-split: a
; genuinely empty buffer reports CountGadgetItems 0, not 1.
;
; Flags (#PB_Editor_ReadOnly/#PB_Editor_WordWrap) have no PB-visible effect
; on Debug output - covered by a Catch2 unit test instead (checking the
; real GtkTextView's own editable/wrap-mode properties directly), the same
; deterministic split every other GUI slice's own e2e case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug EditorGadget(1, 10, 10, 200, 150)
Debug "[" + GetGadgetText(1) + "]"
Debug GetGadgetState(1)
Debug CountGadgetItems(1)
Debug "[" + GetGadgetItemText(1, 0) + "]"

SetGadgetText(1, "Hello" + Chr(10) + "World")
Debug GetGadgetText(1)
Debug CountGadgetItems(1)

AddGadgetItem(1, -1, "Line 1")
AddGadgetItem(1, -1, "Line 2")
AddGadgetItem(1, 0, "Line 0")
Debug GetGadgetText(1)
Debug CountGadgetItems(1)
Debug GetGadgetItemText(1, 0)
Debug GetGadgetItemText(1, 1)
Debug GetGadgetItemText(1, 2)

ClearGadgetItems(1)
Debug "[" + GetGadgetText(1) + "]"
Debug CountGadgetItems(1)
