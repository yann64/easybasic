; Exercises the M4d File library: CreateFile/WriteString/WriteStringN,
; ReadFile/ReadString/Eof, FileSize, OpenFile (read+write in place,
; positioned at the start), FileSeek/Loc/Lof, DeleteFile, RenameFile, and a
; variable (not just literal) file number. Uses plain relative filenames
; (not an absolute /tmp/... path - that isn't a real path on Windows, a
; real portability bug this project's own first-ever Windows CI run (M6)
; caught) so the test runner's own per-test scratch working directory
; (see run_case.sh/diff_against_pbcompilerc.sh) is where these land, and
; cleans up after itself so the test is idempotent across repeated runs.
DeleteFile("easybasic_filelib_test_a.txt")
DeleteFile("easybasic_filelib_test_b.txt")

n.i = 5
If CreateFile(n, "easybasic_filelib_test_a.txt")
  WriteStringN(n, "Hello, World!")
  WriteStringN(n, "Second line")
  CloseFile(n)
EndIf

If ReadFile(n, "easybasic_filelib_test_a.txt")
  While Not Eof(n)
    Debug ReadString(n)
  Wend
  CloseFile(n)
EndIf

Debug FileSize("easybasic_filelib_test_a.txt")
Debug FileSize("easybasic_nonexistent_xyz.txt")

If CreateFile(0, "easybasic_filelib_test_b.txt")
  WriteString(0, "0123456789")
  Debug Loc(0)
  FileSeek(0, 3)
  Debug Loc(0)
  WriteString(0, "X")
  Debug Lof(0)
  CloseFile(0)
EndIf

If OpenFile(0, "easybasic_filelib_test_b.txt")
  Debug Loc(0)
  Debug ReadString(0)
  FileSeek(0, 2)
  WriteString(0, "ZZ")
  CloseFile(0)
EndIf

If ReadFile(0, "easybasic_filelib_test_b.txt")
  Debug ReadString(0)
  CloseFile(0)
EndIf

RenameFile("easybasic_filelib_test_b.txt", "easybasic_filelib_test_c.txt")
Debug FileSize("easybasic_filelib_test_b.txt")
Debug FileSize("easybasic_filelib_test_c.txt")

DeleteFile("easybasic_filelib_test_a.txt")
DeleteFile("easybasic_filelib_test_c.txt")
Debug FileSize("easybasic_filelib_test_a.txt")
Debug FileSize("easybasic_filelib_test_c.txt")
