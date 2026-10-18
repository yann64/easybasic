; Exercises ExplorerListGadget/ExplorerComboGadget. Oracle-verified
; entirely via plain Debug probes - no synthetic input needed. See
; detail::gadgetKindExplorerList()'s own doc comment in guilib.hpp for
; the full set of findings this is based on: "sort order" is really
; just directories-first-then-files, each group in whatever raw order
; the OS's own directory enumeration returns (not a deliberate sort by
; name or modification time - confirmed not to match either, and known
; to differ between filesystems, which is exactly why this fixture
; directory has only one entry of each type, sidestepping any intra-
; group ordering ambiguity); items are only reachable via
; GetGadgetItemText, never GetGadgetState/SetGadgetState/plain
; GetGadgetText (all red herrings for selection); GetGadgetItemState is
; a plain #PB_Explorer_File=1/Directory=2 type code, not a ListIcon-
; style Selected/Checked bitmask; ExplorerComboGadget exposes no real,
; PB-queryable item list at all.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug ExplorerListGadget(1, 10, 10, 200, 150, "browsedir/")
Debug CountGadgetItems(1)
Debug GetGadgetItemText(1, 0) + " " + Str(GetGadgetItemState(1, 0))
Debug GetGadgetItemText(1, 1) + " " + Str(GetGadgetItemState(1, 1))
Debug GetGadgetItemText(1, 2) + " " + Str(GetGadgetItemState(1, 2))
Debug "[" + GetGadgetText(1) + "]"
Debug GetGadgetState(1)
SetGadgetState(1, 0)
Debug GetGadgetState(1)
Debug SetGadgetItemState(1, 1, 99)
Debug GetGadgetItemState(1, 1)

Debug "--- #PB_Explorer_HiddenFiles ---"
Debug ExplorerListGadget(2, 10, 170, 200, 100, "browsedir/", #PB_Explorer_HiddenFiles)
Debug CountGadgetItems(2)

Debug "--- #PB_Explorer_NoFiles ---"
Debug ExplorerListGadget(3, 10, 170, 200, 100, "browsedir/", #PB_Explorer_NoFiles)
Debug CountGadgetItems(3)

Debug "--- #PB_Explorer_NoFolders ---"
Debug ExplorerListGadget(4, 10, 170, 200, 100, "browsedir/", #PB_Explorer_NoFolders)
Debug CountGadgetItems(4)

Debug "--- #PB_Explorer_NoParentFolder ---"
Debug ExplorerListGadget(5, 10, 170, 200, 100, "browsedir/", #PB_Explorer_NoParentFolder)
Debug CountGadgetItems(5)

Debug "--- no trailing slash ---"
Debug ExplorerListGadget(6, 10, 170, 200, 100, "browsedir/subdir")
Debug CountGadgetItems(6)
Debug GetGadgetItemText(6, 0)
Debug GetGadgetItemText(6, 1)

Debug "--- SetGadgetText re-browses ---"
SetGadgetText(1, "browsedir/subdir/")
Debug "[" + GetGadgetText(1) + "]"
Debug CountGadgetItems(1)

Debug "--- ExplorerComboGadget ---"
Debug ExplorerComboGadget(7, 10, 170, 200, 25, "browsedir/")
Debug "[" + GetGadgetText(7) + "]"
Debug CountGadgetItems(7)
Debug "[" + GetGadgetItemText(7, 0) + "]"
Debug GetGadgetItemState(7, 0)
Debug GetGadgetState(7)
SetGadgetText(7, "browsedir/subdir/")
Debug "[" + GetGadgetText(7) + "]"

Debug "done"
