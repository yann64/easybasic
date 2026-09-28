; Exercises M0's arithmetic subset and PB's oracle-verified conversion rules:
; `/` is target-typed (integer division with no Float destination in sight,
; real division once one is), `%` binds tighter than `*`/`/`, and storing a
; float into an integer rounds half-to-even rather than truncating.
Define a.i = 7
Define b.i = 2
Debug a / b ; no Float destination anywhere -> plain integer division -> 3
Debug 2 * 3 % 4
Define rounded.i = 2.5
Debug rounded
Define name.s = "PB"
Define greeting.s = "Hello, " + name
Debug greeting

; Proves the Double destination really did force real division (3.5) rather
; than integer division (3): banker's rounding of 3.5 gives 4, of 3 gives 3 -
; printing an Integer sidesteps float Debug formatting, which pbcxx doesn't
; yet replicate exactly (see docs/architecture/roadmap.md's M0 notes).
Define ratio.d = a / b
Define displayable.i = ratio
Debug displayable
