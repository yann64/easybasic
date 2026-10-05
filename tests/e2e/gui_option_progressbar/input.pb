; Exercises M7b's fifteenth GUI slice: OptionGadget/ProgressBarGadget.
;
; OptionGadget's own radio-button grouping - oracle-verified directly:
; consecutive OptionGadget calls join the same mutually-exclusive group,
; with the first one defaulting to selected; any other gadget type
; called in between starts a brand new, independent group. GetGadgetText/
; SetGadgetText/GetGadgetState/SetGadgetState all reuse the generic
; GtkButton/GtkToggleButton dispatch already in place since CheckBoxGadget's
; own second GUI slice - no new branches needed there - except
; SetGadgetState(id, 0), which (oracle-verified) is a no-op: a radio
; button can't be deselected directly, only by selecting a different
; group member.
;
; ProgressBarGadget maps its own [Minimum, Maximum] integer range onto
; GtkProgressBar's [0.0, 1.0] fraction - SetGadgetState clamps into range
; except for the literal #PB_ProgressBar_Unknown (-1) sentinel, which is
; stored and read back verbatim. GetGadgetAttribute/SetGadgetAttribute
; round-trip Minimum/Maximum without retroactively re-clamping an
; existing value.
;
; Deliberately avoids anything requiring a simulated click (xdotool) -
; a real toggle firing #PB_Event_Gadget/#PB_EventType_LeftClick (for both
; the outgoing and incoming radio button) is covered by a Catch2 unit
; test instead (driving the real GTK signal directly), the same
; deterministic split every other GUI slice's own e2e case already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug OptionGadget(0, 10, 10, 100, 20, "Option 1")
OptionGadget(1, 10, 40, 100, 20, "Option 2")
OptionGadget(2, 10, 70, 100, 20, "Option 3")
Debug GetGadgetState(0) ; first option defaults selected
Debug GetGadgetState(1)
Debug GetGadgetState(2)

SetGadgetState(1, 1)
Debug GetGadgetState(0)
Debug GetGadgetState(1)
Debug GetGadgetState(2)
Debug GetGadgetText(1)

SetGadgetText(1, "Renamed")
Debug GetGadgetText(1)

; Can't deselect a radio button directly.
SetGadgetState(1, 0)
Debug GetGadgetState(1)

; A different gadget type breaks the group.
TextGadget(10, 10, 100, 100, 20, "sep")
OptionGadget(3, 10, 130, 100, 20, "Option A")
OptionGadget(4, 10, 160, 100, 20, "Option B")
Debug GetGadgetState(3) ; first-in-new-group defaults selected
Debug GetGadgetState(4)
SetGadgetState(4, 1)
Debug GetGadgetState(3)
Debug GetGadgetState(4)
Debug GetGadgetState(1) ; the first group's own state is untouched

Debug ProgressBarGadget(5, 10, 190, 250, 30, 0, 200)
Debug GetGadgetState(5)
SetGadgetState(5, 50)
Debug GetGadgetState(5)
Debug GetGadgetAttribute(5, #PB_ProgressBar_Minimum)
Debug GetGadgetAttribute(5, #PB_ProgressBar_Maximum)
SetGadgetAttribute(5, #PB_ProgressBar_Maximum, 400)
Debug GetGadgetAttribute(5, #PB_ProgressBar_Maximum)
Debug GetGadgetState(5) ; unchanged - no retroactive re-clamping
SetGadgetState(5, 1000)
Debug GetGadgetState(5) ; clamped to the new Maximum
SetGadgetState(5, -50)
Debug GetGadgetState(5) ; clamped to Minimum
SetGadgetState(5, #PB_ProgressBar_Unknown)
Debug GetGadgetState(5) ; stored verbatim, not clamped

; Harmless failures: ProgressBar-only attributes on an unrelated gadget
; type.
ButtonGadget(6, 10, 230, 60, 20, "B")
Debug GetGadgetAttribute(6, #PB_ProgressBar_Minimum)
Debug SetGadgetAttribute(6, #PB_ProgressBar_Minimum, 1)
