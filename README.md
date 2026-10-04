# easybasic

easybasic transpiles [PureBasic](https://www.purebasic.com/) source code to C++, then
compiles the generated C++ with a real backend compiler (g++/clang++).

## Status

`M0`-`M6` done, M7a (threads) and M7b's first three GUI-core slices done: core literals, `Define`/`Debug`, assignment, the full oracle-derived
operator/precedence table (arithmetic, bitwise, comparisons, logical), `If`/`Select`/
`For`/`While`/`Repeat`, `EnableExplicit`, `#Name`/`Enumeration` constants,
`Procedure`/`ProcedureReturn` with recursion and genuinely isolated per-procedure scope,
`Global`/`Shared`/`Protected`, static `Dim` arrays, `Structure` (incl. nested/arrays-of-
Structure), pointers (`*Var`/`@Var`, `\`-dereference, `AllocateMemory`/
`AllocateStructure`), `NewList`/`NewMap` (`ForEach`, cursor navigation, by-key Map
access), `Declare` (mutual recursion), the core String/Math/Memory/File/Date
libraries (`Left`/`Mid`/`Str`/`Val`, `Sqr`/`Pow`/`Round`, `Peek*`/`Poke*`,
`CreateFile`/`ReadFile`/`Loc`/`Lof`, `Date`/`Year`/`FormatDate`/`AddDate`),
`CompilerIf`/`CompilerSelect` with the `#PB_Compiler_OS`/`#PB_OS_*`/
`#PB_Compiler_Processor`/`#PB_Processor_*` constants (resolved entirely at transpile time -
non-taken branches are never type-checked, matching real PB), `DataSection`/`Data`/
`Read`/`Restore` (a global, concatenated data pool with forward-referenceable labels and a
debug-mode-only "out of data" fatal error, matching real PB exactly), and non-recursive
`Macro`/`EndMacro` (a genuine token-level textual substitution pass run before parsing,
since a macro parameter is pasted as raw, unparenthesized text rather than a pre-evaluated
value - oracle-verified), threads (`CreateThread`/`IsThread`/`WaitThread`, re-entrant `Mutex`,
`Semaphore`, plus `@ProcedureName()` - a procedure's own address - and `Delay`/
`ElapsedMilliseconds`), and GTK3-backed GUI window/event core and basic gadgets
(`OpenWindow`/`CloseWindow`/`IsWindow`/`ResizeWindow`/`HideWindow`, `WindowEvent`/
`WaitWindowEvent`, `EventWindow`/`EventGadget`/`EventType`, the `#PB_Event_*`/
`#PB_Window_*`/`#PB_EventType_*` constants, `ButtonGadget`/`TextGadget`/`StringGadget`/
`CheckBoxGadget`/`FrameGadget`, `IsGadget`/`FreeGadget`/`ResizeGadget`/`HideGadget`/
`DisableGadget`/`GetGadgetText`/`SetGadgetText`/`GetGadgetState`/`SetGadgetState`, and
`MessageRequester` with the `#PB_MessageRequester_*` constants - an optional build-time
dependency, only pulled in for a program that actually calls a GUI command) - all
cross-checked against PureBasic's own compiler, including
real bugs (a target-typed `/`, a two-tier vs. one-tier bitwise precedence mix-up, a wrong
initial assumption about primitive-typed pointers, an unaligned-memory-access UB in
`Peek*`/`Poke*`) the differential test suite or a targeted oracle probe caught before they
shipped. See [`docs/architecture/roadmap.md`](docs/architecture/roadmap.md) for the full
milestone plan and the per-milestone implementation notes.

## Building

```sh
cmake --preset linux-gcc
cmake --build --preset linux-gcc
ctest --preset linux-gcc
```

Other presets (see `CMakePresets.json`): `linux-clang`, `linux-clang-sanitize` (ASan +
UBSan), `windows-mingw` (run from an MSYS2 mingw64 shell), `haiku`.

GTK3 dev files (e.g. `libgtk-3-dev` on Debian/Ubuntu, `gtk3_devel` on Haiku) are an
*optional* build-time dependency: `pbcxx` itself only needs them to compile a `.pb`
program that actually calls a GUI command, and the unit test binary only builds the GUI
test cases (`runtime_guilib_test.cpp`) when `pkg-config gtk+-3.0` succeeds - everything
else builds and passes without it.

## Usage

```sh
./build/linux-gcc/compiler/pbcxx examples/hello.pb -o hello -d
./hello
```

`-d` enables debug mode: `Debug` statements print (exactly like real PureBasic's own
`pbcompilerc -d`); without it, `Debug` compiles to nothing at all, matching a plain
PureBasic build.

## Documentation

- **[Architecture & roadmap](docs/architecture/roadmap.md)** - the milestone plan and the
  oracle-verified PureBasic semantics discovered along the way
- **[Developer architecture](docs/developer/architecture.md)** - the compile pipeline and
  module map, for anyone extending the compiler itself
- **[Using `pbcompilerc` as a semantic oracle](docs/developer/oracle-testing.md)** - the
  methodology behind this project's differential-testing approach
- **API docs** - generated with [Doxygen](https://www.doxygen.nl/)
  (`cmake --build build/linux-gcc --target docs`, output at
  `build/linux-gcc/docs/html/index.html`)

## Testing philosophy

Three complementary test layers (see `tests/`):

- **Golden e2e** (`tests/e2e/`): fixed expected output, runs anywhere.
- **Differential e2e** (`tests/e2e_diff/`, `scripts/diff_against_pbcompilerc.sh`):
  compiles the same program with both the real PureBasic compiler and `pbcxx`, and
  diffs their output - the oracle's live output *is* the expected output. Skips
  gracefully when `pbcompilerc` isn't installed (most CI runners).
- **Unit tests** (`tests/unit/`, Catch2): Lexer/Parser/Sema/runtime in isolation.

Plus, per-commit CI gates for clang-tidy/cppcheck and ASan/UBSan, on Linux, Windows/MinGW, and
Haiku (via a self-hosted runner bridging to a real Haiku machine over SSH, since the official
Actions runner needs .NET, which Haiku doesn't have), and a nightly Valgrind memcheck pass - see
`.github/workflows/`.

## License

MIT - see [`LICENSE`](LICENSE).
