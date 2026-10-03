; Exercises M5a: CompilerIf/CompilerElseIf/CompilerElse/CompilerEndIf and
; CompilerSelect/CompilerCase/CompilerDefault/CompilerEndSelect, the
; #PB_Compiler_OS/#PB_OS_*/#PB_Compiler_Processor/#PB_Processor_* constants,
; and a user-defined #Constant used as a CompilerIf condition. Oracle-
; verified on this Linux/x64 machine: #PB_Compiler_OS=2 (Linux),
; #PB_Compiler_Processor=4 (x64) - this test's expected output is therefore
; Linux/x64-specific, matching how the rest of this suite is Linux-x64-first
; (see docs/architecture/roadmap.md's M5 notes).
;
; Also verifies two oracle-confirmed facts central to this feature's design:
; (1) a non-taken CompilerIf/CompilerSelect branch is never even type-
;     checked - ThisIsNotARealFunctionAtAll() below would be a hard error if
;     Sema ever visited it, since no such procedure exists anywhere;
; (2) a Procedure declared inside a *selected* CompilerIf branch is legal
;     (unlike inside a runtime If, which Sema rejects) - InsideCompilerIf()
;     below is defined that way and called normally afterward.
#MyFlag = 1

CompilerIf #PB_Compiler_OS = #PB_OS_Windows
  Debug "windows"
CompilerElseIf #PB_Compiler_OS = #PB_OS_Linux
  Debug "linux"
CompilerElse
  Debug "other"
CompilerEndIf

CompilerSelect #PB_Compiler_Processor
  CompilerCase #PB_Processor_x86
    Debug "x86"
  CompilerCase #PB_Processor_x64
    Debug "x64"
  CompilerDefault
    Debug "unknown"
CompilerEndSelect

CompilerIf #MyFlag = 1
  Debug "flag set"
CompilerElse
  Debug "flag not set"
CompilerEndIf

CompilerIf #PB_Compiler_OS = #PB_OS_Windows
  ThisIsNotARealFunctionAtAll()
CompilerEndIf

Procedure Foo()
  ProcedureReturn 42
EndProcedure

CompilerIf #PB_Compiler_OS = #PB_OS_Linux
  Procedure InsideCompilerIf()
    ProcedureReturn 99
  EndProcedure
CompilerEndIf

Debug Foo()
Debug InsideCompilerIf()
