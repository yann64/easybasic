; Exercises static Dim arrays: 1D and 2D, indexed read/write, string
; elements, and a runtime (non-literal) dimension size.
Dim arr.i(4)
arr(0) = 10
arr(4) = 50
arr(2) = arr(0) + arr(4)
Debug arr(0)
Debug arr(2)
Debug arr(4)

Dim names.s(2)
names(0) = "a"
names(1) = "b"
Debug names(0)
Debug names(1)
Debug names(2)

Dim grid.i(2, 2)
grid(1, 1) = 99
grid(0, 2) = 7
Debug grid(1, 1)
Debug grid(0, 0)
Debug grid(0, 2)

Define n.i = 3
Dim dynamic.i(n)
dynamic(3) = 42
Debug dynamic(3)
