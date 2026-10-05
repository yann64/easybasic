; Exercises M7d's own final remaining piece: qualified `Module::Macro()`
; invocation, teaching the separate MacroExpander pass (a pure token-level
; preprocessing stage, running before Module/DeclareModule constructs even
; have AST shape) its own independent copy of Sema's module-namespacing
; rules. Oracle-verified end to end: a Macro declared inside DeclareModule
; is public (qualified access from outside, and UseModule'd unqualified
; access, both work); a Macro declared only inside Module is private
; (qualified access from outside is a real, fatal "Module item 'X' is not
; declared as public." error); two different modules' own same-named
; macros stay genuinely independent; and a macro used unqualified, purely
; from inside its own defining module, still works exactly as it did
; before this slice (the M7d third-slice finding this work follows up on).
DeclareModule Shapes
  Macro Square(x)
    (x) * (x)
  EndMacro
  Declare.d ComputeArea(side.d)
EndDeclareModule

Module Shapes
  Procedure.d ComputeArea(side.d)
    ProcedureReturn Square(side)
  EndProcedure
EndModule

DeclareModule Other
  Macro Square(x)
    (x) * (x) * (x)
  EndMacro
  Declare.d ComputeCube(side.d)
EndDeclareModule

Module Other
  Procedure.d ComputeCube(side.d)
    ProcedureReturn Square(side)
  EndProcedure
EndModule

; Unqualified, same-module use (works since the first M7d third-slice
; finding): ComputeArea's own body calls Square unqualified.
Debug Shapes::ComputeArea(4)
Debug Other::ComputeCube(3)

; Qualified invocation from outside both modules.
Debug Shapes::Square(5)
Debug Other::Square(5)

; UseModule brings the public macro into unqualified scope.
UseModule Shapes
Debug Square(6)
UnuseModule Shapes
