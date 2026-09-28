#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../diagnostics/diagnostics.hpp"
#include "../lexer/token.hpp"

namespace easybasic::ast {

enum class BinaryOp { Add, Sub, Mul, Div, Mod };
enum class UnaryOp { Negate };

/// Cheap dispatch tag, one per concrete Expr subclass - avoids RTTI
/// (dynamic_cast) so Sema/Codegen visitors work identically whether or not
/// the target toolchain builds with RTTI enabled (Haiku's GCC does by
/// default, but there's no reason to depend on it for a simple closed set
/// of node types known entirely at compile time).
enum class ExprKind { IntLiteral, FloatLiteral, StringLiteral, VarRef, Binary, Unary };

/// Base of every expression node. Untyped: Sema annotates/validates types in
/// place over this same tree rather than building a second, typed tree - the
/// pipeline only ever has one AST.
struct Expr {
    SourceLoc loc;
    ExprKind kind;
    explicit Expr(ExprKind k) : kind(k) {}
    virtual ~Expr() = default;
    // Every concrete node lives behind a unique_ptr<Expr> and is downcast via
    // its `kind` tag (see Sema/Codegen) - copying/moving through the base
    // class would slice the derived fields, so both are deleted rather than
    // left to the (dangerous) implicitly-defined versions.
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;
};

struct IntLiteralExpr : Expr {
    IntLiteralExpr() : Expr(ExprKind::IntLiteral) {}
    long long value = 0;
    TypeSuffix suffix = TypeSuffix::None;
};

struct FloatLiteralExpr : Expr {
    FloatLiteralExpr() : Expr(ExprKind::FloatLiteral) {}
    double value = 0.0;
};

struct StringLiteralExpr : Expr {
    StringLiteralExpr() : Expr(ExprKind::StringLiteral) {}
    std::string value;
};

struct VarRefExpr : Expr {
    VarRefExpr() : Expr(ExprKind::VarRef) {}
    std::string name;      ///< Canonical (lowercased) for lookup.
    std::string spelling;  ///< Original source casing, for diagnostics/codegen.
    TypeSuffix suffix = TypeSuffix::None;
};

struct BinaryExpr : Expr {
    BinaryExpr() : Expr(ExprKind::Binary) {}
    BinaryOp op = BinaryOp::Add;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct UnaryExpr : Expr {
    UnaryExpr() : Expr(ExprKind::Unary) {}
    UnaryOp op = UnaryOp::Negate;
    std::unique_ptr<Expr> operand;
};

enum class StmtKind { Define, Assign, Debug };

/// Base of every statement node.
struct Stmt {
    SourceLoc loc;
    StmtKind kind;
    explicit Stmt(StmtKind k) : kind(k) {}
    virtual ~Stmt() = default;
    // See Expr's identical rationale just above.
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    Stmt(Stmt&&) = delete;
    Stmt& operator=(Stmt&&) = delete;
};

/// One `Define a.i[, b.s = "x", ...]` statement. PB allows several
/// comma-separated declarators per Define, each with its own optional
/// initializer.
struct DefineStmt : Stmt {
    DefineStmt() : Stmt(StmtKind::Define) {}
    struct Declarator {
        std::string name;
        std::string spelling;
        TypeSuffix suffix = TypeSuffix::None;
        std::unique_ptr<Expr> init; ///< May be null (default-initialized).
    };
    std::vector<Declarator> declarators;
};

/// `name[.suffix] = expr`.
struct AssignStmt : Stmt {
    AssignStmt() : Stmt(StmtKind::Assign) {}
    std::string name;
    std::string spelling;
    TypeSuffix suffix = TypeSuffix::None;
    std::unique_ptr<Expr> value;
};

/// `Debug expr`. Mirrors real PureBasic: compiled out entirely unless the
/// program is built in debug mode (verified against pbcompilerc - a plain,
/// non-`-d` build emits no code at all for Debug statements; `pbcompilerc
/// -d` links successfully and prints the value prefixed with
/// "[Debugger]  "). `pbcxx` reproduces this rather than always printing, so
/// differential testing against the real compiler stays meaningful.
struct DebugStmt : Stmt {
    DebugStmt() : Stmt(StmtKind::Debug) {}
    std::unique_ptr<Expr> value;
};

/// A whole compiled translation unit.
struct Module {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace easybasic::ast
