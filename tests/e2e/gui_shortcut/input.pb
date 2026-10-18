; Exercises M7b's twenty-fourth GUI slice: ShortcutGadget's own creation
; and every #PB_Shortcut_*/#PB_GadgetType_Shortcut constant - a brand
; new command in this PureBasic beta. Oracle-verified: creation, every
; constant, and standard gadget-table behavior (IsGadget/FreeGadget) all
; work - but the shortcut itself was never observed to activate at all
; in this project's own extensive testing (synthetic input on a virtual
; display, with/without a real window manager; a hand-rolled plain GTK
; accelerator fires fine under the same conditions; most conclusively,
; even a genuine physical keypress on real hardware, with confirmed
; real X keyboard focus, produced nothing). See pbShortcutGadget's own
; doc comment in guilib.hpp for the full investigation - event-firing
; support is deliberately not implemented, so this only exercises the
; non-interactive, oracle-verifiable parts.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug ShortcutGadget(1, 50, 60, 70, 80, #PB_Shortcut_Control | #PB_Shortcut_F1)
Debug IsGadget(1)
FreeGadget(1)
Debug IsGadget(1)

Debug #PB_Shortcut_Control
Debug #PB_Shortcut_Shift
Debug #PB_Shortcut_Alt
Debug #PB_Shortcut_Command
Debug #PB_Shortcut_All
Debug #PB_Shortcut_A
Debug #PB_Shortcut_9
Debug #PB_Shortcut_F1
Debug #PB_Shortcut_Pad0
Debug #PB_Shortcut_Escape
Debug #PB_Shortcut_Capital
Debug #PB_Shortcut_LeftWindows
Debug #PB_GadgetType_Shortcut

Debug "done"
