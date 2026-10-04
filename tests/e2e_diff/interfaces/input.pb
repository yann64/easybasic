; Exercises M7c: Interface/EndInterface's manually-built vtable dispatch,
; and its own prerequisite, ?Label (the compile-time address of a
; DataSection label). Oracle-verified end to end (docs/architecture/
; roadmap.md's M7c notes): a Structure whose first field holds a vtable
; address (?Label, written there explicitly - no automatic wiring, no
; compile-time conformance checking between the Structure and the
; Interface at all, exactly like a hand-written C vtable), an
; Interface-typed pointer that reinterprets the Structure's own address,
; and \Method(args) calls that dispatch through the vtable by the method's
; declared position. Two implementing "classes" (Circle/Square) confirm
; dispatch is genuinely polymorphic (driven by the vtable each pointer
; targets, not any static type), and a parameterized, no-return-value
; method (Scale) exercises a plain argument passed through the vtable call
; alongside the implicit "this".
;
; Deliberately uses a plain `.i` field (`VTable.i`) rather than real PB's
; own idiomatic `*VTable` (untyped pointer field) - oracle-verified
; behaviorally identical for this purpose (both are just an 8-byte integer
; written at the Structure's own first-field byte offset); pointer-typed
; Structure fields themselves are a separate, unimplemented gap, not
; needed for this feature and deliberately not rushed in as a side effect
; of it.
;
; Deliberately avoids StrF()/string concatenation (plain separate Debug
; statements for the name and the numeric area instead) - a real, pre-
; existing, unrelated StrF trailing-zero-formatting divergence from real PB
; (confirmed via a standalone StrF probe, nothing to do with Interface/
; ?Label) would otherwise fail this diff on an unrelated stdlib gap.
Interface Shape
  Area.d()
  Scale(factor.d)
  Name.s()
EndInterface

Structure CircleData
  VTable.i
  radius.d
EndStructure

Structure SquareData
  VTable.i
  side.d
EndStructure

Procedure.d Circle_Area(*this.CircleData)
  ProcedureReturn 3.14159 * *this\radius * *this\radius
EndProcedure
Procedure Circle_Scale(*this.CircleData, factor.d)
  *this\radius = *this\radius * factor
EndProcedure
Procedure.s Circle_Name(*this.CircleData)
  ProcedureReturn "Circle"
EndProcedure

Procedure.d Square_Area(*this.SquareData)
  ProcedureReturn *this\side * *this\side
EndProcedure
Procedure Square_Scale(*this.SquareData, factor.d)
  *this\side = *this\side * factor
EndProcedure
Procedure.s Square_Name(*this.SquareData)
  ProcedureReturn "Square"
EndProcedure

DataSection
  CircleVTable:
  Data.i @Circle_Area()
  Data.i @Circle_Scale()
  Data.i @Circle_Name()
  SquareVTable:
  Data.i @Square_Area()
  Data.i @Square_Scale()
  Data.i @Square_Name()
EndDataSection

Define c.CircleData
c\VTable = ?CircleVTable
c\radius = 5

Define s.SquareData
s\VTable = ?SquareVTable
s\side = 4

Define *shape1.Shape = @c
Define *shape2.Shape = @s

Debug *shape1\Name()
Debug *shape1\Area()
*shape1\Scale(2)
Debug *shape1\Area()

Debug *shape2\Name()
Debug *shape2\Area()
*shape2\Scale(3)
Debug *shape2\Area()
