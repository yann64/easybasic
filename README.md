# easybasic

easybasic transpiles [PureBasic](https://www.purebasic.com/) source code to C++, then
compiles the generated C++ with a real backend compiler (g++/clang++).

## Status

`M0`/`M1` done: core literals, `Define`/`Debug`, assignment, the full oracle-derived
operator/precedence table (arithmetic, bitwise, comparisons, logical), `If`/`Select`/
`For`/`While`/`Repeat`, `EnableExplicit`, and `#Name`/`Enumeration` constants - all
cross-checked against PureBasic's own compiler, including two real bugs (target-typed
`/`, and a two-tier vs. one-tier bitwise precedence mix-up) the differential test suite
caught before they shipped. See
[`docs/architecture/roadmap.md`](docs/architecture/roadmap.md) for the full milestone
plan and the per-milestone implementation notes.

## Building

```sh
cmake --preset linux-gcc
cmake --build --preset linux-gcc
ctest --preset linux-gcc
```

Other presets (see `CMakePresets.json`): `linux-clang`, `linux-clang-sanitize` (ASan +
UBSan), `windows-mingw` (run from an MSYS2 mingw64 shell), `haiku`.

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

Plus, per-commit CI gates for clang-tidy/cppcheck and ASan/UBSan, and a nightly Valgrind
memcheck pass - see `.github/workflows/`.

## License

MIT - see [`LICENSE`](LICENSE).
