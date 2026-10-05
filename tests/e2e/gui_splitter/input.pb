; Exercises M7b's eleventh GUI slice: SplitterGadget - unlike Container/
; Panel (the ninth/tenth slices), this isn't a gadget-list nesting type
; at all: it reparents two *already-existing* gadgets into itself. Also
; exercises GetGadgetAttribute/SetGadgetAttribute, two brand new, generic,
; dispatch-based functions this slice introduces, scoped to Splitter's
; own four attributes for now.
;
; Deliberately avoids anything requiring a simulated click (xdotool) -
; this gadget has no documented EventType() support at all (oracle-
; verified by its own absence from SplitterGadget.html's own remarks),
; so only return-value and accessor round-trips are exercised here,
; the same deterministic split every other GUI slice's own e2e case
; already uses.
OpenWindow(0, 10, 10, 320, 320, "Test")

ButtonGadget(1, 0, 0, 0, 0, "B1")
ButtonGadget(2, 0, 0, 0, 0, "B2")

; #PB_Splitter_Vertical means the divider itself is vertical (panes
; side by side, split by width) - oracle-verified directly, not assumed
; from the flag's own name.
Debug SplitterGadget(10, 10, 10, 300, 200, 1, 2, #PB_Splitter_Vertical)
Debug GetGadgetState(10) ; half of width (300/2=150)

ButtonGadget(3, 0, 0, 0, 0, "B3")
ButtonGadget(4, 0, 0, 0, 0, "B4")
Debug SplitterGadget(11, 10, 220, 300, 80, 3, 4) ; no flag - split by height
Debug GetGadgetState(11) ; half of height (80/2=40)

SetGadgetState(10, 77)
Debug GetGadgetState(10)

Debug GetGadgetAttribute(10, #PB_Splitter_FirstGadget)
Debug GetGadgetAttribute(10, #PB_Splitter_SecondGadget)

SetGadgetAttribute(10, #PB_Splitter_FirstMinimumSize, 42)
Debug GetGadgetAttribute(10, #PB_Splitter_FirstMinimumSize)
SetGadgetAttribute(10, #PB_Splitter_SecondMinimumSize, 24)
Debug GetGadgetAttribute(10, #PB_Splitter_SecondMinimumSize)

; FirstFixed keeps the first pane's own size unchanged when the
; Splitter itself is resized - oracle-verified directly.
ButtonGadget(5, 0, 0, 0, 0, "B5")
ButtonGadget(6, 0, 0, 0, 0, "B6")
SplitterGadget(12, 10, 10, 300, 200, 5, 6, #PB_Splitter_Vertical | #PB_Splitter_FirstFixed)
Debug GetGadgetState(12)
ResizeGadget(12, 10, 10, 400, 200)
Debug GetGadgetState(12)

; Replacing a pane's own gadget doesn't free the old one - it's
; reparented back onto the window, not destroyed.
ButtonGadget(7, 0, 0, 0, 0, "B7")
Debug SetGadgetAttribute(10, #PB_Splitter_FirstGadget, 7)
Debug GetGadgetAttribute(10, #PB_Splitter_FirstGadget)
Debug IsGadget(1) ; old gadget, still alive
Debug IsGadget(7) ; new gadget, now in the splitter

; Harmless failures: an unsupported attribute, and a non-Splitter
; gadget - oracle-verified each is a harmless 0, not a crash. A
; genuinely *unknown* #Gadget (to either GetGadgetAttribute/
; SetGadgetAttribute, or SplitterGadget's own #Gadget1/#Gadget2) is
; instead a real, fatal debugger error in real PB itself (not
; replicated by this project - see pbSplitterGadget's own doc comment -
; and left out of this golden case specifically *because* it's fatal in
; the oracle; both are covered by dedicated Catch2 unit tests instead).
Debug GetGadgetAttribute(10, 99)
Debug GetGadgetAttribute(1, #PB_Splitter_FirstGadget)
Debug SetGadgetAttribute(10, 99, 1)
