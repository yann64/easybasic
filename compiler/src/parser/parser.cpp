#include "parser.hpp"

#include <cctype>

namespace easybasic {

namespace {
std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}
} // namespace

Parser::Parser(std::vector<Token> tokens, DiagnosticEngine& diagnostics)
    : tokens_(std::move(tokens)), diagnostics_(diagnostics) {}

const Token& Parser::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= tokens_.size()) {
        return tokens_.back(); // EndOfFile sentinel
    }
    return tokens_[idx];
}

const Token& Parser::advance() {
    const Token& tok = peek();
    if (pos_ + 1 < tokens_.size()) {
        ++pos_;
    }
    return tok;
}

bool Parser::check(TokenKind kind) const { return peek().kind == kind; }

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenKind kind, const char* context) {
    if (check(kind)) {
        return advance();
    }
    diagnostics_.error(peek().loc, std::string("expected ") + tokenKindName(kind) + " " +
                                        context + ", found " + tokenKindName(peek().kind));
    return peek();
}

bool Parser::isAtStatementEnd() const {
    TokenKind k = peek().kind;
    return k == TokenKind::NewLine || k == TokenKind::Colon || k == TokenKind::EndOfFile;
}

void Parser::skipStatementSeparators() {
    while (check(TokenKind::NewLine) || check(TokenKind::Colon)) {
        advance();
    }
}

std::unique_ptr<ast::Module> Parser::parseModule() {
    auto module = std::make_unique<ast::Module>();
    skipStatementSeparators();
    while (!check(TokenKind::EndOfFile)) {
        auto stmt = parseStatement();
        if (stmt) {
            module->statements.push_back(std::move(stmt));
        }
        if (!isAtStatementEnd()) {
            diagnostics_.error(peek().loc, std::string("expected end of statement, found ") +
                                                tokenKindName(peek().kind));
            // Recover by skipping to the next separator so one bad token
            // doesn't cascade into unrelated errors for the rest of the file.
            while (!isAtStatementEnd()) {
                advance();
            }
        }
        skipStatementSeparators();
    }
    return module;
}

std::unique_ptr<ast::Stmt> Parser::parseStatement() {
    if (check(TokenKind::KwDefine)) {
        return parseDefine();
    }
    if (check(TokenKind::KwDebug)) {
        return parseDebug();
    }
    if (check(TokenKind::Identifier)) {
        return parseAssignment();
    }
    diagnostics_.error(peek().loc, std::string("expected statement, found ") +
                                        tokenKindName(peek().kind));
    advance();
    return nullptr;
}

std::unique_ptr<ast::Stmt> Parser::parseDefine() {
    auto stmt = std::make_unique<ast::DefineStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Define'

    do {
        const Token& nameTok = expect(TokenKind::Identifier, "after 'Define'");
        ast::DefineStmt::Declarator decl;
        decl.spelling = nameTok.text;
        decl.name = toLower(nameTok.text);
        decl.suffix = nameTok.suffix;
        if (match(TokenKind::Equal)) {
            decl.init = parseExpr();
        }
        stmt->declarators.push_back(std::move(decl));
    } while (match(TokenKind::Comma));

    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseDebug() {
    auto stmt = std::make_unique<ast::DebugStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Debug'
    stmt->value = parseExpr();
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseAssignment() {
    const Token& nameTok = advance(); // Identifier
    auto stmt = std::make_unique<ast::AssignStmt>();
    stmt->loc = nameTok.loc;
    stmt->spelling = nameTok.text;
    stmt->name = toLower(nameTok.text);
    stmt->suffix = nameTok.suffix;
    expect(TokenKind::Equal, "in assignment");
    stmt->value = parseExpr();
    return stmt;
}

std::unique_ptr<ast::Expr> Parser::parseExpr() {
    auto lhs = parseMul();
    while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
        ast::BinaryOp op = check(TokenKind::Plus) ? ast::BinaryOp::Add : ast::BinaryOp::Sub;
        SourceLoc loc = advance().loc;
        auto rhs = parseMul();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

std::unique_ptr<ast::Expr> Parser::parseMul() {
    auto lhs = parseMod();
    while (check(TokenKind::Star) || check(TokenKind::Slash)) {
        ast::BinaryOp op = check(TokenKind::Star) ? ast::BinaryOp::Mul : ast::BinaryOp::Div;
        SourceLoc loc = advance().loc;
        auto rhs = parseMod();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

std::unique_ptr<ast::Expr> Parser::parseMod() {
    // `%` binds tighter than `*`/`/` (oracle-verified: `2 * 3 % 4` folds to
    // `2 * (3 % 4)` = 6, not `(2*3) % 4` = 2) - hence its own tier between
    // parseMul and parseUnary rather than sharing parseMul's tier.
    auto lhs = parseUnary();
    while (check(TokenKind::Percent)) {
        SourceLoc loc = advance().loc;
        auto rhs = parseUnary();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = ast::BinaryOp::Mod;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

std::unique_ptr<ast::Expr> Parser::parseUnary() {
    if (check(TokenKind::Minus)) {
        SourceLoc loc = advance().loc;
        auto operand = parseUnary();
        auto un = std::make_unique<ast::UnaryExpr>();
        un->loc = loc;
        un->op = ast::UnaryOp::Negate;
        un->operand = std::move(operand);
        return un;
    }
    return parsePrimary();
}

std::unique_ptr<ast::Expr> Parser::parsePrimary() {
    const Token& tok = peek();
    switch (tok.kind) {
        case TokenKind::IntegerLiteral: {
            advance();
            auto lit = std::make_unique<ast::IntLiteralExpr>();
            lit->loc = tok.loc;
            lit->value = tok.intValue;
            return lit;
        }
        case TokenKind::FloatLiteral: {
            advance();
            auto lit = std::make_unique<ast::FloatLiteralExpr>();
            lit->loc = tok.loc;
            lit->value = tok.floatValue;
            return lit;
        }
        case TokenKind::StringLiteral: {
            advance();
            auto lit = std::make_unique<ast::StringLiteralExpr>();
            lit->loc = tok.loc;
            lit->value = tok.text;
            return lit;
        }
        case TokenKind::Identifier: {
            advance();
            auto ref = std::make_unique<ast::VarRefExpr>();
            ref->loc = tok.loc;
            ref->spelling = tok.text;
            ref->name = toLower(tok.text);
            ref->suffix = tok.suffix;
            return ref;
        }
        case TokenKind::LParen: {
            advance();
            auto inner = parseExpr();
            expect(TokenKind::RParen, "to close '('");
            return inner;
        }
        default:
            diagnostics_.error(tok.loc,
                                std::string("expected expression, found ") + tokenKindName(tok.kind));
            advance();
            // Return a harmless placeholder so codegen never sees a null
            // expression pointer after a parse error.
            auto lit = std::make_unique<ast::IntLiteralExpr>();
            lit->loc = tok.loc;
            lit->value = 0;
            return lit;
    }
}

} // namespace easybasic
