; Exercises WindowX/WindowY/WindowWidth/WindowHeight, GetWindowTitle/
; SetWindowTitle, GetWindowData/SetWindowData, GetActiveWindow/
; SetActiveWindow, GetWindowState/SetWindowState, and GetWindowColor/
; SetWindowColor. Oracle-verified entirely via plain Debug probes - see
; guilib.hpp's own pbWindowX/pbGetActiveWindow/pbGetWindowState/
; pbGetWindowColor doc comments for the full set of findings this is
; based on. DisableWindow is deliberately NOT exercised here - its own
; real return value is oracle-confirmed to carry no consistent signal
; (an inconsistent small number across repeated calls), a documented
; divergence covered in the unit tests instead, not safe for a byte-
; exact golden comparison. GetActiveWindow/GetWindowState are both
; oracle-confirmed to stay unchanged regardless of Set calls first,
; specifically because this project's own headless Xvfb test
; environment has no window manager to ever actually grant focus or
; honor a maximize/iconify request - not a guess, the real, correct
; GTK mechanism behaves this way in this same environment for both the
; oracle and this implementation.
Debug "--- WindowX/Y/Width/Height ---"
OpenWindow(0, 50, 60, 300, 200, "Original")
Debug WindowX(0)
Debug WindowY(0)
Debug WindowWidth(0)
Debug WindowHeight(0)
Debug WindowX(0, #PB_Window_InnerCoordinate)

Debug "--- GetWindowTitle/SetWindowTitle ---"
Debug GetWindowTitle(0)
SetWindowTitle(0, "Changed")
Debug GetWindowTitle(0)

Debug "--- GetWindowData/SetWindowData ---"
Debug GetWindowData(0)
SetWindowData(0, 12345)
Debug GetWindowData(0)

Debug "--- GetActiveWindow/SetActiveWindow ---"
Debug GetActiveWindow()
SetActiveWindow(0)
Debug GetActiveWindow()

Debug "--- GetWindowState/SetWindowState ---"
Debug GetWindowState(0)
SetWindowState(0, #PB_Window_Maximize)
Debug GetWindowState(0)
SetWindowState(0, #PB_Window_Minimize)
Debug GetWindowState(0)
SetWindowState(0, #PB_Window_Normal)
Debug GetWindowState(0)

Debug "--- GetWindowColor/SetWindowColor ---"
Debug GetWindowColor(0)
SetWindowColor(0, RGB(10, 20, 30))
Debug GetWindowColor(0)

Debug "done"
