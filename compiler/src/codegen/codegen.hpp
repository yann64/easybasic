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
    /// Emits every top-level `#Name = expr` / `Enumeration` member as a
    /// global `static const` before `main()` - see the .cpp file's own
    /// comment for why these can't just be handled inline like Define/
    /// Assign statements. Known M1 limitation: only *top-level* constant
    /// declarations are collected this way; one nested inside an If/For/
    /// While/Repeat body is silently dropped (real PB code overwhelmingly
    /// declares constants at module scope, so this hasn't blocked anything
    /// yet, but it's a real gap worth fixing before it does).
    void genGlobalConstants();
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
    /// Wraps `exprCode` (whose family is `fromFamily`) in whatever
    /// conversion is needed to store it into a variable of `toSuffix`,
    /// applying PB's banker's-rounding float-to-integer rule where it
    /// applies (oracle-verified: 2.5->2, 3.5->4, -2.5->-2 - round-half-to-
    /// even, not truncation).
    static std::string convert(const std::string& exprCode, ValueKind fromFamily, TypeSuffix toSuffix);

    const ast::Module& module_;
    const Sema& sema_;
    bool debugMode_;
    std::string out_;
    int tempCounter_ = 0; ///< Disambiguates generated temporaries (For bounds, Select's subject).
};

/// The C++ type easybasic uses to represent each PB type-suffix. Exposed for
/// tests/tooling as well as Codegen itself.
const char* cppTypeFor(TypeSuffix suffix);

} // namespace easybasic
