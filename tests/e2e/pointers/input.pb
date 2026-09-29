; Exercises pointers: *Var/@Var declaration, \field dereference through a
; Structure-typed pointer, AllocateStructure/FreeStructure,
; AllocateMemory/FreeMemory, and a pointer procedure parameter mutating the
; caller's own Structure.
Structure Point
  x.i
  y.i
EndStructure

Define p.Point
p\x = 1
p\y = 2
Define *pp.Point = @p
Debug *pp\x
*pp\y = 20
Debug p\y

Define *sp.Point = AllocateStructure(Point)
*sp\x = 7
*sp\y = 8
Debug *sp\x
Debug *sp\y
FreeStructure(*sp)

Define *mem = AllocateMemory(8)
FreeMemory(*mem)

Procedure SetX(*target.Point, newX.i)
  *target\x = newX
EndProcedure

SetX(@p, 99)
Debug p\x
