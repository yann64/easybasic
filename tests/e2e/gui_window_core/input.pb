; Exercises M7b's first GUI-core slice: OpenWindow/CloseWindow/IsWindow,
; WindowEvent (non-blocking poll)/WaitWindowEvent (with a timeout).
; Deliberately avoids printing anything timing-sensitive (no raw
; Delay/ElapsedMilliseconds values) - only return-value checks, so the
; test is deterministic regardless of event-delivery timing. Run via
; run_gui_case.sh (not the plain run_case.sh), which skips this whole
; case when GTK3/a usable display aren't available - see its own
; comments for why.
OpenWindow(1, 10, 10, 100, 100, "Test")

; Drain whatever startup events window creation itself generates
; (move/resize/repaint - oracle-verified real PB generates some too)
; before relying on "no event pending".
quietStreak = 0
Repeat
  ev = WindowEvent()
  If ev = 0
    quietStreak = quietStreak + 1
  Else
    quietStreak = 0
  EndIf
  Delay(10)
Until quietStreak > 5

Debug IsWindow(1)
Debug IsWindow(999)
Debug WaitWindowEvent(100)

CloseWindow(1)
Debug IsWindow(1)
