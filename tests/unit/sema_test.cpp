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

TEST_CASE("Sema allows implicit variable declaration without EnableExplicit", "[sema][enable-explicit]") {
    DiagnosticEngine diags;
    auto module = parse("x = 5\nDebug x", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects an undeclared variable under EnableExplicit", "[sema][enable-explicit]") {
    // Oracle-verified error text: "With 'EnableExplicit', variables have to
    // be declared: x."
    DiagnosticEngine diags;
    auto module = parse("EnableExplicit\nx = 5", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts a Define'd variable under EnableExplicit", "[sema][enable-explicit]") {
    DiagnosticEngine diags;
    auto module = parse("EnableExplicit\nDefine x.i = 5\nDebug x", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema resolves a #Name constant's family", "[sema][const]") {
    DiagnosticEngine diags;
    auto module = parse("#GREETING = \"hi\"\nDebug #GREETING", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK(sema.constTypeOf("greeting") == TypeSuffix::String);
}

TEST_CASE("Sema rejects a reference to an undeclared constant", "[sema][const]") {
    DiagnosticEngine diags;
    auto module = parse("Debug #NOPE", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema flags logical XOr as unsupported", "[sema][xor]") {
    // Real PB's logical XOr showed an unexplained runtime anomaly under
    // oracle testing (docs/architecture/roadmap.md's M1 notes) - Sema
    // refuses to silently generate possibly-wrong code for it.
    DiagnosticEngine diags;
    auto module = parse("If 1 XOr 0\nDebug 1\nEndIf", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema resolves a procedure's return type and param count", "[sema][procedure]") {
    DiagnosticEngine diags;
    auto module = parse("Procedure.i Add(a.i, b.i)\nProcedureReturn a + b\nEndProcedure", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    const auto* info = sema.procedureInfo("add");
    REQUIRE(info != nullptr);
    CHECK(info->returnSuffix == TypeSuffix::Integer);
    CHECK(info->paramSuffixes.size() == 2);
    CHECK(info->requiredParamCount == 2);
}

TEST_CASE("Sema rejects calling an undeclared procedure", "[sema][procedure]") {
    DiagnosticEngine diags;
    auto module = parse("Debug Nope(1)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a call with the wrong argument count", "[sema][procedure]") {
    DiagnosticEngine diags;
    auto module = parse("Procedure.i Add(a.i, b.i)\nProcedureReturn a + b\nEndProcedure\nDebug Add(1)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts a call omitting a defaulted trailing argument", "[sema][procedure]") {
    DiagnosticEngine diags;
    auto module =
        parse("Procedure.i Add(a.i, b.i = 100)\nProcedureReturn a + b\nEndProcedure\nDebug Add(1)", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema gives a procedure body its own isolated scope", "[sema][procedure]") {
    // Oracle-verified: a procedure reading a same-named outer variable gets
    // a fresh local defaulting to Integer/0, not the outer variable's type
    // or value - see ast::ProcedureDeclStmt's own doc comment.
    DiagnosticEngine diags;
    auto module = parse("Define outer.s = \"hi\"\nProcedure ReadOuter()\nDebug outer\nEndProcedure", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    // The outer `outer` keeps its own String type - unaffected by the
    // procedure body's unrelated, isolated local of the same name.
    CHECK(sema.typeOf("outer") == TypeSuffix::String);
}

TEST_CASE("Sema makes a Global variable visible in a procedure with no Shared", "[sema][scope]") {
    // Oracle-verified: unlike a plain Define, a Global doesn't need Shared
    // at all - a procedure referencing it resolves to the real outer type.
    DiagnosticEngine diags;
    auto module =
        parse("Global g.s = \"hi\"\nProcedure Touch()\nDebug g\nEndProcedure", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    CHECK(sema.typeOf("g") == TypeSuffix::String);
}

TEST_CASE("Sema's Shared pulls in a plain Define'd variable's real type", "[sema][scope]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Define d.s = \"hi\"\nProcedure Touch()\nShared d\nDebug d\nEndProcedure", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects Shared for a name that was never declared", "[sema][scope]") {
    DiagnosticEngine diags;
    auto module = parse("Procedure Touch()\nShared nope\nDebug nope\nEndProcedure", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects Shared used outside a procedure", "[sema][scope]") {
    DiagnosticEngine diags;
    auto module = parse("Define d.i = 1\nShared d", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema resolves a 1D array's element type and dimension count", "[sema][array]") {
    DiagnosticEngine diags;
    auto module = parse("Dim arr.s(4)", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    const auto* info = sema.arrayInfo("arr");
    REQUIRE(info != nullptr);
    CHECK(info->elementSuffix == TypeSuffix::String);
    CHECK(info->dimensionCount == 1);
}

TEST_CASE("Sema disambiguates name(args) as an array read, not a call", "[sema][array]") {
    // `arr(0)` is syntactically identical to a call at parse time - Sema
    // must recognize `arr` as a Dim'd array and not require it to also be a
    // declared procedure.
    DiagnosticEngine diags;
    auto module = parse("Dim arr.i(4)\nDebug arr(0)", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects an array indexed with the wrong number of dimensions", "[sema][array]") {
    DiagnosticEngine diags;
    auto module = parse("Dim grid.i(2, 2)\nDebug grid(0)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects assigning a String into a numeric array element", "[sema][array]") {
    DiagnosticEngine diags;
    auto module = parse("Dim arr.i(4)\narr(0) = \"nope\"", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}
