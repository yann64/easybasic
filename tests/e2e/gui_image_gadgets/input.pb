; Exercises M7b's seventeenth GUI slice: ImageGadget/ButtonImageGadget.
;
; ImageGadget's own state is an ImageID-style handle, not a boolean -
; oracle-verified directly: GetGadgetState reads back whichever handle
; was most recently set (at creation or via SetGadgetState). Since that
; handle's own literal value is a non-reproducible native pointer (the
; same established caveat as OpenWindow/every gadget-creation return),
; this compares it against a freshly-read ImageID() instead of printing
; the raw value, the same way every other handle-returning check in this
; project's own e2e suite already does.
;
; Neither ImageGadget nor ButtonImageGadget has any text capability at
; all - oracle-verified directly: SetGadgetText/GetGadgetText are both
; genuine no-ops for each (unlike a plain ButtonGadget).
;
; ButtonImageGadget's own GetGadgetState stays a plain boolean, always 0
; unless #PB_Button_Toggle was given at creation, in which case it
; behaves exactly like CheckBoxGadget's own plain on/off toggle.
;
; Deliberately avoids anything requiring a simulated click (xdotool) - a
; real click firing #PB_Event_Gadget/#PB_EventType_LeftClick is covered
; by a Catch2 unit test instead (driving the real GTK signal directly),
; the same deterministic split every other GUI slice's own e2e case
; already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug LoadImage(0, "test.bmp")
CreateImage(1, 20, 20)

Debug ImageGadget(1, 10, 10, 0, 0, ImageID(0))
match1 = 0
If GetGadgetState(1) = ImageID(0)
  match1 = 1
EndIf
Debug match1
SetGadgetState(1, ImageID(1))
match2 = 0
If GetGadgetState(1) = ImageID(1)
  match2 = 1
EndIf
Debug match2
Debug SetGadgetText(1, "hello")
Debug GetGadgetText(1)

Debug ImageGadget(2, 10, 50, 50, 20, 0) ; harmless: no image at all

Debug ButtonImageGadget(3, 10, 90, 100, 30, ImageID(0))
Debug GetGadgetState(3)
SetGadgetState(3, 1)
Debug GetGadgetState(3) ; unchanged - no Toggle flag
Debug GetGadgetText(3)
SetGadgetText(3, "caption")
Debug GetGadgetText(3) ; still empty - no text capability at all

Debug ButtonImageGadget(4, 10, 130, 100, 30, ImageID(0), #PB_Button_Toggle)
Debug GetGadgetState(4)
SetGadgetState(4, 1)
Debug GetGadgetState(4) ; a real toggle now, thanks to #PB_Button_Toggle
