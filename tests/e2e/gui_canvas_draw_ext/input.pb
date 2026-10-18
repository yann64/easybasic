; Exercises the 2D Drawing library extension: Line/LineXY/DrawText/
; TextWidth/TextHeight. Oracle-verified entirely via plain Debug
; Point(...) read-backs after drawing, the same proven methodology used
; for Box/Circle/Plot - no synthetic input needed. See pbLine's/
; pbTextWidth's/pbDrawText's own doc comments in guilib.hpp for the full
; set of findings this is based on: Line is delta-based and draws
; nothing at all for a zero Width or Height (a real, reproduced PB
; quirk); LineXY uses two absolute endpoints and has no such quirk;
; both clip silently off-canvas, defaulting their own trailing Color to
; the ambient FrontColor; DrawText's own FrontColor/BackColor instead
; independently default to black, not the ambient color, and paint an
; opaque background over the text's own TextWidth x TextHeight box.
OpenWindow(0, 10, 10, 320, 320, "Test")
CanvasGadget(1, 10, 10, 200, 150)

StartDrawing(CanvasOutput(1))
  ; Line's own zero-delta quirk - draws nothing at all.
  Line(10, 10, 20, 0, RGB(1, 2, 3))
  Debug Point(15, 10)

  ; Line is delta-based, and defaults to the ambient FrontColor.
  FrontColor(RGB(9, 99, 199))
  Line(50, 50, 30, 30)
  Debug Point(50, 50)

  ; LineXY: absolute endpoints, no zero-delta quirk, explicit Color.
  LineXY(100, 100, 150, 100, RGB(5, 15, 25))
  Debug Point(125, 100)
  Debug Point(150, 100)

  ; Both clip silently off-canvas.
  Line(190, 140, 50, 50, RGB(1, 1, 1))
  LineXY(0, 149, -1000, 1000, RGB(2, 2, 2))
  Debug Point(195, 145)
  Debug Point(0, 149)

  ; DrawText/TextWidth/TextHeight.
  Debug TextWidth("")
  Debug TextHeight("")
  w = TextWidth("A")
  h = TextHeight("A")
  If w > 0 : Debug 1 : Else : Debug 0 : EndIf
  If h > 0 : Debug 1 : Else : Debug 0 : EndIf
  If TextWidth("AB") > w : Debug 1 : Else : Debug 0 : EndIf

  Box(50, 150 - 40, 40, 40, RGB(200, 200, 200))
  DrawText(50, 150 - 40, "A")
  Debug Point(50, 150 - 40)

  DrawText(100, 150 - 40, "A", RGB(255, 0, 0), RGB(0, 0, 255))
  Debug Point(100, 150 - 40)
StopDrawing()

Debug "done"
