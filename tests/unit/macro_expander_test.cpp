#include <catch2/catch_test_macros.hpp>

#include "../../compiler/src/diagnostics/diagnostics.hpp"
#include "../../compiler/src/lexer/lexer.hpp"
#include "../../compiler/src/preprocessor/macro_expander.hpp"

using namespace easybasic;

namespace {
std::vector<Token> expandSource(const std::string& source, DiagnosticEngine& diags) {
    Lexer lexer(source, diags.registerFile("<test>"), diags);
    std::vector<Token> tokens = lexer.tokenize();
    MacroExpander expander(diags);
    return expander.expand(tokens);
}

/// The non-separator, non-EOF token kinds in order - what a test actually
/// wants to assert on, since exact NewLine placement isn't the point here.
std::vector<TokenKind> significantKinds(const std::vector<Token>& tokens) {
    std::vector<TokenKind> kinds;
    for (const auto& t : tokens) {
        if (t.kind != TokenKind::NewLine && t.kind != TokenKind::EndOfFile) {
            kinds.push_back(t.kind);
        }
    }
    return kinds;
}
} // namespace

TEST_CASE("MacroExpander removes a Macro definition and expands a bare zero-arg invocation",
          "[preprocessor][macro]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("Macro Greet\nDebug \"hi\"\nEndMacro\nGreet", diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    // Macro/EndMacro/Greet themselves are gone; only the body (`Debug
    // "hi"`) remains, with nothing left over from the invocation site.
    REQUIRE(kinds.size() == 2);
    CHECK(kinds[0] == TokenKind::KwDebug);
    CHECK(kinds[1] == TokenKind::StringLiteral);
}

TEST_CASE("MacroExpander substitutes a parameter as raw tokens, not a pre-evaluated value",
          "[preprocessor][macro]") {
    // Oracle-verified: Square(2+3) with body `x*x` expands to the literal
    // tokens `2+3*2+3`, not a pre-evaluated `5*5`.
    DiagnosticEngine diags;
    auto tokens = expandSource("Macro Square(x)\nx*x\nEndMacro\nSquare(2+3)", diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    REQUIRE(kinds.size() == 7);
    CHECK(kinds[0] == TokenKind::IntegerLiteral);
    CHECK(kinds[1] == TokenKind::Plus);
    CHECK(kinds[2] == TokenKind::IntegerLiteral);
    CHECK(kinds[3] == TokenKind::Star);
    CHECK(kinds[4] == TokenKind::IntegerLiteral);
    CHECK(kinds[5] == TokenKind::Plus);
    CHECK(kinds[6] == TokenKind::IntegerLiteral);
}

TEST_CASE("MacroExpander leaves a zero-parameter macro name followed by '(' unexpanded",
          "[preprocessor][macro]") {
    // Oracle-verified: `Greet()` is a syntax error for a zero-param macro
    // (unlike a zero-arg Procedure call) - MacroExpander itself doesn't try
    // to replicate that exact diagnostic; it just declines to expand,
    // leaving `Greet ( )` for the Parser/Sema to react to as an ordinary
    // (here, undeclared) identifier.
    DiagnosticEngine diags;
    auto tokens = expandSource("Macro Greet\nDebug \"hi\"\nEndMacro\nGreet()", diags);
    auto kinds = significantKinds(tokens);
    REQUIRE(kinds.size() == 3);
    CHECK(kinds[0] == TokenKind::Identifier);
    CHECK(kinds[1] == TokenKind::LParen);
    CHECK(kinds[2] == TokenKind::RParen);
}

TEST_CASE("MacroExpander supports one macro invoking another, fully expanding both",
          "[preprocessor][macro]") {
    DiagnosticEngine diags;
    auto tokens =
        expandSource("Macro Inner(b)\nb*10\nEndMacro\nMacro Outer(a)\nInner(a)+1\nEndMacro\nOuter(3)", diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    // Fully expanded down to `3*10+1` - no Inner/Outer identifier survives.
    REQUIRE(kinds.size() == 5);
    CHECK(kinds[0] == TokenKind::IntegerLiteral);
    CHECK(kinds[1] == TokenKind::Star);
    CHECK(kinds[2] == TokenKind::IntegerLiteral);
    CHECK(kinds[3] == TokenKind::Plus);
    CHECK(kinds[4] == TokenKind::IntegerLiteral);
}

TEST_CASE("MacroExpander rejects a macro that invokes itself, directly", "[preprocessor][macro]") {
    // Oracle-verified: real PB itself detects this ("Endless recursivity
    // detected in the Macro.") rather than expanding forever.
    DiagnosticEngine diags;
    expandSource("Macro Rec(x)\nRec(x)+1\nEndMacro\nRec(1)", diags);
    CHECK(diags.hasErrors());
}

TEST_CASE("MacroExpander does not expand an invocation that precedes the macro's own definition",
          "[preprocessor][macro]") {
    // Oracle-verified (via a full -d -o compile, not just a -k syntax
    // check): a macro must be defined before any invocation of it in the
    // file - no forward-reference support, unlike a DataSection label.
    DiagnosticEngine diags;
    auto tokens = expandSource("Triple(4)\nMacro Triple(x)\nx*3\nEndMacro", diags);
    auto kinds = significantKinds(tokens);
    // `Triple(4)` survives completely untouched - not yet a known macro
    // name at the point this scan reaches it.
    REQUIRE(kinds.size() == 4);
    CHECK(kinds[0] == TokenKind::Identifier);
    CHECK(kinds[1] == TokenKind::LParen);
    CHECK(kinds[2] == TokenKind::IntegerLiteral);
    CHECK(kinds[3] == TokenKind::RParen);
}

TEST_CASE("MacroExpander rejects a call with the wrong number of arguments", "[preprocessor][macro]") {
    DiagnosticEngine diags;
    expandSource("Macro Add(a, b)\na+b\nEndMacro\nAdd(1, 2, 3)", diags);
    CHECK(diags.hasErrors());
}

TEST_CASE("MacroExpander supports a multi-statement macro body", "[preprocessor][macro]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("Macro PrintBoth(a, b)\nDebug a\nDebug b\nEndMacro\nPrintBoth(1, 2)", diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    REQUIRE(kinds.size() == 4);
    CHECK(kinds[0] == TokenKind::KwDebug);
    CHECK(kinds[1] == TokenKind::IntegerLiteral);
    CHECK(kinds[2] == TokenKind::KwDebug);
    CHECK(kinds[3] == TokenKind::IntegerLiteral);
}
