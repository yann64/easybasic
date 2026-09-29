; Exercises Structure/EndStructure: field access, nesting, String fields,
; and arrays of Structures.
Structure Point
  x.i
  y.i
EndStructure

Define p.Point
p\x = 3
p\y = 4
Debug p\x
Debug p\y

Structure Rect
  topLeft.Point
  w.i
  h.i
EndStructure

Define r.Rect
r\topLeft\x = 1
r\topLeft\y = 2
r\w = 10
Debug r\topLeft\x
Debug r\topLeft\y
Debug r\w

Structure Person
  name.s
  age.i
EndStructure

Define person.Person
person\name = "Alice"
person\age = 30
Debug person\name
Debug person\age

Dim points.Point(2)
points(0)\x = 100
points(1)\y = 200
points(2)\x = points(0)\x + 5
Debug points(0)\x
Debug points(1)\y
Debug points(2)\x
