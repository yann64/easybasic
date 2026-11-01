; Exercises AddWindowTimer/RemoveWindowTimer/EventTimer. Oracle-verified
; entirely via plain Debug probes - see guilib.hpp's own
; pbAddWindowTimer/pbRemoveWindowTimer doc comments for the full set of
; findings this is based on: a real, repeating #PB_Event_Timer (15),
; with EventTimer() correctly distinguishing several independent
; timers running concurrently on the same window.
;
; The exact number of times a timer fires in a given wall-clock window
; is inherently timing-dependent (the same reason this project's own
; gui_canvas_events interactive e2e case never counts raw mouse-motion
; events) - this test is deterministic instead by looping until a
; fixed, small count of each timer's own events has actually been
; seen (not until some amount of time has passed), so it always prints
; the same thing regardless of how fast or slow the real timers
; happened to fire along the way.
OpenWindow(0, 10, 10, 100, 100, "W")
Debug AddWindowTimer(0, 1, 20)
Debug AddWindowTimer(0, 2, 25)

seen1 = 0
seen2 = 0
Repeat
  Event = WaitWindowEvent()
  If Event = #PB_Event_Timer
    If EventWindow() = 0
      If EventTimer() = 1
        seen1 = seen1 + 1
      ElseIf EventTimer() = 2
        seen2 = seen2 + 1
      EndIf
    EndIf
  EndIf
Until seen1 >= 3 And seen2 >= 3

Debug "both timers fired at least 3 times each"
Debug RemoveWindowTimer(0, 1)
; RemoveWindowTimer on an unknown Timer id returns undefined, native-
; handle-ish content (oracle-confirmed) - called here but not printed,
; the same "Aucune" treatment several other functions' own golden
; cases already follow. A real, fatal error on an unknown *window* id
; instead is not exercised in this golden case at all either, since it
; would crash the real oracle this is diffed against - see the unit
; test for that case.
RemoveWindowTimer(0, 999)
RemoveWindowTimer(0, 2)
Debug "done"
