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
| **M0** | Repo/CMake/CI skeleton; minimal lexer+parser+codegen for `Define`, `Debug`, integer/float/string literals, assignment, `+ - * / %`; trivial Sema; `PBString` skeleton | In progress (this document's own M0 notes below) |
| **M1** | All 11 type suffixes; oracle-derived operator/precedence table; `If/Select/For/While/Repeat`; `EnableExplicit`; `#`-constants/`Enumeration` | Not started |
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
- **`/` always performs real (floating-point) division**, regardless of operand types -
  oracle-verified (`10.0/3.0` and `10/3` both compute a real quotient; only the
  *destination* variable's type decides whether the result then gets stored as a float
  or converted to an integer). `Sema::familyOfExpr` hard-codes `Div` as always
  float-family for exactly this reason; `Codegen` always casts both operands to
  `double` before dividing rather than ever using C++'s native `/n` on two integers.
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
