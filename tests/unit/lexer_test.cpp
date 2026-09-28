#include <catch2/catch_test_macros.hpp>

#include "../../compiler/src/diagnostics/diagnostics.hpp"
#include "../../compiler/src/lexer/lexer.hpp"

using namespace easybasic;

namespace {
std::vector<Token> lexAll(const std::string& source, DiagnosticEngine& diags) {
    Lexer lexer(source, diags.registerFile("<test>"), diags);
    return lexer.tokenize();
}
} // namespace

TEST_CASE("Lexer attaches type suffixes to identifiers", "[lexer]") {
    DiagnosticEngine diags;
    auto tokens = lexAll("x.i = 5", diags);
    REQUIRE(tokens.size() >= 4);
    CHECK(tokens[0].kind == TokenKind::Identifier);
    CHECK(tokens[0].text == "x");
    CHECK(tokens[0].suffix == TypeSuffix::Integer);
    CHECK(tokens[1].kind == TokenKind::Equal);
    CHECK(tokens[2].kind == TokenKind::IntegerLiteral);
    CHECK(tokens[2].intValue == 5);
}

TEST_CASE("Lexer distinguishes % modulo from a binary literal", "[lexer]") {
    // Oracle-verified: `%101 % 2` lexes as (binary-literal 5) (modulo) 2 -
    // `%` immediately followed by a binary digit is the literal prefix,
    // otherwise it's the modulo operator.
    DiagnosticEngine diags;
    auto tokens = lexAll("%101 % 2", diags);
    REQUIRE(tokens.size() >= 3);
    CHECK(tokens[0].kind == TokenKind::IntegerLiteral);
    CHECK(tokens[0].intValue == 5);
    CHECK(tokens[1].kind == TokenKind::Percent);
    CHECK(tokens[2].kind == TokenKind::IntegerLiteral);
    CHECK(tokens[2].intValue == 2);
}

TEST_CASE("Lexer reads hex literals", "[lexer]") {
    DiagnosticEngine diags;
    auto tokens = lexAll("$1A", diags);
    REQUIRE(tokens.size() >= 1);
    CHECK(tokens[0].kind == TokenKind::IntegerLiteral);
    CHECK(tokens[0].intValue == 26);
}

TEST_CASE("Lexer treats ';' as a comment to end of line", "[lexer]") {
    DiagnosticEngine diags;
    auto tokens = lexAll("x.i = 1 ; trailing comment\ny.i = 2", diags);
    // Expect: x . = 1 NEWLINE y . = 2 EOF  (comment produces no tokens)
    bool sawComment = false;
    for (const auto& t : tokens) {
        if (t.kind == TokenKind::Unknown) {
            sawComment = true;
        }
    }
    CHECK_FALSE(sawComment);
    CHECK(tokens.back().kind == TokenKind::EndOfFile);
}

TEST_CASE("Lexer is case-insensitive for keywords", "[lexer]") {
    DiagnosticEngine diags;
    auto tokens = lexAll("DEFINE x.i = 1", diags);
    CHECK(tokens[0].kind == TokenKind::KwDefine);
}
