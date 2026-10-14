; Exercises `Bool(condition)` - the one documented, oracle-verified way to
; turn a comparison (or an And/Or/Not combination of them) into a plain
; Integer 0/1 usable anywhere a value is expected, unlike a *bare*
; comparison, which real PB restricts to If/While/Until conditions only
; (oracle-verified: a bare `Debug (a = b)` is a real, named compile error -
; "Comparisons (=, <, >, =< and >=) are only supported with keywords like
; If, While, Until or within Bool().").
Procedure.i Echo(n.i)
  ProcedureReturn n
EndProcedure

a = 5
b = 5
c = 6

; As a plain Debug argument - the motivating case.
Debug Bool(a = b)
Debug Bool(a = c)
Debug Bool(a < c)
Debug Bool(a > c)

; Non-comparison argument - PB's standard nonzero-is-true rule still
; applies inside Bool() too.
Debug Bool(5)
Debug Bool(0)
Debug Bool(-1)

; And/Or/Not combinations, the same grammar an If/While condition uses.
Debug Bool(a = b And a = 5)
Debug Bool(a = 999 Or a = b)
Debug Bool(Not a = b)

; As a function call argument.
Debug Echo(Bool(a = b))
Debug Echo(Bool(a = c))

; As a plain assignment's RHS, including into a Float destination (no
; float-ness propagates into Bool()'s own always-Integer 0/1 result).
x = Bool(a = b)
Debug x
Define f.d = Bool(a = b) + 0.5
Debug f

; Composes normally with surrounding arithmetic/string conversion.
Debug Str(Bool(a = b)) + "-" + Str(Bool(a = c))

; Still legal (if redundant) directly inside an actual condition too.
If Bool(a = b)
  Debug "if-true"
EndIf
