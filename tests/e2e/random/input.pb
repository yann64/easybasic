; Random/RandomSeed: this project's own PRNG never produces the same
; sequence as real PB's undocumented one (see docs/architecture/roadmap.md's
; M4b notes), so this checks this implementation's own internal determinism
; (same seed -> same sequence) and range bounds, not exact values - which
; both real PB and pbcxx satisfy identically, so it works as an e2e_diff
; test too even though the underlying random numbers themselves differ.
RandomSeed(42)
a.i = Random(100)
b.i = Random(100)
RandomSeed(42)
c.i = Random(100)
If a = c
  Debug 1
Else
  Debug 0
EndIf
If a >= 0 And a <= 100
  Debug 1
Else
  Debug 0
EndIf
If b >= 0 And b <= 100
  Debug 1
Else
  Debug 0
EndIf

RandomSeed(1)
d.i = Random(10, 5)
If d >= 5 And d <= 10
  Debug 1
Else
  Debug 0
EndIf
