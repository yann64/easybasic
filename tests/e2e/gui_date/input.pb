; Exercises M7b's twentieth GUI slice: DateGadget.
;
; GetGadgetState keeps only the date/time *components* whose own token
; actually appears in Mask$ - every other component resets to the Unix
; epoch's own value for that field (year 1970, month/day 1, hour/minute/
; second 0) - oracle-verified directly, including the sub-day-magnitude
; result a time-only mask (no date tokens at all) gives. SetGadgetState
; re-applies the exact same filtering; SetGadgetText is a genuine escape
; hatch that writes the literal text given and independently resets the
; tracked state to 0, never re-parsing the text as a Date.
;
; Deliberately never prints a raw epoch value - only round-tripped
; Year/Month/Day/Hour/Minute components, the same discipline used
; throughout the M4 Date library's own tests (a raw epoch's absolute
; magnitude depends on the host's timezone).
OpenWindow(0, 10, 10, 320, 320, "Test")

d.q = Date(2020, 6, 15, 10, 30, 45)

Debug DateGadget(1, 10, 10, 200, 25, "%dd/%mm/%yyyy", d)
Debug GetGadgetText(1)
r.q = GetGadgetState(1)
Debug Year(r)
Debug Month(r)
Debug Day(r)
Debug Hour(r)

Debug DateGadget(2, 10, 50, 200, 25, "%dd/%mm/%yyyy %hh", d)
r2.q = GetGadgetState(2)
Debug Hour(r2)
Debug Minute(r2)

Debug DateGadget(3, 10, 90, 200, 25, "%yyyy", d)
r3.q = GetGadgetState(3)
Debug Year(r3)
Debug Month(r3)
Debug Day(r3)

d2.q = Date(2021, 3, 9, 14, 22, 33)
SetGadgetState(1, d2)
Debug GetGadgetText(1)
r4.q = GetGadgetState(1)
Debug Year(r4)
Debug Month(r4)
Debug Day(r4)

SetGadgetText(1, "12345")
Debug GetGadgetText(1)
Debug GetGadgetState(1)
