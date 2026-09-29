; Exercises the M4b Math library: Abs/Sqr/Pow (always Double, oracle-
; verified, regardless of argument type), trig/exp/log (radians), Round's
; three #PB_Round_* modes (half-away-from-zero / floor / ceil - distinct
; from this project's usual banker's-rounding conversion rule), and Int
; (truncates toward zero, distinct from Round's floor). Random/RandomSeed
; are exercised separately (tests/e2e only - this project's own PRNG never
; produces the same sequence as real PB's, so no differential test can
; assert exact values).
Debug Abs(-5)
Debug Abs(5)
Debug Abs(-5.5)
Debug Sqr(16)
Debug Sqr(2)
Debug Pow(2, 10)
Debug Pow(2.0, 0.5)

Debug Int(3.7)
Debug Int(-3.7)

Debug Round(3.5, #PB_Round_Nearest)
Debug Round(2.5, #PB_Round_Nearest)
Debug Round(-2.5, #PB_Round_Nearest)
Debug Round(3.9, #PB_Round_Down)
Debug Round(3.1, #PB_Round_Up)
Debug Round(-3.1, #PB_Round_Down)

Debug Sin(0)
Debug Cos(0)
Debug ATan2(1, 1)
Debug Exp(1)
Debug Log(2.718281828)
Debug Log10(100)
