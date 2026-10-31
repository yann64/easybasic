; Exercises WindowResolutionX/WindowResolutionY/WindowScaledX/
; WindowScaledY/WindowUnscaledX/WindowUnscaledY. Oracle-verified
; entirely via plain Debug probes - see guilib.hpp's own
; pbWindowResolutionX doc comment for the full set of findings this is
; based on: these return a real Double (not an Integer - confirmed by
; assigning to typed variables and observing which decimal formatting
; survives), computed from the window's own real screen DPI relative
; to the traditional 96 DPI baseline - naturally an identity (1.0/no
; change) here, not because it's hardcoded that way, but because this
; headless test environment's own screen is already at that standard
; 96 DPI baseline, the same real reason the oracle's own output is an
; identity too.
OpenWindow(0, 10, 10, 200, 200, "W")
Debug WindowResolutionX(0)
Debug WindowResolutionY(0)
r.d = WindowResolutionX(0)
Debug r.d
r2.f = WindowResolutionX(0)
Debug r2.f

Debug WindowScaledX(0, 100)
Debug WindowScaledY(0, 100)
Debug WindowUnscaledX(0, 100)
Debug WindowUnscaledY(0, 100)
Debug WindowScaledX(0, -50)
Debug WindowUnscaledX(0, -50)

Debug "done"
