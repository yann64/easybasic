; Exercises AddKeyboardShortcut/RemoveKeyboardShortcut. Oracle-verified
; entirely via plain Debug probes - see guilib.hpp's own detail::
; WindowShortcutState doc comment for the full set of findings this is
; based on, including the one, honestly-flagged, not-independently-
; verifiable gap: what a real key press actually fires could not be
; confirmed at all in this project's own sandboxed test environment
; (several real-input delivery methods were tried against the real
; oracle binary itself, including a real window manager paired with
; Xvfb, none of them made the real oracle's own shortcut fire). This
; golden case is scoped to what IS verifiable: registration/removal's
; own static behavior.
OpenWindow(0, 10, 10, 200, 200, "W")
Debug AddKeyboardShortcut(0, #PB_Shortcut_A, 42)
Debug AddKeyboardShortcut(0, #PB_Shortcut_Control | #PB_Shortcut_S, 43)
; Re-adding the same Key replaces the old binding rather than erroring
; or stacking a second one.
Debug AddKeyboardShortcut(0, #PB_Shortcut_A, 99)

Debug RemoveKeyboardShortcut(0, #PB_Shortcut_A)
Debug RemoveKeyboardShortcut(0, #PB_Shortcut_A) ; already removed
Debug RemoveKeyboardShortcut(0, 999999) ; never registered
Debug RemoveKeyboardShortcut(0, #PB_Shortcut_Control | #PB_Shortcut_S)

Debug "done"
