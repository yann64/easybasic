; Exercises M5c: Macro/EndMacro. `Square`'s body is deliberately
; unparenthesized (`x*x`, not `(x)*(x)`) to exercise the single most
; important oracle finding this slice turned up: Macro parameters are
; substituted as raw, unparenthesized TEXT, not a pre-evaluated value -
; Square(2+3) expands to the literal tokens `2+3*2+3`, which is `11` under
; normal operator precedence, not `25` (a classic C-preprocessor-style
; "forgot the parens" result, confirmed byte-for-byte against the oracle).
; `Greet` is a zero-parameter macro, invoked *bare* with no parens at all
; (oracle-verified: `Greet()` is a syntax error, unlike a zero-arg
; Procedure call). `Outer`/`Inner` exercise one macro invoking another
; (nonrecursive), and `PrintBoth` exercises a multi-statement macro body.
Macro Square(x)
  x*x
EndMacro

Macro Greet
  Debug "hello"
EndMacro

Macro Inner(b)
  b * 10
EndMacro

Macro Outer(a)
  Inner(a) + 1
EndMacro

Macro PrintBoth(a, b)
  Debug a
  Debug b
EndMacro

Debug Square(2+3)
Greet
Debug Outer(3)
PrintBoth(1, 2)
PrintBoth("x", "y")
