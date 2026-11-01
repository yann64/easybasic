; Exercises StickyWindow and WindowBounds. Both are purely visual/WM-
; level with no real Debug-observable effect in real PB itself (no
; IsStickyWindow accessor exists at all, and WindowBounds is oracle-
; confirmed to have no observable effect under this project's own
; headless Xvfb test environment either way - see guilib.hpp's own
; pbStickyWindow/pbWindowBounds doc comments). This golden case only
; confirms both are accepted without error/crash and carry their own
; established "Aucune" (no real return value) convention - the more
; interesting finding (WindowBounds must NOT clamp a later
; ResizeWindow the way the obvious real GTK mapping was found to) is
; covered in the unit tests instead, which can reliably wait out the
; real GTK event-loop latency a plain PB script has no equivalent for.
OpenWindow(0, 10, 10, 200, 200, "W")
Debug StickyWindow(0, 1)
Debug StickyWindow(0, 0)
Debug WindowBounds(0, 300, 300, 500, 500)
Debug "done"
