; Exercises the 2D Drawing library extension: Ellipse/RoundBox/
; FillArea. Oracle-verified entirely via plain Debug Point(...)
; read-backs after drawing, the same proven methodology used for
; Box/Circle/Plot/Line/LineXY - no synthetic input needed. See
; pbEllipse's/pbRoundBox's/pbFillArea's own doc comments in guilib.hpp
; for the full set of findings this is based on: Ellipse is a filled
; disc (like Circle) whose exact boundary pixel at x +/- xRadius/
; y +/- yRadius is itself still filled, with a perfectly crisp edge (no
; anti-aliased blend at all); RoundBox cuts its own 4 corners away by a
; quarter-ellipse of the given corner dimensions, including in the
; degenerate case where a corner dimension is exactly half the box's
; own width/height; both default their own trailing Color to the
; ambient FrontColor and clip silently off-canvas. FillArea is a
; raster-level boundary fill - FillArea(x, y, BorderColor [,
; FillColor]) - replacing every reachable pixel regardless of its own
; original color, stopping only at a pixel already matching
; BorderColor, with FillColor defaulting to the ambient FrontColor.
OpenWindow(0, 10, 10, 320, 320, "Test")
CanvasGadget(1, 10, 10, 200, 150)

StartDrawing(CanvasOutput(1))
  ; Ellipse: exact axis-aligned boundary, crisp edge, FrontColor default,
  ; silent off-canvas clipping.
  Ellipse(50, 50, 30, 20, RGB(1, 2, 3))
  Debug Point(50, 50)
  Debug Point(80, 50)
  Debug Point(81, 50)
  Debug Point(50, 70)
  Debug Point(50, 71)
  Debug Point(10, 10)

  FrontColor(RGB(11, 12, 13))
  Ellipse(150, 50, 20, 20)
  Debug Point(150, 50)

  Debug Ellipse(1000, 1000, 10, 10, RGB(2, 2, 2))

  ; RoundBox: corners cut away, flat edges filled, including the
  ; degenerate case where CornerHeight is exactly half the own Height.
  RoundBox(10, 100, 40, 20, 10, 10, RGB(4, 5, 6))
  Debug Point(30, 110)
  Debug Point(10, 100)
  Debug Point(49, 119)
  Debug Point(25, 100)
  Debug Point(10, 110)
  Debug Point(50, 100)

  FrontColor(RGB(21, 22, 23))
  RoundBox(70, 100, 40, 20, 10, 10)
  Debug Point(90, 110)

  Debug RoundBox(1000, 1000, 10, 10, 2, 2, RGB(3, 3, 3))

  ; FillArea: boundary fill, not a start-color-matching flood fill.
  Box(100, 10, 50, 50, RGB(0, 0, 0))
  Box(110, 20, 20, 20, RGB(255, 255, 255))
  Debug FillArea(115, 25, RGB(0, 0, 0), RGB(9, 8, 7))
  Debug Point(115, 25)
  Debug Point(129, 39)
  Debug Point(105, 15)
  Debug Point(10, 140)

  FrontColor(RGB(31, 32, 33))
  FillArea(115, 25, RGB(0, 0, 0))
  Debug Point(115, 25)
StopDrawing()

Debug "done"
