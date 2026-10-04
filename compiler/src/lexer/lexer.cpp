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
        {"if", TokenKind::KwIf},
        {"elseif", TokenKind::KwElseIf},
        {"else", TokenKind::KwElse},
        {"endif", TokenKind::KwEndIf},
        {"select", TokenKind::KwSelect},
        {"case", TokenKind::KwCase},
        {"default", TokenKind::KwDefault},
        {"endselect", TokenKind::KwEndSelect},
        {"for", TokenKind::KwFor},
        {"to", TokenKind::KwTo},
        {"step", TokenKind::KwStep},
        {"next", TokenKind::KwNext},
        {"while", TokenKind::KwWhile},
        {"wend", TokenKind::KwWend},
        {"repeat", TokenKind::KwRepeat},
        {"until", TokenKind::KwUntil},
        {"forever", TokenKind::KwForEver},
        {"break", TokenKind::KwBreak},
        {"continue", TokenKind::KwContinue},
        {"enableexplicit", TokenKind::KwEnableExplicit},
        {"enumeration", TokenKind::KwEnumeration},
        {"endenumeration", TokenKind::KwEndEnumeration},
        {"procedure", TokenKind::KwProcedure},
        {"procedurereturn", TokenKind::KwProcedureReturn},
        {"endprocedure", TokenKind::KwEndProcedure},
        {"declare", TokenKind::KwDeclare},
        {"global", TokenKind::KwGlobal},
        {"shared", TokenKind::KwShared},
        {"protected", TokenKind::KwProtected},
        {"dim", TokenKind::KwDim},
        {"structure", TokenKind::KwStructure},
        {"endstructure", TokenKind::KwEndStructure},
        {"newlist", TokenKind::KwNewList},
        {"newmap", TokenKind::KwNewMap},
        {"foreach", TokenKind::KwForEach},
        {"compilerif", TokenKind::KwCompilerIf},
        {"compilerelseif", TokenKind::KwCompilerElseIf},
        {"compilerelse", TokenKind::KwCompilerElse},
        {"compilerendif", TokenKind::KwCompilerEndIf},
        {"compilerselect", TokenKind::KwCompilerSelect},
        {"compilercase", TokenKind::KwCompilerCase},
        {"compilerdefault", TokenKind::KwCompilerDefault},
        {"compilerendselect", TokenKind::KwCompilerEndSelect},
        {"datasection", TokenKind::KwDataSection},
        {"enddatasection", TokenKind::KwEndDataSection},
        {"data", TokenKind::KwData},
        {"read", TokenKind::KwRead},
        {"restore", TokenKind::KwRestore},
        {"macro", TokenKind::KwMacro},
        {"endmacro", TokenKind::KwEndMacro},
        {"interface", TokenKind::KwInterface},
        {"endinterface", TokenKind::KwEndInterface},
        {"and", TokenKind::KwAnd},
        {"or", TokenKind::KwOr},
        {"not", TokenKind::KwNot},
        {"xor", TokenKind::KwXOr},
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
        case TokenKind::KwIf: return "'If'";
        case TokenKind::KwElseIf: return "'ElseIf'";
        case TokenKind::KwElse: return "'Else'";
        case TokenKind::KwEndIf: return "'EndIf'";
        case TokenKind::KwSelect: return "'Select'";
        case TokenKind::KwCase: return "'Case'";
        case TokenKind::KwDefault: return "'Default'";
        case TokenKind::KwEndSelect: return "'EndSelect'";
        case TokenKind::KwFor: return "'For'";
        case TokenKind::KwTo: return "'To'";
        case TokenKind::KwStep: return "'Step'";
        case TokenKind::KwNext: return "'Next'";
        case TokenKind::KwWhile: return "'While'";
        case TokenKind::KwWend: return "'Wend'";
        case TokenKind::KwRepeat: return "'Repeat'";
        case TokenKind::KwUntil: return "'Until'";
        case TokenKind::KwForEver: return "'ForEver'";
        case TokenKind::KwBreak: return "'Break'";
        case TokenKind::KwContinue: return "'Continue'";
        case TokenKind::KwEnableExplicit: return "'EnableExplicit'";
        case TokenKind::KwEnumeration: return "'Enumeration'";
        case TokenKind::KwEndEnumeration: return "'EndEnumeration'";
        case TokenKind::KwProcedure: return "'Procedure'";
        case TokenKind::KwProcedureReturn: return "'ProcedureReturn'";
        case TokenKind::KwEndProcedure: return "'EndProcedure'";
        case TokenKind::KwDeclare: return "'Declare'";
        case TokenKind::KwGlobal: return "'Global'";
        case TokenKind::KwShared: return "'Shared'";
        case TokenKind::KwProtected: return "'Protected'";
        case TokenKind::KwDim: return "'Dim'";
        case TokenKind::KwStructure: return "'Structure'";
        case TokenKind::KwEndStructure: return "'EndStructure'";
        case TokenKind::KwNewList: return "'NewList'";
        case TokenKind::KwNewMap: return "'NewMap'";
        case TokenKind::KwForEach: return "'ForEach'";
        case TokenKind::KwCompilerIf: return "'CompilerIf'";
        case TokenKind::KwCompilerElseIf: return "'CompilerElseIf'";
        case TokenKind::KwCompilerElse: return "'CompilerElse'";
        case TokenKind::KwCompilerEndIf: return "'CompilerEndIf'";
        case TokenKind::KwCompilerSelect: return "'CompilerSelect'";
        case TokenKind::KwCompilerCase: return "'CompilerCase'";
        case TokenKind::KwCompilerDefault: return "'CompilerDefault'";
        case TokenKind::KwCompilerEndSelect: return "'CompilerEndSelect'";
        case TokenKind::KwDataSection: return "'DataSection'";
        case TokenKind::KwEndDataSection: return "'EndDataSection'";
        case TokenKind::KwData: return "'Data'";
        case TokenKind::KwRead: return "'Read'";
        case TokenKind::KwRestore: return "'Restore'";
        case TokenKind::KwMacro: return "'Macro'";
        case TokenKind::KwEndMacro: return "'EndMacro'";
        case TokenKind::KwInterface: return "'Interface'";
        case TokenKind::KwEndInterface: return "'EndInterface'";
        case TokenKind::KwAnd: return "'And'";
        case TokenKind::KwOr: return "'Or'";
        case TokenKind::KwNot: return "'Not'";
        case TokenKind::KwXOr: return "'XOr'";
        case TokenKind::Plus: return "'+'";
        case TokenKind::Minus: return "'-'";
        case TokenKind::Star: return "'*'";
        case TokenKind::Slash: return "'/'";
        case TokenKind::Percent: return "'%'";
        case TokenKind::Equal: return "'='";
        case TokenKind::NotEqual: return "'<>'";
        case TokenKind::Less: return "'<'";
        case TokenKind::Greater: return "'>'";
        case TokenKind::LessEqual: return "'<='";
        case TokenKind::GreaterEqual: return "'>='";
        case TokenKind::Ampersand: return "'&'";
        case TokenKind::Pipe: return "'|'";
        case TokenKind::Bang: return "'!'";
        case TokenKind::Tilde: return "'~'";
        case TokenKind::ShiftLeft: return "'<<'";
        case TokenKind::ShiftRight: return "'>>'";
        case TokenKind::Hash: return "'#'";
        case TokenKind::Backslash: return "'\\'";
        case TokenKind::At: return "'@'";
        case TokenKind::Question: return "'?'";
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
    // Two-character operators needing lookahead, checked before the
    // generic single-character dispatch below.
    if (c == '<' && peek(1) == '>') {
        advance();
        advance();
        Token tok;
        tok.kind = TokenKind::NotEqual;
        tok.loc = loc;
        return tok;
    }
    if (c == '<' && peek(1) == '=') {
        advance();
        advance();
        Token tok;
        tok.kind = TokenKind::LessEqual;
        tok.loc = loc;
        return tok;
    }
    if (c == '<' && peek(1) == '<') {
        advance();
        advance();
        Token tok;
        tok.kind = TokenKind::ShiftLeft;
        tok.loc = loc;
        return tok;
    }
    if (c == '>' && peek(1) == '=') {
        advance();
        advance();
        Token tok;
        tok.kind = TokenKind::GreaterEqual;
        tok.loc = loc;
        return tok;
    }
    if (c == '>' && peek(1) == '>') {
        advance();
        advance();
        Token tok;
        tok.kind = TokenKind::ShiftRight;
        tok.loc = loc;
        return tok;
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
        case '<': tok.kind = TokenKind::Less; break;
        case '>': tok.kind = TokenKind::Greater; break;
        case '&': tok.kind = TokenKind::Ampersand; break;
        case '|': tok.kind = TokenKind::Pipe; break;
        case '!': tok.kind = TokenKind::Bang; break;
        case '~': tok.kind = TokenKind::Tilde; break;
        case '#': tok.kind = TokenKind::Hash; break;
        case '\\': tok.kind = TokenKind::Backslash; break;
        case '@': tok.kind = TokenKind::At; break;
        case '?': tok.kind = TokenKind::Question; break;
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
    std::string structSuffix;
    std::string structSuffixSpelling;
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
        } else if (isIdentStart(peek(1))) {
            // Not one of the 11 primitive letters (or followed by more
            // identifier characters, so it's a longer name regardless) -
            // this is a named Structure type annotation instead
            // (oracle-verified: `Define p.Point`).
            advance(); // '.'
            std::string typeName;
            while (!isAtEnd() && isIdentContinue(peek())) {
                typeName += advance();
            }
            suffix = TypeSuffix::Struct;
            structSuffixSpelling = typeName;
            structSuffix = toLower(typeName);
        }
    } else if (peek() == '$') {
        // `$` is a general alternative String-suffix sigil, usable on any
        // identifier (oracle-verified: `name$ = "hi"` lowers to the same
        // string-typed variable as `name.s`), not just on `Procedure`.
        advance();
        suffix = TypeSuffix::String;
    }

    std::string lower = toLower(text);
    auto it = keywordTable().find(lower);
    Token tok;
    tok.loc = loc;
    tok.text = text;
    tok.suffix = suffix;
    tok.structSuffix = structSuffix;
    tok.structSuffixSpelling = structSuffixSpelling;
    if (it != keywordTable().end()) {
        // Unlike every other keyword, `Procedure` legitimately carries a
        // type suffix (`Procedure.i`/`Procedure$ Name(...)`) - the suffix
        // still rides along on the token either way (harmless for keywords
        // that don't expect one; the parser simply never reads it there).
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
