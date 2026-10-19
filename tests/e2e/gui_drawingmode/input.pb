; Exercises DrawingMode. Oracle-verified entirely via plain Debug
; Point(...) read-backs after drawing, the same proven methodology used
; for the rest of the 2D Drawing library - no synthetic input needed.
; See pbDrawingMode's own doc comment in guilib.hpp for the full set of
; findings this is based on: DrawingMode is an ambient setting, like
; FrontColor/BackColor (persists across calls, resets to
; #PB_2DDrawing_Default on the next StartDrawing); Outlined draws only
; a filled shape's own 1-pixel boundary, not its interior (no effect on
; Plot); XOr does a plain per-channel bitwise XOR of the given color's
; own RGB bytes against whatever is already there; AlphaBlend does a
; standard "source over destination" alpha composite using the color's
; own top byte as alpha (RGB() always packs 0 there, RGBA() sets it
; explicitly); Transparent has no effect on Box, but skips DrawText's
; own background fill entirely, letting whatever was already there
; show through.
OpenWindow(0, 10, 10, 320, 320, "Test")
CanvasGadget(1, 10, 10, 200, 150)

StartDrawing(CanvasOutput(1))
  ; Outlined: Box draws only its own boundary.
  DrawingMode(#PB_2DDrawing_Outlined)
  Box(10, 10, 20, 20, RGB(1, 2, 3))
  Debug Point(10, 10)
  Debug Point(15, 15)

  ; No effect on Plot; DrawingMode is ambient (no new call in between).
  Plot(50, 50, RGB(9, 9, 9))
  Debug Point(50, 50)
  Box(50, 10, 20, 20, RGB(4, 5, 6))
  Debug Point(55, 15)

  ; XOr: bitwise XOR against the existing destination pixel.
  DrawingMode(#PB_2DDrawing_Default)
  Box(10, 50, 5, 5, RGB(255, 0, 0))
  DrawingMode(#PB_2DDrawing_XOr)
  Box(10, 50, 5, 5, RGB(255, 255, 255))
  Debug Point(12, 52)

  ; AlphaBlend: source-over-destination using the color's own top byte.
  DrawingMode(#PB_2DDrawing_Default)
  Box(50, 50, 20, 20, RGB(200, 0, 0))
  DrawingMode(#PB_2DDrawing_AlphaBlend)
  Box(50, 50, 20, 20, RGB(0, 200, 0)) ; RGB() alpha=0 - fully invisible
  Debug Point(55, 55)
  Box(50, 50, 20, 20, RGBA(0, 200, 0, 255)) ; alpha=255 - fully opaque
  Debug Point(55, 55)
  Box(90, 50, 20, 20, RGB(200, 0, 0))
  Box(90, 50, 20, 20, RGBA(50, 60, 70, 100)) ; partial alpha
  c = Point(95, 55)
  Debug Red(c)
  Debug Green(c)
  Debug Blue(c)

  ; Transparent: no effect on Box, still opaque.
  DrawingMode(#PB_2DDrawing_Transparent)
  Box(130, 50, 20, 20, RGB(7, 7, 7))
  Debug Point(135, 55)

  ; Transparent's real effect: skips DrawText's own background fill.
  DrawingMode(#PB_2DDrawing_Default)
  Box(10, 90, 40, 40, RGB(50, 60, 70))
  DrawingMode(#PB_2DDrawing_Transparent)
  DrawText(15, 95, "A", RGB(0, 0, 0), RGB(9, 9, 9))
  Debug Point(15, 95)
StopDrawing()

; A new StartDrawing resets DrawingMode back to Default.
StartDrawing(CanvasOutput(1))
  Box(10, 10, 20, 20, RGB(8, 8, 8))
  Debug Point(15, 15)
StopDrawing()

Debug "done"
