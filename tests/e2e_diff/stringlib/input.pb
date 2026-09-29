; Exercises the M4a String library: Len/Left/Right/Mid (UTF-16-code-unit-
; aware, verified via Chr()-constructed Unicode text - PB's own compiler
; does not decode multi-byte UTF-8 literals embedded directly in a source
; file, so that path is deliberately not exercised here), UCase/LCase,
; Trim/LTrim/RTrim, Str/Val, StrF/ValF, Chr/Asc.
s.s = "Hello, World!"
Debug Len(s)
Debug Left(s, 5)
Debug Right(s, 6)
Debug Mid(s, 8, 5)
Debug Mid(s, 8)
Debug UCase(s)
Debug LCase(s)

Debug Len(Left(s, 100))
Debug Len(Right(s, 100))
Debug Left(s, 0)
Debug Mid(s, 1, 100)

t.s = "  hi there  "
Debug "[" + Trim(t) + "]"
Debug "[" + LTrim(t) + "]"
Debug "[" + RTrim(t) + "]"

Debug Str(42)
Debug Str(-17)
Debug Str(2.5)
Debug Str(3.5)
Debug Val("123")
Debug Val("  45abc")
Debug Val("notanumber")

Debug StrF(3.14159, 2)
Debug ValF("3.14")

Debug Chr(65)
Debug Asc("A")
Debug Asc("")

; Chr()-constructed Unicode (PB's own internal representation is correct
; here, unlike a raw non-ASCII byte embedded directly in the source file).
u.s = "caf" + Chr(233)
Debug Len(u)
Debug Right(u, 1)
