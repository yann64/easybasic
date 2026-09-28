#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../diagnostics/diagnostics.hpp"
#include "../lexer/token.hpp"

namespace easybasic::ast {

/// Oracle-verified precedence groups (docs/architecture/roadmap.md's M1
/// notes have the full derivation and the two-directional reversal-test
/// methodology used to nail them down after an initial, buggier attempt
/// mis-merged two of these into one tier - caught by the differential e2e
/// suite). Tightest to loosest: unary `-`/`~` > flat tier {`%`, `!`(xor),
/// `<<`, `>>`} > flat tier {`&`, `|`} > `*`/`/` > binary `+`/`-`. Comparisons
/// sit looser still and are restricted to boolean contexts (If/While/Until
/// conditions, or `Bool()`). `And`/`Or` are likewise ONE FLAT left-to-right
/// tier, not nested (And does *not* bind tighter than Or, unlike virtually
/// every other language).
/// `LogicalXOr` is parsed but deliberately not lowered by Codegen yet - its
/// runtime truth table showed a genuine, unexplained anomaly under oracle
/// testing that needs more investigation before being trusted.
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    BitAnd, BitOr, BitXor, ShiftLeft, ShiftRight,
    Eq, Ne, Lt, Gt, Le, Ge,
    LogicalAnd, LogicalOr, LogicalXOr,
};
enum class UnaryOp { Negate, BitNot, LogicalNot };

/// Cheap dispatch tag, one per concrete Expr subclass - avoids RTTI
/// (dynamic_cast) so Sema/Codegen visitors work identically whether or not
/// the target toolchain builds with RTTI enabled (Haiku's GCC does by
/// default, but there's no reason to depend on it for a simple closed set
/// of node types known entirely at compile time).
enum class ExprKind { IntLiteral, FloatLiteral, StringLiteral, VarRef, ConstRef, Binary, Unary };

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

/// A `#Name` reference to a compile-time constant (own namespace, separate
/// from variables - `#Foo` and `Foo` never collide).
struct ConstRefExpr : Expr {
    ConstRefExpr() : Expr(ExprKind::ConstRef) {}
    std::string name;
    std::string spelling;
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

enum class StmtKind {
    Define, Assign, Debug,
    If, Select, For, While, Repeat,
    Break, Continue, EnableExplicit, ConstDecl, Enumeration,
};

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

/// A statement sequence, reused for every construct with a nested body
/// (If/Select/For/While/Repeat).
using Block = std::vector<std::unique_ptr<Stmt>>;

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

/// `If cond ... [ElseIf cond ...]* [Else ...] EndIf`. Each branch's
/// `condition` is null exactly for a trailing `Else` (never for `If`/
/// `ElseIf`, which always have one).
struct IfStmt : Stmt {
    IfStmt() : Stmt(StmtKind::If) {}
    struct Branch {
        std::unique_ptr<Expr> condition; ///< Null only for the `Else` branch.
        Block body;
    };
    std::vector<Branch> branches;
};

/// `Select selector ... [Case v1[, v2, ...] ...]* [Default ...] EndSelect`.
/// An empty `values` list marks the `Default` branch (PB has no `CaseElse` -
/// oracle-verified rejected as a syntax error; `Default` is the only form).
struct SelectStmt : Stmt {
    SelectStmt() : Stmt(StmtKind::Select) {}
    struct CaseBranch {
        std::vector<std::unique_ptr<Expr>> values; ///< Empty means `Default`.
        Block body;
    };
    std::unique_ptr<Expr> selector;
    std::vector<CaseBranch> cases;
};

/// `For var = from To to [Step step] ... Next [var]`.
struct ForStmt : Stmt {
    ForStmt() : Stmt(StmtKind::For) {}
    std::string varName;
    std::string varSpelling;
    TypeSuffix suffix = TypeSuffix::None;
    std::unique_ptr<Expr> from;
    std::unique_ptr<Expr> to;
    std::unique_ptr<Expr> step; ///< Null means the default step of 1.
    Block body;
};

/// `While cond ... Wend`.
struct WhileStmt : Stmt {
    WhileStmt() : Stmt(StmtKind::While) {}
    std::unique_ptr<Expr> condition;
    Block body;
};

/// `Repeat ... Until cond` or `Repeat ... ForEver` (`untilCondition` null
/// means the latter - an unconditional loop).
struct RepeatStmt : Stmt {
    RepeatStmt() : Stmt(StmtKind::Repeat) {}
    Block body;
    std::unique_ptr<Expr> untilCondition;
};

struct BreakStmt : Stmt {
    BreakStmt() : Stmt(StmtKind::Break) {}
};

struct ContinueStmt : Stmt {
    ContinueStmt() : Stmt(StmtKind::Continue) {}
};

/// The `EnableExplicit` directive: sets a Sema-wide flag rather than
/// generating any code of its own.
struct EnableExplicitStmt : Stmt {
    EnableExplicitStmt() : Stmt(StmtKind::EnableExplicit) {}
};

/// `#Name = expr`, a compile-time constant (own namespace from variables).
struct ConstDeclStmt : Stmt {
    ConstDeclStmt() : Stmt(StmtKind::ConstDecl) {}
    std::string name;
    std::string spelling;
    std::unique_ptr<Expr> value;
};

/// `Enumeration [#First[=v]] ... EndEnumeration`: each `#Name` becomes an
/// integer ConstDecl, auto-incrementing from the previous member's value
/// (or 0 for the very first member) unless given its own `= expr`.
struct EnumerationStmt : Stmt {
    EnumerationStmt() : Stmt(StmtKind::Enumeration) {}
    struct Member {
        std::string name;
        std::string spelling;
        std::unique_ptr<Expr> explicitValue; ///< Null means auto-increment.
    };
    std::vector<Member> members;
};

/// A whole compiled translation unit.
struct Module {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace easybasic::ast
