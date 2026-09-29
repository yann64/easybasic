#include "parser.hpp"

#include <algorithm>
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

ast::Block Parser::parseBlockUntil(std::initializer_list<TokenKind> terminators) {
    auto isTerminator = [&] {
        return std::ranges::any_of(terminators, [&](TokenKind t) { return check(t); });
    };

    ast::Block block;
    skipStatementSeparators();
    while (!isTerminator() && !check(TokenKind::EndOfFile)) {
        auto stmt = parseStatement();
        if (stmt) {
            block.push_back(std::move(stmt));
        }
        if (!isAtStatementEnd() && !isTerminator()) {
            diagnostics_.error(peek().loc, std::string("expected end of statement, found ") +
                                                tokenKindName(peek().kind));
            while (!isAtStatementEnd() && !isTerminator() && !check(TokenKind::EndOfFile)) {
                advance();
            }
        }
        skipStatementSeparators();
    }
    return block;
}

std::unique_ptr<ast::Stmt> Parser::parseStatement() {
    if (check(TokenKind::KwDefine) || check(TokenKind::KwProtected)) {
        return parseDefine(/*isGlobal=*/false);
    }
    if (check(TokenKind::KwGlobal)) {
        return parseDefine(/*isGlobal=*/true);
    }
    if (check(TokenKind::KwShared)) {
        return parseShared();
    }
    if (check(TokenKind::KwDebug)) {
        return parseDebug();
    }
    if (check(TokenKind::KwIf)) {
        return parseIf();
    }
    if (check(TokenKind::KwSelect)) {
        return parseSelect();
    }
    if (check(TokenKind::KwFor)) {
        return parseFor();
    }
    if (check(TokenKind::KwWhile)) {
        return parseWhile();
    }
    if (check(TokenKind::KwRepeat)) {
        return parseRepeat();
    }
    if (check(TokenKind::KwEnumeration)) {
        return parseEnumeration();
    }
    if (check(TokenKind::KwBreak)) {
        auto stmt = std::make_unique<ast::BreakStmt>();
        stmt->loc = advance().loc;
        return stmt;
    }
    if (check(TokenKind::KwContinue)) {
        auto stmt = std::make_unique<ast::ContinueStmt>();
        stmt->loc = advance().loc;
        return stmt;
    }
    if (check(TokenKind::KwEnableExplicit)) {
        auto stmt = std::make_unique<ast::EnableExplicitStmt>();
        stmt->loc = advance().loc;
        return stmt;
    }
    if (check(TokenKind::KwProcedure)) {
        return parseProcedureDecl();
    }
    if (check(TokenKind::KwProcedureReturn)) {
        return parseProcedureReturn();
    }
    if (check(TokenKind::KwDim)) {
        return parseDim();
    }
    if (check(TokenKind::KwStructure)) {
        return parseStructureDecl();
    }
    if (check(TokenKind::Hash)) {
        return parseConstDecl();
    }
    if (check(TokenKind::Identifier)) {
        return parseIdentifierStatement();
    }
    diagnostics_.error(peek().loc, std::string("expected statement, found ") +
                                        tokenKindName(peek().kind));
    advance();
    return nullptr;
}

std::unique_ptr<ast::Stmt> Parser::parseDefine(bool isGlobal) {
    auto stmt = std::make_unique<ast::DefineStmt>();
    stmt->loc = peek().loc;
    stmt->isGlobal = isGlobal;
    advance(); // 'Define' / 'Global' / 'Protected'

    do {
        const Token& nameTok = expect(TokenKind::Identifier, "in declaration");
        ast::DefineStmt::Declarator decl;
        decl.spelling = nameTok.text;
        decl.name = toLower(nameTok.text);
        decl.suffix = nameTok.suffix;
        decl.structTypeName = nameTok.structSuffix;
        decl.structTypeSpelling = nameTok.structSuffixSpelling;
        if (match(TokenKind::Equal)) {
            decl.init = parseExpr();
        }
        stmt->declarators.push_back(std::move(decl));
    } while (match(TokenKind::Comma));

    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseShared() {
    auto stmt = std::make_unique<ast::SharedStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Shared'
    do {
        const Token& nameTok = expect(TokenKind::Identifier, "after 'Shared'");
        ast::SharedStmt::Name name;
        name.spelling = nameTok.text;
        name.name = toLower(nameTok.text);
        stmt->names.push_back(std::move(name));
    } while (match(TokenKind::Comma));
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseDim() {
    auto stmt = std::make_unique<ast::DimStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Dim'
    const Token& nameTok = expect(TokenKind::Identifier, "after 'Dim'");
    stmt->spelling = nameTok.text;
    stmt->name = toLower(nameTok.text);
    stmt->suffix = nameTok.suffix;
    stmt->structTypeName = nameTok.structSuffix;
    stmt->structTypeSpelling = nameTok.structSuffixSpelling;
    expect(TokenKind::LParen, "after array name");
    stmt->dimensionSizes.push_back(parseExpr());
    while (match(TokenKind::Comma)) {
        stmt->dimensionSizes.push_back(parseExpr());
    }
    expect(TokenKind::RParen, "to close 'Dim'");
    if (stmt->dimensionSizes.size() > 2) {
        diagnostics_.error(stmt->loc, "arrays with more than 2 dimensions are not yet supported");
    }
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseStructureDecl() {
    auto stmt = std::make_unique<ast::StructureDeclStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Structure'
    const Token& nameTok = expect(TokenKind::Identifier, "after 'Structure'");
    stmt->spelling = nameTok.text;
    stmt->name = toLower(nameTok.text);
    skipStatementSeparators();

    while (check(TokenKind::Identifier)) {
        const Token& fieldTok = advance();
        ast::StructureDeclStmt::Field field;
        field.spelling = fieldTok.text;
        field.name = toLower(fieldTok.text);
        field.suffix = fieldTok.suffix;
        field.structTypeName = fieldTok.structSuffix;
        field.structTypeSpelling = fieldTok.structSuffixSpelling;
        stmt->fields.push_back(std::move(field));
        skipStatementSeparators();
    }

    expect(TokenKind::KwEndStructure, "to close 'Structure'");
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseConstDecl() {
    SourceLoc loc = advance().loc; // '#'
    const Token& nameTok = expect(TokenKind::Identifier, "after '#'");
    auto stmt = std::make_unique<ast::ConstDeclStmt>();
    stmt->loc = loc;
    stmt->spelling = nameTok.text;
    stmt->name = toLower(nameTok.text);
    expect(TokenKind::Equal, "in constant declaration");
    stmt->value = parseExpr();
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseIdentifierStatement() {
    SourceLoc loc = peek().loc;
    const Token& nameTok = advance(); // Identifier

    std::unique_ptr<ast::Expr> base;
    if (check(TokenKind::LParen)) {
        // Ambiguous with a call until Sema resolves `nameTok` as an array
        // or a procedure (see ast::IndexAssignStmt's own doc comment) -
        // both share this identical `Name(args)` parse either way.
        advance(); // '('
        auto call = std::make_unique<ast::CallExpr>();
        call->loc = loc;
        call->spelling = nameTok.text;
        call->name = toLower(nameTok.text);
        if (!check(TokenKind::RParen)) {
            do {
                call->args.push_back(parseExpr());
            } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RParen, "to close call arguments");
        base = std::move(call);
    } else {
        auto ref = std::make_unique<ast::VarRefExpr>();
        ref->loc = loc;
        ref->spelling = nameTok.text;
        ref->name = toLower(nameTok.text);
        ref->suffix = nameTok.suffix;
        base = std::move(ref);
    }

    bool hadField = check(TokenKind::Backslash);
    base = parsePostfixFieldAccess(std::move(base));

    if (match(TokenKind::Equal)) {
        if (hadField) {
            auto stmt = std::make_unique<ast::FieldAssignStmt>();
            stmt->loc = loc;
            stmt->target = std::move(base);
            stmt->value = parseExpr();
            return stmt;
        }
        if (base->kind == ast::ExprKind::Call) {
            auto& call = static_cast<ast::CallExpr&>(*base);
            auto stmt = std::make_unique<ast::IndexAssignStmt>();
            stmt->loc = loc;
            stmt->name = call.name;
            stmt->spelling = call.spelling;
            stmt->indices = std::move(call.args);
            stmt->value = parseExpr();
            return stmt;
        }
        auto& ref = static_cast<ast::VarRefExpr&>(*base);
        auto stmt = std::make_unique<ast::AssignStmt>();
        stmt->loc = loc;
        stmt->spelling = ref.spelling;
        stmt->name = ref.name;
        stmt->suffix = ref.suffix;
        stmt->value = parseExpr();
        return stmt;
    }

    if (!hadField && base->kind == ast::ExprKind::Call) {
        // `Name(args)` as a whole statement with no trailing `=` - a call,
        // its return value (if any) discarded.
        auto stmt = std::make_unique<ast::ExprStmt>();
        stmt->loc = loc;
        stmt->expr = std::move(base);
        return stmt;
    }

    diagnostics_.error(peek().loc, "expected '=' to complete this statement, found " +
                                        std::string(tokenKindName(peek().kind)));
    auto stmt = std::make_unique<ast::ExprStmt>();
    stmt->loc = loc;
    stmt->expr = std::move(base);
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseDebug() {
    auto stmt = std::make_unique<ast::DebugStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Debug'
    stmt->value = parseExpr();
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseIf() {
    auto stmt = std::make_unique<ast::IfStmt>();
    stmt->loc = peek().loc;
    advance(); // 'If'

    ast::IfStmt::Branch ifBranch;
    ifBranch.condition = parseCondition();
    ifBranch.body = parseBlockUntil({TokenKind::KwElseIf, TokenKind::KwElse, TokenKind::KwEndIf});
    stmt->branches.push_back(std::move(ifBranch));

    while (check(TokenKind::KwElseIf)) {
        advance();
        ast::IfStmt::Branch branch;
        branch.condition = parseCondition();
        branch.body = parseBlockUntil({TokenKind::KwElseIf, TokenKind::KwElse, TokenKind::KwEndIf});
        stmt->branches.push_back(std::move(branch));
    }

    if (check(TokenKind::KwElse)) {
        advance();
        ast::IfStmt::Branch elseBranch; // condition stays null
        elseBranch.body = parseBlockUntil({TokenKind::KwEndIf});
        stmt->branches.push_back(std::move(elseBranch));
    }

    expect(TokenKind::KwEndIf, "to close 'If'");
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseSelect() {
    auto stmt = std::make_unique<ast::SelectStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Select'
    stmt->selector = parseExpr();
    skipStatementSeparators();

    while (check(TokenKind::KwCase) || check(TokenKind::KwDefault)) {
        ast::SelectStmt::CaseBranch branch;
        if (match(TokenKind::KwCase)) {
            do {
                branch.values.push_back(parseExpr());
            } while (match(TokenKind::Comma));
        } else {
            advance(); // 'Default' - values stays empty
        }
        branch.body = parseBlockUntil({TokenKind::KwCase, TokenKind::KwDefault, TokenKind::KwEndSelect});
        stmt->cases.push_back(std::move(branch));
    }

    expect(TokenKind::KwEndSelect, "to close 'Select'");
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseFor() {
    auto stmt = std::make_unique<ast::ForStmt>();
    stmt->loc = peek().loc;
    advance(); // 'For'

    const Token& varTok = expect(TokenKind::Identifier, "after 'For'");
    stmt->varSpelling = varTok.text;
    stmt->varName = toLower(varTok.text);
    stmt->suffix = varTok.suffix;
    expect(TokenKind::Equal, "in 'For'");
    stmt->from = parseExpr();
    expect(TokenKind::KwTo, "in 'For'");
    stmt->to = parseExpr();
    if (match(TokenKind::KwStep)) {
        stmt->step = parseExpr();
    }
    stmt->body = parseBlockUntil({TokenKind::KwNext});
    expect(TokenKind::KwNext, "to close 'For'");
    match(TokenKind::Identifier); // optional `Next <var>` - not cross-checked against varName in M1
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseWhile() {
    auto stmt = std::make_unique<ast::WhileStmt>();
    stmt->loc = peek().loc;
    advance(); // 'While'
    stmt->condition = parseCondition();
    stmt->body = parseBlockUntil({TokenKind::KwWend});
    expect(TokenKind::KwWend, "to close 'While'");
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseRepeat() {
    auto stmt = std::make_unique<ast::RepeatStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Repeat'
    stmt->body = parseBlockUntil({TokenKind::KwUntil, TokenKind::KwForEver});
    if (match(TokenKind::KwUntil)) {
        stmt->untilCondition = parseCondition();
    } else {
        expect(TokenKind::KwForEver, "to close 'Repeat'");
    }
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseEnumeration() {
    auto stmt = std::make_unique<ast::EnumerationStmt>();
    stmt->loc = peek().loc;
    advance(); // 'Enumeration'
    skipStatementSeparators();

    while (check(TokenKind::Hash)) {
        advance(); // '#'
        const Token& nameTok = expect(TokenKind::Identifier, "after '#' in 'Enumeration'");
        ast::EnumerationStmt::Member member;
        member.spelling = nameTok.text;
        member.name = toLower(nameTok.text);
        if (match(TokenKind::Equal)) {
            member.explicitValue = parseExpr();
        }
        stmt->members.push_back(std::move(member));
        skipStatementSeparators();
    }

    expect(TokenKind::KwEndEnumeration, "to close 'Enumeration'");
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseProcedureDecl() {
    auto stmt = std::make_unique<ast::ProcedureDeclStmt>();
    stmt->loc = peek().loc;
    const Token& procTok = advance(); // 'Procedure[.suffix|$]'
    stmt->returnSuffix = procTok.suffix;

    const Token& nameTok = expect(TokenKind::Identifier, "after 'Procedure'");
    stmt->spelling = nameTok.text;
    stmt->name = toLower(nameTok.text);

    expect(TokenKind::LParen, "after procedure name");
    if (!check(TokenKind::RParen)) {
        do {
            const Token& paramTok = expect(TokenKind::Identifier, "in parameter list");
            ast::ProcedureDeclStmt::Param param;
            param.spelling = paramTok.text;
            param.name = toLower(paramTok.text);
            param.suffix = paramTok.suffix;
            if (match(TokenKind::Equal)) {
                param.defaultValue = parseExpr();
            }
            stmt->params.push_back(std::move(param));
        } while (match(TokenKind::Comma));
    }
    expect(TokenKind::RParen, "to close parameter list");

    stmt->body = parseBlockUntil({TokenKind::KwEndProcedure});
    expect(TokenKind::KwEndProcedure, "to close 'Procedure'");
    return stmt;
}

std::unique_ptr<ast::Stmt> Parser::parseProcedureReturn() {
    auto stmt = std::make_unique<ast::ProcedureReturnStmt>();
    stmt->loc = peek().loc;
    advance(); // 'ProcedureReturn'
    if (!isAtStatementEnd()) {
        stmt->value = parseExpr();
    }
    return stmt;
}

// --- Condition grammar (If/While/Until only - see parser.hpp's own notes) ---

std::unique_ptr<ast::Expr> Parser::parseCondition() {
    auto lhs = parseLogicalNot();
    for (;;) {
        ast::BinaryOp op = ast::BinaryOp::Add;
        if (check(TokenKind::KwAnd)) {
            op = ast::BinaryOp::LogicalAnd;
        } else if (check(TokenKind::KwOr)) {
            op = ast::BinaryOp::LogicalOr;
        } else if (check(TokenKind::KwXOr)) {
            op = ast::BinaryOp::LogicalXOr;
        } else {
            break;
        }
        SourceLoc loc = advance().loc;
        auto rhs = parseLogicalNot();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

std::unique_ptr<ast::Expr> Parser::parseLogicalNot() {
    if (check(TokenKind::KwNot)) {
        SourceLoc loc = advance().loc;
        auto operand = parseLogicalNot();
        auto un = std::make_unique<ast::UnaryExpr>();
        un->loc = loc;
        un->op = ast::UnaryOp::LogicalNot;
        un->operand = std::move(operand);
        return un;
    }
    return parseComparison();
}

std::unique_ptr<ast::Expr> Parser::parseComparison() {
    auto lhs = parseExpr();
    ast::BinaryOp op = ast::BinaryOp::Add;
    switch (peek().kind) {
        case TokenKind::Equal: op = ast::BinaryOp::Eq; break;
        case TokenKind::NotEqual: op = ast::BinaryOp::Ne; break;
        case TokenKind::Less: op = ast::BinaryOp::Lt; break;
        case TokenKind::Greater: op = ast::BinaryOp::Gt; break;
        case TokenKind::LessEqual: op = ast::BinaryOp::Le; break;
        case TokenKind::GreaterEqual: op = ast::BinaryOp::Ge; break;
        default:
            return lhs; // A bare arithmetic expression is also a valid condition (PB truthiness).
    }
    SourceLoc loc = advance().loc;
    auto rhs = parseExpr();
    auto bin = std::make_unique<ast::BinaryExpr>();
    bin->loc = loc;
    bin->op = op;
    bin->lhs = std::move(lhs);
    bin->rhs = std::move(rhs);
    return bin;
}

// --- General arithmetic grammar (no comparisons/logicals - see parser.hpp) ---

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
    auto lhs = parseBitwiseOrAnd();
    while (check(TokenKind::Star) || check(TokenKind::Slash)) {
        ast::BinaryOp op = check(TokenKind::Star) ? ast::BinaryOp::Mul : ast::BinaryOp::Div;
        SourceLoc loc = advance().loc;
        auto rhs = parseBitwiseOrAnd();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

std::unique_ptr<ast::Expr> Parser::parseBitwiseOrAnd() {
    // `&`/`|` are their own flat precedence tier, left-to-right -
    // oracle-verified (docs/architecture/roadmap.md's M1 notes: `1 & 6 | 3`
    // and `4 | 1 & 3` both match whichever operator is textually first,
    // proving no internal ordering between just these two) - looser than
    // parseBitwiseTight's tier (`%`/`!`/`<<`/`>>`), which is a SEPARATE flat
    // tier, not the same one (an earlier, buggy version of this parser
    // treated all six operators as one combined flat tier, which silently
    // mis-parsed `12 & 1 << 2` as `0` instead of the correct `4` - caught by
    // the differential e2e suite against the real pbcompilerc).
    auto lhs = parseBitwiseTight();
    while (check(TokenKind::Ampersand) || check(TokenKind::Pipe)) {
        ast::BinaryOp op = check(TokenKind::Ampersand) ? ast::BinaryOp::BitAnd : ast::BinaryOp::BitOr;
        SourceLoc loc = advance().loc;
        auto rhs = parseBitwiseTight();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
    return lhs;
}

std::unique_ptr<ast::Expr> Parser::parseBitwiseTight() {
    // `%`/`!`(xor)/`<<`/`>>` are ONE FLAT precedence tier, left-to-right,
    // tighter than `&`/`|`'s tier - oracle-verified via two-directional
    // reversal tests for every pair among these four (see
    // docs/architecture/roadmap.md's M1 notes).
    auto lhs = parseUnary();
    for (;;) {
        ast::BinaryOp op = ast::BinaryOp::Add;
        switch (peek().kind) {
            case TokenKind::Percent: op = ast::BinaryOp::Mod; break;
            case TokenKind::Bang: op = ast::BinaryOp::BitXor; break;
            case TokenKind::ShiftLeft: op = ast::BinaryOp::ShiftLeft; break;
            case TokenKind::ShiftRight: op = ast::BinaryOp::ShiftRight; break;
            default:
                return lhs;
        }
        SourceLoc loc = advance().loc;
        auto rhs = parseUnary();
        auto bin = std::make_unique<ast::BinaryExpr>();
        bin->loc = loc;
        bin->op = op;
        bin->lhs = std::move(lhs);
        bin->rhs = std::move(rhs);
        lhs = std::move(bin);
    }
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
    if (check(TokenKind::Tilde)) {
        SourceLoc loc = advance().loc;
        auto operand = parseUnary();
        auto un = std::make_unique<ast::UnaryExpr>();
        un->loc = loc;
        un->op = ast::UnaryOp::BitNot;
        un->operand = std::move(operand);
        return un;
    }
    return parsePrimary();
}

std::unique_ptr<ast::Expr> Parser::parsePrimary() {
    return parsePostfixFieldAccess(parsePrimaryAtom());
}

std::unique_ptr<ast::Expr> Parser::parsePostfixFieldAccess(std::unique_ptr<ast::Expr> base) {
    while (check(TokenKind::Backslash)) {
        SourceLoc loc = advance().loc;
        const Token& fieldTok = expect(TokenKind::Identifier, "after '\\'");
        auto access = std::make_unique<ast::FieldAccessExpr>();
        access->loc = loc;
        access->base = std::move(base);
        access->field = toLower(fieldTok.text);
        access->fieldSpelling = fieldTok.text;
        base = std::move(access);
    }
    return base;
}

std::unique_ptr<ast::Expr> Parser::parsePrimaryAtom() {
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
            if (check(TokenKind::LParen)) {
                // `Name(args)` as a value - a call expression, not a
                // variable reference (PB requires parens for a call, so
                // this single token of lookahead is unambiguous).
                advance(); // '('
                auto call = std::make_unique<ast::CallExpr>();
                call->loc = tok.loc;
                call->spelling = tok.text;
                call->name = toLower(tok.text);
                if (!check(TokenKind::RParen)) {
                    do {
                        call->args.push_back(parseExpr());
                    } while (match(TokenKind::Comma));
                }
                expect(TokenKind::RParen, "to close call arguments");
                return call;
            }
            auto ref = std::make_unique<ast::VarRefExpr>();
            ref->loc = tok.loc;
            ref->spelling = tok.text;
            ref->name = toLower(tok.text);
            ref->suffix = tok.suffix;
            return ref;
        }
        case TokenKind::Hash: {
            advance();
            const Token& nameTok = expect(TokenKind::Identifier, "after '#'");
            auto ref = std::make_unique<ast::ConstRefExpr>();
            ref->loc = tok.loc;
            ref->spelling = nameTok.text;
            ref->name = toLower(nameTok.text);
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
