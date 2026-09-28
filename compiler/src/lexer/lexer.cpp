#include "lexer.hpp"

#include <cctype>
#include <unordered_map>

namespace easybasic {

namespace {

/// PB is case-insensitive; canonicalize to lowercase for keyword/identifier
/// comparison while Token::text keeps the source's original casing.
std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

TypeSuffix suffixFromLetter(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'b': return TypeSuffix::Byte;
        case 'a': return TypeSuffix::Ascii;
        case 'c': return TypeSuffix::Character;
        case 'w': return TypeSuffix::Word;
        case 'u': return TypeSuffix::Unicode;
        case 'l': return TypeSuffix::Long;
        case 'q': return TypeSuffix::Quad;
        case 'f': return TypeSuffix::Float;
        case 'd': return TypeSuffix::Double;
        case 's': return TypeSuffix::String;
        case 'i': return TypeSuffix::Integer;
        default: return TypeSuffix::None;
    }
}

bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool isIdentContinue(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

const std::unordered_map<std::string, TokenKind>& keywordTable() {
    static const std::unordered_map<std::string, TokenKind> table = {
        {"define", TokenKind::KwDefine},
        {"debug", TokenKind::KwDebug},
    };
    return table;
}

} // namespace

const char* tokenKindName(TokenKind kind) {
    switch (kind) {
        case TokenKind::EndOfFile: return "end of file";
        case TokenKind::NewLine: return "end of line";
        case TokenKind::Colon: return "':'";
        case TokenKind::Identifier: return "identifier";
        case TokenKind::IntegerLiteral: return "integer literal";
        case TokenKind::FloatLiteral: return "float literal";
        case TokenKind::StringLiteral: return "string literal";
        case TokenKind::KwDefine: return "'Define'";
        case TokenKind::KwDebug: return "'Debug'";
        case TokenKind::Plus: return "'+'";
        case TokenKind::Minus: return "'-'";
        case TokenKind::Star: return "'*'";
        case TokenKind::Slash: return "'/'";
        case TokenKind::Percent: return "'%'";
        case TokenKind::Equal: return "'='";
        case TokenKind::LParen: return "'('";
        case TokenKind::RParen: return "')'";
        case TokenKind::Comma: return "','";
        case TokenKind::Unknown: return "unknown token";
    }
    return "?";
}

Lexer::Lexer(std::string source, int fileId, DiagnosticEngine& diagnostics)
    : source_(std::move(source)), fileId_(fileId), diagnostics_(diagnostics) {}

bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

char Lexer::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= source_.size()) {
        return '\0';
    }
    return source_[idx];
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

void Lexer::skipInlineWhitespaceAndComments() {
    for (;;) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == ';') {
            // A comment runs to end of physical line; PB has no block
            // comments at all.
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (;;) {
        Token tok = next();
        bool isEof = tok.kind == TokenKind::EndOfFile;
        tokens.push_back(std::move(tok));
        if (isEof) {
            break;
        }
    }
    return tokens;
}

Token Lexer::next() {
    skipInlineWhitespaceAndComments();

    if (isAtEnd()) {
        Token tok;
        tok.kind = TokenKind::EndOfFile;
        tok.loc = here();
        return tok;
    }

    SourceLoc loc = here();
    char c = peek();

    if (c == '\n') {
        advance();
        Token tok;
        tok.kind = TokenKind::NewLine;
        tok.loc = loc;
        return tok;
    }
    if (c == ':') {
        advance();
        Token tok;
        tok.kind = TokenKind::Colon;
        tok.loc = loc;
        return tok;
    }
    if (c == '"') {
        return lexString();
    }
    if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '$') {
        return lexNumber();
    }
    // `%` is a binary-literal prefix only when immediately followed by a
    // binary digit (verified against pbcompilerc: `%1010` = 10, but
    // `%101 % 2` still parses the second `%` as modulo since it's followed
    // by a space, not a digit) - otherwise it's the modulo operator.
    if (c == '%' && (peek(1) == '0' || peek(1) == '1')) {
        return lexNumber();
    }
    if (isIdentStart(c)) {
        return lexIdentifierOrKeyword();
    }

    advance();
    Token tok;
    tok.loc = loc;
    switch (c) {
        case '+': tok.kind = TokenKind::Plus; break;
        case '-': tok.kind = TokenKind::Minus; break;
        case '*': tok.kind = TokenKind::Star; break;
        case '/': tok.kind = TokenKind::Slash; break;
        case '%': tok.kind = TokenKind::Percent; break;
        case '=': tok.kind = TokenKind::Equal; break;
        case '(': tok.kind = TokenKind::LParen; break;
        case ')': tok.kind = TokenKind::RParen; break;
        case ',': tok.kind = TokenKind::Comma; break;
        default:
            tok.kind = TokenKind::Unknown;
            tok.text = std::string(1, c);
            diagnostics_.error(loc, std::string("unexpected character '") + c + "'");
            break;
    }
    return tok;
}

Token Lexer::lexIdentifierOrKeyword() {
    SourceLoc loc = here();
    std::string text;
    while (!isAtEnd() && isIdentContinue(peek())) {
        text += advance();
    }

    TypeSuffix suffix = TypeSuffix::None;
    // A `.` immediately after an identifier is always a type-suffix marker
    // in PB, never member access (PB uses `\` for that instead) - so this
    // is unambiguous, unlike e.g. C-family languages.
    if (peek() == '.') {
        char suffixLetter = peek(1);
        TypeSuffix candidate = suffixFromLetter(suffixLetter);
        char afterSuffix = peek(2);
        if (candidate != TypeSuffix::None && !isIdentContinue(afterSuffix)) {
            advance(); // '.'
            advance(); // suffix letter
            suffix = candidate;
        }
    }

    std::string lower = toLower(text);
    auto it = keywordTable().find(lower);
    Token tok;
    tok.loc = loc;
    tok.text = text;
    tok.suffix = suffix;
    if (it != keywordTable().end() && suffix == TypeSuffix::None) {
        tok.kind = it->second;
    } else {
        tok.kind = TokenKind::Identifier;
    }
    return tok;
}

Token Lexer::lexNumber() {
    SourceLoc loc = here();
    std::string digits;
    int base = 10;

    if (peek() == '$') {
        advance();
        base = 16;
        while (!isAtEnd() && std::isxdigit(static_cast<unsigned char>(peek())) != 0) {
            digits += advance();
        }
    } else if (peek() == '%') {
        advance();
        base = 2;
        while (!isAtEnd() && (peek() == '0' || peek() == '1')) {
            digits += advance();
        }
    } else {
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
            digits += advance();
        }
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1))) != 0) {
            std::string floatText = digits + advance(); // consume '.'
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                floatText += advance();
            }
            Token tok;
            tok.kind = TokenKind::FloatLiteral;
            tok.loc = loc;
            tok.text = floatText;
            tok.floatValue = std::stod(floatText);
            return tok;
        }
    }

    Token tok;
    tok.kind = TokenKind::IntegerLiteral;
    tok.loc = loc;
    tok.text = digits;
    if (digits.empty()) {
        diagnostics_.error(loc, "malformed numeric literal");
        tok.intValue = 0;
    } else {
        tok.intValue = std::stoll(digits, nullptr, base);
    }
    return tok;
}

Token Lexer::lexString() {
    SourceLoc loc = here();
    advance(); // opening '"'
    std::string value;
    // Plain `"..."` strings only for now - no escape processing. PB's
    // escaped-string form (`~"...\n..."`) is a separate, deferred feature.
    while (!isAtEnd() && peek() != '"' && peek() != '\n') {
        value += advance();
    }
    if (peek() != '"') {
        diagnostics_.error(loc, "unterminated string literal");
    } else {
        advance(); // closing '"'
    }
    Token tok;
    tok.kind = TokenKind::StringLiteral;
    tok.loc = loc;
    tok.text = value;
    return tok;
}

} // namespace easybasic
