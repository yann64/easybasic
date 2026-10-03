; Exercises M5a: CompilerIf/CompilerElseIf/CompilerElse/CompilerEndIf and
; CompilerSelect/CompilerCase/CompilerDefault/CompilerEndSelect, the
; #PB_Compiler_OS/#PB_OS_*/#PB_Compiler_Processor/#PB_Processor_* constants,
; and a user-defined #Constant used as a CompilerIf condition.
;
; Deliberately platform-independent (unlike the e2e_diff variant of this
; same test, which instead compares the actual detected OS/Processor
; against the live oracle on whatever machine it runs on): a real
; platform-dependent CompilerIf branch here is only checked for the
; invariant "exactly one of the known OS/Processor values matches" (true on
; every real platform), never by printing a specific name - printing the
; literal "linux"/"windows" would make this golden test's own fixed
; expected.stdout wrong whenever it's compiled on a *different* platform.
; This was a real gap this project's own first-ever Windows CI run (M6)
; caught directly: it runs this exact suite on real Windows, which quite
; literally does select the #PB_OS_Windows branch there, not a
; hypothetical concern.
#MyFlag = 1

isKnownOS.i = 0
CompilerIf #PB_Compiler_OS = #PB_OS_Windows
  isKnownOS = 1
CompilerElseIf #PB_Compiler_OS = #PB_OS_Linux
  isKnownOS = 1
CompilerElseIf #PB_Compiler_OS = #PB_OS_MacOS
  isKnownOS = 1
CompilerEndIf
Debug isKnownOS

isKnownProcessor.i = 0
CompilerSelect #PB_Compiler_Processor
  CompilerCase #PB_Processor_x86
    isKnownProcessor = 1
  CompilerCase #PB_Processor_x64
    isKnownProcessor = 1
  CompilerDefault
    isKnownProcessor = 0
CompilerEndSelect
Debug isKnownProcessor

CompilerIf #MyFlag = 1
  Debug "flag set"
CompilerElse
  Debug "flag not set"
CompilerEndIf

; Oracle-verified: a non-taken CompilerIf/CompilerSelect branch is never
; even type-checked - ThisIsNotARealFunctionAtAll() below would be a hard
; error if Sema ever visited it, since no such procedure exists anywhere.
; Uses a condition that's false on every real platform (not tied to one
; specific OS this test might actually be compiled on), so it stays
; portable.
CompilerIf #PB_Compiler_OS = 99999
  ThisIsNotARealFunctionAtAll()
CompilerEndIf

Procedure Foo()
  ProcedureReturn 42
EndProcedure

; Oracle-verified: a Procedure declared inside a *selected* CompilerIf
; branch is legal (unlike inside a runtime If, which Sema rejects). Uses a
; condition that's always true, so this works on every platform.
CompilerIf 1 = 1
  Procedure InsideCompilerIf()
    ProcedureReturn 99
  EndProcedure
CompilerEndIf

Debug Foo()
Debug InsideCompilerIf()
