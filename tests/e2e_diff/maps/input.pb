; Exercises NewMap: direct name(key) access (auto-creating on read/write,
; both moving the cursor), AddMapElement (always resets to zero, even for
; an existing key), FindMapElement, DeleteMapElement (both 1-arg cursor and
; 2-arg by-key forms), ClearMap, MapSize, MapKey, ForEach, and a Map of
; Structures.
NewMap ages.i()
ages("alice") = 30
ages("bob") = 25
Debug ages("alice")
Debug ages("bob")

ForEach ages()
  Debug MapKey(ages())
  Debug ages()
Next
Debug MapSize(ages())

NewMap m.i()
AddMapElement(m(), "x")
m() = 10
Debug m("x")

If FindMapElement(m(), "x")
  Debug "found x"
EndIf
If FindMapElement(m(), "y")
  Debug "found y"
Else
  Debug "no y"
EndIf

m("y") = 20
AddMapElement(m(), "x")
Debug m("x")

FindMapElement(m(), "y")
DeleteMapElement(m())
Debug MapSize(m())
Debug FindMapElement(m(), "y")

DeleteMapElement(m(), "x")
Debug MapSize(m())

ClearMap(m())
Debug MapSize(m())

Structure Point
  x.i
  y.i
EndStructure

NewMap pts.Point()
pts("a")\x = 1
pts("a")\y = 2
Debug pts("a")\x
Debug pts("a")\y
