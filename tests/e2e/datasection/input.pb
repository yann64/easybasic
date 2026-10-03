; Exercises M5b: DataSection/Data/Read/Restore. Covers: a Data value that's
; a constant expression (#Two + 1), multiple DataSections concatenating
; into one flat pool (no Restore needed at the boundary), Restore jumping
; back to an earlier label, Restore *forward-referencing* a label defined
; later in the file, and Read continuing from the shared global cursor
; across separate Procedure calls. Every Read here matches its Data item's
; own declared type (s1/s2 are pre-declared .s before Read.s, since a bare
; Read.s into a *fresh* variable would auto-declare it Integer instead -
; oracle-verified, see ast::ReadStmt's own doc comment) - a deliberately
; type-mismatched Read desyncs real PB's own raw-byte cursor in a way this
; project's own pool model doesn't replicate bit-for-bit (documented
; divergence, see docs/architecture/roadmap.md's M5b notes), so this test
; only exercises the well-typed, intended usage.
#Two = 2

DataSection
  Block1:
  Data.l 1, #Two + 1
  Data.s "hello", "world"
EndDataSection

DataSection
  Block2:
  Data.l 100, 200
EndDataSection

Define s1.s
Define s2.s
Read.l a
Read.l b
Read.s s1
Read.s s2
Debug a
Debug b
Debug s1
Debug s2

Restore Block1
Read.l d
Debug d

Restore Block2
Read.l e
Read.l f
Debug e
Debug f

Procedure UseData()
  Define v.l
  Read.l v
  Debug v
EndProcedure
Restore Block2
UseData()
UseData()

Restore ForwardLabel
Read.l g
Debug g

DataSection
  ForwardLabel:
  Data.l 999
EndDataSection
