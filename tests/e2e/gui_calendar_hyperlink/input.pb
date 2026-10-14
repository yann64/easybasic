; Exercises M7b's eighteenth GUI slice: CalendarGadget/HyperLinkGadget.
;
; CalendarGadget's own state is a real Date() value, truncated to midnight
; (oracle-verified: Hour/Minute/Second all read back 0) - this only ever
; prints round-tripped Year/Month/Day/Hour components, never a raw epoch
; value, the same discipline the M4 Date library's own tests already use
; (a raw epoch's own absolute magnitude depends on the host's timezone,
; so printing one here could spuriously fail on a CI runner in a
; different timezone than this fixture was captured in).
;
; HyperLinkGadget is a plain, clickable, styled-as-a-link label - not a
; stateful toggle (GetGadgetState always 0). Color is oracle-verified
; *required*, unlike every other trailing Options argument in this whole
; gadget family.
;
; Deliberately avoids anything requiring a simulated click (xdotool) - a
; real click firing #PB_Event_Gadget/#PB_EventType_LeftClick is covered by
; a Catch2 unit test instead (driving the real GTK signal directly), the
; same deterministic split every other GUI slice's own e2e case already
; uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

d.q = Date(2020, 6, 15, 0, 0, 0)
Debug CalendarGadget(1, 10, 10, 200, 150, d)
r.q = GetGadgetState(1)
Debug Year(r)
Debug Month(r)
Debug Day(r)
Debug Hour(r)
Debug GetGadgetText(1) ; no text representation at all

d2.q = Date(2021, 3, 9, 0, 0, 0)
SetGadgetState(1, d2)
r2.q = GetGadgetState(1)
Debug Year(r2)
Debug Month(r2)
Debug Day(r2)

Debug HyperLinkGadget(2, 10, 170, 200, 20, "Click me", RGB(0, 0, 255))
Debug GetGadgetText(2)
Debug GetGadgetState(2)
SetGadgetText(2, "Renamed")
Debug GetGadgetText(2)
Debug GetGadgetState(2)
