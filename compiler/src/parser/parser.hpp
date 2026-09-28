#pragma once

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
    std::unique_ptr<ast::Stmt> parseDefine();
    std::unique_ptr<ast::Stmt> parseDebug();
    std::unique_ptr<ast::Stmt> parseAssignment();
    void skipStatementSeparators();

    // --- expressions, precedence climbing; see ast.hpp / codegen for the
    // oracle-verified precedence rationale (`%` binds tighter than `*`/`/`,
    // there is no `^` operator at all) ---
    std::unique_ptr<ast::Expr> parseExpr();      // + -
    std::unique_ptr<ast::Expr> parseMul();       // * /
    std::unique_ptr<ast::Expr> parseMod();       // %
    std::unique_ptr<ast::Expr> parseUnary();     // unary -
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
