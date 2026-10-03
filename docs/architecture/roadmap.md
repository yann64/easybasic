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
| **M5** | `CompilerIf`/`CompilerSelect` + `#PB_*` constants, `DataSection`, non-recursive `Macro` | Not started |
| **M6** | Cross-platform CI (Windows/Haiku via qemu), clang-tidy/cppcheck gates, ASan/UBSan, nightly Valgrind | Not started |
| **M7 (deferred/optional)** | `Interface`, `Module`, threads; GUI/3D as a separate future effort | Not scoped |

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

