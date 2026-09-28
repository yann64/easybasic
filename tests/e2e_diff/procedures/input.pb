; Exercises Procedure/ProcedureReturn (incl. .s/$ return forms), recursion,
; default parameter values, by-value parameter passing, fallthrough-
; returns-zero-value semantics, and a call used as a whole statement.
Procedure.i Add(a.i, b.i)
  ProcedureReturn a + b
EndProcedure

Procedure.i Fact(n.i)
  If n <= 1
    ProcedureReturn 1
  EndIf
  ProcedureReturn n * Fact(n - 1)
EndProcedure

Procedure$ Greet(name.s)
  ProcedureReturn "Hello, " + name
EndProcedure

Procedure.i DefaultArg(a.i, b.i = 100)
  ProcedureReturn a + b
EndProcedure

Procedure ModifyInt(a.i)
  a = 999
EndProcedure

Procedure DoubleAndPrint(a.i)
  Debug a * 2
EndProcedure

Procedure.i NoExplicitReturn(a.i)
  If a > 0
    ProcedureReturn 1
  EndIf
EndProcedure

Debug Add(3, 4)
Debug Fact(5)
Debug Greet("World")
Debug DefaultArg(1)
Debug DefaultArg(1, 2)

Define x.i = 5
ModifyInt(x)
Debug x

DoubleAndPrint(10)

Debug NoExplicitReturn(-5)
Debug NoExplicitReturn(5)
