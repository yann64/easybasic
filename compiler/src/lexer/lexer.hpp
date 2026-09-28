#pragma once

#include <string>
#include <vector>

#include "../diagnostics/diagnostics.hpp"
#include "token.hpp"

namespace easybasic {

/// Turns PureBasic source text into a flat token stream.
///
/// PB is case-insensitive for both identifiers and keywords (verified: `x`,
/// `X`, `MyVar`/`myvar`/`MYVAR` all name the same symbol) - the Lexer
/// preserves the original casing in Token::text for diagnostics, but
/// keyword lookup and identifier equality are both done case-insensitively.
class Lexer {
public:
    Lexer(std::string source, int fileId, DiagnosticEngine& diagnostics);

    /// Lexes the whole source and returns every token, terminated by a
    /// single trailing TokenKind::EndOfFile.
    std::vector<Token> tokenize();

private:
    Token next();
    Token lexIdentifierOrKeyword();
    Token lexNumber();
    Token lexString();

    [[nodiscard]] char peek(int offset = 0) const;
    char advance();
    [[nodiscard]] bool isAtEnd() const;
    void skipInlineWhitespaceAndComments();

    [[nodiscard]] SourceLoc here() const { return SourceLoc{line_, column_, fileId_}; }

    std::string source_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    int fileId_;
    DiagnosticEngine& diagnostics_;
};

} // namespace easybasic
