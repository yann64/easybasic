; Exercises M7d's third slice: `Interface`/`EndInterface` declared inside a
; `DeclareModule`/`Module`, the one remaining declaration kind the second
; slice left deliberately deferred. Mirrors tests/e2e_diff/interfaces's own
; polymorphic Circle/Square dispatch, but with the Interface itself (and
; the Structures implementing it) declared inside a Module - confirming
; the exact same mangling/qualified-access/UseModule machinery the second
; slice already built for Structure/Enumeration/etc. extends to Interface
; with no behavioral surprises.
;
; `?Label` is used from *inside* a Procedure here (each module-scoped
; "Init" procedure wires up its own vtable field this way) - oracle-
; verified this is the *only* working idiom for a module's own vtable:
; neither `?Module::Label` nor `UseModule` + unqualified `?Label` give a
; module's own DataSection label any cross-module visibility in real PB at
; all (confirmed directly - both are rejected, "Garbage at the end of the
; line"/"Label not found"), unlike every *other* module member kind this
; project supports. So the vtable field can only ever be wired up by code
; textually inside the same module - this project's own general "?Label
; inside a Procedure" fix (a real, pre-existing gap unrelated to modules,
; also covered by tests/e2e_diff/interfaces's own extended case) is a
; genuine prerequisite for Interface-in-Module to be usable at all, not an
; independent nicety.
DeclareModule Shapes
  Interface Shape
    Area.d()
    Name.s()
  EndInterface

  Structure CircleImpl
    VTable.i
    radius.d
  EndStructure

  Structure SquareImpl
    VTable.i
    side.d
  EndStructure

  Declare InitCircle(addr.i, r.d)
  Declare InitSquare(addr.i, side.d)
EndDeclareModule

Module Shapes
  Procedure.d CircleArea(*self.CircleImpl)
    ProcedureReturn 3.14159 * *self\radius * *self\radius
  EndProcedure
  Procedure.s CircleName(*self.CircleImpl)
    ProcedureReturn "Circle"
  EndProcedure

  Procedure.d SquareArea(*self.SquareImpl)
    ProcedureReturn *self\side * *self\side
  EndProcedure
  Procedure.s SquareName(*self.SquareImpl)
    ProcedureReturn "Square"
  EndProcedure

  DataSection
    CircleVT:
    Data.i @CircleArea()
    Data.i @CircleName()
    SquareVT:
    Data.i @SquareArea()
    Data.i @SquareName()
  EndDataSection

  Procedure InitCircle(addr.i, r.d)
    Define *c.CircleImpl = addr
    *c\VTable = ?CircleVT
    *c\radius = r
  EndProcedure

  Procedure InitSquare(addr.i, side.d)
    Define *s.SquareImpl = addr
    *s\VTable = ?SquareVT
    *s\side = side
  EndProcedure
EndModule

; Qualified access from outside, no UseModule.
Define c.Shapes::CircleImpl
Shapes::InitCircle(@c, 5)
Define *shape1.Shapes::Shape = @c

Define sq.Shapes::SquareImpl
Shapes::InitSquare(@sq, 4)
Define *shape2.Shapes::Shape = @sq

Debug *shape1\Name()
Debug *shape1\Area()
Debug *shape2\Name()
Debug *shape2\Area()

; UseModule, unqualified access.
UseModule Shapes
Define c2.CircleImpl
InitCircle(@c2, 2)
Define *shape3.Shape = @c2
Debug *shape3\Name()
Debug *shape3\Area()
