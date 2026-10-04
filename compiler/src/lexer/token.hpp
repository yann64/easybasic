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
    KwIf,
    KwElseIf,
    KwElse,
    KwEndIf,
    KwSelect,
    KwCase,
    KwDefault,
    KwEndSelect,
    KwFor,
    KwTo,
    KwStep,
    KwNext,
    KwWhile,
    KwWend,
    KwRepeat,
    KwUntil,
    KwForEver,
    KwBreak,
    KwContinue,
    KwEnableExplicit,
    KwEnumeration,
    KwEndEnumeration,
    KwProcedure,
    KwProcedureReturn,
    KwEndProcedure,
    KwDeclare,   ///< `Declare[.suffix] Name(params)` - forward declaration (M2-closure).
    KwGlobal,
    KwShared,
    KwProtected,
    KwDim,
    KwStructure,
    KwEndStructure,
    KwNewList,   ///< `NewList name.type()` - declares a List (M3e).
    KwNewMap,    ///< `NewMap name.type()` - declares a Map (M3f).
    KwForEach,   ///< `ForEach name() ... Next` - iterates a List/Map (M3e/M3f).
    KwCompilerIf,       ///< `CompilerIf cond ... CompilerEndIf` - compile-time branch (M5a).
    KwCompilerElseIf,
    KwCompilerElse,
    KwCompilerEndIf,
    KwCompilerSelect,   ///< `CompilerSelect sel ... CompilerEndSelect` - compile-time branch (M5a).
    KwCompilerCase,
    KwCompilerDefault,
    KwCompilerEndSelect,
    KwDataSection,      ///< `DataSection ... EndDataSection` (M5b).
    KwEndDataSection,
    KwData,             ///< `Data.suffix v1[, v2, ...]` inside a DataSection (M5b).
    KwRead,             ///< `Read[.suffix] varname` (M5b).
    KwRestore,          ///< `Restore label` (M5b).
    KwMacro,            ///< `Macro name[(params)] ... EndMacro` (M5c).
    KwEndMacro,
    KwInterface,        ///< `Interface Name ... EndInterface` (M7c).
    KwEndInterface,
    KwDeclareModule,    ///< `DeclareModule Name ... EndDeclareModule` (M7d).
    KwEndDeclareModule,
    KwModule,           ///< `Module Name ... EndModule` (M7d).
    KwEndModule,
    KwUseModule,        ///< `UseModule Name` (M7d).
    KwUnuseModule,      ///< `UnuseModule Name` (M7d).
    KwAnd,
    KwOr,
    KwNot,
    KwXOr,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equal,       ///< `=` - both assignment and the equality comparison,
                 ///< disambiguated by the parser's grammatical position.
    NotEqual,    ///< `<>`
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Ampersand,   ///< `&` - bitwise AND
    Pipe,        ///< `|` - bitwise OR
    Bang,        ///< `!` - bitwise XOR (not logical negation - that's `Not`)
    Tilde,       ///< `~` - unary bitwise NOT
    ShiftLeft,   ///< `<<`
    ShiftRight,  ///< `>>`
    Hash,        ///< `#` - constant-name sigil (`#MyConst`)
    Backslash,   ///< `\` - structure field access (`var\field`), never division.
    At,          ///< `@` - address-of (M3d, pointers).
    Question,    ///< `?` - address of a DataSection label, `?Label` (M7c prerequisite).
    ColonColon,  ///< `::` - qualified module member access, `Module::Member` (M7d).
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
    Struct,    ///< `.StructName` - a *named* Structure type, not a primitive
               ///< (oracle-verified: `Define p.Point`, or a Structure field
               ///< naming another Structure, e.g. `topLeft.Point`). The
               ///< actual structure name is carried separately (see
               ///< Token::structSuffix and every AST node with a matching
               ///< `structTypeName` field) since it's an open-ended
               ///< user-defined name, not one of the 11 fixed letters above.
    Interface, ///< Never produced by the lexer (a `.Name` suffix is always
               ///< tokenized as `Struct` - lexically a Structure name and an
               ///< Interface name look identical). `Sema::resolvePointeeType`
               ///< re-tags a pointer's own `ResolvedType` with this instead,
               ///< once it resolves `structTypeName` against `interfaces_`
               ///< rather than `structures_` (M7c) - everything downstream
               ///< (Codegen's vtable-call lowering) switches on *this* tag,
               ///< never a raw token-level suffix.
};

struct Token {
    TokenKind kind = TokenKind::Unknown;
    std::string text;               ///< Raw lexeme (identifier name without
                                     ///< its suffix, unescaped string
                                     ///< contents, or the literal digits).
    TypeSuffix suffix = TypeSuffix::None; ///< Only meaningful for Identifier.
    /// Only meaningful when `suffix == TypeSuffix::Struct`: the named
    /// Structure type's name, lowercased (`structSuffix`) and as originally
    /// spelled (`structSuffixSpelling`, for diagnostics).
    std::string structSuffix;
    std::string structSuffixSpelling;
    long long intValue = 0;         ///< Only meaningful for IntegerLiteral.
    double floatValue = 0.0;        ///< Only meaningful for FloatLiteral.
    SourceLoc loc;
};

/// Human-readable name for diagnostics, e.g. "identifier" or "'+' ".
const char* tokenKindName(TokenKind kind);

} // namespace easybasic
