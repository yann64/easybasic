#pragma once

#include <string>

#include "../diagnostics/diagnostics.hpp"

namespace easybasic {

/// Every distinct token category the Lexer can produce. Deliberately small in
/// M0 - it grows alongside the language subset each milestone adds (see
/// docs/architecture/roadmap.md).
enum class TokenKind {
    EndOfFile,
    NewLine,      ///< A real line break. PureBasic has no line-continuation
                  ///< syntax at all (verified against pbcompilerc - neither a
                  ///< trailing `_` nor `...` compiles), so every NewLine is a
                  ///< genuine statement boundary, exactly like `:`.
    Colon,        ///< `:` - the other statement separator.
    Identifier,   ///< A name, optionally carrying a type-suffix (see below).
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    KwDefine,
    KwDebug,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equal,
    LParen,
    RParen,
    Comma,
    Unknown,
};

/// PureBasic's 11 confirmed type suffixes (oracle-verified against
/// pbcompilerc): b/a/c/w/u/l/q/f/d/s/i. `None` means the identifier carried
/// no suffix at all (its type is inferred/defaulted by Sema).
enum class TypeSuffix {
    None,
    Byte,      ///< .b
    Ascii,     ///< .a
    Character, ///< .c (storage-identical to Unicode/.u, per the oracle)
    Word,      ///< .w
    Unicode,   ///< .u
    Long,      ///< .l
    Quad,      ///< .q
    Float,     ///< .f
    Double,    ///< .d
    String,    ///< .s
    Integer,   ///< .i
};

struct Token {
    TokenKind kind = TokenKind::Unknown;
    std::string text;               ///< Raw lexeme (identifier name without
                                     ///< its suffix, unescaped string
                                     ///< contents, or the literal digits).
    TypeSuffix suffix = TypeSuffix::None; ///< Only meaningful for Identifier.
    long long intValue = 0;         ///< Only meaningful for IntegerLiteral.
    double floatValue = 0.0;        ///< Only meaningful for FloatLiteral.
    SourceLoc loc;
};

/// Human-readable name for diagnostics, e.g. "identifier" or "'+' ".
const char* tokenKindName(TokenKind kind);

} // namespace easybasic
