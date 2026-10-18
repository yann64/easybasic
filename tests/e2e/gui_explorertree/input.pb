; Exercises ExplorerTreeGadget. Oracle-verified entirely via plain
; Debug probes - no synthetic input needed. See
; detail::gadgetKindExplorerTree()'s own doc comment in guilib.hpp for
; the full set of findings this is based on: its own PB-level API
; surface is almost entirely a dead end, the same way
; ExplorerComboGadget's own is - CountGadgetItems/GetGadgetItemText/
; GetGadgetItemState stay 0/empty/0 regardless of the browsed
; directory's own real content (so no fixture directory is needed here
; at all - a well-known always-present absolute path like "/tmp/" is
; just as good a probe target), and GetGadgetState is a fixed 2 (a
; third, distinct value from ExplorerListGadget's own -1 and
; ExplorerComboGadget's own 0). GetGadgetText/SetGadgetText always
; ensure a trailing slash for a real, existing *absolute* directory,
; falling back to "/" (not the given path's own parent) for a
; nonexistent one - a relative Directory$ has its own separate, quirky
; behavior (empty or "/" depending on trailing-slash presence,
; regardless of whether it actually exists) deliberately not
; replicated here, a narrow documented gap (see guilib.hpp).
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug ExplorerTreeGadget(1, 10, 10, 200, 150, "/tmp")
Debug "[" + GetGadgetText(1) + "]"
Debug GetGadgetState(1)
SetGadgetState(1, 0)
Debug GetGadgetState(1)
Debug CountGadgetItems(1)
Debug "[" + GetGadgetItemText(1, 0) + "]"
Debug GetGadgetItemState(1, 0)

Debug "--- nonexistent absolute path ---"
Debug ExplorerTreeGadget(2, 10, 170, 200, 100, "/nonexistent/path/at/all/")
Debug "[" + GetGadgetText(2) + "]"

Debug "--- SetGadgetText re-browses ---"
SetGadgetText(1, "/tmp/")
Debug "[" + GetGadgetText(1) + "]"
SetGadgetText(1, "/nonexistent/again/")
Debug "[" + GetGadgetText(1) + "]"

Debug "done"
