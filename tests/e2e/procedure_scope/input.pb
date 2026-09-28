; Procedures get a completely isolated local scope by default (oracle-
; verified: reading a same-named outer variable from inside a procedure
; gets a fresh local defaulting to 0, NOT the outer value).
Define outer.i = 99

Procedure ReadOuter()
  Debug outer
EndProcedure

ReadOuter()
Debug outer
