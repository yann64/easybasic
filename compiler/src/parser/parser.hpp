#pragma once

#include <initializer_list>
#include <memory>
#include <vector>

#include "../ast/ast.hpp"
#include "../diagnostics/diagnostics.hpp"
#include "../lexer/token.hpp"

namespace easybasic {

/// Hand-written recursive-descent parser: a flat Token stream in, an
/// untyped ast::Module out. Chosen over a generated parser for the same
/// reason eBasic's own parser is hand-written: substantially better,
/// PB-specific error messages than a generic parser-generator gives.
class Parser {
public:
    Parser(std::vector<Token> tokens, DiagnosticEngine& diagnostics);

    std::unique_ptr<ast::Module> parseModule();

private:
    // --- statements ---
    std::unique_ptr<ast::Stmt> parseStatement();
    /// `isGlobal` is true only for the `Global` keyword - `Define` and
    /// `Protected` both parse to the same node with `isGlobal` false (see
    /// ast::DefineStmt's own doc comment for why `Protected` needs nothing
    /// more than that).
    std::unique_ptr<ast::Stmt> parseDefine(bool isGlobal);
    std::unique_ptr<ast::Stmt> parseShared();
    std::unique_ptr<ast::Stmt> parseDim();
    /// Called once `Identifier(` has been seen at statement position -
    /// parses the full `Name(args)`, then decides between an array-element
    /// assignment (`Name(args) = expr`) and a call-as-statement based on
    /// whether `=` follows (see ast::IndexAssignStmt's own doc comment for
    /// why this can't be decided any earlier, at the token level alone).
    std::unique_ptr<ast::Stmt> parseCallOrIndexAssignStatement();
    std::unique_ptr<ast::Stmt> parseDebug();
    std::unique_ptr<ast::Stmt> parseAssignmentOrConstDecl();
    std::unique_ptr<ast::Stmt> parseIf();
    std::unique_ptr<ast::Stmt> parseSelect();
    std::unique_ptr<ast::Stmt> parseFor();
    std::unique_ptr<ast::Stmt> parseWhile();
    std::unique_ptr<ast::Stmt> parseRepeat();
    std::unique_ptr<ast::Stmt> parseEnumeration();
    std::unique_ptr<ast::Stmt> parseProcedureDecl();
    std::unique_ptr<ast::Stmt> parseProcedureReturn();
    ast::Block parseBlockUntil(std::initializer_list<TokenKind> terminators);
    void skipStatementSeparators();

    // --- expressions, precedence climbing; see ast.hpp / docs/architecture/
    // roadmap.md's M1 notes for the oracle-verified precedence rationale
    // (no `^` operator; `%`/`&`/`|`/`!`/`<<`/`>>` are one flat left-to-right
    // tier tighter than `*`/`/`, which is tighter than binary `+`/`-`;
    // `And`/`Or` are likewise one flat left-to-right tier, not nested).
    //
    // parseCondition() is the entry point for If/While/Until conditions,
    // where bare comparisons and And/Or/Not/XOr are legal; parseExpr() (the
    // general arithmetic grammar used everywhere else - Define/Assign/
    // Debug/operands of a comparison) does NOT accept them at all, mirroring
    // PB's own restriction (oracle-verified: a bare comparison outside a
    // conditional is a compile error in real PB).
    std::unique_ptr<ast::Expr> parseCondition();     // And/Or/XOr (flat)
    std::unique_ptr<ast::Expr> parseLogicalNot();    // prefix Not
    std::unique_ptr<ast::Expr> parseComparison();    // = <> < > <= >=
    std::unique_ptr<ast::Expr> parseExpr();          // + -
    std::unique_ptr<ast::Expr> parseMul();           // * /
    std::unique_ptr<ast::Expr> parseBitwiseOrAnd();  // & | (flat, its own tier - looser than parseBitwiseTight)
    std::unique_ptr<ast::Expr> parseBitwiseTight();  // % ! << >> (flat, tighter than & |)
    std::unique_ptr<ast::Expr> parseUnary();         // unary - ~
    std::unique_ptr<ast::Expr> parsePrimary();

    // --- token stream plumbing ---
    [[nodiscard]] const Token& peek(int offset = 0) const;
    const Token& advance();
    [[nodiscard]] bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& expect(TokenKind kind, const char* context);
    [[nodiscard]] bool isAtStatementEnd() const;

    std::vector<Token> tokens_;
    std::size_t pos_ = 0;
    DiagnosticEngine& diagnostics_;
};

} // namespace easybasic
