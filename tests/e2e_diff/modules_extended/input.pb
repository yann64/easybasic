; Exercises M7d's second slice: Structures, Enumerations, constants, Dim
; arrays, NewList/NewMap, and DataSection declared inside a Module/
; DeclareModule section - extending the first slice's own Procedure/Global
; namespacing to every other declaration kind the oracle accepts there
; (Macro and Interface remain deliberately out of scope - see
; docs/architecture/roadmap.md's M7d notes). Oracle-verified end to end,
; each piece checked in isolation first, then combined here.
DeclareModule Geo
  Declare MakePoint()
  Structure Point
    x.i
    y.i
  EndStructure
EndDeclareModule

Module Geo
  Procedure MakePoint()
    Define p.Point
    p\x = 1
    p\y = 2
    Debug p\x
  EndProcedure
EndModule

Geo::MakePoint()
Define q.Geo::Point
q\x = 5
Debug q\x

DeclareModule Colors
  #MyRed = 11
  Enumeration
    #MyBlue
    #MyGreen
  EndEnumeration
EndDeclareModule
Module Colors
EndModule

Debug Colors::#MyRed
Debug Colors::#MyBlue
Debug Colors::#MyGreen
UseModule Colors
Debug #MyRed

DeclareModule Data1
  Dim Items(2)
EndDeclareModule
Module Data1
  Items(0) = 10
  Items(1) = 20
EndModule

Debug Data1::Items(0)
UseModule Data1
Debug Items(1)

DeclareModule Data2
  NewList Elements.i()
  NewMap Lookup.s()
EndDeclareModule
Module Data2
  AddElement(Elements())
  Elements() = 42
  Lookup("k") = "v"
EndModule

Debug Data2::Elements()
ForEach Data2::Elements()
  Debug "iter"
Next
Debug Data2::Lookup("k")

DeclareModule Dat
  DataSection
    MyLabel:
    Data.i 1, 2, 3
  EndDataSection
EndDeclareModule
Module Dat
EndModule

Restore Dat::MyLabel
Read.i v1
Debug v1
