; Exercises the Font library's own basics (LoadFont/FontID/IsFont/
; FreeFont) and GetGadgetFont/SetGadgetFont. Oracle-verified entirely
; via plain Debug probes - no synthetic input needed. See
; pbLoadFont's/pbGetGadgetFont's own doc comments in guilib.hpp for
; the full set of findings this is based on: LoadFont's own return
; value is the exact same real native handle FontID itself returns for
; the same #Font - a real, usable native handle, never byte-matched
; against the oracle's own pointer value, so every check below is a
; derived boolean/comparison rather than a raw handle value (the same
; convention tests/e2e/threads already uses for ThreadID). A
; nonexistent font family name is not a real error. GetGadgetFont/
; SetGadgetFont are per-gadget, not global - a brand new gadget created
; after a SetGadgetFont call on a different one has its own, unaffected
; default font, and that default is stable across repeated queries but
; genuinely different per gadget.
Debug "--- LoadFont/FontID/IsFont/FreeFont ---"
Debug IsFont(0)
f = LoadFont(0, "Sans", 14)
If FontID(0) = f
  Debug 1
Else
  Debug 0
EndIf
; IsFont's own real return value, like IsWindow's, is a native-
; handle-ish nonzero value, not a clean 1 or 0 - checked via a boolean
; comparison here rather than printed raw, the same way every other
; native-handle value in this test is.
If IsFont(0) <> 0
  Debug 1
Else
  Debug 0
EndIf

; A nonexistent font family name is not a real error.
LoadFont(1, "ThisFontDoesNotExistAtAll12345", 12)
If IsFont(1) <> 0
  Debug 1
Else
  Debug 0
EndIf

FreeFont(0)
Debug IsFont(0) ; after a real free, genuinely 0 either way - safe to print raw.
FreeFont(1)

OpenWindow(0, 10, 10, 320, 320, "Test")
ButtonGadget(1, 10, 10, 100, 30, "B1")
ButtonGadget(2, 10, 50, 100, 30, "B2")

Debug "--- GetGadgetFont/SetGadgetFont ---"
default1a = GetGadgetFont(1)
default1b = GetGadgetFont(1)
default2 = GetGadgetFont(2)
If default1a = default1b
  Debug 1 ; stable across repeated queries
Else
  Debug 0
EndIf
If default1a <> default2
  Debug 1 ; genuinely different per gadget
Else
  Debug 0
EndIf

fontHandle = LoadFont(2, "Sans", 20)
SetGadgetFont(1, fontHandle)
If GetGadgetFont(1) = fontHandle
  Debug 1 ; exact round-trip, not a copy
Else
  Debug 0
EndIf
If GetGadgetFont(2) = default2
  Debug 1 ; the other gadget is unaffected
Else
  Debug 0
EndIf
FreeFont(2)

Debug "done"
