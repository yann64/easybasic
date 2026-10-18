; Exercises M7b's twenty-third GUI slice: the 2D Drawing library,
; scoped to the CanvasOutput target only. Oracle-verified entirely via
; plain Debug Point(...) read-backs after drawing - no synthetic input
; needed at all (unlike gui_canvas_events), since reading a pixel back
; is itself a deterministic, headless-friendly oracle probe. See
; detail::canvasBackingSurfaceKey()'s own doc comment in guilib.hpp for
; the full set of findings this is based on: Box defaults to FrontColor
; when its own trailing Color is omitted; Circle is a filled disc, not
; an outline; BackColor has no effect on any of Box/Circle/Plot; Box/
; Circle clip silently even entirely off-canvas, but Plot/Point raise a
; real, fatal "outside the drawing area" error for an out-of-bounds
; coordinate in real PB - not reproduced here (this project's own
; established stance), a harmless 0 instead.
OpenWindow(0, 10, 10, 320, 320, "Test")
CanvasGadget(1, 10, 10, 200, 150)

StartDrawing(CanvasOutput(1))
  Debug Point(0, 0)

  FrontColor(RGB(0, 255, 0))
  Box(10, 10, 20, 20)
  Debug Point(15, 15)

  Circle(100, 75, 15, RGB(0, 0, 255))
  Debug Point(100, 75)
  Debug Point(100, 61)
  Debug Point(100, 50)

  BackColor(RGB(123, 45, 67))
  Box(150, 10, 10, 10, RGB(9, 9, 9))
  Debug Point(155, 15)

  Plot(5, 5, RGB(1, 2, 3))
  Debug Point(5, 5)

  Box(1000, 1000, 30, 30, RGB(10, 20, 30))
  Circle(1000, 1000, 15, RGB(1, 2, 3))
  Debug "off-canvas box/circle survived"
StopDrawing()

Debug "done"
