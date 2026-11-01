; Exercises WindowMouseX/WindowMouseY and SmartWindowRefresh. The
; pointer-over-window case (a real relative coordinate, confirmed via
; xdotool against the real oracle) isn't exercised here at all - this
; plain golden case's own run_gui_case.sh harness never moves the real
; pointer, so WindowMouseX/Y reliably read the same -1 "not over this
; window" sentinel both here and in the real oracle - see guilib.hpp's
; own pbWindowMouseX doc comment for the full, manually-confirmed
; positive-case finding. SmartWindowRefresh has no real GTK mechanism
; to map onto at all (GTK3 already double-buffers every window by
; default) - accepted but not acted on. Its own real return value is
; oracle-confirmed to carry no consistent signal either (an undefined,
; native-handle-ish value) - called here but not printed, the same
; "Aucune" precedent DisableWindow's own golden case already follows,
; not safe for a byte-exact comparison.
OpenWindow(0, 10, 10, 200, 200, "W")
Debug WindowMouseX(0)
Debug WindowMouseY(0)
SmartWindowRefresh(0, 1)
SmartWindowRefresh(0, 0)
Debug "done"
