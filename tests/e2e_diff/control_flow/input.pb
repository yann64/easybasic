; Exercises If/ElseIf/Else, For/Next with Step (incl. negative), While/Wend,
; Repeat/Until, Repeat/ForEver+Break, Select/Case/Default, and Continue.
Define x.i = 5
If x = 5
  Debug "five"
ElseIf x = 6
  Debug "six"
Else
  Debug "other"
EndIf

For i.i = 1 To 5
  Debug i
Next

For j.i = 10 To 2 Step -2
  Debug j
Next

Define total.i = 0
Define n.i = 1
While n <= 5
  If n = 3
    n = n + 1
    Continue
  EndIf
  total = total + n
  n = n + 1
Wend
Debug total

Define count.i = 0
Repeat
  count = count + 1
Until count >= 3
Debug count

Define k.i = 0
Repeat
  k = k + 1
  If k = 4
    Break
  EndIf
ForEver
Debug k

Define day.i = 3
Select day
  Case 1
    Debug "Mon"
  Case 2, 3
    Debug "Tue-or-Wed"
  Default
    Debug "other-day"
EndSelect
