; Exercises #Name constant declarations and Enumeration auto-increment.
; Names deliberately avoid PureBasic's own built-in constants (e.g. #PI,
; #Red are already reserved - oracle-verified: redeclaring them errors).
#MYPI = 3
#GREETING = "hi"
Debug #MYPI
Debug #GREETING

Enumeration
  #MyRed
  #MyGreen
  #MyBlue
EndEnumeration
Debug #MyRed
Debug #MyGreen
Debug #MyBlue

Enumeration
  #FirstCode = 100
  #SecondCode
  #ThirdCode
EndEnumeration
Debug #FirstCode
Debug #SecondCode
Debug #ThirdCode
