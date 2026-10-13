; Exercises M7b's sixteenth GUI slice: SpinGadget/TrackBarGadget/
; ScrollBarGadget.
;
; SpinGadget's own displayed *text* and internal *state* are genuinely
; independent (oracle-verified) unless #PB_Spin_Numeric was given at
; creation time, in which case SetGadgetState auto-syncs the text too.
; GetGadgetAttribute/SetGadgetAttribute round-trip Minimum/Maximum
; without retroactively re-clamping an existing value, but a later
; SetGadgetState does clamp into the (possibly changed) range.
;
; TrackBarGadget shares the same Minimum/Maximum/clamping shape as
; SpinGadget, minus the text independence (it has no text at all).
;
; ScrollBarGadget's own PageLength narrows the effective maximum
; selectable position to Maximum - PageLength + 1 (oracle-verified) -
; #PB_ScrollBar_Maximum itself still reads back the plain, un-narrowed
; Maximum.
;
; Deliberately avoids anything requiring a simulated click/drag
; (xdotool) - the same deterministic split every other GUI slice's own
; e2e case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug SpinGadget(0, 10, 10, 100, 20, 0, 100)
Debug GetGadgetState(0) ; defaults to Minimum
Debug GetGadgetText(0) ; empty - independent of state without #PB_Spin_Numeric
SetGadgetState(0, 42)
Debug GetGadgetState(0)
Debug GetGadgetText(0) ; still empty
SetGadgetText(0, "hi")
Debug GetGadgetText(0)
Debug GetGadgetAttribute(0, #PB_Spin_Minimum)
Debug GetGadgetAttribute(0, #PB_Spin_Maximum)
SetGadgetAttribute(0, #PB_Spin_Maximum, 200)
Debug GetGadgetAttribute(0, #PB_Spin_Maximum)
SetGadgetState(0, 150)
Debug GetGadgetState(0)
SetGadgetState(0, 1000)
Debug GetGadgetState(0) ; clamped to the new Maximum

Debug SpinGadget(1, 10, 40, 100, 20, 0, 100, #PB_Spin_Numeric)
SetGadgetState(1, 55)
Debug GetGadgetText(1) ; auto-synced to "55" thanks to #PB_Spin_Numeric

Debug TrackBarGadget(2, 10, 70, 200, 30, 0, 50)
Debug GetGadgetState(2)
SetGadgetState(2, 25)
Debug GetGadgetState(2)
Debug GetGadgetAttribute(2, #PB_TrackBar_Minimum)
Debug GetGadgetAttribute(2, #PB_TrackBar_Maximum)
SetGadgetAttribute(2, #PB_TrackBar_Maximum, 80)
Debug GetGadgetAttribute(2, #PB_TrackBar_Maximum)
SetGadgetState(2, 1000)
Debug GetGadgetState(2) ; clamped to the new Maximum

Debug ScrollBarGadget(3, 10, 110, 200, 20, 0, 100, 30)
Debug GetGadgetState(3)
SetGadgetState(3, 50)
Debug GetGadgetState(3)
Debug GetGadgetAttribute(3, #PB_ScrollBar_Minimum)
Debug GetGadgetAttribute(3, #PB_ScrollBar_Maximum)
Debug GetGadgetAttribute(3, #PB_ScrollBar_PageLength)
SetGadgetState(3, 1000)
Debug GetGadgetState(3) ; clamped to Maximum - PageLength + 1 = 71

; Harmless failures: Spin/TrackBar attributes on an unrelated gadget type.
ButtonGadget(6, 10, 230, 60, 20, "B")
Debug GetGadgetAttribute(6, #PB_Spin_Minimum)
Debug SetGadgetAttribute(6, #PB_TrackBar_Minimum, 1)
