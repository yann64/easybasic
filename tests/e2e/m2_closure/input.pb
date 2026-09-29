; Exercises the M2 items originally deferred to M3, closed as a follow-up:
; Declare (mutual recursion / forward calls), a constant declared inside a
; runtime-conditional block (purely textual/compile-time, independent of
; whether the branch actually runs), and one inside a Procedure's own body.
Declare IsOdd(n.i)

Procedure IsEven(n.i)
  If n = 0
    ProcedureReturn 1
  EndIf
  ProcedureReturn IsOdd(n - 1)
EndProcedure

Procedure IsOdd(n.i)
  If n = 0
    ProcedureReturn 0
  EndIf
  ProcedureReturn IsEven(n - 1)
EndProcedure

Debug IsEven(10)
Debug IsOdd(10)

a.i = 0
If a = 1
  #X = 5
EndIf
Debug #X

Procedure GetMyY()
  #Y = 42
  ProcedureReturn #Y
EndProcedure
Debug GetMyY()
