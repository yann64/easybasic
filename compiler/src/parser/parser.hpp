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
    std::unique_ptr<ast::Stmt> parseStructureDecl();
    std::unique_ptr<ast::Stmt> parseNewList();
    std::unique_ptr<ast::Stmt> parseNewMap();
    /// `ForEach name() ... Next` - `name` is always a bare, already-declared
    /// List (see ast::ForEachStmt's own doc comment), so this parses a plain
    /// identifier and requires the `()`, rather than a general expression.
    std::unique_ptr<ast::Stmt> parseForEach();
    /// Called at statement position on a bare `Identifier` - parses a
    /// single unified "base" (a plain variable, or `Name(args)` which is
    /// ambiguous between a call and an array index until Sema resolves it -
    /// see ast::IndexAssignStmt's own doc comment), then any number of
    /// `\field` segments (building an ast::FieldAccessExpr chain), and only
    /// *then* decides what statement this actually is based on whether `=`
    /// follows: a field assignment, an array-element assignment, a plain
    /// assignment, or - if nothing follows and there was no field chain - a
    /// call used as a whole statement. All of these share an identical
    /// `Name(args)...` prefix, so none of them can be told apart any
    /// earlier than this.
    std::unique_ptr<ast::Stmt> parseIdentifierStatement();
    std::unique_ptr<ast::Stmt> parseConstDecl();
    std::unique_ptr<ast::Stmt> parseDebug();
    std::unique_ptr<ast::Stmt> parseIf();
    std::unique_ptr<ast::Stmt> parseSelect();
    /// `CompilerIf`/`CompilerSelect` - structurally identical to `parseIf`/
    /// `parseSelect`, including reusing `parseCondition()` for the exact
    /// same comparison/`And`/`Or`/`Not` grammar (oracle-verified: a
    /// `CompilerIf` condition is a boolean context exactly like a runtime
    /// `If`'s) - see ast::CompilerIfStmt's own doc comment for why `Sema`,
    /// not `Codegen`, is what actually resolves these.
    std::unique_ptr<ast::Stmt> parseCompilerIf();
    std::unique_ptr<ast::Stmt> parseCompilerSelect();
    /// `DataSection ... EndDataSection` - has its own dedicated body-parsing
    /// loop (not `parseBlockUntil`/`parseStatement`) since a DataSection's
    /// body is syntactically restricted to just labels and `Data` lines;
    /// see ast::DataSectionStmt's own doc comment.
    std::unique_ptr<ast::Stmt> parseDataSection();
    std::unique_ptr<ast::Stmt> parseDataStmt();
    std::unique_ptr<ast::Stmt> parseRead();
    std::unique_ptr<ast::Stmt> parseRestore();
    std::unique_ptr<ast::Stmt> parseFor();
    std::unique_ptr<ast::Stmt> parseWhile();
    std::unique_ptr<ast::Stmt> parseRepeat();
    std::unique_ptr<ast::Stmt> parseEnumeration();
    std::unique_ptr<ast::Stmt> parseProcedureDecl();
    std::unique_ptr<ast::Stmt> parseProcedureReturn();
    std::unique_ptr<ast::Stmt> parseDeclare();
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
    /// The literal/identifier/parenthesized-expression cases, before any
    /// `\field` postfix chain is applied - parsePrimary() itself is a thin
    /// wrapper (`parsePostfixFieldAccess(parsePrimaryAtom())`).
    std::unique_ptr<ast::Expr> parsePrimaryAtom();
    /// Wraps `base` in as many `ast::FieldAccessExpr` layers as there are
    /// consecutive `\field` segments following it (zero is fine - most
    /// expressions have none). Shared by both expression-position reads
    /// (`Debug p\x`, via parsePrimary) and Parser::parseIdentifierStatement,
    /// which needs the identical chain-building logic before deciding what
    /// kind of statement it's looking at.
    std::unique_ptr<ast::Expr> parsePostfixFieldAccess(std::unique_ptr<ast::Expr> base);

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
