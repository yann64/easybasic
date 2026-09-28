# Using `pbcompilerc` as a semantic oracle

PureBasic 6.41's own "C Backend" compiler (`pbcompilerc`, typically at
`~/purebasic/compilers/pbcompilerc`) is a live, authoritative reference for exactly how
real PureBasic behaves. Whenever a language question comes up during implementation -
"what's the real operator precedence here", "does this even compile", "what does this
lower to" - settle it empirically with the oracle rather than guessing or trusting
secondary documentation.

## Fast syntax/legality check

```sh
pbcompilerc program.pb -k
```

`-k` ("check") does a syntax check only - no codegen, no linking. Fast, and safe to run
repeatedly. Use this to answer yes/no questions ("does PB accept `2^2`?").

## Semantic/lowering check

```sh
pbcompilerc program.pb -c -o some_output_name
```

`-c` ("commented") produces a `purebasic.c` file in the current working directory,
showing exactly how the official compiler lowers the program - variable types, operator
behavior, constant folding, runtime call shapes. Read `purebasic.c` directly to answer
"what does this actually compute/become" questions. This is how e.g. the banker's-
rounding float-to-integer conversion rule and the real division behavior of `/` were
discovered (see `docs/architecture/roadmap.md`'s M0 notes).

Note: linking can fail for some minimal programs due to a pre-existing, unrelated
toolchain issue in this PB install (`libpbmimalloc.a` referencing an undefined
`__wrap_memcpy` symbol - reproduces only for a program whose *only* content is a bare
`Debug` statement with nothing else; almost any real program links fine). Passing `-d`
(debugger mode) avoids it entirely and is otherwise required anyway to make `Debug`
statements actually print (see below) - so oracle invocations that need to *run* the
resulting binary should default to `-d`.

## Running the binary

```sh
pbcompilerc program.pb -d -o program_bin
./program_bin
```

`-d` enables debugger support, which is also the only way to see `Debug` output at all:
a plain (non-`-d`) build strips every `Debug` statement to nothing (verified: it becomes
a bare comment in `purebasic.c`, with no runtime call emitted). With `-d`, each `Debug`
prints `[Debugger]  <value>` (exactly two spaces after the closing bracket) to stdout.

## The differential testing harness

`scripts/diff_against_pbcompilerc.sh <pbcxx> <program.pb>` automates exactly this
recipe against both `pbcompilerc` and `pbcxx`, diffing their stdout - see
`tests/e2e_diff/`. This is the project's most powerful correctness tool: prefer adding a
differential test over a hand-written golden fixture whenever the construct under test
doesn't yet depend on stdlib `pbcxx` hasn't implemented.

## Precedence probing (planned, M1)

`scripts/pb_precedence_probe.sh` (not yet written) will mechanically generate many
pairwise binary-operator combinations with distinguishable literal operands, compile
each with `pbcompilerc -c`, and extract the constant-folded result to reverse-engineer
the complete operator precedence table - cheaper and more reliable than further one-off
hand testing once more operators than `+ - * / %` are in scope.
