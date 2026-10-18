; Exercises M7b's twenty-second GUI slice: CanvasGadget's genuinely
; interactive mouse/keyboard events - the part no plain Debug probe can
; verify, since it needs real input actually happening while the program
; runs (see run_gui_interactive_case.sh and pbCanvasGadget's own doc
; comment in guilib.hpp for the Xvfb+xdotool methodology this was
; oracle-verified against).
;
; MouseEnter/MouseMove/MouseLeave are deliberately *not* counted or
; Debug-ed here - real X11 mouse-motion event coalescing is inherently
; timing-dependent (confirmed directly during this project's own
; development: replaying the exact same xdotool script against the exact
; same binary twice did not always produce the exact same number of
; incidental motion events), so a golden, pinned-output test can't
; depend on them without flaking. Every event actually asserted on below
; (button/double-click/wheel/key/focus) is a discrete, deterministic
; action instead - reads a fixed, known number of them so the program
; exits deterministically on its own once the companion xdotool_script
; finishes driving it, no reliance on ever closing the window.
OpenWindow(0, 10, 10, 320, 320, "Test")
CanvasGadget(1, 10, 10, 200, 150, #PB_Canvas_Keyboard)
count = 0
Repeat
  ev = WaitWindowEvent()
  If ev = #PB_Event_Gadget And EventGadget() = 1
    et = EventType()
    If et <> 65537 And et <> 65538 And et <> 65539
      Debug et
      Debug GetGadgetAttribute(1, #PB_Canvas_MouseX)
      Debug GetGadgetAttribute(1, #PB_Canvas_MouseY)
      Debug GetGadgetAttribute(1, #PB_Canvas_Buttons)
      Debug GetGadgetAttribute(1, #PB_Canvas_Modifiers)
      Debug GetGadgetAttribute(1, #PB_Canvas_WheelDelta)
      Debug GetGadgetAttribute(1, #PB_Canvas_Key)
      count = count + 1
    EndIf
  EndIf
Until count >= 17
Debug "DONE"
