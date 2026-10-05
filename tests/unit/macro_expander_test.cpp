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

/// True if `needle` appears as a contiguous subsequence of `haystack` -
/// used by the Module/Macro tests below instead of exact-length/exact-
/// position assertions, since the surrounding `DeclareModule`/`Module`/
/// `EndModule`/etc. boilerplate's own exact token count isn't the point
/// of any of them (only whether the macro's own body was, or wasn't,
/// actually substituted in).
bool containsSubsequence(const std::vector<TokenKind>& haystack, const std::vector<TokenKind>& needle) {
    if (needle.empty() || haystack.size() < needle.size()) {
        return needle.empty();
    }
    for (std::size_t start = 0; start + needle.size() <= haystack.size(); ++start) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size() && match; ++j) {
            match = haystack[start + j] == needle[j];
        }
        if (match) {
            return true;
        }
    }
    return false;
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

// M7d's own Macro/Interface-in-Module notes own follow-up: qualified
// `Module::Macro()` invocation, oracle-verified end to end (every case
// below was checked against a full `-d -o` compile+run of the real
// oracle, not just a `-k` syntax check - confirmed unreliable for macro-
// related behavior specifically, see docs/architecture/roadmap.md's own
// notes on this).

TEST_CASE("MacroExpander expands a Macro declared inside a Module's own body, used unqualified from "
          "inside that same module",
          "[preprocessor][macro][modules]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("DeclareModule Foo\nEndDeclareModule\n"
                                "Module Foo\nMacro Double(x)\n(x)*2\nEndMacro\nDebug Double(21)\nEndModule",
                                diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    // Double/Macro/EndMacro themselves are gone, replaced by its own body
    // (21*2) right after Debug - no "Double" identifier survives anywhere.
    CHECK(containsSubsequence(
        kinds, {TokenKind::KwDebug, TokenKind::LParen, TokenKind::IntegerLiteral, TokenKind::RParen, TokenKind::Star, TokenKind::IntegerLiteral}));
    for (std::size_t idx = 0; idx + 1 < kinds.size(); ++idx) {
        bool looksLikeACall = (kinds[idx] == TokenKind::Identifier) && (kinds[idx + 1] == TokenKind::LParen);
        CHECK_FALSE(looksLikeACall);
    }
}

TEST_CASE("MacroExpander expands a qualified Module::Macro() invocation from outside the module",
          "[preprocessor][macro][modules]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("DeclareModule Foo\nMacro Triple(x)\n(x)*3\nEndMacro\nEndDeclareModule\n"
                                "Module Foo\nEndModule\nDebug Foo::Triple(10)",
                                diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    CHECK(containsSubsequence(
        kinds, {TokenKind::KwDebug, TokenKind::LParen, TokenKind::IntegerLiteral, TokenKind::RParen, TokenKind::Star, TokenKind::IntegerLiteral}));
    CHECK_FALSE(containsSubsequence(kinds, {TokenKind::ColonColon})); // The qualifier itself is consumed, not left over.
}

TEST_CASE("MacroExpander's UseModule brings a module's own public Macro into unqualified scope",
          "[preprocessor][macro][modules]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("DeclareModule Foo\nMacro Triple(x)\n(x)*3\nEndMacro\nEndDeclareModule\n"
                                "Module Foo\nEndModule\nUseModule Foo\nDebug Triple(10)",
                                diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    CHECK(containsSubsequence(kinds, {TokenKind::KwUseModule, TokenKind::Identifier}));
    CHECK(containsSubsequence(
        kinds, {TokenKind::KwDebug, TokenKind::LParen, TokenKind::IntegerLiteral, TokenKind::RParen, TokenKind::Star, TokenKind::IntegerLiteral}));
}

TEST_CASE("MacroExpander's UnuseModule removes a module's own Macro from unqualified scope again",
          "[preprocessor][macro][modules]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("DeclareModule Foo\nMacro Triple(x)\n(x)*3\nEndMacro\nEndDeclareModule\n"
                                "Module Foo\nEndModule\n"
                                "UseModule Foo\nUnuseModule Foo\nDebug Triple(10)",
                                diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    // Triple is no longer a known macro here - survives as a plain,
    // unexpanded call for the Parser/Sema to react to (here, an
    // undeclared-procedure error later, not this pass's own concern).
    REQUIRE(kinds.size() >= 1);
    CHECK(kinds.back() == TokenKind::RParen);
    bool sawBareDouble = false;
    for (auto k : kinds) {
        sawBareDouble = sawBareDouble || k == TokenKind::Star;
    }
    CHECK_FALSE(sawBareDouble); // Never expanded - no '*' from the macro's own body appears anywhere.
}

TEST_CASE("MacroExpander rejects a qualified reference to a Macro declared only in Module (private)",
          "[preprocessor][macro][modules]") {
    // Oracle-verified: the exact same "Module item 'X' is not declared as
    // public." error Sema's own checkModuleAccess already uses for every
    // other declaration kind - confirmed directly, attributed to "the
    // expanded macro" in real PB's own error text, consistent with this
    // check belonging to the macro layer itself, not a later Sema pass.
    DiagnosticEngine diags;
    expandSource("DeclareModule Foo\nEndDeclareModule\n"
                 "Module Foo\nMacro Secret(x)\n(x)*9\nEndMacro\nEndModule\n"
                 "Debug Foo::Secret(3)",
                 diags);
    CHECK(diags.hasErrors());
}

TEST_CASE("MacroExpander does not expand a top-level Macro's name from inside a Module's own body "
          "('sealed box' applies to Macro too)",
          "[preprocessor][macro][modules]") {
    // Oracle-verified directly (a real, fatal "Double() is not a function,
    // array, list, map or macro" compile error in real PB, not a
    // hypothetical): a module's own code never sees a top-level macro
    // unqualified, even with no better match in scope at all - no
    // "implicit top-level fallback" the way some other PB constructs have.
    DiagnosticEngine diags;
    auto tokens = expandSource("Macro Double(x)\n(x)*2\nEndMacro\n"
                                "DeclareModule Foo\nEndDeclareModule\n"
                                "Module Foo\nDebug Double(5)\nEndModule",
                                diags);
    CHECK_FALSE(diags.hasErrors()); // MacroExpander itself doesn't error - just declines to expand.
    auto kinds = significantKinds(tokens);
    bool sawStar = false;
    for (auto k : kinds) {
        sawStar = sawStar || k == TokenKind::Star;
    }
    CHECK_FALSE(sawStar); // Never expanded.
}

TEST_CASE("MacroExpander keeps two different modules' own same-named Macros independent",
          "[preprocessor][macro][modules]") {
    DiagnosticEngine diags;
    auto tokens = expandSource("DeclareModule A\nMacro Combine(x)\n(x)+100\nEndMacro\nEndDeclareModule\n"
                                "Module A\nEndModule\n"
                                "DeclareModule B\nMacro Combine(x)\n(x)+200\nEndMacro\nEndDeclareModule\n"
                                "Module B\nEndModule\n"
                                "Debug A::Combine(5)\nDebug B::Combine(5)",
                                diags);
    CHECK_FALSE(diags.hasErrors());
    auto kinds = significantKinds(tokens);
    // Each Debug got its own module's own Combine - both genuinely
    // expanded (two separate "Debug Int + Int" sequences), not just one
    // shared/collided expansion.
    std::vector<TokenKind> debugPlusPattern = {TokenKind::KwDebug, TokenKind::LParen, TokenKind::IntegerLiteral,
                                                TokenKind::RParen, TokenKind::Plus, TokenKind::IntegerLiteral};
    CHECK(containsSubsequence(kinds, debugPlusPattern));
    int debugPlusCount = 0;
    for (std::size_t start = 0; start + debugPlusPattern.size() <= kinds.size(); ++start) {
        bool match = true;
        for (std::size_t j = 0; j < debugPlusPattern.size() && match; ++j) {
            match = kinds[start + j] == debugPlusPattern[j];
        }
        debugPlusCount += match ? 1 : 0;
    }
    CHECK(debugPlusCount == 2);
}

TEST_CASE("MacroExpander: a macro's own body referencing another same-module macro resolves only "
          "when invoked from inside that module, not via qualified access from outside",
          "[preprocessor][macro][modules]") {
    // A genuinely surprising oracle finding, confirmed directly (not
    // assumed): real PB's own macro-body resolution depends entirely on
    // the *call site's* own textual module context, not the referencing
    // macro's own defining module - Wrapper's own body (`Helper(x)+1`)
    // fails to resolve Helper at all when Wrapper itself is invoked via
    // qualified access from outside the module ("Helper() is not a
    // function, array, list, map or macro" in real PB), but succeeds when
    // Wrapper is invoked from code textually inside the same module.
    DiagnosticEngine diags1;
    auto outsideTokens =
        expandSource("DeclareModule M\nMacro Helper(x)\n(x)*2\nEndMacro\n"
                      "Macro Wrapper(x)\nHelper(x)+1\nEndMacro\nEndDeclareModule\n"
                      "Module M\nEndModule\nDebug M::Wrapper(5)",
                      diags1);
    auto outsideKinds = significantKinds(outsideTokens);
    // Helper(x) survives completely unexpanded inside Wrapper's own
    // substituted body - so the overall result still contains a bare
    // "Helper" identifier call, not a numeric expansion.
    bool sawHelperIdentifier = false;
    for (std::size_t idx = 0; idx + 1 < outsideKinds.size(); ++idx) {
        if (outsideKinds[idx] == TokenKind::Identifier && outsideKinds[idx + 1] == TokenKind::LParen) {
            sawHelperIdentifier = true;
        }
    }
    CHECK(sawHelperIdentifier);

    DiagnosticEngine diags2;
    auto insideTokens =
        expandSource("DeclareModule M\nMacro Helper(x)\n(x)*2\nEndMacro\n"
                      "Macro Wrapper(x)\nHelper(x)+1\nEndMacro\nDeclare DoWrap()\nEndDeclareModule\n"
                      "Module M\nProcedure DoWrap()\nDebug Wrapper(5)\nEndProcedure\nEndModule\nM::DoWrap()",
                      diags2);
    CHECK_FALSE(diags2.hasErrors());
    auto insideKinds = significantKinds(insideTokens);
    bool sawPlus = false;
    bool sawStar = false;
    for (auto k : insideKinds) {
        sawPlus = sawPlus || k == TokenKind::Plus;
        sawStar = sawStar || k == TokenKind::Star;
    }
    CHECK(sawPlus); // Wrapper's own "+1" survives.
    CHECK(sawStar); // Helper's own "*2" was genuinely substituted in - fully expanded this time.
}
