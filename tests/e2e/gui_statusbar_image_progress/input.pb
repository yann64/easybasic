; Exercises StatusBarImage/StatusBarProgress. Oracle-verified entirely
; via plain Debug probes - no synthetic input needed. See
; pbStatusBarImage's/pbStatusBarProgress's own doc comments in
; guilib.hpp for the full set of findings this is based on: both
; return 0 ("Aucune"); StatusBarImage's own Appearance accepts the
; full 0-15 flag range (like StatusBarText), but StatusBarProgress's
; own only accepts 0-3 in real PB (a real fatal "invalid value" error
; for 4+, not replicated here); StatusBarText/StatusBarImage/
; StatusBarProgress all target the same field, whichever was called
; most recently replacing whatever the field was previously showing -
; confirmed directly that calling each of the three, in turn, on the
; very same field never crashes either way. StatusBarHeight is
; deliberately not exercised here - a separate, pre-existing
; discrepancy (unrelated to any of these three) was found while
; testing this slice; see the roadmap's own notes.
OpenWindow(0, 10, 10, 320, 320, "Test")
CreateStatusBar(0, WindowID(0))
AddStatusBarField(100)
AddStatusBarField(100)
CreateImage(0, 16, 16)

Debug "--- StatusBarImage ---"
Debug StatusBarImage(0, 0, ImageID(0))
Debug StatusBarImage(0, 0, ImageID(0), #PB_StatusBar_Right)

Debug "--- StatusBarProgress ---"
Debug StatusBarProgress(0, 1, 50, 0, 0, 100)
Debug StatusBarProgress(0, 1, 75, #PB_StatusBar_Raised | #PB_StatusBar_BorderLess)
Debug StatusBarProgress(0, 1, 30) ; Appearance/Min/Max all omitted

Debug "--- same field, three different content types in turn ---"
StatusBarText(0, 0, "text")
StatusBarImage(0, 0, ImageID(0))
StatusBarProgress(0, 0, 50)
StatusBarText(0, 0, "text again")

Debug "done"
