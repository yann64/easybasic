; Exercises M7d: DeclareModule/Module/EndModule/EndDeclareModule, Module::
; Member qualified access, and UseModule/UnuseModule. Oracle-verified
; against the official PureBasic help's own "Ferrari" and "common module"
; (Voitures) worked examples (docs/architecture/roadmap.md's M7d notes).
;
; First part (the "Ferrari" example): a DeclareModule's public interface
; (one Declare'd procedure) paired with a Module's own private
; implementation (a private Init() procedure and a private Global,
; invisible from outside - oracle-verified: accessing Ferrari::Init()
; directly is a real compile error, not attempted here since this is a
; positive-path diff test, not an error-case one) plus the public
; CreateFerrari() procedure that calls Init() internally. Confirms
; qualified access (Ferrari::CreateFerrari()) and, after UseModule,
; unqualified access both reach the same procedure, and that the private
; Init() guard logic only runs its "first call" branch once across both.
DeclareModule Ferrari
  Declare CreateFerrari()
EndDeclareModule

Module Ferrari
  Global Initialized = 0
  Procedure Init()
    If Initialized = 0
      Initialized = 1
      Debug "InitFerrari()"
    EndIf
  EndProcedure
  Procedure CreateFerrari()
    Init()
    Debug "CreateFerrari()"
  EndProcedure
EndModule

Procedure Init()
  Debug "main Init()"
EndProcedure

Init()
Ferrari::CreateFerrari()
UseModule Ferrari
CreateFerrari()
UnuseModule Ferrari

; Second part (the "common module" example): a module with no procedures
; of its own, existing purely to hold a shared public Global two other
; modules each UseModule internally and mutate - confirms a Module's own
; top-level body can contain real executable code (not just declarations),
; running at its own textual position, and that UseModule works correctly
; from *inside* another module's own body, scoped to it.
DeclareModule Voitures
  Global NbVoitures = 0
EndDeclareModule
Module Voitures
EndModule

DeclareModule Porsche
EndDeclareModule
Module Porsche
  UseModule Voitures
  NbVoitures = NbVoitures + 1
EndModule

Debug Voitures::NbVoitures
