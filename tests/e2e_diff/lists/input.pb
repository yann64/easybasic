; Exercises NewList: AddElement/InsertElement/DeleteElement cursor
; semantics, ForEach, FirstElement/LastElement/NextElement/PreviousElement,
; ListSize, SelectElement/ListIndex, ClearList, and a List of Structures.
NewList names.s()
AddElement(names())
names() = "Alice"
AddElement(names())
names() = "Bob"
AddElement(names())
names() = "Carol"

ForEach names()
  Debug names()
Next

Debug ListSize(names())
FirstElement(names())
Debug names()
LastElement(names())
Debug names()

NewList n.i()
AddElement(n()) : n() = 1
AddElement(n()) : n() = 2
AddElement(n()) : n() = 3
SelectElement(n(), 1)
Debug n()
Debug ListIndex(n())

InsertElement(n())
n() = 99
ForEach n()
  Debug n()
Next

DeleteElement(n())
ForEach n()
  Debug n()
Next

ClearList(n())
Debug ListSize(n())
Debug FirstElement(n())

Structure Point
  x.i
  y.i
EndStructure

NewList pts.Point()
AddElement(pts())
pts()\x = 1
pts()\y = 2
AddElement(pts())
pts()\x = 3
pts()\y = 4
ForEach pts()
  Debug pts()\x
  Debug pts()\y
Next
