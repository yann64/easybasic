; Exercises M7b's twenty-second GUI slice: CanvasGadget's own creation,
; Flags, and GetGadgetAttribute defaults - the parts oracle-verifiable
; with a plain Debug probe, no synthetic mouse/keyboard input needed (see
; gui_canvas_events for that). GetGadgetAttribute's own MouseX/MouseY/
; Buttons/Modifiers/WheelDelta/Key all default to 0 before any
; interaction - oracle-verified directly. #PB_Canvas_Container auto-opens
; its own gadget list on creation, the same as ContainerGadget - also
; oracle-verified directly (an un-flagged Canvas's own CloseGadgetList()
; call instead raises a fatal "can only be called after..." debugger
; error).
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug CanvasGadget(1, 10, 10, 200, 150)
Debug GetGadgetAttribute(1, #PB_Canvas_MouseX)
Debug GetGadgetAttribute(1, #PB_Canvas_MouseY)
Debug GetGadgetAttribute(1, #PB_Canvas_Buttons)
Debug GetGadgetAttribute(1, #PB_Canvas_Modifiers)
Debug GetGadgetAttribute(1, #PB_Canvas_WheelDelta)
Debug GetGadgetAttribute(1, #PB_Canvas_Key)
Debug SetGadgetAttribute(1, #PB_Canvas_MouseX, 50)
Debug GetGadgetAttribute(1, #PB_Canvas_MouseX)

Debug CanvasGadget(2, 10, 170, 200, 100, #PB_Canvas_Container)
CloseGadgetList()
ButtonGadget(3, 10, 10, 80, 25, "nested")
Debug IsGadget(3)
Debug "done"
