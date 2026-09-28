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
| **M2** | `Procedure`/`ProcedureReturn`, `Protected`/`Global`/`Shared`, parameter passing, recursion, static `Dim` arrays | Not started |
| **M3** | `Structure`, pointers, `NewList`/`NewMap` families | Not started |
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

