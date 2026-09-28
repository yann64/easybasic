#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../ast/ast.hpp"
#include "../diagnostics/diagnostics.hpp"

namespace easybasic {

/// The three type "families" Sema distinguishes for M0's numeric/string
/// subset. This is deliberately coarser than the full 11-suffix type system
/// (that distinction matters for codegen's chosen C++ type, but not for the
/// handful of family-level rules Sema itself needs to enforce - e.g. "is
/// this side of `/` promoted to floating-point").
enum class ValueKind { IntegerFamily, FloatFamily, StringFamily };

ValueKind familyOf(TypeSuffix suffix);

/// Resolves variable types (via `Define` or PB's own implicit-declaration
/// behavior - a plain assignment to an unseen name declares it, defaulting
/// to Integer, exactly like real PureBasic with `EnableExplicit` off) and
/// annotates enough about each expression for Codegen to get PB's numeric
/// conversion rules right without re-deriving them itself.
class Sema {
public:
    explicit Sema(DiagnosticEngine& diagnostics);

    /// Walks the whole module, populating the symbol table in place.
    /// Returns false if any error was reported (mirrors DiagnosticEngine's
    /// own hasErrors(), exposed here for driver convenience).
    bool analyze(ast::Module& module);

    /// The resolved type-suffix for a declared/implicitly-declared name, or
    /// TypeSuffix::Integer if never seen (should not happen after a
    /// successful analyze() on a well-formed module).
    TypeSuffix typeOf(const std::string& lowerName) const;

    /// Bottom-up family classification with no destination context - what an
    /// expression "naturally" is when nothing forces it otherwise. Used for
    /// a `Debug`'d expression (which has no destination type at all) and for
    /// checkAssignable's String-vs-numeric check (context never turns a
    /// String into a numeric family or vice versa, so the distinction below
    /// doesn't matter for that check).
    ValueKind familyOfExpr(const ast::Expr& expr) const;

    /// PB's `/` (and, by consistent extrapolation - not yet independently
    /// oracle-disambiguated, see docs/architecture/roadmap.md's M0 notes -
    /// `%` too) is *target-typed*: oracle-verified that `Define g.d = 1 +
    /// 7/2` evaluates to `4.5`, not `4` - a Double destination forces real
    /// division through the *entire* initializer expression tree, not just
    /// a shallow top-level check, whereas the exact same expression with no
    /// float destination at all (a bare `Debug 7/2`, or an Integer
    /// destination) performs plain integer division and yields `3`.
    /// `floatContext` is true exactly when some enclosing destination is
    /// Float-family; Codegen threads the same flag through genExpr so the
    /// generated code matches this classification node-for-node.
    ValueKind classify(const ast::Expr& expr, bool floatContext) const;

    /// Every declared (explicit `Define` or implicitly-declared) variable,
    /// in first-seen order, for Codegen to emit as C++ globals.
    const std::vector<std::pair<std::string, TypeSuffix>>& declarationOrder() const {
        return order_;
    }

private:
    void visitStmt(ast::Stmt& stmt);
    void declare(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                 SourceLoc loc);
    void visitExpr(ast::Expr& expr);
    /// Rejects String<->numeric assignments, which real PB also rejects
    /// without an explicit Str()/Val() conversion (not modeled yet - M4).
    void checkAssignable(const std::string& targetSpelling, TypeSuffix targetSuffix,
                          const ast::Expr& value, SourceLoc loc);

    DiagnosticEngine& diagnostics_;
    std::unordered_map<std::string, TypeSuffix> symbols_;
    std::vector<std::pair<std::string, TypeSuffix>> order_; ///< First-seen declaration order.
};

} // namespace easybasic
