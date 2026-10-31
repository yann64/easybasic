; Exercises UseGadgetList and GadgetItemID. Oracle-verified entirely via
; plain Debug probes - see guilib.hpp's own pbUseGadgetList/
; pbGadgetItemID doc comments for the full set of findings this is
; based on: UseGadgetList switches which already-open window
; subsequent AddXxxGadget calls attach to (by real native WindowID()
; handle, not a #Window id) and resets the open gadget-list stack to
; that window's own top level; GadgetItemID is always a plain 0 on
; this platform (no GtkTreeView equivalent to a Win32 listview's own
; per-row system id). Invalid-#Gadget/closed-window cases are real,
; fatal errors in real PB itself (see the unit tests for those instead,
; where the simplification can be checked without crashing the oracle
; this golden case is diffed against).
OpenWindow(0, 10, 10, 200, 200, "W0")
OpenWindow(1, 250, 10, 200, 200, "W1")

UseGadgetList(WindowID(0))
ButtonGadget(10, 10, 10, 100, 30, "In W0")
UseGadgetList(WindowID(1))
ButtonGadget(11, 10, 10, 100, 30, "In W1")
Debug GadgetX(10)
Debug GadgetX(11)

; UseGadgetList resets the open gadget-list stack - a gadget created
; right after it, with no matching CloseGadgetList first, attaches to
; the new window's own top level, not nested inside this still-open
; ContainerGadget.
ContainerGadget(20, 10, 50, 150, 100)
UseGadgetList(WindowID(1))
ButtonGadget(12, 10, 10, 60, 20, "After container, in W1")
Debug GadgetX(12)
Debug GadgetY(12)

; A native handle that matches no open window is a harmless no-op.
UseGadgetList(0)
ButtonGadget(13, 50, 60, 60, 20, "After invalid UseGadgetList")
Debug GadgetX(13)

Debug "--- GadgetItemID: always 0 ---"
ListIconGadget(30, 10, 10, 100, 60, "Col", 50)
AddGadgetItem(30, 0, "Row0")
Debug GadgetItemID(30, 0)
TreeGadget(31, 10, 80, 100, 60)
AddGadgetItem(31, -1, "A")
Debug GadgetItemID(31, 0)
ListViewGadget(32, 10, 150, 100, 40)
AddGadgetItem(32, 0, "X")
Debug GadgetItemID(32, 0)

Debug "done"
