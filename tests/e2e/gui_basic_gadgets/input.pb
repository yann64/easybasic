; Exercises M7b's second GUI slice: basic gadgets (ButtonGadget/TextGadget/
; StringGadget/CheckBoxGadget/FrameGadget) + generic gadget management
; (IsGadget/FreeGadget/ResizeGadget/HideGadget/DisableGadget) and the
; text/state accessors (GetGadgetText/SetGadgetText/GetGadgetState/
; SetGadgetState). Deliberately avoids anything requiring a simulated
; click/keystroke (xdotool) - that's covered by the Catch2 unit tests
; instead, via direct GTK signal emission/driving (no window manager is
; available under this run_gui_case.sh's own headless environment either).
; Only return-value and accessor round-trips here, so this stays
; deterministic. Run via run_gui_case.sh (not the plain run_case.sh) - see
; its own comments for why.
OpenWindow(1, 10, 10, 200, 200, "Test")
ButtonGadget(1, 10, 10, 100, 30, "Click me")
TextGadget(2, 10, 50, 100, 20, "Label")
StringGadget(3, 10, 80, 100, 20, "init")
CheckBoxGadget(4, 10, 110, 100, 20, "Check")
FrameGadget(5, 10, 140, 150, 40, "Frame")

Debug IsGadget(1)
Debug IsGadget(999)

Debug GetGadgetText(1)
SetGadgetText(1, "New Label")
Debug GetGadgetText(1)

Debug GetGadgetText(3)
SetGadgetText(3, "new string")
Debug GetGadgetText(3)

Debug GetGadgetState(4)
SetGadgetState(4, 1)
Debug GetGadgetState(4)
SetGadgetState(4, 0)
Debug GetGadgetState(4)

Debug ResizeGadget(1, 20, 20, 120, 40)
Debug HideGadget(1, 1)
Debug DisableGadget(1, 1)
Debug DisableGadget(1, 0)
Debug FreeGadget(1)
Debug IsGadget(1)

CloseWindow(1)
Debug IsGadget(2)
