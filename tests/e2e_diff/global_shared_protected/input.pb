; Exercises Global (auto-visible in every procedure, no Shared needed),
; Shared (opts a procedure into an otherwise-invisible top-level Define'd
; variable), and Protected (behaves like an ordinary local Define).
Global g.i = 10

Procedure BumpGlobalNoShared()
  g = g + 1
EndProcedure

Define d.i = 100

Procedure BumpSharedDefine()
  Shared d
  d = d + 1
EndProcedure

Procedure UseProtected()
  Protected p.i = 1
  p = p + 1
  Debug p
EndProcedure

BumpGlobalNoShared()
Debug g

BumpSharedDefine()
Debug d

UseProtected()
UseProtected()
