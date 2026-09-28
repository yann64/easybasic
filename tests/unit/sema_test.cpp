#include <catch2/catch_test_macros.hpp>

#include "../../compiler/src/diagnostics/diagnostics.hpp"
#include "../../compiler/src/lexer/lexer.hpp"
#include "../../compiler/src/parser/parser.hpp"
#include "../../compiler/src/sema/sema.hpp"

using namespace easybasic;

namespace {
std::unique_ptr<ast::Module> parse(const std::string& source, DiagnosticEngine& diags) {
    Lexer lexer(source, diags.registerFile("<test>"), diags);
    Parser parser(lexer.tokenize(), diags);
    return parser.parseModule();
}
} // namespace

TEST_CASE("Sema defaults an unsuffixed Define to Integer", "[sema]") {
    DiagnosticEngine diags;
    auto module = parse("Define x = 5", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK(sema.typeOf("x") == TypeSuffix::Integer);
}

TEST_CASE("Sema implicitly declares a plain assignment as Integer", "[sema]") {
    // Mirrors real PB with EnableExplicit off: assigning to an unseen name
    // declares it rather than erroring.
    DiagnosticEngine diags;
    auto module = parse("x = 5", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK(sema.typeOf("x") == TypeSuffix::Integer);
}

TEST_CASE("Sema keeps an explicit String suffix", "[sema]") {
    DiagnosticEngine diags;
    auto module = parse("Define s.s = \"hi\"", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK(sema.typeOf("s") == TypeSuffix::String);
}

TEST_CASE("Sema rejects assigning a String to a numeric variable", "[sema]") {
    DiagnosticEngine diags;
    auto module = parse("Define x.i = \"hi\"", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema's `/` is target-typed: real division only under a Float destination", "[sema]") {
    // Oracle-verified: `Debug a/b` (two Integer operands, no destination at
    // all) prints `3` (integer division), but `Define e.d = a/b` prints
    // `3.5` (the Double destination forces real division) - the *same*
    // expression classifies differently depending on floatContext.
    DiagnosticEngine diags;
    auto module = parse("Define a.i = 7\nDefine b.i = 2\nDefine c.d = a / b", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));

    auto* defC = static_cast<ast::DefineStmt*>(module->statements[2].get());
    const ast::Expr& divExpr = *defC->declarators[0].init;
    CHECK(sema.classify(divExpr, false) == ValueKind::IntegerFamily); // bare, no destination
    CHECK(sema.classify(divExpr, true) == ValueKind::FloatFamily);    // under a Double destination
}

TEST_CASE("Sema's float destination context propagates through nested arithmetic", "[sema]") {
    // Oracle-verified: `Define g.d = 1 + 7/2` evaluates to `4.5`, meaning
    // the Double destination's float-ness reaches the nested `/` two levels
    // down, not just a shallow top-level check.
    DiagnosticEngine diags;
    auto module = parse("Define g.d = 1 + 7 / 2", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    auto* def = static_cast<ast::DefineStmt*>(module->statements[0].get());
    CHECK(sema.classify(*def->declarators[0].init, true) == ValueKind::FloatFamily);
}

TEST_CASE("Sema treats `%` as integer-family unless a float operand promotes it", "[sema]") {
    DiagnosticEngine diags;
    auto module = parse("Define a.i = 2 * 3 % 4", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    auto* def = static_cast<ast::DefineStmt*>(module->statements[0].get());
    CHECK(sema.familyOfExpr(*def->declarators[0].init) == ValueKind::IntegerFamily);
}
