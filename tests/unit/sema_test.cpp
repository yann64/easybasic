#include <catch2/catch_test_macros.hpp>

#include <algorithm>

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

TEST_CASE("Sema resolves a Structure's field types", "[sema][struct]") {
    DiagnosticEngine diags;
    auto module = parse("Structure Point\nx.i\nname.s\nEndStructure", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    const auto* info = sema.structureInfo("point");
    REQUIRE(info != nullptr);
    REQUIRE(info->fields.size() == 2);
    CHECK(info->fields[0].suffix == TypeSuffix::Integer);
    CHECK(info->fields[1].suffix == TypeSuffix::String);
}

TEST_CASE("Sema resolves a field-access chain's type, including through nesting", "[sema][struct]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Structure Point\nx.i\nEndStructure\n"
        "Structure Rect\ntopLeft.Point\nEndStructure\n"
        "Define r.Rect\nDebug r\\topLeft\\x",
        diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    auto* dbg = static_cast<ast::DebugStmt*>(module->statements[2].get());
    CHECK(sema.resolveType(*dbg->value).suffix == TypeSuffix::Integer);
}

TEST_CASE("Sema resolves a field's type through an array-of-Structure element", "[sema][struct]") {
    DiagnosticEngine diags;
    auto module = parse("Structure Point\nx.i\nEndStructure\nDim points.Point(2)\nDebug points(0)\\x", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a field access on a non-Structure value", "[sema][struct]") {
    DiagnosticEngine diags;
    auto module = parse("Define x.i = 5\nDebug x\\field", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects an unknown field name on a real Structure", "[sema][struct]") {
    DiagnosticEngine diags;
    auto module = parse("Structure Point\nx.i\nEndStructure\nDefine p.Point\nDebug p\\nope", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema keeps Structure and variable Define'd with the wrong type name separate errors", "[sema][struct]") {
    DiagnosticEngine diags;
    auto module = parse("Define p.NotDeclared", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema gives a pointer variable itself Integer type, regardless of its pointee", "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse("Structure Point\nx.i\nEndStructure\nDefine *pp.Point", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    CHECK(sema.typeOf("*pp") == TypeSuffix::Integer);
}

TEST_CASE("Sema tracks a Structure-typed pointer's pointee and namespaces it from a same-named variable",
          "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse("Structure Point\nx.i\nEndStructure\nDefine pp.s = \"hi\"\nDefine *pp.Point", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    // "pp" and "*pp" are genuinely separate namespaces (oracle-verified) -
    // the plain String variable must be untouched by the pointer sharing
    // its base name.
    CHECK(sema.typeOf("pp") == TypeSuffix::String);
    CHECK(sema.typeOf("*pp") == TypeSuffix::Integer);
    CHECK(sema.pointeeTypeOf("*pp").suffix == TypeSuffix::Struct);
    CHECK(sema.pointeeTypeOf("*pp").structName == "point");
}

TEST_CASE("Sema resolves a Structure-typed pointer's dereferenced field type", "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Structure Point\nx.i\nEndStructure\n"
        "Define p.Point\nDefine *pp.Point = @p\nDebug *pp\\x",
        diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    auto* dbg = static_cast<ast::DebugStmt*>(module->statements[3].get());
    CHECK(sema.resolveType(*dbg->value).suffix == TypeSuffix::Integer);
}

TEST_CASE("Sema rejects a primitive-typed pointer, exactly like real PB", "[sema][pointer]") {
    // Oracle-verified (`pbcompilerc`): "Error: ... Native types can't be
    // used with pointers." for `Define *pa.i`.
    DiagnosticEngine diags;
    auto module = parse("Define *pa.i", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a field dereference on an untyped pointer", "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse("Define *pa\nDebug *pa\\x", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts an untyped pointer used only by its own address value", "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse("Define a.i = 5\nDefine *pa = @a\nDebug *pa", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema resolves a pointer parameter's Structure pointee inside the procedure body", "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Structure Point\nx.i\nEndStructure\n"
        "Procedure SetX(*p.Point, v.i)\n*p\\x = v\nEndProcedure",
        diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    CHECK(sema.pointeeTypeOf("*p").suffix == TypeSuffix::Struct);
    CHECK(sema.pointeeTypeOf("*p").structName == "point");
}

TEST_CASE("Sema recognizes the pointer/memory built-ins by name", "[sema][pointer]") {
    CHECK(Sema::isPointerBuiltinName("allocatememory"));
    CHECK(Sema::isPointerBuiltinName("freememory"));
    CHECK(Sema::isPointerBuiltinName("allocatestructure"));
    CHECK(Sema::isPointerBuiltinName("freestructure"));
    CHECK_FALSE(Sema::isPointerBuiltinName("somethingelse"));
}

TEST_CASE("Sema resolves AllocateStructure's Structure-name argument without declaring a bogus variable",
          "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse("Structure Point\nx.i\nEndStructure\nDefine *sp.Point = AllocateStructure(Point)", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    // AllocateStructure's argument names a *type*, not a variable - Sema
    // must not have implicitly declared "point" as an ordinary variable.
    const auto& order = sema.declarationOrder();
    bool declaredPointAsVariable =
        std::any_of(order.begin(), order.end(), [](const auto& entry) { return entry.first == "point"; });
    CHECK_FALSE(declaredPointAsVariable);
}

TEST_CASE("Sema rejects AllocateStructure given an undeclared Structure name", "[sema][pointer]") {
    DiagnosticEngine diags;
    auto module = parse("Define *sp.Point = AllocateStructure(NotDeclared)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema defaults a bare NewList to an Integer element type", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList n()", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    const auto* info = sema.listInfo("n");
    REQUIRE(info != nullptr);
    CHECK(info->elementSuffix == TypeSuffix::Integer);
}

TEST_CASE("Sema resolves a String-typed NewList's element type", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList names.s()", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    const auto* info = sema.listInfo("names");
    REQUIRE(info != nullptr);
    CHECK(info->elementSuffix == TypeSuffix::String);
}

TEST_CASE("Sema rejects redeclaring the same List name", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList n.i()\nNewList n.s()", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema resolves a bare name() as the List's element type", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList n.i()\nAddElement(n())\nDebug n()", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    auto* dbg = static_cast<ast::DebugStmt*>(module->statements[2].get());
    CHECK(sema.classify(*dbg->value, false) == ValueKind::IntegerFamily);
}

TEST_CASE("Sema accepts name() = expr as setting a List's current element", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList n.i()\nAddElement(n())\nn() = 5", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects assigning a String into an Integer List's current element", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList n.i()\nAddElement(n())\nn() = \"nope\"", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema resolves a field access through a List of Structures", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Structure Point\nx.i\nEndStructure\n"
        "NewList pts.Point()\nAddElement(pts())\npts()\\x = 3\nDebug pts()\\x",
        diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a List built-in given a non-List argument", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("Define x.i = 5\nAddElement(x)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects ForEach on an undeclared name", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("ForEach nope()\nDebug 1\nNext", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts ForEach over a declared List and resolves its body", "[sema][list]") {
    DiagnosticEngine diags;
    auto module = parse("NewList n.i()\nAddElement(n())\nn() = 1\nForEach n()\nDebug n()\nNext", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema recognizes the List built-ins by name", "[sema][list]") {
    CHECK(Sema::isListBuiltinName("addelement"));
    CHECK(Sema::isListBuiltinName("insertelement"));
    CHECK(Sema::isListBuiltinName("deleteelement"));
    CHECK(Sema::isListBuiltinName("clearlist"));
    CHECK(Sema::isListBuiltinName("firstelement"));
    CHECK(Sema::isListBuiltinName("lastelement"));
    CHECK(Sema::isListBuiltinName("nextelement"));
    CHECK(Sema::isListBuiltinName("previouselement"));
    CHECK(Sema::isListBuiltinName("listsize"));
    CHECK(Sema::isListBuiltinName("selectelement"));
    CHECK(Sema::isListBuiltinName("listindex"));
    CHECK_FALSE(Sema::isListBuiltinName("somethingelse"));
}

TEST_CASE("Sema visits SelectElement's index argument as an ordinary expression", "[sema][list]") {
    DiagnosticEngine diags;
    // The undeclared `idx` inside SelectElement's index argument must still
    // be caught under EnableExplicit - proving it's visited normally, not
    // skipped like the List-naming argument is.
    auto module = parse("EnableExplicit\nNewList n.i()\nSelectElement(n(), idx)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema defaults a bare NewMap to an Integer element type", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m()", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    const auto* info = sema.mapInfo("m");
    REQUIRE(info != nullptr);
    CHECK(info->elementSuffix == TypeSuffix::Integer);
}

TEST_CASE("Sema rejects redeclaring the same Map name", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nNewMap m.s()", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts name(key) as reading a Map's element type", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nDebug m(\"x\")", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    auto* dbg = static_cast<ast::DebugStmt*>(module->statements[1].get());
    CHECK(sema.classify(*dbg->value, false) == ValueKind::IntegerFamily);
}

TEST_CASE("Sema accepts name() = expr as setting a Map's current element", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nAddMapElement(m(), \"x\")\nm() = 5", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema accepts name(key) = expr as a Map write, auto-creating the key", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nm(\"x\") = 5", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a non-String Map key, exactly like real PB", "[sema][map]") {
    // Oracle-verified (`pbcompilerc`): "Error: ... A string expression is
    // expected ('number' not allowed)." for `m(5) = 1`.
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nm(5) = 1", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema resolves MapKey(...) as a String-family expression", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nAddMapElement(m(), \"x\")\nDebug MapKey(m())", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    auto* dbg = static_cast<ast::DebugStmt*>(module->statements[2].get());
    CHECK(sema.classify(*dbg->value, false) == ValueKind::StringFamily);
}

TEST_CASE("Sema resolves a field access through a Map of Structures", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Structure Point\nx.i\nEndStructure\n"
        "NewMap pts.Point()\npts(\"a\")\\x = 3\nDebug pts(\"a\")\\x",
        diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a Map built-in given a non-Map argument", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("Define x.i = 5\nAddMapElement(x, \"k\")", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts ForEach over a declared Map and resolves its body", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nm(\"x\") = 1\nForEach m()\nDebug m()\nNext", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema recognizes the Map built-ins by name", "[sema][map]") {
    CHECK(Sema::isMapBuiltinName("addmapelement"));
    CHECK(Sema::isMapBuiltinName("deletemapelement"));
    CHECK(Sema::isMapBuiltinName("clearmap"));
    CHECK(Sema::isMapBuiltinName("mapsize"));
    CHECK(Sema::isMapBuiltinName("mapkey"));
    CHECK(Sema::isMapBuiltinName("resetmap"));
    CHECK(Sema::isMapBuiltinName("nextmapelement"));
    CHECK(Sema::isMapBuiltinName("findmapelement"));
    CHECK_FALSE(Sema::isMapBuiltinName("somethingelse"));
}

TEST_CASE("Sema accepts DeleteMapElement's 1-arg and 2-arg forms", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nm(\"x\") = 1\nDeleteMapElement(m())\nDeleteMapElement(m(), \"x\")", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a non-String key argument to FindMapElement", "[sema][map]") {
    DiagnosticEngine diags;
    auto module = parse("NewMap m.i()\nFindMapElement(m(), 5)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

// --- M2-closure: items originally deferred to M3, picked up as a follow-up
// (mutual recursion / Declare, call-argument type-checking, and a
// constant/procedure declared inside a nested block). ---

TEST_CASE("Sema allows mutual recursion via a Declare forward declaration", "[sema][declare]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Declare IsOdd(n.i)\n"
        "Procedure IsEven(n.i)\nIf n = 0\nProcedureReturn 1\nEndIf\nProcedureReturn IsOdd(n - 1)\nEndProcedure\n"
        "Procedure IsOdd(n.i)\nIf n = 0\nProcedureReturn 0\nEndIf\nProcedureReturn IsEven(n - 1)\nEndProcedure\n"
        "Debug IsEven(10)",
        diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a Declare left unfulfilled by any matching Procedure", "[sema][declare]") {
    // Oracle-verified: "The procedure 'name()' has been declared but not
    // defined."
    DiagnosticEngine diags;
    auto module = parse("Declare NeverDefined()\nDebug NeverDefined()", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a Procedure whose signature doesn't match its own Declare", "[sema][declare]") {
    // Oracle-verified: "Declare doesn't match with real Procedure." for any
    // parameter or return type mismatch, not just an arity mismatch.
    DiagnosticEngine diags;
    auto module = parse("Declare Foo()\nProcedure Foo(x.i)\nProcedureReturn x\nEndProcedure\nDebug Foo(1)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts a Declare and Procedure with a matching default-valued parameter", "[sema][declare]") {
    DiagnosticEngine diags;
    auto module =
        parse("Declare Foo(x.i = 5)\nProcedure Foo(x.i = 5)\nProcedureReturn x\nEndProcedure\nDebug Foo()", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a String argument passed to an Integer parameter", "[sema][call-types]") {
    // Oracle-verified: "Bad parameter type, number expected instead of
    // string." - only caught by a full compile, not `-k`'s syntax check.
    DiagnosticEngine diags;
    auto module = parse("Procedure Foo(x.i)\nProcedureReturn x\nEndProcedure\nDebug Foo(\"hi\")", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a numeric argument passed to a String parameter", "[sema][call-types]") {
    // Oracle-verified: "Bad parameter type: a string is expected."
    DiagnosticEngine diags;
    auto module = parse("Procedure Foo(x.s)\nProcedureReturn 1\nEndProcedure\nDebug Foo(5)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts a matching-family call argument", "[sema][call-types]") {
    DiagnosticEngine diags;
    auto module = parse("Procedure Foo(x.i)\nProcedureReturn x\nEndProcedure\nDebug Foo(5)", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema resolves a constant declared inside a never-taken If branch", "[sema][nested-decl]") {
    // Oracle-verified: a constant is purely compile-time/textual and
    // entirely independent of runtime control flow - usable afterward
    // regardless of whether the branch that declared it actually runs.
    DiagnosticEngine diags;
    auto module = parse("a.i = 0\nIf a = 1\n#X = 5\nEndIf\nDebug #X", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema resolves a constant declared inside a Procedure body", "[sema][nested-decl]") {
    DiagnosticEngine diags;
    auto module = parse("Procedure Foo()\n#X = 5\nProcedureReturn #X\nEndProcedure\nDebug Foo()", diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a Procedure declared inside an If block", "[sema][nested-decl]") {
    // Oracle-verified: "A procedure can't be declared inside an If, Repeat,
    // While or For."
    DiagnosticEngine diags;
    auto module = parse("a.i = 1\nIf a = 1\nProcedure Foo()\nProcedureReturn 1\nEndProcedure\nEndIf", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a Procedure declared inside a ForEach block", "[sema][nested-decl]") {
    DiagnosticEngine diags;
    auto module =
        parse("NewList n.i()\nAddElement(n())\nForEach n()\nProcedure Foo()\nProcedureReturn 1\nEndProcedure\nNext",
              diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a Procedure declared inside another Procedure", "[sema][nested-decl]") {
    // Oracle-verified: "Can't define a procedure inside another procedure."
    // - distinct wording from the control-flow-nesting case above.
    DiagnosticEngine diags;
    auto module = parse(
        "Procedure Outer()\nProcedure Inner()\nProcedureReturn 1\nEndProcedure\nProcedureReturn Inner()\n"
        "EndProcedure",
        diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

// --- M4a: String library builtins ---

TEST_CASE("Sema resolves each String-library builtin's declared return type", "[sema][stringlib]") {
    DiagnosticEngine diags;
    auto module = parse(
        "Define s.s = \"hi\"\n"
        "Debug Len(s)\nDebug Left(s, 1)\nDebug Val(s)\nDebug ValF(s)\nDebug Str(1)",
        diags);
    Sema sema(diags);
    REQUIRE(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
    auto* len = static_cast<ast::DebugStmt*>(module->statements[1].get());
    auto* left = static_cast<ast::DebugStmt*>(module->statements[2].get());
    auto* val = static_cast<ast::DebugStmt*>(module->statements[3].get());
    auto* valf = static_cast<ast::DebugStmt*>(module->statements[4].get());
    auto* str = static_cast<ast::DebugStmt*>(module->statements[5].get());
    CHECK(sema.classify(*len->value, false) == ValueKind::IntegerFamily);
    CHECK(sema.classify(*left->value, false) == ValueKind::StringFamily);
    CHECK(sema.classify(*val->value, false) == ValueKind::IntegerFamily);
    CHECK(sema.classify(*valf->value, false) == ValueKind::FloatFamily);
    CHECK(sema.classify(*str->value, false) == ValueKind::StringFamily);
}

TEST_CASE("Sema rejects a String-library builtin called with too few arguments", "[sema][stringlib]") {
    DiagnosticEngine diags;
    auto module = parse("Debug Left(\"hi\")", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema accepts Mid's optional count argument being omitted", "[sema][stringlib]") {
    DiagnosticEngine diags;
    auto module = parse("Debug Mid(\"hi\", 1)", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}

TEST_CASE("Sema rejects a numeric argument passed to a String-library String parameter", "[sema][stringlib]") {
    DiagnosticEngine diags;
    auto module = parse("Debug Len(5)", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema rejects a String argument passed to a String-library numeric parameter", "[sema][stringlib]") {
    DiagnosticEngine diags;
    auto module = parse("Debug Left(\"hi\", \"nope\")", diags);
    Sema sema(diags);
    CHECK_FALSE(sema.analyze(*module));
    CHECK(diags.hasErrors());
}

TEST_CASE("Sema banker's-rounds a Float argument passed to Str", "[sema][stringlib]") {
    // Str's parameter is Integer-typed, so a Float argument goes through the
    // same target-typed conversion as everywhere else (Codegen actually
    // performs the rounding; this just confirms Sema accepts the call and
    // classifies it as String, oracle-verified end-to-end via the
    // stringlib e2e_diff test: Str(2.5) is "2", Str(3.5) is "4").
    DiagnosticEngine diags;
    auto module = parse("Debug Str(2.5)", diags);
    Sema sema(diags);
    CHECK(sema.analyze(*module));
    CHECK_FALSE(diags.hasErrors());
}
