; Exercises the M4d File library: CreateFile/WriteString/WriteStringN,
; ReadFile/ReadString/Eof, FileSize, OpenFile (read+write in place,
; positioned at the start), FileSeek/Loc/Lof, DeleteFile, RenameFile, and a
; variable (not just literal) file number. Uses /tmp paths and cleans up
; after itself so the test is idempotent across repeated runs.
DeleteFile("/tmp/easybasic_filelib_test_a.txt")
DeleteFile("/tmp/easybasic_filelib_test_b.txt")

n.i = 5
If CreateFile(n, "/tmp/easybasic_filelib_test_a.txt")
  WriteStringN(n, "Hello, World!")
  WriteStringN(n, "Second line")
  CloseFile(n)
EndIf

If ReadFile(n, "/tmp/easybasic_filelib_test_a.txt")
  While Not Eof(n)
    Debug ReadString(n)
  Wend
  CloseFile(n)
EndIf

Debug FileSize("/tmp/easybasic_filelib_test_a.txt")
Debug FileSize("/tmp/easybasic_nonexistent_xyz.txt")

If CreateFile(0, "/tmp/easybasic_filelib_test_b.txt")
  WriteString(0, "0123456789")
  Debug Loc(0)
  FileSeek(0, 3)
  Debug Loc(0)
  WriteString(0, "X")
  Debug Lof(0)
  CloseFile(0)
EndIf

If OpenFile(0, "/tmp/easybasic_filelib_test_b.txt")
  Debug Loc(0)
  Debug ReadString(0)
  FileSeek(0, 2)
  WriteString(0, "ZZ")
  CloseFile(0)
EndIf

If ReadFile(0, "/tmp/easybasic_filelib_test_b.txt")
  Debug ReadString(0)
  CloseFile(0)
EndIf

RenameFile("/tmp/easybasic_filelib_test_b.txt", "/tmp/easybasic_filelib_test_c.txt")
Debug FileSize("/tmp/easybasic_filelib_test_b.txt")
Debug FileSize("/tmp/easybasic_filelib_test_c.txt")

DeleteFile("/tmp/easybasic_filelib_test_a.txt")
DeleteFile("/tmp/easybasic_filelib_test_c.txt")
Debug FileSize("/tmp/easybasic_filelib_test_a.txt")
Debug FileSize("/tmp/easybasic_filelib_test_c.txt")
