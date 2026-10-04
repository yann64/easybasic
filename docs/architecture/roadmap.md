# easybasic — Compiler Architecture & Roadmap

## Context

easybasic transpiles PureBasic source to C++, then hands that C++ to a real backend
compiler (g++/clang++) to produce native binaries on Linux, Haiku, and Windows. The
architecture mirrors the user's own existing, complete FreeBASIC-to-C++ transpiler,
`eBasic` (`~/git/cpp/eBasic`), adapted throughout for PureBasic's own, different grammar
and semantics rather than FreeBASIC's.

Two things make this project unusually tractable for a from-scratch language
implementation, and both are used throughout: (1) PureBasic 6.41's own "C Backend"
compiler, `pbcompilerc`, is installed and works as a live semantic oracle -
`pbcompilerc <file>.pb -k` for a fast syntax check, `pbcompilerc <file>.pb -c -o <bin>`
to inspect the official `purebasic.c` lowering of any construct (see
`docs/developer/oracle-testing.md`); (2) `eBasic`'s proven pipeline/CMake/CI/Doxygen
conventions are reused directly rather than re-invented.

GUI/3D PureBasic support is explicitly and deliberately out of scope for the whole
project (not just deferred) - it dominates the official example corpus by file count but
is cleanly separable, and would need its own separate, much larger subsystem effort.

## Milestone table

| Milestone | Scope | Status |
|---|---|---|
| **M0** | Repo/CMake/CI skeleton; minimal lexer+parser+codegen for `Define`, `Debug`, integer/float/string literals, assignment, `+ - * / %`; trivial Sema; `PBString` skeleton | Done (see M0 notes below) |
| **M1** | All 11 type suffixes; oracle-derived operator/precedence table; `If/Select/For/While/Repeat`; `EnableExplicit`; `#`-constants/`Enumeration` | Done (see M1 notes below) |
| **M2** | `Procedure`/`ProcedureReturn` (incl. `.s`/`$` return forms), by-value parameters with defaults, recursion, isolated per-procedure scope | Done (see M2 notes below) - `Global`/`Shared`/`Protected` cross-scope access and static `Dim` arrays deferred to and closed by M3a/M3b; mutual recursion/`Declare`, call-argument type-checking, and constants in nested blocks deferred further, closed by a dedicated M2-closure pass (see its own notes) |
| **M3** | `Structure`, pointers, `NewList`/`NewMap` families, static `Dim` arrays, `Global`/`Shared`/`Protected` | Done - `Global`/`Shared`/`Protected` (M3a), static `Dim` arrays (M3b), `Structure` (M3c), pointers (M3d), `NewList` (M3e), and `NewMap` (M3f) all land |
| **M4** | Core stdlib: String, Math, Memory, File, Date | Done - core String (M4a), Math (M4b), Memory (M4c), File (M4d), and Date (M4e) libraries all land |
| **M5** | `CompilerIf`/`CompilerSelect` + `#PB_*` constants, `DataSection`, non-recursive `Macro` | Done - `CompilerIf`/`CompilerSelect` + `#PB_*` constants (M5a), `DataSection`/`Data`/`Read`/`Restore` (M5b), non-recursive `Macro` (M5c) |
| **M6** | Cross-platform CI (Windows/Haiku via qemu), clang-tidy/cppcheck gates, ASan/UBSan, nightly Valgrind | Done - linux-gcc/linux-clang/ASan+UBSan/clang-tidy+cppcheck/windows-mingw/haiku all green on real GitHub Actions CI (the first time this project's CI, written since M0, ever actually ran - see its own notes), plus a nightly Valgrind job verified via manual dispatch |
| **M7a** | Threads (`CreateThread`/`WaitThread`/`IsThread`/`KillThread`, `Mutex`, `Semaphore`) | Done - see M7a notes (`KillThread`/`PauseThread`/`ResumeThread`/`ThreadID` deliberately deferred) |
| **M7b** | GUI core on GTK3 (`Window`/`Event`/`Gadget`/`Requester`, phased - see notes) | First four slices done (Window + event core, basic gadgets, `MessageRequester`, `Menu`/`StatusBar` - see M7b notes); `ToolBar` (blocked on an Image library - see the fourth slice's own notes) and everything past that still open |
| **M7c** | `Interface`/`EndInterface` (needs `?Label` address-of-DataSection-label first) | Done - see M7c notes |
| **M7d** | `Module`/`DeclareModule`/`EndModule` | Not started - scoped, see M7 scoping notes |

## M0 Implementation Notes

**Scope landed**: repo skeleton (CMake + presets for `linux-gcc`/`linux-clang`/
`linux-clang-sanitize`/`windows-mingw`/`haiku`), the `pbcxx_frontend` static library
(Diagnostics + Lexer + Parser, split out from day one per the architecture doc below),
`pbcxx` itself (Sema + Codegen + driver), the `easybasic_runtime` header-only library
(`PBString` + `debugPrint`), Catch2 unit tests, and the golden-e2e / differential-e2e
test harnesses.

**Oracle-verified facts baked into M0's design** (see `docs/developer/oracle-testing.md`
for the general methodology):

- **No line continuation exists at all** in PB (neither trailing `_` nor `...` compiles)
  - the Lexer has no continuation-token state machine at all.
- **`Debug` compiles to nothing in a plain (non-debug) build.** Verified with
  `pbcompilerc`: a plain build of a program whose only content is `Debug "..."` strips
  it to a bare comment in the generated C, emitting no call at all. `pbcompilerc -d`
  both makes it print AND resolves an unrelated, pre-existing linker bug in this PB
  install (`libpbmimalloc.a` referencing an undefined `__wrap_memcpy` - only reproduces
  for a program whose *only* content is a bare `Debug` statement; any program using an
  actual library dependency links fine without `-d`). `pbcxx` reproduces the "no `-d`,
  no code" behavior exactly (see `ast::DebugStmt`'s own doc comment and `Codegen::genStmt`),
  and prints byte-for-byte the same `"[Debugger]  <value>"\n` line `pbcompilerc -d` does
  when `-d` is passed to `pbcxx` too - this is what makes the golden e2e suite's output
  checkable, and what makes the differential-against-`pbcompilerc` suite meaningful at
  all (`scripts/diff_against_pbcompilerc.sh` always passes `-d` to both compilers).
- **No-suffix variables default to Integer.** `Define x` / `x = 5` with no `.suffix`
  declares an `Integer` (`.i`), oracle-verified (`static integer v_x` in the generated
  C). `pbcxx`'s Sema mirrors this, including PB's implicit-declaration behavior (a plain
  assignment to an unseen name declares it, rather than erroring - `EnableExplicit`'s
  stricter behavior is M1's job).
- **`/` performs real (floating-point) division only when a Float/Double destination is
  in play** - **superseded by M1's finding below**: M0 originally (incorrectly) believed
  `/` was unconditionally real division; M1's differential e2e suite caught the actual
  rule (target-typed, propagating through the whole expression tree) the very first time
  a bare `Debug a/b` with two Integer operands was tested. See M1's notes for the full,
  corrected story - kept here only so this section's history stays honest.
- **Float-to-integer conversion rounds half-to-even (banker's rounding), not
  truncation.** Oracle-verified with `Define a.i = 2.5` / `3.5` / `-2.5` -> `2` / `4` /
  `-2` respectively (2 and 4 are the nearest *even* integers to their ties; -2 likewise).
  `Codegen::convert` uses `llrint()` (which follows the IEEE default round-to-nearest-
  even rounding mode) for every float-family-to-integer-family conversion, never a
  plain truncating `static_cast`.
- **`%` binds tighter than `*`/`/`.** Oracle-verified: `2 * 3 % 4` constant-folds to `6`
  (`2 * (3 % 4)`), not `2` (`(2*3) % 4`). The Parser gives `%` its own precedence tier
  between `parseMul` and `parseUnary` rather than sharing `*`/`/`'s tier.
- **`%` is ambiguous with a binary-literal prefix, resolved by lookahead.** `%1010` is
  the integer 10 (binary literal); `%` followed by anything else (e.g. whitespace) is
  the modulo operator - oracle-verified with `%101 % 2` (binary literal `5`, then modulo,
  giving `1`). The Lexer implements exactly this lookahead rule.
- **`$` is a hex-literal prefix** (`$1A` = 26, oracle-verified) - no ambiguity with any
  operator, unlike `%`.
- **`.` after an identifier is always a type-suffix marker, never member access** (PB
  uses `\` for that instead) - so the Lexer can commit to consuming a suffix without any
  further disambiguation once it sees `identifier.<suffix-letter>`.
- **The type-suffix table has 11 members**, one more than a naive port of BASIC-family
  conventions would assume: `.b`/`.a`/`.c`/`.w`/`.u`/`.l`/`.q`/`.f`/`.d`/`.s`/`.i`, where
  `.c` (Character) and `.u` (Unicode) are storage-identical (`unsigned short`) but
  presumably distinct at the PB source level - both map to `std::uint16_t` in `pbcxx`.

**Deliberately deferred past M0** (tracked, not forgotten): `EnableExplicit` (M1, so
implicit declaration is the only behavior for now); full operator/precedence table
beyond `+ - * / %` (M1, via a planned `scripts/pb_precedence_probe.sh`); PB's exact
`Str()`-style float-to-string formatting for `Debug` output (M4 - M0 uses a plain
`std::to_string`, which already happens to match PB's own 6-decimal-place default for
values like `3.5 -> 3.500000`, but that agreement isn't guaranteed for every value yet,
so `docs/reference/` should not claim it as a settled fact until the String library
milestone actually verifies it against the oracle systematically); string+numeric mixed
`+` (real PB likely rejects this the same way `pbcxx`'s Sema currently does via
`checkAssignable`'s family check, but that check is scoped to assignment targets today,
not general binary `+` operands - a real diagnostic for `"x" + 5` specifically is a
follow-up, not yet verified against the oracle).

## M1 Implementation Notes

**Scope landed**: all 11 type suffixes (already in place since M0), the full oracle-
derived operator/precedence table (comparisons, bitwise `& | ! ~ << >>`, logical
`And`/`Or`/`Not`), `If/ElseIf/Else/EndIf`, `Select/Case/Default/EndSelect`, `For/To/
Step/Next`, `While/Wend`, `Repeat/Until`/`Repeat/ForEver`, `Break`/`Continue`,
`EnableExplicit`, `#Name` constants, and `Enumeration`.

**The headline finding: PB's `/` (and `%`) is *target-typed*, not unconditionally real
division.** M0 assumed `/` always performs real division because `10/3` and `10.0/3.0`
both gave a real quotient in isolation. The very first differential e2e test written for
M1 (`Debug a / b` with two plain `.i` variables, no destination at all) immediately
failed: the real `pbcompilerc` printed `3` (plain integer division), while `pbcxx`
printed `3.500000`. Systematic oracle probing (see the reversal-testing methodology
below) established the real rule: **a Float/Double destination forces real division
through the *entire* initializer expression tree** - `Define g.d = 1 + 7/2` evaluates to
`4.5`, not `4`, proving the float-ness propagates recursively through nested `+`, not
just a shallow top-level check - **but the exact same expression with no Float
destination in sight (a bare `Debug 7/2`, or an Integer destination) performs plain
integer division.** `Sema::classify(expr, floatContext)` implements this by threading a
`floatContext` flag down through the whole tree from whatever statement is consuming the
expression (`Define`/`Assign`'s declared type; `Debug` has no destination, so it's always
un-forced); `Codegen::genExpr` threads the identical flag so generated code matches
node-for-node. `%` is assumed to follow the same target-typed rule by consistent
extrapolation only - the two oracle tests that could have distinguished it both landed on
values consistent with either interpretation (a real, acknowledged gap, not a verified
fact).

**A second, distinct bug the differential suite caught: the bitwise operators are TWO
separate flat tiers, not one.** The initial implementation treated `%`, `&`, `|`, `!`
(xor), `<<`, `>>` as a single flat left-to-right precedence tier, based on an incomplete
set of pairwise oracle tests. `tests/e2e/operators` (`12 & 1 << 2`) immediately exposed
the bug: `pbcxx` computed `0` (treating `&`/`<<` as one tier, left-to-right: `(12&1)<<2`),
the real compiler computed `4` (`12&(1<<2)`). The error traced back to a methodological
mistake: distinguishing genuine precedence from same-tier left-to-right chaining requires
testing **both orderings** of a pair - "A op1 B op2 C" *and* "A op2 B op1 C" - since
whichever operator is *positioned first* coincidentally matches the "genuinely tighter"
prediction too, making a single-direction test ambiguous. Several pairs had only been
tested in one direction. Redone properly (see `docs/developer/oracle-testing.md` for the
reusable methodology), the real structure is:

- Tightest: unary `-`, unary `~` (prefix)
- Flat tier A (left-to-right, verified via two-directional reversal for every pair):
  `%`, `!` (xor), `<<`, `>>`
- Flat tier B (left-to-right, separately verified): `&`, `|`
- Tier A is tighter than tier B in every direction tested (e.g. `&`/`|` never win against
  any tier-A operator regardless of which is positioned first)
- `*`, `/` (flat, standard)
- binary `+`, `-` (flat, standard)
- (conditions only, restricted exactly like comparisons) `Not`, then `And`/`Or` - also
  ONE FLAT tier, left-to-right, **not nested** (`0 Or 1 And 0` = `(0 Or 1) And 0` = false,
  not the `And`-binds-tighter result every other mainstream language would give)

**`XOr` remains unsupported.** Runtime-tested (not just constant-folded) truth tables for
`Bool(1 XOr 0)` etc. didn't match standard XOR, PB's own bitwise `!` (which *does* work
correctly as XOR), or any other consistent interpretation found. Rather than guess,
`Sema::visitCondition` reports an explicit error whenever logical `XOr` is used, and
`Codegen` never lowers it. This needs a dedicated follow-up investigation before landing.

**Other oracle-verified facts**: `Select` has no `CaseElse` (`Default` is the only form -
oracle-confirmed `CaseElse` is a syntax error); `Repeat`/`Until` loops *until* the
condition becomes true (the opposite sense of C++'s `do`/`while`, handled with an
explicit negation in Codegen); `Enumeration` members auto-increment from the *previous*
member's value (`Codegen` emits `k_next = k_previous + 1` in the generated C++ itself
rather than trying to constant-fold this in the compiler, which also makes an explicit
non-literal starting value work for free); redeclaring one of PB's own built-in constants
(`#PI`, `#Red`, etc.) is a real PB error - test programs use non-colliding names like
`#MYPI` for exactly this reason.

**Deliberately deferred past M1**: constants/`Enumeration` declared *inside* an
If/For/While/Repeat body are silently dropped by `Codegen::genGlobalConstants` (which
only walks top-level statements) - real PB code overwhelmingly declares these at module
scope, so this hasn't blocked anything yet, but it's a real, tracked gap; bitwise ops
(`&`/`|`/`!`/`<<`/`>>`) are assumed integer-only regardless of any enclosing Float
destination, not independently oracle-verified (every realistic PB program already uses
them on integers, so this is a reasonable assumption, not a confirmed fact); `Next`'s
optional trailing loop-variable name (`Next x`) is parsed but not cross-checked against
the actual loop variable.

## M2 Implementation Notes

**Scope landed**: `Procedure`/`ProcedureReturn` (including `.s`/`$` typed-return forms),
by-value parameters with default values, recursion, a call used either as a value
(`Debug Add(3,4)`) or as a whole statement (`DoSomething(1,2)`), and - the headline
finding - genuinely isolated per-procedure local scope.

**Two oracle-verified facts shaped the whole design**:

- **Parameters are by-value for every type, including String** - simpler than
  FreeBASIC's rule (which eBasic's own roadmap documents as "byval by default except
  String, which defaults byref"). Verified directly: a procedure mutating its own `.i`
  and `.s` parameters left the caller's variables of both types completely unchanged.
  This is the only parameter-passing mode M2 implements; by-reference access is what
  pointers (`*param`, M3) are for in PB, not a `ByRef` keyword (PB has none).
- **Procedures get a completely isolated local scope by default - they cannot see outer
  variables at all**, not even to read them. `Procedure ReadOuter() : Debug outer :
  EndProcedure` reading a same-named `outer.i = 99` declared at module scope printed
  `0`, not `99` - the procedure's `outer` is a brand-new, unrelated local. This is a
  stronger form of isolation than "shadowing" (there's no fallback to the outer scope at
  all) and made the Sema implementation simpler than initially expected: processing a
  procedure body is just a save-current-scope / swap-in-an-empty-one / restore-afterward
  operation (`Sema::visitStmt`'s `ProcedureDecl` case) - not a scope *stack*, since PB
  procedures never nest. `Global`/`Shared` (PB's explicit opt-*in* to cross-scope access)
  and `Protected` are deliberately deferred to M3 rather than guessed at, since a quick
  probe already turned up a real ordering constraint (`Shared g` inside a procedure
  textually *before* that name's `Global g.i = ...` declaration is a compile error) that
  deserves its own proper investigation rather than a rushed implementation.

**PB requires a procedure to be fully defined before any call to it - no forward
declarations or hoisting at all.** Oracle-verified: calling a procedure declared later in
the same file is a compile error ("`Later() is not a function, array, list, map or
macro`"), even though a procedure calling *itself* (recursion) works fine, since by the
time its own body is being compiled, its own signature is already known. This is
actually good news for the implementation: `Sema` registers a procedure's signature the
moment it's encountered (before visiting its body, so self-recursion resolves) and
`Codegen::genProcedures` emits each one as a real, complete C++ function in source
order - which, given PB's own restriction, automatically satisfies C++'s identical
"declared before use" requirement with no separate prototype-emission pass needed.
Mutual recursion (which *would* need forward declarations, via PB's `Declare` keyword)
is not yet supported - not independently verified to even need special handling beyond
what a future `Declare` implementation would provide.

**Falling off the end of a procedure without an explicit `ProcedureReturn` returns the
declared return type's zero value** (oracle-verified: `0` for an Integer-returning
procedure). This is undefined behavior for a non-`void` C++ function if left as-is, so
`Codegen::genProcedureDecl` always appends a trailing `return <zero-value>;` after a
procedure's body - mirroring eBasic's own identical, previously-battle-tested pattern
for exactly this hazard - regardless of whether every control-flow path already hit an
explicit `ProcedureReturn`.

**Default parameter values are real C++ default arguments** (`int64_t f_add(int64_t a,
int64_t b = 100)`), not filled in at every call site - the C++ compiler's own default-
argument mechanism does the work, so `Codegen`'s call-site logic needs no special case
for an omitted trailing argument at all.

**A real gap the switch-based dispatch design nearly hid**: before this milestone added
`-Wall -Wextra` to the top-level `CMakeLists.txt`, `Codegen::genStmt`/`genExpr`'s
switches over `ast::StmtKind`/`ExprKind` had no case at all for several of M2's new
node kinds for a short window during development, and the build **still succeeded with
zero warnings** - an unmatched `case` with no `default:` is well-defined C++ (control
just falls through to after the switch), not an error, so nothing short of `-Wswitch`
(bundled in `-Wall`) would have caught it. Worth remembering for every future milestone
that adds new AST node kinds: the switch-based dispatch pattern this project relies on
(see `docs/developer/architecture.md`) is only as safe as the warning flags checking it.

**Deliberately deferred past M2**: `Global`/`Shared`/`Protected` (see above); static
`Dim` arrays; mutual recursion / `Declare` forward declarations; any real type-checking
of call arguments beyond arity (a `String` passed where an `Integer` parameter is
declared is not yet flagged as its own diagnostic - `checkAssignable`-style validation
for call arguments is a follow-up); a constant or procedure declared *inside* an
If/For/While/Repeat body is unsupported by `Codegen` (same top-level-only limitation as
constants, see M1's notes) and is additionally not even fully specified by PB itself,
since procedures can't nest in the first place.

## M3a Implementation Notes (Global/Shared/Protected)

**Scope landed**: `Global` (module-scope variables automatically visible/writable from
every procedure), `Shared` (opts a specific procedure into an otherwise-invisible
top-level `Define`d - or `Global` - variable by name), and `Protected` (parses to the
same node as `Define`, since every case actually tested behaved identically to an
ordinary procedure-local `Define`).

**The key oracle-verified surprise: `Global` needs no `Shared` at all - it's
unconditionally visible everywhere.** Before testing this, the natural assumption
(carried over from `Shared`'s name) was that `Global` declares the variable but
`Shared` is still required inside each procedure that wants to touch it. Verified
otherwise directly: a procedure incrementing a `Global g.i` with no `Shared` declaration
in sight still mutated the real outer `g` (`10` -> `11`, visible from the caller
afterward) - `Shared` turned out to be for a *different* case entirely: opting into a
plain top-level `Define`'d variable (which M2 already established procedures cannot see
by default) using the *same* underlying storage, confirmed separately (a `Shared d`
inside a procedure incrementing a plain `Define d.i = 100` also correctly mutated the
outer `d`).

**This has a pleasant consequence for the implementation**: since every top-level
variable - `Define`'d or `Global`'d - already becomes a real, ordinary C++ file-scope
global in `pbcxx`'s generated output (`Codegen` has never distinguished PB-level
visibility rules at the storage level, only Sema enforces them), **`Global` and `Shared`
need zero `Codegen` changes** - only `Sema` needs to know which names a given procedure
body is allowed to *resolve* without triggering its usual "unshared name gets a fresh,
isolated local" behavior (M2's `declareImplicit`/scope-swap). Concretely:
`Sema::bringIntoScope` inserts the resolved name+type directly into the procedure's
already-swapped-in local `symbols_` map, but **deliberately never adds it to `order_`**
(the list `Codegen` turns into a function's own local variable declarations) - so
`Codegen` naturally just emits a direct `v_name` reference inside the procedure body,
which - now that global variable declarations are emitted *before* procedure
definitions in the generated C++ (reordered specifically for this) - correctly resolves
to the real, single, shared C++ global rather than shadowing it with a fresh
zero-initialized local. `Global`'s auto-visibility is implemented as a simple
pre-population of every currently-known `Global` name (tracked in `globalNames_`) into a
procedure's scope right after its parameters are declared (params take precedence -
never overwritten); `Shared name` triggers the identical `bringIntoScope` lookup against
the outer scope captured (as a raw, appropriately-scoped-lifetime pointer,
`outerScopeForShared_`) at the moment that specific procedure's body started being
visited.

**Deliberately not chased down**: the exact interaction between `Protected` and an
already-existing same-named `Global` (does `Protected x` inside a procedure create a
genuinely separate local that shadows a `Global x`, the way a plain `Define x` inside
that same procedure implicitly *would* under M2's model, since `Global` pre-population
only happens for names not already occupied by a parameter - not, as written, checked
against a `Protected`/`Define` collision specifically)? Every scenario actually tested
behaved identically for `Protected` and `Define`, and this edge case is rare enough in
real PB code that chasing it further wasn't worth the time against M3's remaining, much
larger scope (`Structure`, pointers, `NewList`/`NewMap`, static `Dim` arrays). A `Global`
declared *inside* a procedure body (as opposed to at module scope) is untested and not
specifically supported - PB itself may or may not even allow this; not yet investigated.

## M3b Implementation Notes (static `Dim` arrays)

**Scope landed**: 1D and 2D static arrays (`Dim arr.i(4)`, `Dim grid.i(2, 2)`), indexed
read and write, with sizes that can be arbitrary runtime expressions, not just
compile-time constants (oracle-verified: `Dim dynamic.i(n)` where `n` is a variable
works). Higher dimensions are a documented, deliberate gap - real PB code overwhelmingly
uses 1D/2D.

**The real parsing challenge: `name(args)` is genuinely ambiguous with a procedure call
until Sema resolves it.** `arr(0)` (an array read) and `Add(3, 4)` (a procedure call)
are exactly the same shape at the token level, and the parser has no symbol table to
consult - it runs strictly before Sema. Rather than trying to thread lookahead or
backtracking through the parser, both forms parse to the *same* `CallExpr` node, and
Sema decides what it actually means by checking its `arrays_` table before falling back
to `procedures_` (see `ArrayInfo`'s own doc comment). The equivalent problem exists for
*assignment* (`arr(i) = expr` vs. a call used as a bare statement, `DoThing(1, 2)`) -
`Parser::parseCallOrIndexAssignStatement` resolves it the same way procedurally: parse
the full `Name(args)` once via the existing call-parsing logic, then look at whether `=`
follows to decide whether to reinterpret the freshly-parsed `CallExpr` as an
`IndexAssignStmt` or keep it as a call-statement.

**Element access uses `std::vector::at()`, not `operator[]`** - a deliberate,
essentially-free memory-safety choice (this project's whole reason for existing over a
hand-rolled backend): an out-of-bounds PB array access now terminates via a well-defined
`std::out_of_range` exception (verified: `arr(10)` on a 5-element array aborts cleanly
with a clear message) rather than silently corrupting memory or invoking undefined
behavior the way a raw C array or `operator[]` would. This is exactly the kind of
correctness win the ASan/UBSan CI job (and Valgrind, once M6 lands it) exists to catch
even when `.at()` *isn't* used somewhere - but here it's caught for free, at zero
runtime cost beyond what bounds-checking always costs.

**2D arrays are one flat, row-major `std::vector`, not a vector-of-vectors** - one
allocation instead of one-per-row, with a hidden per-array `v_name_dim1` companion
global (set by the `Dim` statement itself) supplying the row stride for the index
arithmetic (`row * (dim1+1) + col`) both at the `Dim` site and at every subsequent
read/write. This trades a small amount of indexing arithmetic for meaningfully simpler
memory layout and fewer allocations - a reasonable choice for a first array
implementation, revisit only if a real workload needs true jagged/ragged 2D arrays
(which PB's own fixed-rectangular `Dim` semantics don't support anyway).

**`Dim` (re-)sizes and resets the array at its own statement position** (via
`std::vector::assign`, which resizes *and* fills with the element type's zero value in
one call), matching PB's own timing - the size expression is evaluated exactly when the
`Dim` statement executes, not at some earlier global-initialization point, since it may
depend on runtime values computed just before it (as in the `Dim dynamic.i(n)` example
above).

**Deliberately deferred past M3b**: 3+ dimensional arrays; `ReDim` (resizing an existing
array while preserving its contents - real PB has `ReDim`/`ReDim ... Preserve` as
distinct forms, neither modeled yet, though `std::vector::resize` vs. the `assign`-based
reset this milestone uses would be the natural building blocks); `ArraySize()` and other
array-introspection builtins, whose special `name()` (empty-parens) argument syntax is
its own small parsing wrinkle distinct from everything above, deferred alongside the
rest of the standard library to M4; passing an array to a procedure (PB's
`Array Name.type(N)` parameter syntax is a distinct feature from a plain scalar
parameter, not yet supported).

## M3c Implementation Notes (`Structure`)

**Scope landed**: `Structure`/`EndStructure` declarations with primitive and nested-
Structure fields, field access and assignment via `\` (arbitrarily deep chains),
Structure-typed variables (including as procedure-local `Define`s, with their own
isolated-scope treatment from M2 applying unchanged), and arrays of Structures
(`Dim points.Point(2)` then `points(0)\x`).

**The type system needed a real extension, done with minimal disruption.** Every
declaration site (`Define`, `Dim`, a Structure's own field list) previously only ever
carried a `TypeSuffix`. Rather than replacing that with a general "type" variant
everywhere (touching every existing call site across four milestones' worth of code), a
single new enumerator was added - `TypeSuffix::Struct` - meaning "look elsewhere for the
real type": the lexer now also captures a `.Name` annotation that isn't one of the 11
primitive letters as a `structSuffix` field on the token (oracle-verified: `Define
p.Point` and a Structure field naming another Structure, e.g. `topLeft.Point` inside
`Rect`, both use this), and every AST node that can carry a Structure type gained a
companion `structTypeName` string used only when its `suffix` is `Struct`. This kept
every *existing* primitive-only code path (M0-M3b) completely unchanged while adding
Structure support as a parallel, opt-in case.

**A single recursive resolver (`Sema::resolveType`) is the one place that understands
how field types chain together** - given any expression that denotes a storage location
(a plain variable, an array element, or a field-access chain however deeply nested), it
walks down to the root and re-resolves each field lookup on the way back up via
`resolveField`. This is what makes `r\topLeft\x` (a field of a field), `points(0)\x` (a
field of an array element), and a bare `p\x` all resolve correctly through the exact
same code path, and it's the single source of truth both `Sema::classify` (for the usual
numeric-family/target-typed-conversion machinery) and `Codegen` (to pick a field
assignment's real conversion target type) go through - no second, drifting notion of
"what type is this" exists anywhere else in the compiler.

**`name(args)` was already ambiguous between an array read and a procedure call (M3b);
adding `\field` chains needed the identical treatment applied one level up, for
*statements*.** `Parser::parseIdentifierStatement` now parses one unified base (a plain
name, or `Name(args)`) followed by zero or more `\field` segments, and only *afterward*
decides what kind of statement it actually is based on what follows: `=` with a field
chain present is a field assignment, `=` with array-index args is an element assignment,
`=` with neither is a plain assignment, and no `=` at all (with no field chain) is a call
used as a statement. All four share an identical prefix and genuinely cannot be told
apart any earlier - this single function replaced what had been two separate, narrower
parsing functions (`parseCallOrIndexAssignStatement` and the non-`#` half of
`parseAssignmentOrConstDecl`) from M3b.

**Structures are real C++ structs, emitted before anything that could be an instance of
one.** `Codegen::genStructures` walks `Sema::structureDeclarationOrder()` (already a
valid emission order for free, since PB requires a field's Structure type to be declared
*before* the Structure that uses it - the same declare-before-use rule enforced
everywhere else in this project) and emits a plain `struct s_<name> { ... };` per
declaration, before global variables, arrays, or procedures - any of which might need the
type already defined. Field access (`base\field`) lowers to plain C++ member access
(`(base).f_field`); no getter/setter machinery, no virtual dispatch (PB has no
inheritance at all, so there's nothing to dispatch).

**Deliberately rejected rather than silently mishandled**: assigning directly to a
Structure-typed field or array element as a whole (`r\topLeft = ...`, `points(0) =
...`) - PB's own field-assignment model only ever targets a *leaf* primitive field
(`r\topLeft\x = ...`), and silently treating a whole-struct target as if it had a
numeric type (which `familyOf(TypeSuffix::Struct)` would otherwise quietly do, via its
catch-all `default: IntegerFamily` case) would have let invalid assignments through
without any diagnostic at all - both `FieldAssignStmt` and `IndexAssignStmt`'s Sema
handling now explicitly check for this and report a real error instead.

**Deliberately deferred past M3c**: `StructureUnion`; array fields *within* a Structure
(a field like `data.i[10]`, distinct from a top-level `Dim`); fixed-length string fields
(PB's `name.s{20}` syntax, as opposed to a plain dynamic `.s` field, which already works
exactly like a top-level String variable); Structure-typed procedure parameters and
return values (procedures remain primitive-only for now); pointers to Structures
(`*ptr.Point`, `*ptr\field`) - the natural next increment, since the field-access
machinery this milestone built is what pointer dereferencing through a Structure will
reuse directly.

## M3d Implementation Notes (pointers)

**Scope landed**: `*Var`/`@Var` pointer declaration and address-of syntax, `\field`
dereference through a Structure-typed pointer (reusing M3c's field-access machinery
directly, as predicted above), pointer procedure parameters (mutating the caller's own
Structure through the pointer, oracle-verified), and the four allocation built-ins that
make pointers usable without waiting for the full Memory library (M4):
`AllocateMemory`/`FreeMemory`/`AllocateStructure`/`FreeStructure`.

**Oracle correction made mid-implementation: a pointer can only be *untyped* or
*Structure-typed*, never primitive.** The initial design (before checking the oracle)
assumed `Define *pa.i` was legal, dereferenced via a `\i`-style pseudo-field matching the
suffix letter. `pbcompilerc -k` rejected it outright: `Error: ... Native types can't be
used with pointers.` The real rule, confirmed empirically: `Define *pa` (untyped -
dereferenced via the Memory library's `Peek*`/`Poke*` functions, not yet implemented, so
`\field` on one is rejected by Sema with a clear diagnostic rather than silently
miscompiling) or `Define *pp.Point` (Structure-typed - dereferenced with `\field`,
exactly like a plain Structure variable) are the only two legal forms. This is a good
example of why this project treats the installed compiler as authoritative over a
plausible-sounding guess, even mid-implementation - the wrong assumption was caught by a
single `-k` syntax check before it shipped in the differential test suite, let alone in a
release.

**Pointer variables live in a genuinely separate namespace from same-named non-pointer
variables (oracle-verified), modeled with zero new plumbing.** Every existing Sema table
(`symbols_`, `pointerPointeeType_`, `varStructType_`, `order_`) is already keyed by a
plain string name - so a pointer's name/spelling simply carries a literal leading `*`
wherever it's declared or referenced (`Define *pa` produces the key `"*pa"`, distinct from
plain `"pa"`), and every existing table keeps the two apart for free. The one place this
*did* need a new C++-identifier-legal mapping is Codegen, since `*` isn't a valid C++
identifier character: `Codegen::cppVarName` maps a plain name to `v_name` (unchanged) and
a pointer name to `vp_name` (stripping the `*`), and every call site that could see a
pointer-flavored name (`VarRef`, `Define`/`Assign` codegen, procedure param/local
declarations, the global-variable emission loop) now goes through it instead of
constructing `"v_" + name` inline.

**A pointer's own C++ storage is a plain `std::int64_t` address, never a typed C++
pointer** - deliberately mirroring PB's own internal model (oracle-verified: a pointer
variable's default value is a plain integer zero, comparisons and reassignment work like
ordinary integer operations) rather than introducing a second, parallel "this is really a
pointer" representation that Codegen's existing target-typed-conversion machinery would
need to special-case everywhere. The pointee type it was declared to point at
(`Sema::ResolvedType`, either `TypeSuffix::None` for untyped or `TypeSuffix::Struct` +
a structure name) is tracked in a new side table, `Sema::pointerPointeeType_`, keyed by
the same `*`-prefixed name; dereferencing (`Codegen`'s `FieldAccess` case, when the base
is a bare pointer-value `VarRef`) is the *only* place a `reinterpret_cast<s_Point*>(...)`
appears, exactly at the point real PB itself would follow the pointer.

**`pointerPointeeType_` is deliberately NOT saved/restored around a `ProcedureDecl`'s
scope swap, matching `varStructType_`'s existing (pre-M3d) convention.** Both
`symbols_`/`order_` get a real per-procedure save/swap/restore (see M2's isolated-scope
notes) because Codegen never reads them directly - it reads `Sema::ProcedureInfo::locals`
instead, captured explicitly right before the restore. But `varStructType_` and now
`pointerPointeeType_` *are* read directly by Codegen (`structTypeOfVar`/`pointeeTypeOf`),
and that reading happens in an entirely separate pass, well after `Sema::analyze()` (and
every one of its internal scope swaps) has already finished - so an entry added while
visiting a procedure's body must survive past that procedure's own swap-back, or Codegen
would see a wiped/wrong entry for that procedure's own params when it later re-walks the
body. (A first attempt at implementing this added a save/restore anyway, by analogy with
`symbols_` - it broke exactly this way, caught immediately by an end-to-end pointer-
parameter test failing to compile.) The accepted tradeoff, inherited unchanged from
`varStructType_`: a pointer parameter or local sharing a base name with an unrelated
global pointer of a different pointee type can leak the wrong pointee type across
procedures. Not yet observed in practice and not worth the added complexity to close
until it is.

**The four allocation built-ins are recognized by name, not through the normal
user-declared-procedure table**, via `Sema::isPointerBuiltinName`/
`Sema::visitPointerBuiltinCall`, checked before the ordinary array-read/procedure-call
dispatch in both `Sema::visitExpr` and `Codegen::genExpr`'s `Call` handling.
`AllocateStructure`'s sole argument is a bare Structure *type name* (oracle-verified
syntax: `AllocateStructure(Point)`), not a variable read - it is deliberately never
passed through `visitExpr`, since doing so would implicitly declare a bogus variable
named after the type. `FreeStructure(*p)` determines which C++ type to `delete` as by
looking up its argument's pointee type via `Sema::pointeeTypeOf` - this only works when
the argument is literally a bare pointer-value `VarRef`, which covers every oracle-
verified usage seen so far; a computed pointer expression falls back to treating the
delete as untyped (`std::int64_t`), a known, narrow limitation rather than a silent
miscompile of anything actually exercised.

**Deliberately deferred past M3d**: the Memory library's `Peek*`/`Poke*` functions
(needed to make an *untyped* pointer's own dereference actually useful - M4); pointers to
arrays or to other pointers; `NewList`/`NewMap` (the remaining, unrelated piece of M3).

## M3e Implementation Notes (`NewList`)

**Scope landed**: `NewList name.type()` declarations (primitive or Structure element
types), `AddElement`/`InsertElement`/`DeleteElement`, `ForEach name() ... Next`,
`FirstElement`/`LastElement`/`NextElement`/`PreviousElement`, `ListSize`,
`SelectElement`/`ListIndex`, `ClearList`, and reading/writing the current element via a
bare `name()` (including through a `\field` chain for a List of Structures, reusing
M3c/M3d's field-access machinery directly, the same way M3d's pointer dereference did).

**The parser needed almost no new grammar at all - `name()` (zero args) already parsed
correctly, for free, as a side effect of M3b's array/call disambiguation.** `Name(args)`
already parses to a generic `CallExpr` regardless of argument count, including zero, and
`Name() = expr` already parses to an `IndexAssignStmt` with an empty `indices` vector,
also for free - `parseIdentifierStatement`'s array/call-disambiguation logic from M3b
never assumed at least one argument anywhere. The only genuinely new grammar this
milestone added was two new statement headers (`NewList name.type()` and `ForEach
name() ... Next`, the latter closed by the already-existing `Next` keyword `For` also
uses) - everything else (`name()` read, `name() = expr` write, `name()` as an argument to
a list built-in, `name()\field`) is the *identical* `CallExpr`/`IndexAssignStmt`/
`FieldAccessExpr` shape M3b/M3c already produce, disambiguated in Sema by checking a new
`lists_` table before falling through to the existing array/procedure checks - exactly
the same layered-disambiguation pattern `ArrayInfo` already established, extended by one
more layer rather than redesigned.

**Oracle-derived cursor semantics, verified through several rounds of targeted
probing** (see `docs/developer/oracle-testing.md`'s methodology): `AddElement`/
`InsertElement` insert after/before the cursor respectively and move the cursor to the
new element (an empty list is a degenerate case of both); a *failed* `FirstElement`/
`LastElement`/`NextElement`/`PreviousElement` never disturbs the current cursor (verified
by inserting immediately after a `ForEach` loop exhausts - the insertion landed right
before the *last* element, proving the loop's own final, failing `NextElement` call left
the cursor sitting on the last element rather than invalidating it); `DeleteElement`
moves the cursor to the *previous* element if one exists, else the *next* one (verified
by deleting the first element of a 3-element list and observing the cursor land on the
new first element, not become invalid). `PBList<T>` (`runtime/include/.../pblist.hpp`),
a `std::list<T>` plus an explicit cursor, encodes exactly this rule set - see its own doc
comments for the one genuinely deliberate simplification: real PB's debugger raises "The
LinkedList has no current element" if `DeleteElement` empties a list and a later
operation is attempted without re-establishing the cursor; `PBList` simply leaves the
cursor invalid instead of reproducing that debug-build-only crash.

**`ForEach` lowers to exactly the two-line pattern real PB's own generated C already
uses** (confirmed by reading `pbcompilerc`'s own `-c` output): `v_name.resetForEach();
while (v_name.nextElement()) { ... }` - `resetForEach()` just invalidates the cursor
without touching the list's contents, so the loop's first `nextElement()` call falls into
the same "invalid cursor, non-empty list -> move to the first element" case
`FirstElement` uses, and every subsequent call is a plain advance-or-fail. No separate
"before the beginning" sentinel state was needed beyond the one `hasCursor_` flag
`PBList` already needed for everything else.

**Lists reuse `ArrayInfo`'s existing, previously-undocumented scoping simplification
rather than getting their own, more careful treatment**: `lists_`/`listOrder_` (like
`arrays_`/`arrayOrder_` before them) are never part of a `ProcedureDecl`'s `symbols_`/
`order_` save-swap-restore, so a `NewList` declared inside a procedure body is - like a
`Dim`'d array inside one - emitted as a genuine C++ global, not a true per-call-isolated
local. Not a new gap this milestone introduces; simply inherited as-is rather than fixed
in passing, consistent with `arrays_`'s own precedent.

**Deliberately deferred past M3e**: `NewMap`/`AddMapElement`/`FindMapElement`/`MapKey`
and the rest of the Map family (a separate, not-yet-designed data structure, even though
it shares `ForEach` and much of the cursor vocabulary conceptually); passing a List to a
procedure (PB's `List Name.type()` parameter syntax, distinct from a plain scalar
parameter - the same kind of gap M3b already left open for arrays); `CopyList`/
`SwapList`/`MergeLists`; `ArrayList`/`ArrayToList`-style conversions.

## M3f Implementation Notes (`NewMap`) - M3 complete

**Scope landed**: `NewMap name.type()` declarations (primitive or Structure element
types), direct-by-key access `name(key)` (read/write, auto-creating the key if absent),
`name()` (the current cursor's element, identical in shape to a List's own), `ForEach
name() ... Next`, `AddMapElement`/`FindMapElement`/`DeleteMapElement` (both its 1-arg
cursor and 2-arg by-key forms)/`ClearMap`/`MapSize`/`MapKey`/`ResetMap`/`NextMapElement`.
This is the last piece of M3 - `Structure`, pointers, static `Dim` arrays, `Global`/
`Shared`/`Protected`, `NewList`, and now `NewMap` are all done.

**A Map genuinely has *two* element-access shapes sharing one disambiguation problem
each with something else already in the language** - oracle-verified by testing, not
assumed: `name()` (zero args) is identical in shape to a List's own current-element
access (extending the same `listInfo`-then-`mapInfo` check already used for `ForEach`'s
own target), while `name(key)` (exactly one arg) is genuinely ambiguous with a 1D array
read at parse time - the exact same `Name(args)` shape `ArrayInfo` already disambiguates
against a procedure call (M3b). Both share the same layered-lookup pattern: `mapInfo`
is checked before `arrayInfo` in `visitExpr`'s `Call` case, `resolveType`'s `Call` case,
and `classify`'s `Call` case (in that priority order alongside `listInfo`), and in
`IndexAssignStmt`'s handling for the write side. No new AST node was needed for either
form - `CallExpr`/`IndexAssignStmt` already cover both arities.

**A wrong assumption caught immediately by probing before implementing anything**: `-k`
(syntax-check-only) accepts *any* `Identifier(args)` shape, including a function that
doesn't exist at all (verified: `TotallyBogusFunctionXYZ(m())` syntax-checks fine) - so
an early pass at confirming which Map iteration functions are real by just running `-k`
against candidate names was worthless. The only reliable check is a full `-d` compile
and *run*, which is what caught that `FirstMapElement`/`LastMapElement`/
`PreviousMapElement` are **not real PB functions** (real error: "... is not a function,
array, list, map or macro.") - a Map only supports forward iteration via
`NextMapElement`, unlike a List's full First/Last/Next/Previous set, because a hash map
has no well-ordered "previous" or "last" to speak of. Recorded here as a reusable lesson
for future oracle work, not just a M3f-specific fact: `-k` verifies grammar, never that a
called name is real.

**`AddMapElement` has a genuinely surprising, oracle-verified quirk that plain
`name(key)` access does *not* share**: `AddMapElement(map(), key)` always resets the
target key's value to zero, *even when the key already existed* (verified: setting a key
to 1, then calling `AddMapElement` on that same key again, then reading it back gives 0,
not 1) - whereas `name(key)` on an existing key preserves its current value (a standard
"auto-vivifying" access, verified separately). `PBMap::addMapElement` vs. `PBMap::access`
(`runtime/include/.../pbmap.hpp`) encode this exact distinction; getting it backwards
would have been an easy, plausible-looking bug this project's oracle-first methodology
caught before it shipped.

**`PBMap<T>` is a `std::list<Entry>` plus an `unordered_map<string, iterator>` index,
not `std::unordered_map` directly** - real PB's own Map iteration order is hash-bucket-
dependent and explicitly not something this project set out to replicate bit-for-bit (a
well-formed PB program has no business depending on it either); giving *this*
implementation's own `ForEach`/`NextMapElement` iteration a stable, deterministic
(insertion) order instead is strictly more useful for testing, at the cost of one extra
pointer indirection per lookup - a good trade. Cursor semantics (`resetForEach`/
`nextElement`/`current`) are written to be textually interchangeable with `PBList`'s own
so `Codegen::genStmt`'s `ForEach` case needs zero Map-specific logic: it already just
calls those two method names on whatever `cppVarName` resolves to, List or Map alike.

**Deliberately deferred past M3f (and hence past all of M3)**: passing a Map to a
procedure (the same kind of gap M3b left open for arrays and M3e left open for Lists);
`CopyMap`/`RenameMapElement`; `PolymorphicListElement`-family constructs (rare, tied to
PB's OOP-adjacent features this project doesn't model); a Map key type other than String
(not supported by real PB itself, so nothing was actually deferred here - confirmed
oracle-verified, not assumed).

## M2-closure Implementation Notes

M2's own "Deliberately deferred" list (see above) had five items. `Global`/`Shared`/
`Protected` and static `Dim` arrays were closed by M3a/M3b as part of the normal
roadmap. The other three - mutual recursion/`Declare`, call-argument type-checking
beyond arity, and a constant declared inside a nested block - were never in scope for
any of M3's own sub-milestones (all about Structure/pointers/arrays/List/Map, not
procedure-call rigor) and sat open until this follow-up pass closed them explicitly.

**`Declare` enables exactly the forward reference PB otherwise forbids, and needed a
real C++ forward declaration to match, not just a Sema-level allowance.** Oracle-
verified: `Declare IsOdd(n.i)` before `Procedure IsEven` lets `IsEven`'s body call the
not-yet-defined `IsOdd`, and the real `Procedure` fulfilling a `Declare` must match its
promised signature *exactly* - return type and every parameter type, not just arity
("Declare doesn't match with real Procedure." for any mismatch) - while a `Declare` left
unfulfilled by any matching `Procedure` rejects the whole program ("The procedure
'name()' has been declared but not defined."). `Sema::visitStmt`'s new `Declare` case
pre-registers the promised signature in the same `procedures_` table an ordinary
`Procedure` uses, tracking still-unfulfilled ones in `declaredNotDefined_` (checked once
at the end of `analyze()`, since the fulfilling `Procedure` can appear anywhere later in
the file) and validating the match when the real `ProcedureDecl` for that name is
reached. The first implementation attempt stopped there and still failed at the C++
level with `'f_isodd' was not declared in this scope` - Sema allowing the *PB-level*
forward call doesn't make the generated C++ compile, since `Codegen::genProcedures`
still emits each function only in source order; `Codegen::genDeclarePrototypes` (run
before `genProcedures`) closes that second half of the gap with a genuine C++ prototype
per `Declare`. `Declare`'s own parameter list is deliberately primitive-suffix-only (no
pointer/Structure params) - not independently oracle-verified and judged rare enough not
to hold up closing the primary gap (mutual recursion among ordinary procedures).

**A procedure can't be declared just anywhere - PB enforces this itself, with two
distinct wordings this project wasn't reproducing at all.** `Procedure` nested inside
`If`/`Select`/`For`/`While`/`Repeat`/`ForEach` is rejected ("A procedure can't be
declared inside an If, Repeat, While or For." - real PB's own generic wording, used
verbatim even for `Select`/`ForEach`, which it doesn't actually name); nested inside
*another* `Procedure` gets a different message ("Can't define a procedure inside another
procedure."). Before this pass, `pbcxx` didn't just fail to reproduce either rejection -
it silently miscompiled: a nested `Procedure`'s body was accepted by Sema, silently
dropped by `Codegen` (which only ever scans top-level statements for `ProcedureDecl`,
same as it does for constants - see below), and any call to it downstream produced an
undefined-C++-symbol failure with no PB-level diagnostic at all pointing at the actual
mistake - or, if the resulting call was itself never reached, no error whatsoever (exit
code 0). `Sema::visitNestedBlock` (wrapping every control-flow body's own `visitBlock`
call) tracks a `controlFlowDepth_` counter purely so `ProcedureDecl`'s own handling can
check it (alongside the pre-existing `insideProcedure_` flag, which already existed for
`ProcedureReturn`'s own type-checking) and reject both illegal positions with real PB's
own wording, stopping the pipeline before `Codegen` ever sees the malformed input.

**A constant is purely compile-time and textual, entirely independent of runtime
control flow - a genuinely different situation from the Procedure-nesting case just
above, verified rather than assumed to be analogous.** `#X = 5` declared inside a
never-taken `If a = 1` branch (`a` is 0) is still usable afterward, reading `5` - PB
resolves constants by source position, not by actually executing the branch; a
constant declared inside a `Procedure`'s own body works the same way and is equally
legal (unlike nesting a `Procedure` itself, which is flatly rejected). Since `Sema`
itself already handled this correctly for free (`visitBlock`'s ordinary recursion
already reaches a nested `ConstDecl`/`Enumeration` and declares it exactly like a
top-level one, regardless of depth), the actual gap was entirely in `Codegen`:
`genGlobalConstants` only ever scanned `module_.statements` directly when deciding what
to hoist as a global `static const` before `main()`, silently dropping anything nested.
Fixed by refactoring it into a recursive `genConstantsIn` that walks every nested block
(`If`/`Select`/`For`/`While`/`Repeat`/`ForEach`/a `ProcedureDecl`'s own body) in the same
document order `Sema` already enforces declare-before-use in - safe to recurse into a
`ProcedureDecl`'s body unconditionally now, precisely because the fix just above means a
`Procedure` itself can never legally be the thing doing the nesting.

**Call-argument type-checking beyond arity has its own oracle-verified wording,
distinct from `checkAssignable`'s existing String-vs-numeric message, and its own
oracle-testing-methodology lesson.** `Foo("hello")` against `Procedure Foo(x.i)`
syntax-checks fine under `-k` - the same "`-k` verifies grammar, never real semantic
validity" lesson M3f's `TotallyBogusFunctionXYZ` finding already recorded, now
confirmed for a second, unrelated kind of check. Only a full compile surfaces "Bad
parameter type, number expected instead of string." (or, for the reverse direction,
"Bad parameter type: a string is expected."). `Sema::visitCall` now checks each
argument's family against its corresponding declared parameter's, once arity itself has
already been validated, reusing the existing per-argument `visitExpr` loop rather than
adding a second pass over `call.args`.

## M4a Implementation Notes (core String library)

**Scope landed**: `Len`, `Left`, `Right`, `Mid`, `UCase`, `LCase`, `Trim`/`LTrim`/`RTrim`,
`Str`/`Val`, `StrF`/`ValF`, `Chr`/`Asc` - the highest-value subset of PB's String library.
`FindString`/`ReplaceString`/`StringField` (genuinely more involved search/split logic)
and a custom trim character (`Trim(s, char)`) are deliberately deferred to a follow-up
slice.

**These builtins needed almost no new Sema machinery at all - registering them as fake
`ProcedureInfo` entries reuses the *entire* existing call-checking pipeline verbatim.**
`Sema::registerStringLibBuiltins()` (called once from the constructor) inserts each
function's signature directly into the same `procedures_` table a real `Procedure`
populates; from that point on, `Len(s)`/`Left(s, n)`/etc. are - as far as
`Sema::visitCall`, `classify`, and `resolveType` are concerned - indistinguishable from a
call to a user-defined procedure, getting arity checking, the M2-closure per-argument
String-vs-numeric type check, and (for `Str`'s Float argument) the exact same banker's-
rounding target-typed conversion every other call site already gets, all for free.
`Codegen` is the *only* place that needs to know these names are special
(`Sema::isStringLibBuiltinName`), routing a call to one of them to its
`easybasic::runtime::pb*` implementation instead of an `f_<name>` that was never
declared - `Mid`'s optional `count` and `StrF`'s optional `decimals` reuse the same
"fewer call-site arguments than `paramSuffixes.size()`" mechanism a real Procedure's own
default-valued trailing parameters already use, backed by a genuine C++ default
parameter on the runtime function itself (`pbMid`'s `count = INT64_MAX`, relying on
`pbUtf16Slice`'s own end-of-string clamping rather than a value that depends on the
specific string passed, which couldn't be a compile-time-constant C++ default).

**A major, oracle-driven correction made mid-implementation: `pbcompilerc` does not
UTF-8-decode multi-byte literal text embedded directly in a `.pb` source file.**
`s.s = "café"` (the source file itself UTF-8-encoded, `é` as its usual 2-byte sequence)
gave `Len(s)` = 5, not 4, and `Right(s, 1)` produced a single mis-decoded byte, not `é` -
real PB's compiler reads source text as raw bytes/Latin-1 for string literals, assigning
each individual byte its own internal UTF-16 code unit, rather than decoding UTF-8
multi-byte sequences into single Unicode code points. Building the *same* string via
`s.s = "caf" + Chr(233)` instead gives the correct `Len(s)` = 4 and `Right(s,1)` = `"é"` -
proving PB's own *internal* string handling is genuinely Unicode-correct; the divergence
is purely in how the compiler's source-file reader treats literal text. Given `PBString`
is deliberately UTF-8 internally (an M0 decision that explicitly flagged and accepted
this exact risk "for any non-ASCII/non-BMP string"), and that matching PB's byte-
shredding quirk for raw source literals would require *also* not-UTF-8-decoding this
project's own `.pb` source files (a worse, more confusing default for a new tool doing
Unicode "properly"), the implementation keeps genuine UTF-8-decoding for `Len`/`Left`/
`Right`/`Mid`/`Chr`/`Asc` and explicitly does not chase the oracle for this one narrow,
legacy-encoding-specific case - documented here as a deliberate, understood divergence,
not an unnoticed gap. Every String-library e2e/differential test consequently sticks to
ASCII literals plus `Chr()`-constructed Unicode text (which agrees with the oracle
exactly), never a raw non-ASCII byte embedded directly in a `.pb` file.

**A second, related finding, not yet acted on**: real `Chr()` itself rejects any code
point outside the Basic Multilingual Plane minus the surrogate range ("Invalid value for
Chr(), should be between 0 and $D7FF or between $E000 and $FFFF") - meaning a genuine
UTF-16 surrogate pair is essentially unreachable through legitimate PB code at all (not
just unlikely in practice). `pbChr`/`pbUtf16Length`/`pbUtf16Slice` still handle an astral
code point generally (encoding/counting it as a surrogate pair) purely for this
implementation's own internal consistency - not because any real, valid PB program could
ever ask for it. `pbChr`'s own missing range validation is a known, minor gap.

**A second, unrelated real regression caught by testing before it shipped**:
`pbUtf16Slice`'s first implementation detected "the requested slice is already empty"
one whole code point too late - `Left("Hi", 0)` came back `"H"` instead of `""`, because
the loop's own "have we reached the end" check only ran *after* unconditionally
consuming the next code point. Fixed with an upfront `if (endExclusive <= start) return
{};` guard before the per-code-point loop even starts, verified against exactly this
case (and the oracle) afterward.

**`Str`'s Float-to-Integer conversion uses the *same* banker's-rounding rule as
everywhere else in this project - oracle-verified, not assumed to carry over.**
`Str(2.5)` is `"2"`, `Str(3.5)` is `"4"`, `Str(-2.5)` is `"-2"` - round-half-to-even,
exactly matching the M0-established rule for every other Float-to-Integer target-typed
conversion. This is what let `Str`'s builtin registration simply declare its parameter as
`TypeSuffix::Integer` and get correct rounding for free through the existing `convert()`
machinery, rather than needing any Str-specific logic.

**`StrF`'s argument is a Float (single precision), not a Double - confirmed by a
precision-loss artifact, not PB documentation.** `StrF(3.14159)` with no explicit
decimal count printed `"3.1415901184"` (10 digits, the oracle-verified default decimal
count) - the trailing digits are exactly what you'd expect from `3.14159` first being
narrowed to a 32-bit float (losing precision beyond ~7 significant digits) and *then*
formatted to 10 decimal places, not from the literal's own full double-precision value.

**Deliberately deferred past M4a**: `FindString`/`ReplaceString`/`StringField` (their
own, more involved search/split semantics, including optional start-position and
case-sensitivity arguments not yet oracle-verified); a custom trim character for
`Trim`/`LTrim`/`RTrim` (default space-only trimming verified, the optional second
argument naming a different character is not); `Chr`'s own BMP-range validation (see
above); full Unicode case-folding for `UCase`/`LCase` (ASCII-only, safe on arbitrary
UTF-8 but not a correctness guarantee beyond ASCII); Math, Memory (`Peek*`/`Poke*` -
the piece M3d's pointer work deferred here specifically), File, and Date - the rest of
M4.

## M4b Implementation Notes (core Math library)

**Scope landed**: `Abs`, `Sqr`, `Pow`, `Sin`/`Cos`/`Tan`/`ASin`/`ACos`/`ATan`/`ATan2`,
`Exp`/`Log`/`Log10`, `Round` (all three of its real modes), `Int`, `Random`/`RandomSeed`.
`Min`/`Max` are not real PB functions at all (oracle-verified: "is not a function, array,
list, map or macro.") - a plausible-sounding guess this project didn't make without
checking first.

**Every one of these (except `Int`) is oracle-verified to always return a Double,
regardless of its argument's own type** - `Abs(-5)` with an *Integer* argument still
prints via the Double debug-format (see next finding), not a plain Integer one. This
reuses M4a's exact registration mechanism (`Sema::registerMathLibBuiltins`, a second
`ProcedureInfo`-faking constructor call alongside `registerStringLibBuiltins`) with zero
new Sema machinery - `Codegen` again is the only place that needs to know these names are
special.

**A real, previously-deferred gap this milestone was forced to actually fix: `Debug`ing a
raw Double or Float uses its own distinct format, neither of which is `std::to_string`.**
M4a's own notes already flagged this as still-open; M4b made it unavoidable, since every
Math builtin's Double return value needs to print correctly to even write a working
oracle test. Oracle-verified: a raw Double prints via a 16-character-wide, right-
justified `%g`-style field (6 significant digits, scientific notation for extremes - e.g.
`Debug 1000000.0` is `"           1e+06"`), while a raw Float prints via plain `%f` with
exactly 6 decimals, no padding, no scientific notation. `Codegen`'s `Debug` case now asks
`Sema::resolveType()` for the exact suffix (`Float` vs `Double`) whenever `classify()`
says `FloatFamily`, routing to `pbDebugFormatFloat`/`pbDebugFormatDouble`
(`runtime/include/.../debug.hpp`) accordingly; `resolveType()`'s existing Integer
fallback (for a Binary/Unary expression it can't resolve, e.g. `Debug d + 1.0`) becomes
"assume Double" in this one context, since the surrounding code already knows the
expression is FloatFamily - the fallback only ever needs to pick *which* float format,
never whether to use one at all.

**`Round`'s `mode` argument introduced a new category this project didn't have yet:
built-in `#PB_*` constants**, not something a user's own `#Name = expr`/`Enumeration`
ever declares. Oracle-verified values: `#PB_Round_Down` = 0, `#PB_Round_Up` = 1,
`#PB_Round_Nearest` = 2 - and a fourth, entirely plausible-sounding `#PB_Round_Truncate`
does **not** exist ("Constant not found"), caught by checking rather than assuming the
obvious symmetric name existed. `Sema::registerBuiltinConstants` pre-populates
`constants_` (but deliberately *not* `constOrder_`, since there is no `ConstDeclStmt` in
the AST for `Codegen::genGlobalConstants` to hoist) so `#PB_Round_Nearest` resolves and
type-checks exactly like a real constant; `Codegen::genExpr`'s `ConstRef` case checks
`Sema::builtinConstantValue` first and emits the literal value directly instead of a
`k_<name>` that was never actually emitted anywhere.

**`Round`'s `Nearest` mode is round-half-*away-from-zero*, genuinely different from this
project's own established banker's-rounding rule for an *implicit* Float-to-Integer
conversion** (e.g. a `Define x.i = someFloat` or a procedure call argument) - oracle-
verified: `Round(2.5, #PB_Round_Nearest)` is `3`, `Round(-2.5, ...)` is `-3`, whereas the
implicit-conversion rule established back in M0 gives `2` and `-2` for the same inputs.
Real PB itself is internally inconsistent between these two rounding rules for what looks
like "the same operation" - not a design choice this project gets to reconcile, just one
to faithfully reproduce on each of its own two separate paths. Similarly, `Round`'s
`Down`/`Up` modes are genuine mathematical floor/ceiling (`Round(-3.1, Down)` is `-4`, not
`-3` - rounding toward zero is not what "Down" means), while `Int` truncates toward zero
(`Int(-3.7)` is `-3`) - three visually-similar "make this a whole number" operations that
are all subtly different from each other, each verified independently rather than
assumed to share one behavior.

**`Random`/`RandomSeed` are a deliberate, documented non-goal for oracle fidelity on
exact values** - real PB's own generator algorithm is undocumented and not a reasonable
reverse-engineering target for this project. `easybasic::runtime`'s own `Random`/
`RandomSeed` use a plain `std::mt19937`, giving this implementation's own output
reproducibility for a given seed (verified) and correct `[min, max]` range bounds
(verified), but never the same *specific* values real PB's own seeded sequence would
produce. The `random` e2e_diff test accordingly asserts only reproducibility and range
(both hold identically for either compiler, via `If`/`Bool()`-wrapped comparisons that
print `1`), never a captured raw value - the one test in this project's whole
differential-testing suite that can't just capture the oracle's literal stdout as the
expected fixture.

**Deliberately deferred past M4b**: `Min`/`Max` don't exist as real PB functions at all
(confirmed, not assumed - no gap to fill here); `Sinh`/`Cosh`/`Tanh` and other
hyperbolic/inverse-hyperbolic variants; `Degree`/`Radian` conversion helpers; `Memory`
(`Peek*`/`Poke*` - the piece M3d's pointer work deferred here specifically), File, and
Date - the rest of M4.

## M4c Implementation Notes (core Memory library, `Peek*`/`Poke*`)

**Scope landed**: `PeekB`/`PeekA`/`PeekC`/`PeekW`/`PeekU`/`PeekL`/`PeekQ`/`PeekF`/`PeekD`/
`PeekS` and their `Poke*` counterparts, against a plain Integer address - this is the
piece M3d's own pointer work explicitly deferred ("needed to make an *untyped* pointer's
own dereference actually useful"), now closed: `Define *ptr` with no Structure type was
only ever usable as a bare address value before this.

**These needed even less new Sema machinery than the String/Math libraries did** - every
argument and return value here is a plain Integer address or primitive value (no special
"must be a bare `name()`" argument shape the way the M3d pointer/List/Map built-ins
needed), so `registerMemoryLibBuiltins` is a third, near-identical call to the exact same
fake-`ProcedureInfo`-registration mechanism `registerStringLibBuiltins`/
`registerMathLibBuiltins` already established, with zero new per-argument validation
logic anywhere.

**A real memory-safety bug this project's own testing requirements exist to catch, found
and fixed before it shipped**: a `Peek*`/`Poke*` call can legally target any byte offset
in real PB, with no guarantee of natural alignment for the type being read/written
(`PokeL(*blk + 1, ...)` is completely ordinary PB code) - the first implementation used a
plain `*reinterpret_cast<int32_t*>(address)` dereference, which is undefined behavior for
a misaligned address even though it happens to produce the correct answer on x86 in
practice. A **targeted UBSan run of this project's own generated code** (not caught by
the regular `ctest` sanitizer job, which only builds `pbcxx` itself with sanitizers, not
the C++ programs it generates - compiling a generated program directly with
`-fsanitize=address,undefined` was necessary to catch this) reported "store to misaligned
address ... requires 4 byte alignment" on exactly this pattern. Fixed by routing every
`Peek*`/`Poke*` through a `memcpy`-based `unalignedLoad`/`unalignedStore` helper pair
instead (`runtime/include/.../memorylib.hpp`) - verified both against a re-run of the
same targeted UBSan check (clean) and against the oracle (real PB tolerates the same
misaligned access identically, giving the same answer pbcxx now does, both by relying on
the underlying hardware's own tolerance for it - x86 permits misaligned access, just less
efficiently, whereas the C++ *language* itself still calls dereferencing a misaligned
typed pointer undefined behavior regardless of what the hardware permits). Worth noting
as a gap in this project's *testing infrastructure*, not just this one bug: the
established `ctest`-driven ASan/UBSan job has never covered code path executed only
inside a *generated* PB program, which is exactly where a runtime-library bug like this
one lives - a manual, one-off compile-with-sanitizers-directly step was the only way
this was actually caught, and doing that systematically for e2e-tested `.pb` programs is
a worthwhile future improvement this milestone surfaced but didn't build.

**`PeekS`/`PokeS` needed a genuine UTF-16LE encode/decode step, not a raw byte copy -
real PB's default Unicode compile mode uses 2 bytes per character in memory, verified by
directly inspecting the bytes `PokeS` wrote** (`PokeS(*blk, "Hi")` produces the byte
sequence `[72, 0, 105, 0, 0, 0]` - `'H'`, `'i'`, then a 2-byte null terminator - read back
byte-by-byte with `PeekB`, not assumed from documentation). Since `PBString` is
deliberately UTF-8 internally (the same M0 architectural decision M4a's own notes already
discuss), `pbPokeS`/`pbPeekS` convert explicitly between UTF-8 and UTF-16LE bytes at the
point memory is actually touched, astral code points becoming a real surrogate pair on
the wire - the correct layout for interoperating with anything that reads/writes memory
at the byte level (`Peek*`/`Poke*`'s entire reason to exist), at the cost of being
noticeably more code than "just `memcpy` the string's own buffer" would have been.

**A related case deliberately left unsupported, not silently broken: `PeekS(@someString
Var)`.** Oracle-verified, `PeekS(@s)` for a String *variable* `s` correctly reads its
character data directly - meaning real PB's own String variables are internally a
pointer to a persistent UTF-16 character buffer, and `@` on a String specifically yields
a pointer to *that buffer*, not to the variable's own storage slot (unlike `@` on every
other type, which does give the address of the variable's own storage). `pbcxx`'s
`AddressOfExpr` codegen has no equivalent special case - `@s` for a `PBString`-typed
`v_s` yields the address of the `PBString` C++ object itself (a ref-counted wrapper, not
a raw character buffer), so `PeekS(@s)` on a variable does not currently produce the
right answer (verified to fail, not assumed to fail: `Debug PeekS(@s)` for `s.s =
"Hello"` printed garbage). Properly closing this would mean giving `PBString` a lazily-
materialized, cached UTF-16LE buffer purely for `@` to point at - a real architecture
change to the type this project's own `PBString`-is-UTF-8-internally decision (M0) didn't
anticipate needing, and out of scope for this milestone. `Peek*`/`Poke*` against an
explicitly `AllocateMemory`'d block - the idiomatic, primary use case - works correctly
and is what every test in this milestone actually exercises; this is a narrower,
documented gap, not a general "Peek/Poke is unreliable" finding.

**`Poke*`'s own return value is a documented placeholder (`0`), not an oracle-matched
one** - a real `Poke*` call appears to return some address-derived value (observed:
`PokeL(*blk, 42)` printed a large, address-looking number rather than anything
meaningful like `0` or the written value), but every real PB example calls `Poke*` as a
bare statement, discarding it, so chasing the exact formula wasn't judged worth the
effort for a return value this project has never seen an actual program depend on.

**Deliberately deferred past M4c**: `CopyMemory`/`FillMemory`/`CompareMemory`/
`MemoryStringLength`; `AllocateMemory`'s own optional flags argument; `Peek*`/`Poke*`
paired with a Structure pointer's fields (only a bare address argument is supported, not
"the address of this specific Structure field"); `@stringVar` (see above) - the rest of
M4 (File, Date) remains after this.

## M4d Implementation Notes (core File library)

**Scope landed**: `CreateFile`/`OpenFile`/`ReadFile`/`CloseFile`, `WriteString`/
`WriteStringN`/`ReadString`, `Eof`, `FileSize`/`DeleteFile`/`RenameFile`, and
`FileSeek`/`Loc`/`Lof` - a complete enough set to create, read, write-in-place, and
manage files, all oracle-verified rather than assumed from memory of the PB API (several
guesses below turned out right, but were still checked, not shipped on faith).

**A file "number" is a plain Integer the PB program itself picks, not a handle PB hands
back - oracle-verified with a *variable*, not just a literal, as the number.** This
mirrors classic BASIC's own numbered-file model exactly, and - like the M4c Memory
library's plain address arguments - needed no special argument-shape handling in Sema at
all: `registerFileLibBuiltins` is a fourth near-identical call to the same fake-
`ProcedureInfo` mechanism the String/Math/Memory libraries already established.

**`std::FILE*` (C stdio), not `std::fstream`, is the deliberate implementation choice -
because `Lof`/`Loc` are oracle-verified to work even on a `CreateFile` handle**, which
looks write-only at the PB source level (`If CreateFile(0, ...) ... Debug Lof(0)`
correctly reports the file's length). `ftell`/`fseek` on a C `FILE*` behave uniformly
regardless of the mode it was opened with; `std::iostream`'s own get/put positioning is
only fully guaranteed by the standard for a stream opened with `ios::in`, an unnecessary
portability risk for something this project could sidestep entirely by picking the other
standard I/O API. Every handle is still wrapped in a `std::unique_ptr<FILE, FileCloser>`
(a small custom deleter calling `fclose`), so a PB program that forgets to `CloseFile`
before exiting still doesn't leak the underlying file descriptor - verified directly (not
assumed) by compiling a generated program with `-fsanitize=address` (which includes leak
detection) and confirming a clean exit.

**`OpenFile` gives a read+write handle positioned at the start, letting a subsequent
write overwrite bytes in place rather than append or truncate** - oracle-verified:
`OpenFile` then `WriteString` after a `FileSeek` correctly overwrites just the targeted
bytes, leaving the rest of the file (and its overall length) unchanged. `CreateFile`
truncates/creates; `ReadFile` is read-only and fails (returns `0`, oracle-verified, not a
crash) if the file doesn't exist yet - the three real fopen modes (`"wb+"`, `"rb+"`,
`"rb"`) this project's own three open functions map to directly.

**`WriteStringN`'s line terminator is a plain `\n`, confirmed by measuring `FileSize`
after two lines, not assumed from the host platform's own text-mode convention** - every
file handle here is opened in *binary* mode specifically so the C library's own text-mode
newline translation (which would silently turn `\n` into `\r\n` on a non-Linux target)
can never make this project's own file I/O diverge from itself across platforms, even
though this specific fact was only checked on Linux.

**`FileSize` returning `-1` for a file that doesn't exist (not a crash, not `0`) is
exactly the kind of "checked, not assumed" fact this project's whole oracle-driven
approach exists to nail down** - a plausible-but-wrong alternative (`0`, indistinguishable
from a genuinely empty file) would have been an easy, silent correctness bug to ship
without the oracle catching it immediately.

**A real regression this milestone's own testing setup nearly reintroduced**: the first
draft of the combined e2e/e2e_diff test left `CreateFile`d files on disk without deleting
them, which would have made a second run of the *same* test see stale leftover state from
the first (e.g. `RenameFile`'s target already existing) - fixed by having the `.pb`
program itself `DeleteFile` both at the very start (idempotent even on a first run, since
deleting a nonexistent file is a harmless no-op) and again at the end, verified by running
the installed e2e test twice in a row rather than just once.

**Deliberately deferred past M4d**: `ReadData`/`WriteData` and the typed binary
`ReadLong`/`WriteLong`/etc. family (distinct from `Peek*`/`Poke*`, which already cover the
same ground against an in-memory buffer rather than a file - not judged worth a second,
file-specific implementation of the identical idea for this pass); `ExamineDirectory`/
`NextDirectoryEntry` and other directory-listing functions; `FileBufferSize`; file
locking/sharing-mode flags; `Date` - the last piece of M4.

## M4e Implementation Notes (core Date library) - M4 complete

**Scope landed**: `Date` (both its 0-arg "current time" and 6-arg "construct from
components" forms), `Year`/`Month`/`Day`/`Hour`/`Minute`/`Second`/`DayOfWeek`,
`FormatDate` (the `%yyyy`/`%mm`/`%dd`/`%hh`/`%ii`/`%ss` placeholder set), and `AddDate`
with all seven oracle-verified `#PB_Date_*` units. This is the last piece of M4 - String,
Math, Memory, File, and now Date are all done.

**`Date` is the first M4 builtin whose real arity genuinely can't be expressed by
`Sema`'s usual continuous `[required, total]` range** - oracle-verified to accept
*exactly* 0 or 6 arguments, rejecting 1-5 outright ("Incorrect number of parameters.").
Registered as `[0, 6]` anyway (a documented, minor imprecision: a 1-5-argument call now
fails at the C++ backend-compile stage instead of with a clean PB-style diagnostic, still
safely rejected either way) rather than inventing new Sema machinery for one function's
bimodal arity. The zero-arg and six-arg forms map to two entirely different C++ functions
(`pbDateNow`/`pbDate`) - `Codegen`'s own `dateLibRuntimeName` is the one dispatcher in
this whole M4 family that needs the call's argument *count*, not just its name, to decide
what to emit (every other optional-argument case elsewhere reused a single function with
a real C++ default parameter, which can't express "omit all six arguments at once, or
none of them").

**A real, oracle-checked design question answered rather than assumed: does `AddDate`'s
`Month`/`Year` arithmetic do genuine calendar normalization, or a fixed day-count
shortcut?** The two are easy to conflate, since `AddDate(d, Month, 1)` on a March 15 date
produces an epoch value that - worked out independently via exact day-of-year arithmetic,
not eyeballed - lands precisely on April 15, the same answer real calendar-aware "same
day, next month" `mktime`-based arithmetic gives. (An earlier arithmetic slip during this
verification briefly suggested a "+31 days, the length of March" shortcut instead,
cheaper to implement but numerically indistinguishable for *this specific example* - redone
carefully, the day-of-year math confirms it's true calendar arithmetic, not a
coincidentally-matching shortcut; `pbAddDate`'s `Month`/`Year` cases accordingly use real
`mktime`-based `tm_mon`/`tm_year` normalization, not a flat seconds offset.)

**`Date`/`Year`/`Month`/etc. all interpret their components as *local* time on both the
construct and extract sides, making the round trip timezone-independent by construction -
verified directly by running the same test under two different system timezones (UTC and
the development machine's own local zone, CEST) and getting byte-identical output either
way**, not merely asserted to be safe. This is also why this milestone's own e2e/e2e_diff
test deliberately never prints a *raw* epoch integer (`Date()`'s or `AddDate()`'s return
value directly) - that value's own absolute magnitude genuinely does depend on the host
timezone, unlike every *component* extracted from it, so a golden fixture captured on one
machine could spuriously fail on a CI runner or contributor's machine in a different
timezone. `Date()`'s own current-time form is (like `Random`) a documented non-goal for
exact-value oracle fidelity, verified only for "returns a plausible, recent-looking
timestamp," consistent with that same project-wide pattern for anything inherently
non-reproducible.

**A real, if minor, portability/correctness fix caught by actually compiling the
generated code, not just reasoning about it**: `pbFormatDate`'s first implementation used
a `char buf[8]` scratch buffer for `snprintf`-formatting each date component - plenty for
any value a real `std::tm` ever holds, but GCC's own static analysis can't prove that for
an arbitrary `int` argument, and flagged `-Wformat-truncation` on every build. Fixed by
sizing the buffer to safely fit any 32-bit `int` (16 bytes) instead of relying on "big
enough in every case that will ever actually occur" - removes compiler-noise that would
otherwise appear in every single program using `FormatDate`, and is the more defensible
choice regardless of whether the compiler can prove it.

**Deliberately deferred past M4e (closing out all of M4)**: `ParseDate` (the inverse of
`FormatDate`); `FormatDate`'s own fuller placeholder set (short year, month/weekday
names, AM/PM); `Date`'s own documented timezone/DST edge cases (a component combination
that doesn't exist, e.g. a "spring-forward" gap hour, is left to `mktime`'s own
platform-specific normalization behavior rather than specially validated) - M4 as a whole
is now done; M5 (`CompilerIf`/`CompilerSelect`, `DataSection`, `Macro`) is next.

## M5a Implementation Notes (`CompilerIf`/`CompilerSelect` + `#PB_Compiler_*`/`#PB_OS_*`/`#PB_Processor_*`)

**Scope landed**: `CompilerIf`/`CompilerElseIf`/`CompilerElse`/`CompilerEndIf`,
`CompilerSelect`/`CompilerCase`/`CompilerDefault`/`CompilerEndSelect`, and
`#PB_Compiler_OS`/`#PB_OS_Windows`/`#PB_OS_Linux`/`#PB_OS_MacOS`/`#PB_Compiler_Processor`/
`#PB_Processor_x86`/`#PB_Processor_x64`. `DataSection`/`Data`/`Read`/`Restore` and
non-recursive `Macro` remain for a later M5 slice.

**The single most consequential oracle finding this slice turned up**: a non-selected
`CompilerIf`/`CompilerSelect` branch is **never type-checked at all** by real PB - a
syntax-check of a program with a bogus, nonexistent function call sitting inside a branch
that the current platform doesn't select succeeds cleanly, with no diagnostic whatsoever.
This directly ruled out the "keep the node in the tree, have `Sema` just skip walking the
untaken side" design (the obvious first idea), because `Codegen`'s own `genDeclarePrototypes`/
`genProcedures` scan `module_.statements` directly rather than recursively, and because a
second, independently oracle-verified fact - **a `Procedure` declared inside a *selected*
`CompilerIf` branch is legal** (unlike inside a runtime `If`, which `Sema` already rejects,
see the M2-closure notes) - meant a naive "leave the node in place, mark which branch Sema
picked" design would also have required teaching `genProcedures`/`genDeclarePrototypes`/
`genConstantsIn` to recurse into `CompilerIf`/`CompilerSelect` nodes specifically, mirroring
the exact "only scans top-level statements" class of bug the M2-closure work fixed once
already for `Declare`/global constants.

**Chosen design**: resolve `CompilerIf`/`CompilerSelect` as a genuine, permanent **AST
rewrite performed by `Sema`, not a runtime construct lowered by `Codegen` at all**.
`Sema::visitBlock` (which already owns the mutable `ast::Block& block` it's walking) now
detects a `CompilerIf`/`CompilerSelect` node at the current position, evaluates its
condition/selector with a new compile-time constant folder (`Sema::evalConstExpr`), and
**splices the selected branch's own statements directly into `block` in the node's place**
(`Sema::resolveCompilerIf`/`resolveCompilerSelect`) - moving the `unique_ptr<Stmt>`
elements out, not copying them. After `Sema::analyze()` returns, a `CompilerIf`/
`CompilerSelect` node no longer exists anywhere in the tree at all: the selected branch's
statements are sitting exactly where the node used to be, as if they'd been written there
directly, as a real preprocessor's textual substitution would behave, just performed at
the AST level instead of the token level. This is why `Codegen` needs no new logic for
either `StmtKind` beyond a `-Wswitch` exhaustiveness placeholder (both `genStmt`'s main
switch and `Sema::visitStmt`'s own switch have one, documented as unreachable) - it runs
as a completely separate pass over the same, already-rewritten `Module`, so every
existing top-level-only or recursive scan (`genDeclarePrototypes`, `genProcedures`,
`genConstantsIn`) "just works" for anything that used to be wrapped in a `CompilerIf`,
with zero changes needed to any of them.

**A correctness subtlety the splicing approach could have gotten wrong**: `evalConstExpr`'s
caller holds a reference to the *selected branch's* `ast::Block` (a member of the
`CompilerIfStmt`/`CompilerSelectStmt` node itself) right up until the moment that node is
erased from the parent `block` - and erasing a `unique_ptr<Stmt>` element destroys the
`CompilerIfStmt`/`CompilerSelectStmt` object it points to, including every one of its
`branches`/`cases`. `resolveCompilerIf`/`resolveCompilerSelect` therefore `std::move` the
selected branch's `Block` out into a local variable **before** erasing the node, so the
local variable owns its statements independently by the time the node (and its now-empty,
moved-from branches) gets destroyed. Caught by reasoning through the ownership chain
during implementation, not by a test failure - but also independently confirmed clean
under the ASan/UBSan preset afterward (no use-after-free, as this bug class would have
produced).

**`CompilerIf`'s condition genuinely needs real constant VALUES, which `Sema` had never
needed to track before this**: every earlier milestone's constant handling
(`constants_`/`constOrder_`) only ever needed each `#Name`'s *type*, never its actual
number - `Codegen` re-evaluates a constant's own init expression directly as C++ rather
than asking `Sema` for a precomputed value, and `Enumeration` member chaining
(`k_previousName + 1`) was likewise deferred entirely to generated C++ arithmetic.
`CompilerIf`/`CompilerSelect` conditions are the first construct that needs `Sema` itself
to do real compile-time arithmetic on a `#Name` reference, since a non-selected branch is
never going to become C++ code that could do that arithmetic at runtime. Closed by adding
a new `constantIntValues_` table (Integer-family only - deliberately not extended to
String/Float constants, since no oracle-verified `CompilerIf` usage needs them),
populated for user `#Name = expr` declarations and `Enumeration` members (replicating the
same auto-increment chaining `Codegen` already does, just as real arithmetic instead of
generated C++ text) alongside the pre-existing built-in `#PB_*` table, and consulted by
`Sema::evalConstExpr` - a small recursive folder covering `IntLiteral`, `ConstRef`,
`Unary`/`Binary` arithmetic, comparison, and `And`/`Or` (not `XOr`, which stays untrusted
here for the same reason it's untrusted in `genCondition` - see that code's own notes).
Oracle-verified end-to-end: a user `#MyFlag = 1` referenced in a later `CompilerIf
#MyFlag = 1` resolves and selects the expected branch, byte-for-byte matching real PB's
own output for the same program.

**`#PB_Compiler_OS`/`#PB_Compiler_Processor` reflect whatever platform `pbcxx` itself was
*built* for**, via standard preprocessor macros (`_WIN32`, `__APPLE__`, `__HAIKU__`,
`__x86_64__`/`__aarch64__`/`__arm__`, falling back to Linux/x86 otherwise) - correct for
this project's single-target model (it shells out to a native g++/clang++ for the same
machine it runs on; there is no cross-compilation story to account for separately).
Oracle-verified on this Linux/x64 development machine: `#PB_Compiler_OS` = `2`
(`#PB_OS_Linux`), `#PB_Compiler_Processor` = `4` (`#PB_Processor_x64`); `#PB_OS_Windows` =
`1` and `#PB_OS_MacOS` = `4` and `#PB_Processor_x86` = `2` are oracle-verified as the
*fixed, always-available* comparison values (e.g. `#PB_OS_Windows` is legal and equal to
`1` even when compiled on Linux, just never equal to the *current* `#PB_Compiler_OS`).
**`#PB_OS_Haiku` has no real-PB value to verify at all** - Haiku isn't an officially
PB-supported platform - so `8` is a pbcxx-specific extension continuing the confirmed
values' power-of-two pattern, documented as such rather than presented as oracle fact;
the ARM processor constants (`#PB_Processor_arm64`=`8`, `#PB_Processor_arm`=`16`) are
likewise unverified best-guesses (no ARM machine available to check against), chosen only
to be internally distinct from the two confirmed x86/x64 values.

**Deliberately deferred to a later M5 slice**: `DataSection`/`Data`/`Read`/`Restore`;
non-recursive `Macro`/`EndMacro`; `IncludeFile`/`XIncludeFile`. None of these have been
oracle-verified yet.

## M5b Implementation Notes (`DataSection`/`Data`/`Read`/`Restore`)

**Scope landed**: `DataSection`/`EndDataSection`, `name:` labels, `Data.<suffix> v1[, v2,
...]`, `Read[.<suffix>] varname`, `Restore label`. Non-recursive `Macro`/`EndMacro` and
`IncludeFile`/`XIncludeFile` remain for a later M5 slice.

**`Data` requires an explicit type suffix - oracle-verified** ("A type or structure must be
specified after 'Data'."), unlike almost every other suffixed construct in this language
(`Define`, `For`, a bare `Read` itself), which default to Integer when the suffix is
omitted. `Read` has no such requirement; a bare `Read x` is legal and, per this project's
own convention, defaults to Integer (a *documented* divergence - real PB's own bare `Read`
appears to default to its platform-width `.i`, which can desync its raw-byte cursor
against mismatched-width `Data.l` items; this project's own pool tracks one logical item
per cursor step rather than raw bytes, so this particular failure mode can't occur, and
there's nothing to match it against).

**The single most surprising oracle finding this slice turned up**: `Read[.suffix]
varname` behaves exactly like a plain assignment (`varname = <next data pool value>`) for
type-*checking* purposes - it is **not** type-checked against its own suffix at all, and
critically, an *undeclared* `varname` is auto-declared as **Integer**, completely ignoring
`Read`'s own suffix. `Read.s s1` into a fresh `s1` leaves `s1` an Integer, not a String -
`Debug s1` then prints `Val("hello")` = `0`, not `"hello"`. Getting a String value out of
`Read.s` requires pre-declaring the destination as `.s` first (`Define s1.s` then `Read.s
s1`) - oracle-verified exactly this way. This single finding shaped the whole feature's
Sema design: `Sema`'s own `Read` case is just `declareImplicit(varname, spelling,
typeOf(varname), loc)` - the same idempotent "declare as Integer if new, otherwise leave
alone" pattern the `Assign` case already uses - with **no** type-checking against `Read`'s
own suffix at all, matching the oracle's complete silence on any such mismatch.

**A second, related finding governs Codegen's own conversion logic**: real PB's `Read`
genuinely allows a String<->numeric cross-family coercion (`Val()`/`Str()`-style) between
the `Data` item's own type and the destination variable's type - something a normal PB
*assignment* never allows (`Sema::checkAssignable` rejects a String<->numeric mix
outright, and `Codegen::convert()` is written assuming that rejection already happened, so
it can't be reused as-is here). A new `Codegen::convertReadValue` handles this one, genuine
exception - cross-family String<->Integer/Double via `pbVal`/`pbStr`/`strtod`/
`std::to_string`, falling back to the existing `convert()` for a same-family pair so its
width/banker's-rounding logic isn't duplicated.

**Exhausting the data pool is a *debug-mode-only* fatal error - oracle-verified**: `Read`ing
past the last `Data` value under `-d` produces `[Debugger Error]  Read data error: no more
data.` and a fatal exit (code 1); the exact same program compiled *without* `-d` just
silently continues (release mode has no such check at all, exactly like `Debug` statements
themselves vanishing in a release build). Codegen emits the `pbDataHasMore()` guard +
`pbDataReadError()` call only when `debugMode_` is true, mirroring the `Debug` statement
case's own established convention precisely. The exact message text/file-line reference
isn't replicated byte-for-byte (no source-location plumbing reaches this runtime call
today) - only the oracle-verified *contract* (debug-only fatal error on exhaustion) is.

**Three oracle findings shaped the "resolve once, globally, before anything else" pre-pass
architecture** (`Sema::collectDataSections`, run once over the whole `Module` before the
main semantic walk): (1) multiple `DataSection`s anywhere in the file - even one nested
inside a `Procedure` (oracle-verified: the resulting data is readable via the ordinary
shared cursor, across separate calls to that Procedure too) - concatenate into one single,
flat, global pool, with `Read` continuing seamlessly across the boundary with no `Restore`
needed; (2) a bare `Restore` with no label is a syntax error - oracle-verified, unlike most
other PB statements with an optional argument; (3) `Restore` can **forward-reference** a
label defined *later* in the file (oracle-verified directly: `Restore LaterLabel` followed
by `Read`, with the `LaterLabel:`/`Data` only appearing afterward, reads correctly) - which
is what rules out resolving labels during the ordinary single top-to-bottom semantic walk,
since a label's index has to be known before anything that might reference it, regardless
of which one textually comes first.

**Resolving `DataSection`/label-indexing correctly *together with* M5a's `CompilerIf`
splicing required one further refactor**: a `DataSection` can legally sit inside a
`CompilerIf` branch (not separately oracle-verified, but there is no reason real PB would
treat it differently from a `Procedure`, which M5a already confirmed is legal there), and
`collectDataSections` needs to see the tree in its *final* shape - with every `CompilerIf`/
`CompilerSelect` already resolved away - or a label's computed index could disagree with
what `Codegen`'s own, identically-shaped recursive scan (`genDataPool`) later emits. Rather
than accept that as a known gap, M5a's own CompilerIf-splicing logic (previously inline
inside `visitBlock`) was pulled out into its own standalone whole-Module pre-pass,
`Sema::preResolveCompilerDirectives` - run first, before `collectDataSections` - so by the
time label-indexing runs, no `CompilerIf`/`CompilerSelect` node exists anywhere in the tree
at all. This pre-pass also does a lightweight preview of `ConstDecl`/`Enumeration`'s own
value computation (just enough to populate `constantIntValues_`), preserving M5a's existing
guarantee that a `CompilerIf` can reference a `#Constant` declared earlier in the same file
- a property that would otherwise have been lost by moving CompilerIf resolution earlier
than the main walk. `visitBlock` itself reverted to its original, simple form as a result.

**A deliberate, documented divergence from real PB's own raw-memory-blob model**: this
project's data pool is a `std::vector` of a small tagged variant (`PBDataValue`, one
logical item per cursor step), not a byte-for-byte replica of PB's own internal
representation (which appears to be a flat byte blob where `Read`'s own suffix determines
exactly how many raw bytes to consume at the current byte offset, with **zero** runtime
type tagging). For a program where every `Read` matches its corresponding `Data` item's
own declared type - the overwhelming common, intended case - the two models are provably
identical (verified directly against the oracle, including a `Restore`-based forward
reference and a `DataSection` inside a `Procedure`, byte-for-byte). They diverge only for a
*type-mismatched* sequence (e.g. `Read.l` immediately after a `Read.s` whose declared
`Data.s` item was narrower or wider than a `.l`'s own byte width) - oracle-verified to
desync real PB's own byte-level cursor, producing outright garbage on every subsequent
read; this project's logical-item cursor can't (and doesn't try to) reproduce that specific
garbage, since byte-for-byte ABI fidelity was never a goal of this project (the same
principle already applied to `PeekS`/`PokeS`'s own `@stringVar` limitation in M4c).

## M5c Implementation Notes (non-recursive `Macro`/`EndMacro`) - M5 complete

**Scope landed**: `Macro name[(param1, param2, ...)] ... EndMacro`, invoked either
`name(arg1, arg2, ...)` or (zero-parameter only) bare `name`. This closes out M5 -
`CompilerIf`/`CompilerSelect` (M5a), `DataSection`/`Data`/`Read`/`Restore` (M5b), and now
`Macro` are all done. `IncludeFile`/`XIncludeFile` were never in this milestone's own scope
and remain unimplemented.

**The single most consequential oracle finding**: a macro parameter is substituted as raw,
unparenthesized *tokens*, not a pre-evaluated value - `Macro Square(x) : x*x : EndMacro`
invoked as `Square(2+3)` expands to the literal token sequence `2+3*2+3`, which is `11`
under ordinary operator precedence, **not** `25` - the classic C-preprocessor "forgot the
parens" result. This single fact rules out doing `Macro` the way `CompilerIf`/`DataSection`
were done (an AST-level transformation inside `Sema`, working with already-parsed,
already-typed expression trees) - by the time anything is parsed into an expression tree,
the "raw text, not a value" distinction that makes `Square(2+3)` give `11` is already lost.
`Macro` is instead a genuine **token-level textual substitution pass** (`MacroExpander`,
new `compiler/src/preprocessor/` - the directory existed empty since M0, originally
intended for exactly this), run between the `Lexer` and the `Parser`, mirroring a C
preprocessor - and, per real `pbcompilerc`'s own error text referencing "the expanded macro
(Macro.out)" when something goes wrong inside one, apparently mirroring real PB's own
implementation strategy too.

**A second oracle finding governs the other half of the design**: a zero-parameter macro is
invoked **bare**, with no parens at all - `Greet` works, but `Greet()` is a syntax error
("Garbage at the end of the line"), the *opposite* of a zero-arg `Procedure` call (which
requires `()`). A bare identifier is otherwise genuinely ambiguous with a plain variable
reference at the grammar level, so recognizing "this identifier is actually a macro
invocation" has to happen by name lookup directly against the token stream, before the
Parser's own grammar-driven disambiguation ever runs - another point in favor of a pre-
Parser token pass rather than anything Parser- or Sema-level.

**A methodologically important correction caught mid-slice**: an early design iteration
collected *every* `Macro` definition in the file into a lookup table first (mirroring
`Sema::collectDataSections`'s own whole-Module pre-pass for `Restore`'s forward-reference
support), then expanded invocations against that complete table - which would make a macro
invocation *forward-reference* a definition appearing later in the file. This looked
correct under a `-k` syntax-only check (already a known pitfall - see the M3f/M2-closure
notes on `-k` not validating that a called name is real) - but a full `-d -o` compile
reveals real PB actually **rejects** this ("... is not a function, array, list, map or
macro."). Caught specifically by re-verifying with a full compile instead of trusting the
earlier `-k` result, this ruled out the whole-file-pre-pass design: `MacroExpander` instead
populates its definition table *incrementally*, in a single left-to-right scan - a macro is
only available for expansion from the point its own `Macro ... EndMacro` is reached onward,
exactly like a genuine single-pass preprocessor (and, oracle-verified the other way,
*unlike* `DataSection`'s own labels, which genuinely do support a forward `Restore`
reference via a full compile+run check, not just `-k`).

**A real bug caught before it ever reached the oracle comparison**: the first working
version of the token-level substitution pasted a macro's *entire* captured body - including
the leading `NewLine` token ending the `Macro name(...)` header line itself, and the
trailing one right before `EndMacro` - verbatim into the invocation site. For an expression-
style macro this silently broke the *invoking* statement: `Debug Double(5)` (body `(x) * 2`)
expanded to `Debug` <newline> `(5) * 2` as two separate, broken statements, since the
leading `NewLine` terminated the `Debug` statement immediately with no expression at all.
Fixed by trimming only the *outermost* leading/trailing `NewLine`/`Colon` from a captured
body - an *internal* separator (between two statements in a multi-statement macro like
`PrintBoth`) is left untouched, since that one is genuinely meaningful content.

**Nested macro invocation and non-recursion are both handled by the same recursive
design**: `MacroExpander::expandTokens` recurses into a macro's own (parameter-substituted)
body to fully expand it before splicing the result into the output, so `Outer(a)` calling
`Inner(a)` (oracle-verified) naturally bottoms out correctly regardless of which macro was
defined first in the file. The same recursion carries an `activeExpansion_` name set,
checked on entry to each nested expansion - oracle-verified real PB itself detects and
rejects direct/indirect self-recursion ("Endless recursivity detected in the Macro.")
rather than hanging forever; since the set tracks the *whole* active expansion chain (not
just the immediate caller), a longer mutual-recursion cycle would be caught the same way,
though that specific case wasn't separately oracle-verified.

**Deliberately not chased byte-for-byte**: the exact wording of real PB's own diagnostics
for a missing/forward-referenced macro, a wrong argument count, or self-recursion - this
project raises its own clear, distinct diagnostic for each (verified to actually fire, via
both targeted `MacroExpander` unit tests and a full pbcxx run), not real PB's exact text.
`IncludeFile`/`XIncludeFile` remain unimplemented and unverified - out of this milestone's
stated scope from the start.

## M6 Implementation Notes (CI, verified for real)

**The headline finding**: this project's `ci.yml`/`nightly.yml` were written back in M0, matching
the original plan's CI-wiring design, but had **never actually run** - the `github` remote had
been silently failing every push since M0 due to a stale/wrong-account git credential, so GitHub
saw an empty repository the whole time. Fixed by switching the active `gh` account, refreshing it
with the `workflow` scope (needed to push changes under `.github/workflows/`), and replacing the
stale cached credential. The very first real run, on 18 commits of accumulated history, failed 3
of 5 jobs - every failure a genuine, previously-undetected bug, not a flake. Closing these out
*is* M6's real work; the YAML itself needed only the cppcheck invocation fixed.

**static-analysis (cppcheck)**: took three attempts to get right, each one only exposed by the
*actual* CI environment, not local reasoning. (1) `--enable=all --error-exitcode=1` turned every
accepted style-only note into a hard failure. (2) Narrowing to `--enable=warning,performance,
portability` passed locally but still failed in CI: Ubuntu 24.04's apt cppcheck (2.13.0) is far
older than this dev machine's own (2.19.0) and classifies several checks differently -
`duplicateAssignExpression` lands under `warning` there, not `style`. (3) Even
`--enable=style,warning,performance,portability` plus explicit suppressions still failed once
more, since `--enable=all` (tried as an alternative) additionally pulls in `information`-severity
notices, which this cppcheck version's `--error-exitcode` counts as failures too. The eventual fix
- explicit `--suppress=` for five specific, already-reviewed style IDs (`useStlAlgorithm`,
`constVariableReference`, `unusedFunction`, `duplicateAssignExpression`, `noExplicitConstructor`,
`shadowFunction`), `--enable=style,warning,performance,portability` (never `all`) - was finally
verified correctly by running cppcheck 2.13.0 itself, via a local `docker run ubuntu:24.04`,
instead of trusting this machine's own newer version a third time. Also fixed a real, independent
gap this exposed: `cppcheck compiler/src runtime/include` had *never* actually scanned
`runtime/include`'s own substantial logic (PBString, PBList/PBMap, every stdlib header) at all -
a header-only directory with no `.cpp` of its own isn't a translation unit cppcheck treats as
checkable from a bare directory argument - fixed by passing its `.hpp` files explicitly.

**sanitizers**: a genuine UBSan vptr-check failure, unrelated to M5 - `sema_test.cpp`'s field-
access-chain test cast `module->statements[2]` to `ast::DebugStmt*` when the `Debug` statement was
actually at index `[3]` (index `[2]` is `Define r.Rect`). This machine's own local heap layout
never happened to trip the invalid downcast; CI's did. Also caught independently by a local
`ctest -T memcheck` run (see below) before the CI fix even landed, confirming it wasn't a CI-
environment fluke.

**windows-mingw**: three real, previously-unverified portability bugs, found by actually building
and running the full suite on Windows for the first time ever. (1)
`runtime_filelib_test.cpp`'s `ScratchPath` hardcoded `/tmp/...`, not a real path on Windows -
fixed with `std::filesystem::temp_directory_path()`. (2) The filelib e2e/e2e_diff `.pb` test had
the identical bug in PB source itself (also hardcoded `/tmp/...` paths) - fixed by switching to
plain relative filenames, which in turn required `run_case.sh`/`diff_against_pbcompilerc.sh` to
`cd` into their own per-test scratch directory before running the compiled binary (previously
they didn't, relying on the hardcoded absolute path instead - a design that also had a latent
parallel-test-collision risk even on Linux, now closed too). (3) The `compilerif` golden e2e test
printed the literal detected OS name (`"linux"`), correct only on the Linux/x64 machine it was
captured on - genuinely wrong once actually compiled on Windows, which does select the
`#PB_OS_Windows` branch there. Redesigned to assert the portable invariant ("exactly one known
OS/Processor value matches") instead of a specific name, and moved the "non-taken branch isn't
type-checked" check onto a condition that's false on every real platform (`#PB_Compiler_OS =
99999`) rather than tied to one specific OS - the differential (`e2e_diff`) variant of the same
test still exists for actually verifying the *real* platform-specific OS/Processor detection
against the live oracle, which stays meaningful since it only ever runs where `pbcompilerc` is
installed (this Linux dev machine).

**Haiku CI - resolved once the machine itself came online**: a real Haiku machine (R1~beta6+
development, hrev60192, x86_64) was unreachable earlier in this same session (wrong/stale IP, then
genuinely powered off), but once reachable, `cmake --preset haiku` configured cleanly, the full
build completed with zero warnings in this project's own code, and `ctest --preset haiku` passed
230/231 on the first try. The one failure was a genuine bug in the `compilerif` e2e test's own
earlier Windows-portability fix: it checked `#PB_Compiler_OS` against `#PB_OS_Windows`/`Linux`/
`MacOS` only, forgetting `#PB_OS_Haiku` (a pbcxx-specific extension constant - Haiku isn't an
officially PB-supported platform) - so `isKnownOS` read `0` instead of `1` when actually compiled
*for* Haiku, a "portability fix" not accounting for this project's own actual target platform.
Fixed by adding the missing `CompilerElseIf`; re-verified 231/231 afterward.

**Automated Haiku CI - the official runner can't run on Haiku at all**: GitHub's own Actions runner
is a .NET application, and Haiku has no .NET/Mono support whatsoever (confirmed: no `dotnet`/`mono`
binary, no matching `pkgman` package). A `haiku` CI job was still added, but as a **self-hosted
runner on the Linux dev machine** (labeled `haiku-bridge`, installed via the standard
`config.sh`/`svc.sh install` flow as a systemd service) whose own steps run normally on Linux but
reach over SSH to the real Haiku machine on the same LAN to do the actual rsync + `cmake --preset
haiku` + `cmake --build` + `ctest` - scripting exactly what was just done by hand. Needed one fix
after its first real run: `rsync` can create the leaf directory on the remote end but not a missing
parent (`/boot/home/ci/` didn't exist yet), so the job now `mkdir -p`s the remote directory itself
first rather than depending on that having been done manually. Both machines - this Linux one (for
the bridge runner) and the Haiku one - need to be reachable for this job to pass; if either is off,
the job fails or hangs rather than silently skipping, a real and accepted tradeoff of this design
versus a GitHub-hosted runner.

**Nightly Valgrind redesigned after discovering it was checking the wrong thing entirely**: the
original `ctest -T memcheck` wraps each e2e test's own `bash run_case.sh ...` command; without
`--trace-children=yes`, Valgrind only ever instruments *bash's own interpreter*, not the `pbcxx`/
compiled-program child processes bash spawns - confirmed directly by inspecting a real memcheck
log, which reported the exact same "32 bytes direct + 24 indirect lost" leak, traced to
`make_simple_command`/`yyparse` inside `/usr/bin/bash` itself, identically for every single e2e
test regardless of which `.pb` program it ran. Adding `--trace-children=yes` does reach the right
processes, but also traces into the `g++` invocation `pbcxx` itself shells out to - correct, but
30+ seconds per e2e test under Valgrind's own instrumentation overhead, impractical for the full
231-test suite even nightly. Replaced with two direct, untraced Valgrind invocations instead: the
Catch2 unit test binary (in-process, no subprocess spawning at all, ~4s for all 183 cases) and
`pbcxx` itself compiling every e2e test's own `input.pb` one at a time (no child-tracing needed,
since the point is `pbcxx`'s *own* memory use, not `g++`'s - ~1-2s each). Verified clean on real
CI via a manual `workflow_dispatch` trigger, not just locally.

## M7 Scoping Notes

M7 was left as "deferred/optional, not scoped" throughout M0-M6. This section records the scoping
pass that splits it into four ordered sub-milestones - **threads (M7a) first, then GUI core (M7b),
then `Interface`/`Module` last (M7c/M7d)** - per explicit user direction, since GUI alone is large
enough that front-loading it would otherwise crowd out the smaller, self-contained pieces.

**A major research resource for M7b**: a separate, independent project at
`~/git/PureBasic/qt6_subsystem` spent 46 milestones empirically reverse-engineering real
PureBasic's entire GUI command surface (by decoding the real, installed Qt5 subsystem's `.pbl`
binaries and cross-referencing real official examples and, eventually, the real PureBasic IDE's own
~100K-line source) while building a *Qt6 subsystem plugin* for PureBasic's own compiler - a
fundamentally different target than this project's (a subsystem is a binary PureBasic's own
compiler loads via its own ABI; `pbcxx` instead just needs its own runtime library functions,
called directly from generated C++, with the right *PB-level* semantics - no subsystem-loading
mechanism, `.pbl`/`.desc` packaging, or binary ABI compatibility needed at all). Despite the
different target, that project's `docs/` are an extremely valuable *reference* for this scoping
pass: a complete, empirically-verified catalog of real command signatures, argument orders, return
conventions, event semantics, and a natural dependency-ordered implementation sequence (window/event
core, basic gadgets, menu, statusbar/toolbar, systray, more gadgets, dialog, ...) - all independently
confirmed against real PureBasic, not assumed. `pbcxx`'s own GUI work should still verify anything
load-bearing against the oracle directly before relying on it (this project's own established
practice), but doesn't need to re-derive this command catalog or ordering from scratch.

### M7a: Threads

Oracle-verified minimal API: `CreateThread(@Procedure(), param)` returns a thread ID; `IsThread`/
`WaitThread`/`KillThread`; `CreateMutex()`/`LockMutex`/`UnlockMutex`/`FreeMutex`; `CreateSemaphore()`/
`SignalSemaphore`/`WaitSemaphore`/`FreeSemaphore`. Maps cleanly onto `std::thread`/`std::mutex`/
`std::counting_semaphore` in the runtime. One oracle-observed detail to not over-engineer: running
threads without a `ThreadSafe` compiler mode produces a `[Debugger Warning]` (not an error) in real
PB, about its own internal debugger/runtime structures needing locking - `pbcxx` doesn't share that
internal-state-sharing concern (its own runtime is written thread-safely in ordinary C++ from the
start), so this warning doesn't need replicating. Smallest, most self-contained of the four pieces -
good first M7 slice.

### M7b: GUI core (GTK3)

GTK3 confirmed available for linking on both primary dev platforms: this Linux machine (3.24.52 via
`pkg-config`) and the real Haiku machine (`gtk3`/`gtk3_devel` packages exist via `pkgman`, though
`gtk3_devel` isn't installed yet - a setup step for whenever Haiku GUI work actually starts, not a
blocker to scoping). Matches real PureBasic's own default Linux GUI backend (GTK3 since PB 6.0),
per the user's explicit direction.

**Deliberately phased, not planned end-to-end up front** - the reference project's own 46-milestone
arc is itself evidence that "full PureBasic GUI" isn't a one-shot scope. Proposed first slices,
each independently shippable and oracle-verified like every other milestone in this project:

1. **Window + event core**: `OpenWindow`/`CloseWindow`/`IsWindow`/`ResizeWindow`/`HideWindow`,
   `WindowEvent`/`WaitWindowEvent`, the `#PB_Event_*` constants (`CloseWindow`, `Gadget`, `Menu`,
   `SizeWindow`, ...), `EventWindow`/`EventGadget`/`EventType`. This alone is enough for the
   simplest real event-loop-driven programs.
2. **Basic gadgets**: `ButtonGadget`/`TextGadget`/`StringGadget`/`CheckBoxGadget`/`FrameGadget` +
   generic gadget management (`ResizeGadget`/`HideGadget`/`DisableGadget`/`FreeGadget`,
   `SetGadgetText`/`GetGadgetText`, `SetGadgetState`/`GetGadgetState`).
3. **`MessageRequester`** - the simplest modal dialog, and genuinely useful on its own (real PB code
   uses it standalone constantly, independent of a full window - see the `Thread.pb` example).
4. **`Menu`/`StatusBar`/`ToolBar`** - the next tier real small desktop apps commonly need.
5. Further gadget types (`ListView`/`ComboBox`/`Option`/`ProgressBar`/`Image`/`Tree`/`ListIcon`/
   `Panel`/...), the fuller `Requester` family (file/color/font/input pickers), `Dialog`/
   `OpenXMLDialog`, and everything past that (2D/vector drawing, multimedia, 3D, web gadgets,
   scripting, OpenGL, drag-drop, clipboard, printer, scintilla, ...) - explicitly open-ended,
   scoped incrementally as real need (or interest) comes up, the same way M4's library coverage
   grew one sub-letter at a time rather than being fully pre-planned.

**Design stance, stated up front to avoid relitigating it per-slice**: `pbcxx`'s GUI runtime will
call GTK3 directly from generated C++ (plain `gtk_window_new`/`gtk_button_new_with_label`/
`g_signal_connect`/etc. wrapped in `runtime/include/easybasic/runtime/guilib.hpp`-style headers),
not attempt to replicate PureBasic's own internal `PB_Object`/gadget-ID-table ABI or ship anything
resembling a loadable subsystem plugin. PB's own event-loop model (`WaitWindowEvent()` as an
explicit, PB-code-visible blocking poll) maps naturally onto GTK3's own main loop via
`gtk_main_iteration()`-style pumping rather than `gtk_main()`'s normal callback-driven model, since
real PB code structures its OWN control flow around the poll, not callbacks, for the base
`WaitWindowEvent`-style API - the first oracle question to settle empirically once implementation
starts, not assumed here.

### M7c: `Interface`/`EndInterface`

**Oracle-verified finding that reshapes this slice's design**: real PB's `Interface` is a thin
syntactic layer over a *manually-built, programmer-visible vtable* - not an automatic OOP feature.
A real, working example compiled and ran correctly:

```purebasic
Interface Shape
  Area.d()
  Name.s()
EndInterface

Structure CircleData
  *VTable
  radius.d
EndStructure

Procedure.d Circle_Area(*this.CircleData) : ProcedureReturn 3.14159 * *this\radius * *this\radius : EndProcedure
Procedure.s Circle_Name(*this.CircleData) : ProcedureReturn "Circle" : EndProcedure

DataSection
  CircleVTable:
  Data.i @Circle_Area()
  Data.i @Circle_Name()
EndDataSection

c.CircleData
c\VTable = ?CircleVTable
c\radius = 5
*shape.Shape = @c
Debug *shape\Area()   ; 78.5397
Debug *shape\Name()   ; Circle
```

The Structure's *first field* is a raw `*VTable` pointer; the `DataSection` lays out function
addresses (`@Procedure()`) in the same order the `Interface` declares its methods; `?Label` (a new
primitive, not yet implemented - see below) takes the compile-time address of that `DataSection`
label; assigning a struct's address to an `Interface`-typed pointer is a bare reinterpretation, with
**no compile-time conformance checking at all** - nothing verifies the vtable's function pointers'
signatures actually match the `Interface`'s declared methods, exactly like a hand-written C vtable.
This actually *simplifies* the implementation versus a "real OOP interface" assumption: `Sema` only
needs, per declared `Interface`, an ordered list of method names (for resolving `\MethodName(args)`
to a vtable slot index) - no structural type-checking between a `Structure` and the `Interface`s it's
used through, since real PB does none either. `Codegen` lowers `*shape\Area()` to indexing the
vtable pointer at `shape`'s own address by the method's declared position and calling through the
resulting function-pointer type, with `*this` as an implicit first argument.

**New prerequisite surfaced by this slice**: `?Label` (address of a `DataSection` label) is a
genuinely separate primitive from M5b's own `Data`/`Read`/`Restore` cursor-based access - it hands
back a raw, directly-usable pointer into the data pool rather than going through the sequential
cursor at all. Not implemented as part of M5b (out of that milestone's own stated scope at the
time); needs to land as a small prerequisite step within M7c rather than a retroactive M5b
addition, since `Interface` is its only currently-known real use case.

### M7d: `Module`/`DeclareModule`/`EndModule`

Oracle-verified: `DeclareModule Name ... EndDeclareModule` (forward declarations/public `Global`s)
paired with `Module Name ... EndModule` (the implementation, which can give those same `Global`s
their initializers) and `Name::Member` qualified access from outside. Maps far more directly onto
existing compiler infrastructure than `Interface` does - this is essentially a C++ `namespace`
wrapped around already-supported `Procedure`/`Global` declarations, with `::`-qualified lookup
added to name resolution. Likely the most mechanically straightforward of the four M7 pieces once
reached, though not yet designed in detail (deprioritized to last, per the user's own explicit
ordering).

## M7a Implementation Notes (Threads)

**Scope landed**: `CreateThread`/`IsThread`/`WaitThread`, `CreateMutex`/`LockMutex`/`UnlockMutex`/
`TryLockMutex`/`FreeMutex`, `CreateSemaphore`/`SignalSemaphore`/`WaitSemaphore`/`TrySemaphore`/
`FreeSemaphore`, plus `Delay`/`ElapsedMilliseconds` (not thread-specific commands in real PB, but
needed immediately by any real thread-timing test, bundled in rather than given their own
single-purpose library). `KillThread`/`PauseThread`/`ResumeThread`/`ThreadID` deliberately
deferred - see below.

**A genuinely new primitive, needed before `CreateThread` itself could work at all**:
`@ProcedureName()` (a procedure's own address, as opposed to `@variable`/`@array(i)`, which were
already supported) wasn't implemented - confirmed by a hard compile error (`lvalue required as
unary '&' operand`, since the existing `AddressOf` codegen tried to take the address of *calling*
the procedure, `&(f_proc())`, not the function itself). Oracle-verified it's always written with
empty parens regardless of the named procedure's own parameter count (`@Worker()` is legal even
though `Worker(n)` itself takes one parameter - the real `Thread.pb` example's own shape), so this
needed its own Sema special-case (checked before the operand is visited at all, mirroring the
List/Map bare-`name()` special-casing already in `visitExpr`'s `Call` case): if `@`'s operand is a
`Call` naming an actual declared procedure, skip the ordinary call-arity validation entirely rather
than reporting a missing-argument error, and emit `reinterpret_cast<std::int64_t>(&f_procname)` in
Codegen (the function's own address, not a call followed by address-of its result). This is a
genuinely reusable primitive beyond M7a - any future feature needing a raw procedure pointer (e.g.
a callback-registration API) can reuse it unchanged.

**Oracle-verified semantics that shaped the runtime design**:
- `WaitThread`'s return value is a plain success flag (`1`), **not** the thread procedure's own
  `ProcedureReturn` value - confirmed directly with a `Procedure Worker(n): ProcedureReturn 999`
  thread whose `WaitThread` result printed `1`, never `999`. Real PB threads communicate back via
  shared globals/`Mutex`/`Semaphore`, not a return channel - so the runtime's own entry-function
  return value is simply discarded, no propagation machinery needed.
- PB's `Mutex` is **re-entrant (recursive)**: a second `TryLockMutex` from the same thread that
  already holds the lock succeeds too, confirmed directly (`TryLockMutex`/`TryLockMutex`/
  `UnlockMutex`/`TryLockMutex` all return `1` in sequence) - mapped to `std::recursive_mutex`, not
  plain `std::mutex` (re-locking the latter from its owning thread is undefined behavior).
- `CreateSemaphore()` defaults its initial count to `0`; `CreateSemaphore(3)` starts with 3 already
  available - confirmed via a `TrySemaphore` exhaustion sequence. Maps directly onto
  `std::counting_semaphore<>`'s own constructor argument, with the runtime function's own C++
  default parameter (`= 0`) handling the no-argument call - Codegen's existing "only emit the args
  actually given, let the callee's own default fill the rest" call-site logic (already established
  for `pbRandom`'s optional min bound) needed no changes.
- `KillThread` on an already-finished thread is a **fatal debugger error** in real PB ("The
  specified Thread does not exists."); calling it on a genuinely still-running thread sent this
  whole test machine's shell a raw `SIGUSR2` that killed the entire process, not just the target
  thread - real PB's own `KillThread` is implemented via OS-level forced thread termination (signal-
  based on Linux), which is inherently unsafe (skips destructors, can corrupt shared state) even in
  the real implementation. Given C++ has no safe, portable equivalent (`pthread_cancel` is POSIX-
  only and still discouraged; Windows' `TerminateThread` is equally unsafe), `KillThread`/
  `PauseThread`/`ResumeThread` (no portable "pause an arbitrary running thread" primitive exists
  either) and `ThreadID` (no example or use case found needing it, and no oracle time spent on it)
  are deliberately left unimplemented rather than built on fundamentally unsafe foundations -
  real-world PB threading code should prefer cooperative shutdown (a shared `finished` flag the
  thread procedure itself checks) over `KillThread`, which this project's own design doesn't
  obstruct.

**A real bug found and fixed before it could ship, by reasoning through the ownership chain (not
by a failing test)**: the first working draft inserted a thread's handle into the global thread
table *before* constructing the real `std::thread` object (`handle->thread = std::thread(...)` came
second). A `WaitThread` racing in immediately after `CreateThread` returns could then observe a
still-default-constructed (non-joinable) `std::thread` and skip joining entirely, silently
returning without actually waiting. Fixed by constructing the thread first, publishing the handle
into the table only once it's fully set up.

**A second real bug, this one caught by thinking through real PB's own documented usage pattern**:
a `std::thread` still joinable at destruction calls `std::terminate()` - but the actual `Thread.pb`
example starts a thread with an infinite `Repeat/ForEver` loop and simply lets the whole process
exit once a blocking `MessageRequester` returns, with no explicit `WaitThread`/cleanup at all. Under
a naive implementation, the global `ThreadHandle` table's own destruction at program exit would
have called `std::terminate()` on that still-running thread, crashing *pbcxx's own generated
program* in a scenario real PB handles cleanly. Fixed with an explicit `ThreadHandle` destructor
that detaches (not joins) a still-joinable thread - matching real behavior exactly: the OS tears the
detached thread down along with the rest of the process at exit. Verified directly (not just
reasoned about): a real program with an infinite-loop background thread and no `WaitThread` call
exits cleanly with code 0, byte-for-byte matching the oracle's own output, both normally and under
a direct ASan/UBSan sanitizer compile of the generated code.

**Toolchain note**: `std::thread`/`std::mutex`/`std::counting_semaphore` need a real threading
backend linked in - added `find_package(Threads REQUIRED)` + `Threads::Threads` to the
`easybasic_runtime` CMake target (for the unit tests and `pbcxx` itself), and `-pthread` to the
backend-compiler invocation `pbcxx`'s own driver shells out to for *generated* programs (a
separate, runtime concern CMake's own linking has no reach into). Confirmed working on both Linux
and a real Haiku machine (`Threads::Threads` resolves automatically on both via CMake's own
platform detection) - Windows/MinGW not yet verified for this specific milestone (will be confirmed
via the existing `windows-mingw` CI job on the next push).

**`diff_against_pbcompilerc.sh` gained an always-on `-t`/`--thread` (`ThreadSafe` mode) flag** for
the oracle side of every differential test, not just thread ones - confirmed it changes nothing for
a non-threaded program (`hello_world`'s own output is byte-identical with or without it), but
without it real PB prints an extra `[Debugger Warning]  ThreadSafe mode should be enabled when
using threads.` pair of lines for any program that actually uses threads, which `pbcxx` has no
equivalent internal-debugger-state concern to warn about. Simpler and more correct than trying to
filter or special-case that warning out of the diff itself.

## M7b Implementation Notes (GUI core, first slice: Window + event core)

**Scope landed**: `OpenWindow`/`CloseWindow`/`IsWindow`/`ResizeWindow`/`HideWindow`,
`WindowEvent`/`WaitWindowEvent`, `EventWindow`/`EventGadget`/`EventType`, the `#PB_Event_*`
(`Menu`=1, `CloseWindow`=2, `Gadget`=3, `Repaint`=4, `MoveWindow`=5, `SizeWindow`=6,
`ActivateWindow`=7, `Timer`=15, `FirstCustomValue`=65536) and `#PB_Window_*` (`Invisible`=1,
`SizeGadget`=2, `SystemMenu`=4, `TitleBar`=8, `MaximizeGadget`=16, `MinimizeGadget`=32,
`ScreenCentered`=64) constants - all oracle-verified by direct `Debug #PB_X` probes.
`#PB_Event_RemoveWindow` does **not** exist in real PB (oracle: "Constant not found") despite
looking like a plausible name next to `CloseWindow`/`Gadget`/etc. - a real case of `-k` correctly
catching a bad *constant* name, distinct from this project's long-standing note that `-k` does
*not* validate called function names.

**New per-program conditional build dependency, kept out of everything else**: `guilib.hpp`
(`runtime/include/easybasic/runtime/`) is deliberately **not** part of the plain `runtime.hpp`
umbrella every other runtime library joins - it transitively pulls in `<gtk/gtk.h>`, which a
program that never touches the GUI library shouldn't need installed at all. `Sema::usesGuiLibrary()`
(set in `visitCall` whenever a GUI builtin is actually called) drives two independent conditional
steps: `Codegen::generate()` only emits `#include <easybasic/runtime/guilib.hpp>` for a program
that needs it, and `main.cpp`'s driver only shells out to `pkg-config --cflags --libs gtk+-3.0`
(via `popen`, printing a clear "install GTK3's dev package" error rather than a confusing
downstream g++ failure if it fails) and appends the resulting flags for that same program's
backend-compiler invocation. **Linker-ordering gotcha hit immediately**: the first working version
put pkg-config's combined `--cflags --libs` output all before the generated `.cpp` source file on
the command line - GNU ld resolves library symbols only against object files it's already seen, so
`-lgtk-3` before the `.cpp` that calls into it is a silent "undefined reference" at link time, not a
configuration error. Fixed by querying `--cflags` and `--libs` *separately* and placing them on
opposite sides of the source file (cflags before, libs after), rather than keeping pkg-config's one
combined invocation.

**The central design problem, same one scoped in the M7 notes above**: PB's own GUI API is
poll-based (`WaitWindowEvent()` blocks and hands back what happened); GTK3's is callback-based (a
signal fires while pumping the main loop). Bridged with a plain FIFO (`detail::eventQueue()`) that
every GTK signal handler this slice registers (`"delete-event"`, `"configure-event"`, `"draw"`)
just pushes onto; `pbWindowEvent`/`pbWaitWindowEvent` are the *only* functions that ever pump GTK's
own main loop (`gtk_main_iteration()`) and pop from the queue. GTK itself is lazily initialized on
first real use (a C++11 "magic static" `gtk_init()` call), so a program that never calls a GUI
builtin never touches GTK at runtime, matching the build-time conditionality above.

**Oracle-verified return-value simplification, consistent with this project's "PB source-level
semantics, not PB's own internal ABI" stance since M0**: `OpenWindow`'s and `IsWindow`'s real
return values are large, internal-pointer-looking nonzero Integers, not clean booleans (confirmed:
two different huge values for the same window from the two different calls) - real PB code only
ever uses them for truthiness (`If OpenWindow(...)`), never compares to a literal, so `pbcxx`'s own
`pbOpenWindow`/`pbIsWindow` return a plain `1`/`0` instead. Similarly, oracle-verified that
re-opening an already-open window ID is a harmless no-op returning the existing window's own value
rather than erroring or creating a second one - **not** replicated exactly (`pbOpenWindow` destroys
any existing widget at that ID and creates a genuinely new one instead), a narrow, documented
divergence for an edge case no real program is likely to rely on deliberately.

**Move vs. resize disambiguation**: GTK's single `"configure-event"` signal fires for *any*
geometry change without saying which part changed. Solved with a per-window `WindowState{x, y,
width, height}` struct (attached via `g_object_set_data`, freed in `CloseWindow`) diffed against
each incoming event, emitting `#PB_Event_MoveWindow` and/or `#PB_Event_SizeWindow` (both, if both
genuinely changed at once).

**A real methodological self-correction worth recording** (mirroring the M4e `AddDate` one):
`WaitWindowEvent`'s real timeout-expiry return value was initially misidentified as `-1`, from an
under-drained test that saw `-1` after only ~2ms against a 300ms timeout. This contradicted the
official docs (`-1` isn't documented as a return value at all; `0` is). Re-testing with a properly
settled drain loop (waiting for a short *quiet streak* of empty polls, not just one) showed `-1` is
actually a real, repeating spurious event value this specific headless-Xvfb-without-a-window-manager
test environment generates as noise (unrelated to any `#PB_Event_*` constant) - once drained away
properly, `WaitWindowEvent(300)` genuinely blocked for ~300ms (confirmed via `ElapsedMilliseconds()`
before/after) and correctly returned `0`, matching the docs exactly. The lesson (a tight
`while (WindowEvent() != 0) {}`-style drain loop can exit prematurely, before an event already in
flight over the X11 socket actually arrives) recurred a second time independently while writing this
slice's own Catch2 unit tests, and was fixed the same way there too (a `drainEvents()` helper
requiring 5 consecutive empty polls, not just one, before considering the queue genuinely settled).

**A real environment gotcha, not a `pbcxx` bug, that blocked GUI testing entirely until diagnosed**:
a from-scratch, hand-written, pbcxx-independent GTK3 program (`gtk_init`/`gtk_window_new`/
`gtk_widget_show_all`/a plain event-pump loop) run under `DISPLAY=:99` (a dedicated Xvfb instance
started for this testing) still showed **zero** windows ever appearing on that X server
(`xwininfo -root -tree` reported "0 children" even while the program was confirmed still running).
Root cause: this dev environment's shell also has a real `WAYLAND_DISPLAY=wayland-0` set (a desktop
Wayland session, separate from the Xvfb instance started purely for GUI testing) - GTK3 silently
prefers Wayland over `DISPLAY` whenever both are present, with no warning that it ignored the X11
display entirely. Fixed by explicitly forcing `GDK_BACKEND=x11` whenever running a GUI test under
Xvfb - confirmed this exact class of gotcha (a toolkit silently preferring the wrong display backend
in an environment with both present) was independently hit and documented by the separate
`~/git/PureBasic/qt6_subsystem` reference project too, for Qt/`QT_QPA_PLATFORM`. `run_gui_case.sh`
and CI's GUI-test steps both set it unconditionally now; harmless on a plain X11-only environment.

**Close-button (`delete-event`) verified by direct GTK signal emission, not a real window-manager
interaction**: this headless Xvfb instance has no window manager running at all, so there's nothing
to translate a "close the window" request into the `WM_DELETE_WINDOW` protocol message GTK's
`delete-event` signal actually listens for - confirmed directly: `xdotool windowclose` and a
simulated `Alt+F4` both bypass it entirely (the former destroys the underlying `GdkWindow` directly,
logging a `Gdk-WARNING: GdkWindow ... unexpectedly destroyed`; the latter does nothing, since no
window manager exists to bind the shortcut). Verified the actual handler logic instead by emitting
the real `"delete-event"` signal directly via `g_signal_emit_by_name` (the exact same code path a
real close-button click would trigger, just invoked without the window-manager middleman) - confirms
the handler returns `TRUE` (window not auto-destroyed, matching real PB), queues
`#PB_Event_CloseWindow` for the right window ID, and that a subsequent `CloseWindow()` call still
closes it normally. This is both a unit test (`runtime_guilib_test.cpp`) and the technique used for
manual verification during development; installing a window manager under Xvfb to test the "real"
interaction path end-to-end was considered and deliberately skipped as disproportionate
infrastructure for what the direct-signal-emission test already covers with high confidence.

**Window chrome/behavior flags (`#PB_Window_*`) are accepted but not yet acted on** - every window
currently gets GTK's default titlebar/resize/close chrome regardless of what's requested. A
deliberately deferred gap for this first slice, like gadgets/menus/everything past "window + event
core" in the M7b scoping notes above.

**Testing**: `runtime_guilib_test.cpp` (Catch2) and one golden e2e case (`tests/e2e/gui_window_core`,
run via a dedicated `run_gui_case.sh` rather than the plain `run_case.sh`) both skip gracefully
(Catch2's `SKIP()`; ctest's `SKIP_RETURN_CODE` property) rather than fail when no usable display is
available - most CI runners have none at all. No `tests/e2e_diff` case was added for this slice: the
deliberate `IsWindow`/`OpenWindow` return-value simplification above means a literal stdout diff
against the real oracle would fail even on fully correct `pbcxx` behavior, making this feature area
a poor fit for that testing style (unlike the stdlib libraries in M4, where output is expected to
match byte-for-byte). GTK3 is an **optional** build-time dependency for the unit test binary itself
too (`tests/unit/CMakeLists.txt` probes for it via `pkg_check_modules(... QUIET ...)`, not
`REQUIRED`, compiling `runtime_guilib_test.cpp` only when found) - Windows-MinGW CI doesn't install
it yet and the real Haiku machine doesn't have `gtk3_devel` installed yet, and neither should lose
the rest of the unit test suite over that. A new `tests/lsan-suppressions.txt` (wired into both the
`linux-clang-sanitize` CMake preset's own `LSAN_OPTIONS` and the `sanitizers` CI job) suppresses a
confirmed-harmless LeakSanitizer false positive from fontconfig/pango-ft2's own process-lifetime
font cache, first touched by `gtk_init()` - not a `pbcxx` leak.

**Not yet done, left for later M7b slices**: gadgets, `MessageRequester`, `Menu`/`StatusBar`/
`ToolBar`, and everything else in the phased scope above. Haiku verification (needs `gtk3_devel`
installed there first) and Windows/MinGW GUI linking feasibility are both still open, deferred
rather than blocking this slice.

## M7b Implementation Notes (GUI core, second slice: basic gadgets)

**Scope landed**: `ButtonGadget`/`TextGadget`/`StringGadget`/`CheckBoxGadget`/`FrameGadget`,
`IsGadget`/`FreeGadget`/`ResizeGadget`/`HideGadget`/`DisableGadget`, and the text/state accessors
`GetGadgetText`/`SetGadgetText`/`GetGadgetState`/`SetGadgetState`. The `#PB_EventType_*` constants
needed to interpret gadget events (`LeftClick`=0, `RightClick`=1, `LeftDoubleClick`=2, `Focus`=256,
`LostFocus`=512, `Change`=768) were oracle-verified by direct `Debug #PB_EventType_X` probes.

**Oracle-verified finding that reshapes this whole slice's design**: gadget-creation functions take
*no* window parameter at all, unlike this project's own first instinct going in. Confirmed directly:
with two windows open, a gadget created afterward lands in the **second** one (`EventWindow()`
reports it when the gadget is later clicked), not the first - real PB tracks an implicit "active
window" context, always the most recently opened one (real PB's own escape hatch for retargeting
this, `UseGadgetList`, is not implemented in this slice - a deliberately deferred gap, the same kind
as `#PB_Window_*` flags were for the first GUI slice). Mapped onto a single `detail::activeWindowId()`
global, set by every `pbOpenWindow` call and read by every gadget-creation function.

**`GtkFixed` as the natural container for PB's absolute-pixel gadget model**: unlike GTK's usual
box/grid layout managers, `GtkFixed` takes explicit `x, y` placement (`gtk_fixed_put`) for each
child, matching PB's own `Gadget(#Gadget, x, y, width, height, ...)` signature directly. One
`GtkFixed` is created per window (a child of the window itself, added right after the window in
`pbOpenWindow`) and tracked in a new `detail::windowFixedTable()`; every gadget-creation function
places its widget into the active window's own `GtkFixed`, which is also how `pbResizeGadget` moves
a gadget later (`gtk_fixed_move`, found via `gtk_widget_get_parent`).

**`FrameGadget` is not a real container** - oracle cross-checked against ordinary PB usage: other
gadgets placed with coordinates that visually overlap a `FrameGadget`'s own area are independent
sibling widgets at the window level, not children reparented into the frame. This matches `GtkFixed`
placement exactly (every gadget, including `FrameGadget` itself, is just another direct child of the
window's one `GtkFixed`) - no special-casing needed.

**Oracle-verified return-value simplification, same stance as `pbOpenWindow`'s own**: all five
gadget-creation functions, and `IsGadget`, return a native-handle-ish nonzero Integer in real PB, not
a clean `1` - simplified to `1`/`0` the same way. More surprisingly, oracle-verified that
`ResizeGadget`/`HideGadget`/`DisableGadget`/`FreeGadget` (and, cross-checked at the same time,
`ResizeWindow`/`HideWindow`/`CloseWindow` from the first GUI slice) all return a **plain `0`**
regardless of success in real PB - not a success flag at all, just an unused/implementation-detail
return slot. `pbcxx`'s own versions return `1` on success/`0` if the ID wasn't found instead, a
documented, deliberate divergence (a more informative value with no real program likely depending on
PB's own literal `0`).

**`GetGadgetText`/`SetGadgetText` dispatch on the gadget's own real GTK widget type** rather than
needing five separate accessor pairs: `GTK_IS_ENTRY` (StringGadget's content), `GTK_IS_BUTTON`
(covers both ButtonGadget's label *and* CheckBoxGadget's, since `GtkToggleButton` is itself a
`GtkButton` subclass - oracle-confirmed both are readable/writable this way), `GTK_IS_LABEL`
(TextGadget), `GTK_IS_FRAME` (FrameGadget's title) - all oracle-verified to support
`GetGadgetText`/`SetGadgetText` for their own primary display text. `GetGadgetState`/
`SetGadgetState` only do something meaningful for `GTK_IS_TOGGLE_BUTTON` (CheckBoxGadget's checked
state, oracle-verified to default to unchecked/`0`); every other gadget type harmlessly no-ops.

**Two real bugs found by oracle cross-checking, not by a failing test**: `SetGadgetState` and
`SetGadgetText` on a `StringGadget` were both initially implemented as plain `gtk_toggle_button_
set_active`/`gtk_entry_set_text` calls - but GTK fires the exact same `"toggled"`/`"changed"`
signals for a *programmatic* change as for a real user click/keystroke, so the first working version
of both functions silently queued a spurious, incorrect `#PB_Event_Gadget`. Oracle-verified real PB
does **not** generate any event at all from either function (confirmed directly: a drained event
poll loop immediately after each call reports zero gadget events) - fixed by wrapping just the
programmatic GTK call in `g_signal_handlers_block_by_func`/`_unblock_by_func` around the specific
signal handler, leaving real user-driven toggles/typing (verified separately, unblocked) still
working correctly.

**Gadget click/toggle/change event types, oracle-verified via `xdotool` mouse-click/keystroke
simulation** (the same technique the first GUI slice used for move/resize/repaint events, extended
here since gadget clicks - unlike a window's close button - don't need a window manager to simulate:
a raw pointer click at the gadget's own screen coordinates works directly): a `ButtonGadget` click
*and* a `CheckBoxGadget` toggle both report `#PB_EventType_LeftClick` (`0`) as their `EventType()` -
**not** `#PB_EventType_Change`, which would have been the more "obvious" guess for a checkbox.
`StringGadget` typing reports `#PB_EventType_Change` (`768`) on every keystroke (each character
queues its own event) and also `#PB_EventType_Focus` (`256`) when first clicked into - the Focus/
LostFocus pair is a deliberately deferred gap in this slice (not wired to any GTK signal), so
`pbcxx`'s own `StringGadget` doesn't emit it; only the per-keystroke `Change` events are implemented.
`TextGadget`/`FrameGadget` are purely static/display gadgets in real PB - no signal wiring at all.

**Closing a window frees its own gadgets too**, oracle-verified (`IsGadget` on a gadget belonging to
a just-closed window returns `0`). `gtk_widget_destroy` on a window already recursively destroys its
`GtkFixed` and every gadget GTK-side, but `gadgetTable()` itself would otherwise be left holding
dangling pointers - a new shared `detail::destroyWindow` helper (used by both `pbCloseWindow` and
`pbOpenWindow`'s own "re-using an already-open ID" branch) scans for and erases every gadget actually
owned by that window first.

**Testing**: extended `runtime_guilib_test.cpp` with gadget creation/management/text/state round-
trips, the two spurious-event bugs above (regression tests), real click/toggle/typing event-type
verification (via `gtk_button_clicked`/`gtk_toggle_button_set_active`/`gtk_entry_set_text` driven
directly, the gadget-event equivalent of the first slice's own close-button signal-emission
technique), the active-window-placement behavior, and the window-close-frees-gadgets behavior. A
second golden e2e case (`tests/e2e/gui_basic_gadgets`) covers creation/management/accessor round-
trips deterministically (no simulated click/keystroke, same reasoning as the first slice's own e2e
case for not attempting an e2e_diff test here: the oracle-verified return-value simplifications would
fail a literal stdout diff even on fully correct behavior).

**Also fixed in this slice**: `#PB_EventType_*` was oracle-verified back in the gadgets work above but
never actually added to `builtinConstantTable()` - a real gap this project's own tests didn't catch
(every gadget-event test used raw numeric literals, not the named constants) - closed alongside the
new `#PB_MessageRequester_*` constants below.

## M7b Implementation Notes (GUI core, third slice: `MessageRequester`)

**Scope landed**: `MessageRequester(Title$, Text$ [, Flags])`, the simplest of PB's modal dialogs and
genuinely useful standalone (real PB code uses it independent of any window). `Flags`' bits 0-1
select the button set (`#PB_MessageRequester_Ok`=0/`YesNo`=1/`YesNoCancel`=2 - oracle-verified via
`Debug #PB_MessageRequester_X`; a plausible-sounding fourth, `#PB_MessageRequester_OkCancel`, does
**not** exist in real PB) and bits 2-4 independently select an icon (`Info`=4/`Error`=8/`Warning`=16;
a plausible `Question` does **not** exist either). `#PB_MessageRequester_Yes`=6/`No`=7/`Cancel`=2 are
the button-pressed return values, a separate group from the button-set flags that happens to reuse
`2` for `Cancel` (coinciding with the `YesNoCancel` flag) - confirmed to be just a coincidence of PB's
own constant numbering, not a bug.

**Genuinely different from every other GUI builtin so far**: `MessageRequester` is *synchronous* -
real PB's own call blocks until a button is clicked and returns which one, with no
`WaitWindowEvent`/event-queue involvement at all. `gtk_dialog_run` (GTK3's own nested-main-loop
"block until response" API, deprecated in favor of an async pattern in newer GTK but still fully
functional) is the exactly-right fit for this, since the deprecation's underlying rationale (don't
block the UI thread) doesn't apply to a call PB's own documented contract says *should* block.

**Oracle-verified finding that reshapes the whole return-value mapping, found by direct
click-and-read-the-Debug-output testing under Xvfb (using `xdotool` mouse clicks plus ImageMagick's
`import -window` to screenshot the dialog and find real button positions first, since PB's own GTK3
backend localizes stock button labels to this machine's system locale - "Valider"/"Oui"/"Non", not
"OK"/"Yes"/"No" - and the dialogs are small enough that guessing coordinates blindly is unreliable)**:
clicking the *only* button on a plain Ok-type dialog returns `6` (`#PB_MessageRequester_Yes`'s own
value), **not** `0` (`#PB_MessageRequester_Ok`) - confirmed repeatedly, including with an icon flag
added. This looks like a genuine real-PB implementation quirk (its own GTK3 backend likely routes a
single-button dialog's response through the same code path as a "Yes" response) rather than a
documented public contract, but unlike `OpenWindow`/`IsWindow`'s own native-handle-value
simplification (an arbitrary internal pointer no reasonable program would compare against), the
`#PB_MessageRequester_*` constants are meaningful values a real program might reasonably check
equality against - so this one *is* replicated exactly (`pbMessageRequester` maps both
`GTK_RESPONSE_OK` and `GTK_RESPONSE_YES` to `6`) rather than "cleaned up" to the more intuitive `0`.

**GTK's own stock-button localization matches the oracle for free**: `GTK_BUTTONS_OK`/
`GTK_BUTTONS_YES_NO` are GTK's own built-in button sets, translated via the system's installed GTK
translation catalogs independent of PB - confirmed `pbcxx`'s own dialog shows the identical
"Valider"/"Oui"/"Non" labels as the oracle's, with no PB-side or `pbcxx`-side localization code
needed at all. GTK has no built-in three-button "Yes/No/Cancel" enum, though, so that one case is
built by hand (`GTK_BUTTONS_NONE` + three `gtk_dialog_add_button` calls, in the oracle's own
left-to-right order: No, Yes, Cancel) with plain English labels - acceptable since button label text
isn't part of any testable contract (no `GetGadgetText`-equivalent exists for a message dialog).

**Testing**: three new `runtime_guilib_test.cpp` cases cover the Ok-returns-Yes finding and the
Yes/No/Cancel return-value mapping, each using a `g_timeout_add`-scheduled callback (found via
`gtk_window_list_toplevels()` + `gtk_dialog_response()`) to safely simulate a button click from
*inside* the same nested main loop `gtk_dialog_run` itself pumps - unlike a gadget click, there's no
window-manager-independent on-screen-coordinate trick available here (the call doesn't return control
until a response happens, so nothing outside that nested loop can act first). No e2e golden test was
added for the same reason the first GUI slice's close-button behavior never got one either: a plain
`run_gui_case.sh`-style "compile, run, diff stdout" test has no way to safely simulate the click a
genuinely blocking dialog needs without risking an e2e test that hangs forever if something regresses
- unit-test coverage with the safe `g_timeout_add` technique above was judged sufficient confidence
for this slice.

## M7b Implementation Notes (GUI core, fourth slice: `Menu`/`StatusBar`)

**Scope landed**: `CreateMenu`/`MenuTitle`/`MenuItem`/`MenuBar` (the separator)/`OpenSubMenu`/
`CloseSubMenu`, generic menu management (`IsMenu`/`FreeMenu`/`HideMenu`/`DisableMenuItem`), the
state/text accessors (`GetMenuItemState`/`SetMenuItemState`/`GetMenuItemText`/`SetMenuItemText`/
`GetMenuTitleText`/`SetMenuTitleText`), `MenuHeight`/`MenuID`, `EventMenu`; `CreateStatusBar`/
`AddStatusBarField`/`StatusBarText` + `IsStatusBar`/`FreeStatusBar`/`StatusBarHeight`/`StatusBarID`;
and `WindowID` - a real, load-bearing gap-filler this slice needed before any of the above could work
at all (see its own notes below). `#PB_StatusBar_*` (`Raised`=1/`BorderLess`=2/`Center`=4/`Right`=8)
and the general-purpose `#PB_Ignore`=-65535/`#PB_All`=-1 (surprisingly, oracle-verified to share the
same value as `#PB_Any`) constants are all oracle-verified via direct `Debug #PB_X` probes.

**`ToolBar` is deliberately out of scope for this slice, unlike the roadmap's own original "Menu/
StatusBar/ToolBar" grouping** - a real, oracle-discovered blocker, not a time-budget cut: real PB has
no way to create a toolbar button without an `ImageID` at all (`ToolBarImageButton` is the *only*
button-creation function; `ToolBarButtonText` only *relabels* an already-created one, and requires the
toolbar to have been created with `#PB_ToolBar_Text` in the first place). This project's Image library
(`LoadImage`/`CreateImage`/`ImageID`) doesn't exist yet - implementing `ToolBar` now would mean either
diverging from real PB's own button-creation contract or rushing a minimal, unplanned Image library as
a side effect of this slice, neither acceptable under this project's oracle-first standard. Revisit once
an Image library milestone lands.

**`WindowID` had to be implemented as a prerequisite, not a extension of this slice's own planned
scope** - oracle-verified `CreateMenu`/`CreateStatusBar`'s own second argument must be real PB's own
`WindowID(#Window)` return value, not the plain PB-level window ID every other GUI builtin accepts
directly. Unlike every native-handle-ish return value this project has simplified to a clean `1`/`0` so
far (`OpenWindow`, `IsWindow`, gadget creation, ...), `WindowID()`'s own return value is genuinely
load-bearing - real programs pass it straight into another function. `pbWindowID` therefore returns the
*real* `GtkWidget*` for the window, reinterpret_cast to an `int64_t`, and `pbCreateMenu`/
`pbCreateStatusBar` reinterpret_cast it straight back - no separate id-to-handle lookup table needed at
all, since the handle already *is* the real pointer.

**Oracle-verified architectural finding, the same "current context" pattern M7b's second GUI slice
established for gadgets, extended to two more unrelated areas independently**: `MenuTitle`/`MenuItem`/
`MenuBar`/`OpenSubMenu`/`CloseSubMenu` all operate on whichever `#Menu` was most recently `CreateMenu`'d
(confirmed directly: with two menus created, building continues into the second one, not the first) -
not an isolated gadget-specific quirk, but a general PB idiom this project hadn't previously confirmed
extends elsewhere. `AddStatusBarField` (no `#StatusBar` argument at all) is the identical pattern for
`CreateStatusBar`. `guilib.hpp`'s own `detail::activeMenuId()`/`detail::activeStatusBarId()` mirror
`detail::activeWindowId()` exactly.

**A second, more surprising oracle correction to the official (French) help docs themselves, found by
testing instead of trusting them**: `MenuHeight()`'s own help page claims Linux (and MacOS) always
return `0`, since "the menu bar isn't part of the window" there. Directly tested against this PB 6.50
beta 1 / GTK3 install instead: a plain `Debug MenuHeight()` right after building a one-title, one-item
menu read `27`, a real nonzero rendered height - GTK3's own menu bar genuinely is embedded in the
window's own content area on this install (unlike whatever older native-X11-WM-chrome model the docs
may predate), so `pbMenuHeight` returns the real allocated height of the `GtkMenuBar` widget, not a
hardcoded `0` - and `pbcxx`'s own generated binary reproduced the oracle's exact value (`27`) for an
identical menu, not just a plausible-looking nonzero number. A documented lesson for this project's own
methodology, not just a `pbcxx` implementation detail: even official PureBasic documentation needs
oracle verification before being trusted, the same standing instruction `docs/developer/oracle-
testing.md` already gives for third-party claims generally.

**A leaf `MenuItem` is a `GtkCheckMenuItem`, not a plain `GtkMenuItem`, needed so `SetMenuItemState`'s
own checkmark has somewhere to render (`MenuItem()` has no separate "checkable" declaration form in
real PB at all - any item can be checked via the state accessors)** - but oracle-verified via direct
`xdotool` click simulation under Xvfb against the real `pbcompilerc` binary (the same technique the
second/third GUI slices used) that a real click does **not** change `GetMenuItemState()`'s own value,
unlike a `CheckBoxGadget` toggle. GTK's own default `"activate"` handler for a `GtkCheckMenuItem`
unconditionally flips its active state first, though, so `detail::onMenuItemToggled` (connected to
`"toggled"`, fired as a side effect of that flip) immediately reverts any change that disagrees with the
item's own stored `menuCheckedKey()` intent (updated only by `pbSetMenuItemState`) - blocking itself
around the corrective call the same way `pbSetGadgetState` already blocks around its own programmatic
change, to avoid recursing into itself. Verified end-to-end against `pbcxx`'s own compiled output too
(not just the runtime library in isolation): a real `xdotool` click against a compiled `pbcxx` binary
produced the identical oracle-verified `EventMenu()`/`EventWindow()`/post-click `GetMenuItemState()`
values the real `pbcompilerc` binary did for the same `.pb` source.

**A real structural change to `pbOpenWindow` itself, needed for a menu bar and a status bar to have
somewhere to attach without disturbing gadget coordinates**: every window's sole child used to be the
`GtkFixed` gadgets are placed into directly; it's now a vertical `GtkBox` ("vbox") holding the `GtkFixed`
(expand/fill, so it still consumes all otherwise-unused space), with `pbCreateMenu` packing a menu bar
at the top (`gtk_box_pack_start` + `gtk_box_reorder_child` to force position `0`, regardless of creation
order relative to a status bar) and `pbCreateStatusBar` packing a status bar at the bottom
(`gtk_box_pack_end`). `pbCreateMenu`/`pbCreateStatusBar` find this vbox back via `gtk_bin_get_child` on
the window handle (`WindowID()`'s own real pointer) they're given, rather than needing a third id-keyed
lookup table - this is also *why* `WindowID()` returning the real pointer (not a simplified `1`/`0`)
mattered architecturally, not just for oracle fidelity. Existing gadget coordinates are unaffected (the
`GtkFixed`'s own top-left origin doesn't move relative to itself), and a gadget now correctly starts
below any menu bar - matching real PB's own documented reason `MenuHeight()` exists in the first place
(the `HideMenu()` doc example uses it to reposition a gadget when a menu bar's presence toggles). All 17
pre-existing GUI unit tests (window/gadget/`MessageRequester`) still pass unchanged against this
restructuring, confirming it's additive, not a behavior change for anything built before this slice.

**`StatusBar` deliberately does *not* reuse any single built-in GTK "statusbar" widget** - a plain
`GtkStatusbar` is a single text line with a push/pop context-ID message stack, nothing like real PB's
own multi-field, independently-widthed/aligned/bordered field list (oracle-verified: two fields with
different widths, alignments, and border styles coexist side by side). A horizontal `GtkBox` of one
`GtkFrame`-wrapped `GtkLabel` per field is the natural fit instead - the same "model PB's own shape, not
whichever same-named GTK widget happens to exist" choice `GtkFixed` already was for gadgets over M7b's
second slice. `Width = #PB_Ignore` auto-sizes a field via the box's own `expand`/`fill` instead of an
explicit size request; `StatusBarText`'s `Apparence` flags independently pick the frame's shadow type
(`Raised`/`BorderLess`) and the label's horizontal alignment (`Center`/`Right`).

**Every menu/status-bar-management function PB's own docs describe as returning `Aucune` ("none") is
still modeled as Integer-returning, success/not-found `1`/`0`** - the identical, already-established
divergence `ResizeGadget`/`HideGadget`/`DisableGadget`/`FreeGadget` (M7b's second slice) and
`ResizeWindow`/`HideWindow`/`CloseWindow` (its first) already chose over real PB's own undocumented or
nonexistent return values, applied consistently here rather than re-litigated per function.

**Window/menu/status-bar cleanup needed a third parallel owned-widget scan, extending `destroyWindow`'s
existing pattern rather than introducing a new one**: `pbCreateMenu`/`pbCreateStatusBar` tag their own
top-level widget with the owning PB window ID (`g_object_set_data`, the same convention
`gadgetWindowIdKey()` already established for gadgets), and `destroyWindow` now also scans
`menuTable()`/`statusBarTable()` for entries owned by the window being closed, erasing their own side
tables (`menuTitleWidgets`/`menuItemWidgets`/`menuBuildStack`/`statusBarFieldFrames`) alongside the
`GtkWidget*` table entry itself - GTK already recursively destroys the actual child widgets when the
window is destroyed, but these id-keyed C++ tables would otherwise be left holding dangling pointers,
the exact same risk `gadgetTable()` already had and was already fixed for.

**Testing**: extended `runtime_guilib_test.cpp` with `WindowID`'s own handle round-trip, `CreateMenu`/
`IsMenu`/`FreeMenu`, full menu-building + text/state-accessor round-trips, `DisableMenuItem`/`HideMenu`
sensitivity/visibility effects, a real menu-item activation (`gtk_menu_item_activate`, the menu
equivalent of the second slice's own `gtk_button_clicked` technique) verifying both the queued event and
the not-auto-toggled state regression, window-close cleanup for both menus and status bars, and a full
`CreateStatusBar`/`AddStatusBarField`/`StatusBarText` round-trip. A third golden e2e case
(`tests/e2e/gui_menu_statusbar`) covers deterministic creation/management/accessor round-trips the same
way the second slice's own `gui_basic_gadgets` case does, for the same reason (the oracle-verified
return-value simplifications above would fail a literal stdout diff even on fully correct behavior) -
simulated-click coverage stays in the Catch2 unit tests only, consistent with every GUI slice so far.
All 278 tests (across both `linux-gcc` and `linux-clang`) pass, including the 261 that predate this
slice - confirming the `pbOpenWindow` vbox restructuring is additive.

## M7c Implementation Notes (`Interface`/`EndInterface`)

**Scope landed**: `?Label` (the address of a `DataSection` label - the roadmap's own stated
prerequisite) and `Interface`/`EndInterface` itself: declarations, Interface-typed pointer
variables, and `base\Method(args)` calls dispatched through a real, manually-built vtable. Both
pieces are oracle-verified end to end against a real, working PureBasic program (not just a syntax
check) - see `tests/e2e_diff/interfaces`, which diffs byte-for-byte against the real `pbcompilerc`
oracle.

**`?Label`'s own design was shaped by a direct oracle finding, not assumption**: `PeekI(?Label)` and
`PeekI(?Label + 8)` on a `DataSection` of three `.i` values read `10`/`20` (the first two values,
exactly 8 bytes apart) - confirming real PB's `DataSection` is laid out as genuine, contiguous,
byte-addressable process memory, not some abstract sequential-access-only pool. Reading
`pbcompilerc`'s own `-c` output for a worked `Interface` example (see below) confirmed the full
mechanics: the whole program's `DataSection` is one flat `unsigned char pb_data[]`, `?Label` is a
compile-time `#define` offset into it (`&pb_data[N]`), and non-constant-foldable values like
`@Procedure()` are written in at runtime (`*(integer*)(&pb_data[0]) = (integer)f_circle_area;`)
before `main()`'s own body runs.

**`pbcxx` deliberately does *not* replicate that flat-byte-pool architecture, scoping `?Label`
narrower instead** - M5b's own `Data`/`Read`/`Restore` pool (`runtime/include/.../datalib.hpp`) is a
`std::vector<PBDataValue>` of a tagged union, fundamentally incompatible with raw pointer arithmetic
across mixed-type items, and already proven/tested; rearchitecting it into a true byte blob purely to
support `?Label` generically (including arbitrary `Peek*`/`Poke*` access, which don't even fully
exist in `pbcxx` yet - no `PeekI`/`PokeI` at all, only the fixed-width `PeekB`/`PeekW`/`PeekL`/`PeekQ`/
etc. family) was judged disproportionate to this slice's only oracle-verified real use case: an
`Interface`'s own vtable, always a short, homogeneous run of `.i` values. `?Label` is scoped to
exactly that: legal only for a label whose own run of `Data` items (up to the next label or
`EndDataSection`) is non-empty and entirely `.i`-suffix, oracle-verified a real, representative shape
(every genuine `Interface` vtable is exactly this) - anything else is a real Sema diagnostic, not a
silent miscompile. `Sema::collectDataSections` (M5b's own pre-pass) was extended to additionally
track each label's own item-suffix run (`dataLabelItemSuffixes_`) purely to support this validation
(`Sema::dataLabelAddressable`), with zero change to the pre-existing `dataLabels_`/`dataCount_`
indexing `Read`/`Restore` already relied on.

**Codegen gives every addressable label its own small, real, contiguous C++ array - not a byte
offset into one shared pool** - `genDataLabelArrays()` (a new pass, structurally mirroring
`genDataPool`'s own recursive module walk) emits one `static const std::array<std::int64_t, N>
pb_label_<name> = { <value0>, <value1>, ... };` per addressable label, as ordinary global static
initialization (not a runtime `pbDataAdd*` call the way `genDataPool`'s own pool is populated) -
valid here specifically because every legal `?Label` item is `.i`-suffix, so no tagged-union
indirection or type dispatch is needed at all. This must run *after* `genProcedures()` (an array
element can be `@Procedure()`, which needs the real C++ function already declared/defined - taking a
function's address is valid as soon as it's declared, unlike *calling* it) - the one genuine ordering
constraint this slice had to get right, confirmed by testing a program whose `DataSection` textually
precedes its procedures compiles and runs identically to one where it follows them.

**The worked `Interface` example from the roadmap's own M7c scoping notes was directly oracle-
verified, including reading `pbcompilerc -c`'s own generated C**, which settled the exact dispatch
ABI to replicate:

```c
typedef struct i_shape {
double (*m_area)(integer);
integer (*m_scale)(integer,double);
void* (*m_name)(integer);
} i_shape;
static i_shape** p_shape1=0;
p_shape1=(void*)((integer)(&v_c));          // *shape1.Shape = @c
integer r0=(*p_shape1)->m_scale(p_shape1,2.0); // *shape1\Scale(2)
```

A manually-built, programmer-visible vtable, exactly as the roadmap's own notes predicted: the
Structure's own first field holds the vtable's address (written there explicitly by the program, via
`?Label` - `v_c.f_vtable=la_l_circlevtable;` in the oracle's own output), the Interface-typed
variable is a pointer *to* that first field (so one dereference reads the vtable base), and a method
call indexes the vtable by the method's declared position and calls through the resulting function
pointer with the object's own address as an implicit first argument - no structural conformance
checking between the Structure and the Interface exists anywhere (confirmed: the implementing
procedures' own parameter types are never even mentioned in the `Interface` declaration itself).
`pbcxx`'s own `Codegen::genExpr`'s `MethodCall` case lowers to the direct C++ equivalent:
```cpp
reinterpret_cast<double(*)(std::int64_t, double)>(
    *reinterpret_cast<std::int64_t*>(*reinterpret_cast<std::int64_t*>(vp_shape1) + 8)
)(vp_shape1, 2.0)
```

**A pbcxx-specific simplification actually makes this *safer* than real PB's own ABI trick, not just
equivalent to it**: real PB's vtable function-pointer types use `integer` for the implicit "this"
parameter - a deliberate, informal stand-in for "whatever typed pointer the real implementation
expects," relying on same-size calling-convention compatibility that works in practice but isn't
strictly guaranteed by the C standard. `pbcxx` doesn't need that leap of faith at all: every pointer
*parameter* in this project (M3d) already has its own C++ storage type forced to plain `std::int64_t`
regardless of its declared pointee type (`Sema::visitStmt`'s `ProcedureDecl` case, `declare(param.name,
param.spelling, TypeSuffix::Integer, ...)`), so an implementing procedure like `Circle_Area(*this.
CircleData)` is *already* generated as `double f_circle_area(std::int64_t)` - the exact real
underlying function pointer type a vtable slot's own `reinterpret_cast` targets, not merely a
same-size stand-in. The cast is therefore exact, not merely conventionally safe.

**A real, pre-existing correctness bug surfaced immediately by testing the most natural, idiomatic
`Interface`-implementing style** - every implementing procedure in a typical `Interface` naming its
own first parameter `*this` (as PB's own convention and this slice's worked examples both do) - and
fixed as part of this slice, not deferred, since it would otherwise make `Interface` nearly unusable
in practice. `Sema::pointerPointeeType_` (M3d) is a single, flat, *never per-procedure-scoped* map
(unlike `symbols_`/`order_`, which *are* saved/restored around each `ProcedureDecl`'s own body visit) -
M3d's own notes already flagged this as a known, accepted gap ("a pointer parameter or local sharing
a base name with an unrelated global pointer of a different pointee type can leak the wrong pointee
type across procedures... not yet observed in practice"). This slice *did* observe it in practice,
immediately: `Circle_Area(*this.CircleData)` and a later `Square_Area(*this.SquareData)` - two
different procedures, same parameter name, different pointee Structure - silently left
`pointerPointeeType_["*this"]` pointing at whichever one `Sema` visited *last*, so `Codegen`'s own
later, separate `genProcedures()` pass (reading that map directly) generated **every** procedure
using that parameter name against the *same, wrong* Structure type - caught immediately as a hard C++
compile error (`'struct s_squaredata' has no member named 'f_radius'`) rather than silently producing
wrong results, but a real, user-facing blocker for the single most natural way to write
`Interface`-implementing code. Fixed properly, not worked around: `Sema::ProcedureInfo` gained its
own `pointerPointeeTypes` field, captured (from the live, still-correct `pointerPointeeType_` map) at
the exact same moment `locals` already is - right before that procedure's own scope swap-back -
giving each procedure a durable, correct snapshot of its own pointer locals' pointee types, immune to
being overwritten by any later procedure. `Codegen` gained a matching `currentProcInfo_`-scoped
`pointeeTypeOf()` wrapper (mirroring the pre-existing `currentProcReturnSuffix_` pattern exactly),
consulted by every pointer-dereference codegen path (`FieldAccess`-on-pointer, the new `MethodCall`,
and `FreeStructure`) in place of calling `Sema::pointeeTypeOf` directly. Verified via the same
two-Structure/two-procedure scenario both before (hard compile error) and after (byte-for-byte
correct, matching the oracle) the fix, and the full 289-test suite (`linux-gcc`/`linux-clang`,
ASan/UBSan) still passes unchanged, confirming the fix is additive - not just a workaround scoped to
`Interface`'s own new code paths.

**Deliberately deferred past this slice, each a genuine, separate feature rather than a shortcut
taken under time pressure**: pointer-typed Structure fields (`*VTable`, real PB's own idiomatic form)
- oracle-verified a plain `.i` field is behaviorally identical for every purpose this slice needed (
`PeekI`-style raw dereferencing through an untyped pointer field is the only thing `*VTable` would add,
and that's its own separate, unimplemented gap already flagged back in M3d); `Interface` inheritance/
`Extends`; a `#PB_Any`-style auto-generated `#Interface`/label numbering (not applicable - `Interface`/
`DataSection` labels are compile-time names, not runtime-allocated IDs like a `#Window`/`#Gadget`);
general `?Label`-based `Peek*`/`Poke*` access into an arbitrary byte offset (would need the full
byte-pool rearchitecture discussed above); an array of Interface-typed pointers (`Dim shapes.Shape(1)`)
- oracle-tested directly and found to be a real PB syntax error in the first place ("`*shapes()` is
not a function, array, list, map or macro"), so this isn't even a gap, just confirmed out of scope by
the oracle itself.

