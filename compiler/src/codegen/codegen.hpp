#pragma once

#include <string>

#include "../ast/ast.hpp"
#include "../sema/sema.hpp"

namespace easybasic {

/// Lowers a Sema-checked ast::Module to a single C++ translation unit's
/// worth of text, ready to hand to g++/clang++.
class Codegen {
public:
    /// `debugMode` mirrors `pbcompilerc -d`: when false (the default, like a
    /// plain `pbcompilerc` build), `Debug` statements emit no code at all;
    /// when true, they emit the exact "[Debugger]  <value>" line real PB's
    /// debugger console prints, byte-for-byte (oracle-verified).
    Codegen(const ast::Module& module, const Sema& sema, bool debugMode);

    /// Returns the generated C++ source.
    std::string generate();

private:
    /// Emits every `#Name = expr` / `Enumeration` member, wherever it
    /// appears in the source, as a global `static const` before `main()` -
    /// see the .cpp file's own comment for why these can't just be handled
    /// inline like Define/Assign statements. A constant is purely compile-
    /// time and entirely independent of runtime control flow (oracle-
    /// verified: one declared inside a never-taken `If` branch is still
    /// usable afterward - PB resolves it positionally/textually, not by
    /// actually executing the branch), so this recurses into every nested
    /// block (`If`/`Select`/`For`/`While`/`Repeat`/`ForEach`/a
    /// `ProcedureDecl`'s own body - oracle-verified legal too) via
    /// `genConstantsIn`, in the same declare-before-use document order Sema
    /// itself already enforces.
    void genGlobalConstants();
    /// The recursive worker behind `genGlobalConstants` - see its own doc
    /// comment.
    void genConstantsIn(const ast::Block& block);
    /// Emits every declared `Structure` as a real C++ `struct s_<name>`
    /// before anything that might be an instance of one (global variables,
    /// arrays, procedures) - `Sema::structureDeclarationOrder()` is already
    /// a valid emission order (see its own doc comment on why source order
    /// suffices).
    void genStructures();
    /// Emits a real C++ function prototype for every top-level `Declare` -
    /// PB itself requires a procedure to be fully defined before any call to
    /// it (oracle-verified: no forward declarations/hoisting - see
    /// ast::ProcedureDeclStmt's own doc comment), *except* through an
    /// explicit `Declare` (ast::DeclareStmt's own doc comment), which is
    /// exactly what C++ itself needs a forward declaration for too - so
    /// unlike genProcedures() below, this one genuinely can't rely on
    /// source order alone. Must run before genProcedures() so a Declare'd
    /// name is already known to the C++ compiler by the time an earlier-
    /// defined procedure's body calls it.
    void genDeclarePrototypes();
    /// Emits every top-level `Procedure` as a standalone C++ function,
    /// before `main()`, in source order - which already satisfies C++'s own
    /// "declared before use" rule for free for anything reachable without a
    /// `Declare` (see genDeclarePrototypes() just above for the one case
    /// that needs its own pass). Same top-level-only limitation as
    /// genGlobalConstants (PB doesn't nest procedures anyway, so this
    /// hasn't been a real gap in practice).
    void genProcedures();
    void genProcedureDecl(const ast::ProcedureDeclStmt& proc);
    /// Emits one `pbDataAddInt`/`Double`/`String` call per `Data` value,
    /// wherever a `DataSection` appears, as the very first statements in
    /// generated `main()` - independent of where the `DataSection` is
    /// textually written (oracle-verified: it has no runtime side effect at
    /// its own source position, and multiple `DataSection`s concatenate
    /// into a single pool - see ast::DataSectionStmt's own doc comment).
    /// Recurses the same way genConstantsIn does (If/Select/For/While/
    /// Repeat/ForEach/a ProcedureDecl's own body - oracle-verified a
    /// DataSection can live inside a Procedure too), in the same order
    /// Sema::collectDataSections already walked to assign each label's
    /// index, which is what keeps the two consistent.
    void genDataPool(const ast::Block& block);
    /// Emits one `static const std::array<std::int64_t, N> pb_label_<name>`
    /// per DataSection label `Sema::dataLabelAddressable` accepts (M7c) -
    /// what `?Label` (`genExpr`'s own `DataLabelAddress` case) lowers to the
    /// address of. Must run *after* `genProcedures()` (an array element can
    /// be `@Procedure()`, needing the real function already declared - see
    /// the .cpp file's own notes) and *before* `main()`'s own body, so
    /// plain global static initialization (not a runtime `pbDataAdd*`
    /// call, unlike genDataPool's own pool) is enough.
    void genDataLabelArrays();
    /// The recursive worker behind `genDataLabelArrays` - collects, per
    /// addressable label, its own ordered list of already-genExpr'd `.i`
    /// item value expressions into `out` (first-seen order, for
    /// deterministic output), mirroring `genDataPool`'s own recursive shape.
    void collectDataLabelArrays(const ast::Block& block,
                                 std::vector<std::pair<std::string, std::vector<std::string>>>& out);
    void genStmt(const ast::Stmt& stmt);
    void genBlock(const ast::Block& block);
    /// `floatContext` mirrors Sema::classify's own parameter: true exactly
    /// when some enclosing destination is Float-family, which is what makes
    /// PB's target-typed `/`/`%` (see Sema::classify's doc comment) pick
    /// real division/modulo instead of plain integer division. Threaded
    /// recursively so a Div/Mod node deep inside an Add/Sub/Mul tree still
    /// sees the destination's float-ness.
    std::string genExpr(const ast::Expr& expr, bool floatContext);
    /// Lowers an If/While/Until condition tree (comparisons, And/Or/Not) to
    /// a C++ `bool` expression - kept separate from genExpr because these
    /// operators are only legal in this one grammatical position in PB
    /// (see parser.hpp's own notes), so they never need PB-value semantics
    /// (no PBString-typed comparison result, no banker's-rounding target).
    std::string genCondition(const ast::Expr& expr);
    /// The flat-offset expression for indexing `v_name` given its (1 or 2)
    /// index expressions - a 2D array is a single row-major `std::vector`
    /// under the hood, using a hidden `v_name_dim1` companion variable (set
    /// by the `Dim` statement itself) for the row stride, rather than a
    /// vector-of-vectors (simpler indexing arithmetic, one allocation
    /// instead of one-per-row).
    std::string genArrayIndexCode(const std::string& name, const std::vector<std::unique_ptr<ast::Expr>>& indices);
    /// Wraps `exprCode` (whose family is `fromFamily`) in whatever
    /// conversion is needed to store it into a variable of `toSuffix`,
    /// applying PB's banker's-rounding float-to-integer rule where it
    /// applies (oracle-verified: 2.5->2, 3.5->4, -2.5->-2 - round-half-to-
    /// even, not truncation).
    static std::string convert(const std::string& exprCode, ValueKind fromFamily, TypeSuffix toSuffix);
    /// Like `convert()`, but for `Read`'s own two-stage coercion - oracle-
    /// verified that `Read.<suffix> varname` allows a String<->numeric
    /// cross-family mix (`Val()`/`Str()`-style, e.g. `Read.s` into an
    /// Integer-typed `varname` silently yields `Val("hello")` = `0`), unlike
    /// a normal assignment, which Sema rejects outright for the same mix.
    /// `convert()` itself can't be reused directly for a genuine cross-
    /// family case since it assumes (correctly, everywhere *else*) that
    /// Sema has already ruled that out. Falls back to `convert()` when the
    /// families already match, to reuse its existing width/banker's-
    /// rounding logic rather than duplicating it.
    static std::string convertReadValue(const std::string& readCall, ValueKind dataFamily, TypeSuffix targetSuffix);
    /// The C++ identifier for a declared name, exactly as Sema keys it
    /// (`symbols_`/`declarationOrder()`/a procedure's `locals`) - a plain
    /// name gets the usual `v_` prefix, while a pointer's name (which always
    /// carries a leading '*', see ast::DefineStmt::Declarator's own doc
    /// comment) gets `vp_` instead, both to strip the `*` - not a legal C++
    /// identifier character - and so a pointer and a same-named non-pointer
    /// variable (a real, distinct PB namespace pair - oracle-verified) never
    /// collide in the generated C++.
    static std::string cppVarName(const std::string& name);

    const ast::Module& module_;
    const Sema& sema_;
    bool debugMode_;
    std::string out_;
    int tempCounter_ = 0; ///< Disambiguates generated temporaries (For bounds, Select's subject).
    /// The return type of the procedure whose body is currently being
    /// emitted - used by a nested `ProcedureReturn`'s own conversion, and by
    /// the fallthrough safety net `genProcedureDecl` appends after the
    /// body. Only meaningful while emitting inside a procedure (PB
    /// procedures don't nest, so a single field - not a stack - is enough).
    TypeSuffix currentProcReturnSuffix_ = TypeSuffix::Integer;
    /// The Sema-resolved signature of the procedure whose body is currently
    /// being emitted, or nullptr at top level - `pointeeTypeOf` (below)
    /// consults its own `pointerPointeeTypes` first, scoped exactly like
    /// `currentProcReturnSuffix_` is (see ProcedureInfo::pointerPointeeTypes'
    /// own doc comment for why this scoping is load-bearing, not cosmetic).
    const Sema::ProcedureInfo* currentProcInfo_ = nullptr;
    /// The pointee type `pointerKey` was declared to point at, preferring
    /// the currently-generating procedure's own locals (see
    /// `currentProcInfo_`) over `Sema::pointeeTypeOf`'s single flat,
    /// cross-procedure-unscoped view - every FieldAccess-on-pointer/
    /// MethodCall dereference goes through this instead of calling
    /// `sema_.pointeeTypeOf` directly.
    Sema::ResolvedType pointeeTypeOf(const std::string& pointerKey) const;
};

/// The C++ type easybasic uses to represent each PB type-suffix. Exposed for
/// tests/tooling as well as Codegen itself. `TypeSuffix::Struct` has no
/// single fixed C++ type (it depends on which Structure), so this returns a
/// placeholder for it - callers that might be dealing with a struct-typed
/// value must use the `structName`-aware overload below instead.
const char* cppTypeFor(TypeSuffix suffix);

/// As above, but resolves `TypeSuffix::Struct` to the real generated C++
/// struct name (`s_<structName>`) using the Structure name Sema already
/// resolved. Every *new* call site that might see a struct-typed variable,
/// array element, or field goes through this one; the plain, single-
/// argument overload remains for existing call sites that are only ever
/// reached for definitely-primitive values (e.g. `Debug`'s
/// `std::to_string`, `Mod`'s `fmod` cast).
std::string cppTypeFor(TypeSuffix suffix, const std::string& structName);

/// The C++ literal representing PB's "zero value" for `suffix` - `0`/`0.0`
/// for numeric types, an empty `PBString` for `.s`, a default-constructed
/// `s_<structName>{}` for a Structure. Used both for a procedure's
/// fallthrough-returns-zero-value semantics (oracle-verified: falling off
/// the end of a procedure body returns the declared return type's zero
/// value, not undefined behavior) and for default-initializing a
/// global/local variable's own C++ declaration.
std::string defaultValueLiteral(TypeSuffix suffix, const std::string& structName = "");

} // namespace easybasic
