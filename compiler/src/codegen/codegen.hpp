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
    void genStmt(const ast::Stmt& stmt);
    /// `floatContext` mirrors Sema::classify's own parameter: true exactly
    /// when some enclosing destination is Float-family, which is what makes
    /// PB's target-typed `/`/`%` (see Sema::classify's doc comment) pick
    /// real division/modulo instead of plain integer division. Threaded
    /// recursively so a Div/Mod node deep inside an Add/Sub/Mul tree still
    /// sees the destination's float-ness.
    std::string genExpr(const ast::Expr& expr, bool floatContext);
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
};

/// The C++ type easybasic uses to represent each PB type-suffix. Exposed for
/// tests/tooling as well as Codegen itself.
const char* cppTypeFor(TypeSuffix suffix);

} // namespace easybasic
