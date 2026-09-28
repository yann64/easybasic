# easybasic architecture

Current-state overview for anyone extending the compiler. For the history of *how* it
got here (oracle-verified facts, deliberate scope cuts, one section per milestone), see
[`docs/architecture/roadmap.md`](../architecture/roadmap.md).

## The compile pipeline

`pbcxx` transpiles a `.pb` file to C++ and hands it to a real backend compiler:

```
.pb source
    |
    v
Preprocessor   (IncludeFile/XIncludeFile, CompilerIf/CompilerSelect, DataSection,
                non-recursive Macro - from M5)
    |
    v
Lexer          (source text -> flat token stream)
    |
    v
Parser         (tokens -> untyped ast::Module, recursive descent)
    |
    v
Sema           (name resolution + type-family inference, in place on the Module)
    |
    v
Codegen        (Module -> a single C++ translation unit's worth of text)
    |
    v
g++ / clang++  (the real backend compiler, invoked as a subprocess)
    |
    v
native executable
```

Each stage is a distinct, independently testable component under `compiler/src/`:

| Stage | Directory | Key type(s) |
|---|---|---|
| Diagnostics | `compiler/src/diagnostics/` | `SourceLoc`, `DiagnosticEngine` |
| Lexer | `compiler/src/lexer/` | `Token`, `TokenKind`, `TypeSuffix`, `Lexer` |
| Parser | `compiler/src/parser/` | `Parser`, produces an `ast::Module` |
| AST | `compiler/src/ast/` | `ast::Expr`/`ast::Stmt` hierarchies, tag-dispatched via `ExprKind`/`StmtKind` |
| Sema | `compiler/src/sema/` | `Sema` - resolves types, classifies each expression's `ValueKind` |
| Codegen | `compiler/src/codegen/` | `Codegen` - lowers a `Module` to C++ text |
| Driver | `compiler/src/driver/` | `main.cpp` (pipeline + backend invocation), `process.cpp` (cross-platform subprocess execution) |

A `SourceLoc` carries a `fileId` (registered once per physical file via
`DiagnosticEngine::registerFile`) alongside line/column, so a diagnostic from deep inside
an `IncludeFile`'d file - or from source reached only after macro/`CompilerIf` expansion,
once those land in M5 - still reports its true originating location.

AST nodes use a cheap enum dispatch tag (`ExprKind`/`StmtKind`) rather than
`dynamic_cast`, so Sema/Codegen's visitor-style `switch` statements don't depend on RTTI
being enabled by the target toolchain.

## Directory / component map

```
compiler/src/    the compiler front end + back end (see pipeline above)
runtime/include/ the C++ runtime backing built-in types (PBString, debugPrint, ...)
tests/           unit/ (Catch2), e2e/ (golden .pb programs), e2e_diff/ (differential
                 against the real pbcompilerc), fixtures/
scripts/         oracle-testing helpers (diff_against_pbcompilerc.sh, ...)
docs/            this documentation
examples/        example .pb programs
```

## Why `pbcxx_frontend` is a separate static library

`compiler/CMakeLists.txt` builds `pbcxx_frontend` (Diagnostics + Lexer + Parser) as its
own `STATIC` library, separate from `pbcxx` itself (which adds Sema + Codegen + the
driver on top). This mirrors eBasic's own `ebasic_frontend` split: a future `docgen` or
language-server tool would need the exact same parsing behavior `pbcxx` itself uses (no
second, drifting parser implementation) without ever needing type-checking or code
generation, since everything such a tool documents is already structurally resolved by
the parser alone.

## Runtime

`runtime/include/easybasic/runtime/` is header-only for now (every type so far is either
a template or a small inline wrapper). `PBString` is a ref-counted/copy-on-write wrapper
around `std::string`, storing UTF-8 - a deliberate departure from PB's own internal
length-prefixed-UTF-16 layout (see the roadmap's M0 notes for the full rationale).
`debug.hpp`'s `debugPrint` reproduces PB's own debugger-console output format
byte-for-byte, since that's what makes both the golden and differential test suites
checkable at all (`Debug` is otherwise silently stripped by both compilers unless built
in debug mode).

## Generated API docs

Doxygen-generated per-symbol API documentation, config at `docs/Doxyfile`:

```sh
cmake --build build/<preset> --target docs
```

Output lands in `build/<preset>/docs/html/index.html`.
