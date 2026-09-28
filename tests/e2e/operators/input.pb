; Exercises the oracle-verified flat bitwise tier (%, &, |, !, <<, >>),
; comparisons, and And/Or (also flat, left-to-right - NOT nested). XOr is
; deliberately not exercised here (see docs/architecture/roadmap.md's M1
; notes on its unresolved runtime anomaly).
Debug 2 * 3 % 4        ; % binds tighter than * -> 2*(3%4) = 6
Debug 2 << 1 + 1        ; << binds tighter than + -> (2<<1)+1 = 5
Debug 12 & 1 << 2       ; << binds tighter than & -> 12&(1<<2) = 4
Debug 6 | 3 & 1         ; | binds tighter than & -> (6|3)&1 = 1
Debug 6 ! 3 & 1         ; ! (xor) binds tighter than & -> (6!3)&1 = 1
Debug ~2 * 3            ; unary ~ binds tightest -> (-3)*3 = -9

Define a.i = 5
Define b.i = 7
If a < b
  Debug "a<b"
EndIf
If a <> b
  Debug "a<>b"
EndIf
If a = 5 And b = 7
  Debug "and-true"
EndIf
If a = 1 Or b = 7
  Debug "or-true"
EndIf
If Not a = 1
  Debug "not-true"
EndIf
; And/Or are a flat left-to-right tier, NOT nested (oracle-verified) -
; `False Or True And False` = (False Or True) And False = False.
If 0 Or 1 And 0
  Debug "unreachable"
Else
  Debug "flat-and-or-confirmed"
EndIf
