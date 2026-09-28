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

    /// The resolved type for a declared `#Name` constant, or
    /// TypeSuffix::Integer if never seen.
    TypeSuffix constTypeOf(const std::string& lowerName) const;

    /// Every declared constant (`#Name = expr` or an `Enumeration` member),
    /// in first-seen order, for Codegen to emit as C++ globals. Constants
    /// live in their own namespace from variables (`#Foo` and `Foo` never
    /// collide), mirrored here by a separate table from `declarationOrder`.
    const std::vector<std::pair<std::string, TypeSuffix>>& constDeclarationOrder() const {
        return constOrder_;
    }

    /// A declared procedure's resolved signature, for Codegen to emit a
    /// matching C++ function and to convert call-site arguments/return
    /// values with the same banker's-rounding rules as everything else.
    struct ProcedureInfo {
        TypeSuffix returnSuffix = TypeSuffix::Integer;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount = 0; ///< Params before the first one with a default.
        /// Every local (parameters first, in declaration order, then any
        /// body-internal `Define`/implicit-declare) - Codegen skips the
        /// first `paramSuffixes.size()` entries when emitting a function's
        /// *non-parameter* locals, since those are already real C++
        /// parameters.
        std::vector<std::pair<std::string, TypeSuffix>> locals;
    };

    /// Returns nullptr if `lowerName` was never declared as a procedure.
    const ProcedureInfo* procedureInfo(const std::string& lowerName) const;

private:
    void visitStmt(ast::Stmt& stmt);
    void visitBlock(ast::Block& block);
    void declare(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                 SourceLoc loc);
    /// Like declare(), but for a name reached through PB's implicit-
    /// declaration behavior (a bare assignment, a For-loop variable, or
    /// reading a never-assigned name) rather than an explicit `Define`.
    /// Errors under `EnableExplicit` instead of silently declaring, exactly
    /// like real PB (oracle-verified error text: "With 'EnableExplicit',
    /// variables have to be declared: <name>.") - but still declares
    /// afterwards regardless, purely so Codegen never sees a symbol with no
    /// recorded type (the pipeline already stops before Codegen runs once
    /// any error is recorded, so this is a defensive fallback, not a way to
    /// let EnableExplicit violations silently through).
    void declareImplicit(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                          SourceLoc loc);
    void declareConst(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                       SourceLoc loc);
    void visitExpr(ast::Expr& expr);
    /// Rejects String<->numeric assignments, which real PB also rejects
    /// without an explicit Str()/Val() conversion (not modeled yet - M4).
    void checkAssignable(const std::string& targetSpelling, TypeSuffix targetSuffix,
                          const ast::Expr& value, SourceLoc loc);
    /// Walks a condition expression (If/While/Until), flagging logical
    /// `XOr` - see BinaryOp::LogicalXOr's own doc comment for why it's not
    /// yet trusted - and resolving/declaring any variables it references.
    void visitCondition(ast::Expr& expr);
    /// Validates a call's argument count against the callee's signature and
    /// visits each argument expression; reports an "undeclared procedure"
    /// error (with a recovery fallback so later statements still resolve
    /// sensibly) if `name` was never declared.
    void visitCall(ast::CallExpr& call);

    DiagnosticEngine& diagnostics_;
    std::unordered_map<std::string, TypeSuffix> symbols_;
    std::vector<std::pair<std::string, TypeSuffix>> order_; ///< First-seen declaration order.
    std::unordered_map<std::string, TypeSuffix> constants_;
    std::vector<std::pair<std::string, TypeSuffix>> constOrder_;
    bool explicitEnabled_ = false;
    std::unordered_map<std::string, ProcedureInfo> procedures_;
    /// The return suffix of the procedure whose body is currently being
    /// visited, used by a nested `ProcedureReturn`'s own type checking; only
    /// meaningful while `insideProcedure_` is true (PB procedures don't
    /// nest, so a single flag - not a stack - is enough).
    TypeSuffix currentProcedureReturnSuffix_ = TypeSuffix::Integer;
    bool insideProcedure_ = false;
};

} // namespace easybasic
