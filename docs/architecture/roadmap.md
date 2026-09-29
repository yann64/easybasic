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
| **M2** | `Procedure`/`ProcedureReturn` (incl. `.s`/`$` return forms), by-value parameters with defaults, recursion, isolated per-procedure scope | Done (see M2 notes below) - `Global`/`Shared`/`Protected` cross-scope access and static `Dim` arrays deliberately deferred to M3 |
| **M3** | `Structure`, pointers, `NewList`/`NewMap` families, static `Dim` arrays, `Global`/`Shared`/`Protected` | In progress - `Global`/`Shared`/`Protected` (M3a), static `Dim` arrays (M3b), and `Structure` (M3c) done; pointers/`NewList`/`NewMap` still to come |
| **M4** | Core stdlib: String, Math, Memory, File, Date | Not started |
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

