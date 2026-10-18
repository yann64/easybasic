; Exercises M7b's twenty-first GUI slice: IPAddressGadget.
;
; SetGadgetText parses text with the real character-by-character field-
; entry rule, not a simple split-and-validate - oracle-verified directly:
; a digit that would push the active field over 255 or past 3 characters
; is dropped (not the whole field); a '.' only advances to the next
; field if the current one already holds a digit; any other junk
; character is simply ignored. GetGadgetState packs all 4 octets as a
; sign-extended, little-endian 32-bit value (octet1 the low byte, octet4
; the high one). SetGadgetState always shows all 4 octets, unlike a
; partially-typed SetGadgetText (0 displays "0.0.0.0", not "").
;
; Takes no Flags at all - oracle-verified (a 6-argument call is a real
; compile error), so there's nothing else to exercise here.
OpenWindow(0, 10, 10, 320, 320, "Test")

Debug IPAddressGadget(1, 10, 10, 200, 25)
Debug "[" + GetGadgetText(1) + "]"
Debug GetGadgetState(1)

SetGadgetText(1, "192.168.1.42")
Debug GetGadgetText(1)
Debug GetGadgetState(1)

SetGadgetText(1, "1.2.3.4.5")
Debug GetGadgetText(1)

SetGadgetText(1, "1.300.3.4")
Debug GetGadgetText(1)

SetGadgetText(1, "1.abc.3.4")
Debug GetGadgetText(1)
Debug GetGadgetState(1)

SetGadgetText(1, "1.2.3")
Debug GetGadgetText(1)
Debug GetGadgetState(1)

SetGadgetState(1, 0)
Debug "[" + GetGadgetText(1) + "]"
Debug GetGadgetState(1)

SetGadgetState(1, -1)
Debug GetGadgetText(1)
Debug GetGadgetState(1)
