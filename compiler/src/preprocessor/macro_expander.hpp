#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../diagnostics/diagnostics.hpp"
#include "../lexer/token.hpp"

namespace easybasic {

/// Expands `Macro name[(params)] ... EndMacro` definitions - a pure,
/// token-level textual substitution pass run between the Lexer and the
/// Parser, mirroring a C preprocessor (and, per real `pbcompilerc`'s own
/// error text referencing "the expanded macro (Macro.out)", real PB's own
/// compiler too). This can't be done at the AST level like `CompilerIf`/
/// `DataSection` (M5a/M5b) were: oracle-verified, `Square(2+3)` (body
/// `x*x`) expands to `2+3*2+3` = `11`, not `25` - a macro parameter is
/// substituted as raw, unparenthesized *tokens*, with the surrounding call
/// site's own operators applying to the pasted text after substitution,
/// not a pre-evaluated value the way a Procedure's own by-value parameter
/// passing works. A second oracle finding drives the other half of the
/// design: a zero-parameter macro is invoked *bare*, with no parens at all
/// (`Greet`, not `Greet()` - the latter is a syntax error) - genuinely
/// ambiguous with a plain variable reference at the grammar level, so
/// recognizing an invocation has to happen by *name lookup against the
/// token stream itself*, before the Parser's own grammar-driven
/// disambiguation ever runs.
class MacroExpander {
public:
    explicit MacroExpander(DiagnosticEngine& diagnostics);

    /// Returns `tokens` with every `Macro`/`EndMacro` definition removed
    /// and every invocation of an *already-defined* macro replaced by its
    /// expanded body. Oracle-verified (via a full `-d -o` compile, not just
    /// a `-k` syntax check, which is lenient enough to let a forward
    /// reference pass unnoticed - see docs/architecture/roadmap.md's M5c
    /// notes) that a macro must be defined *before* any invocation of it in
    /// the file, exactly like a genuine single left-to-right preprocessor
    /// pass: `macros_` is populated incrementally as this scan reaches each
    /// `Macro ... EndMacro` definition, not built from a separate, whole-
    /// file pre-pass the way `Sema::collectDataSections`'s own label table
    /// is (DataSection labels, unlike Macro names, *do* support a forward
    /// `Restore` reference - oracle-verified the other way).
    std::vector<Token> expand(const std::vector<Token>& tokens);

private:
    struct MacroDef {
        std::vector<std::string> params; ///< Lowercased parameter names.
        std::vector<Token> body;         ///< Raw, unexpanded tokens between the header and `EndMacro`.
        SourceLoc loc;
        std::string spelling;
    };

    /// Parses one `Macro name[(params)] ... EndMacro` definition starting
    /// at `input[macroKeywordIndex]` (the `Macro` keyword itself), and
    /// registers it into `macros_` - from this point in the scan onward
    /// only, per `expand`'s own doc comment. Returns the index of the
    /// first token after the matching `EndMacro`.
    std::size_t defineMacro(const std::vector<Token>& input, std::size_t macroKeywordIndex);
    /// Recursively walks `input`, replacing each invocation of an already-
    /// registered name in `macros_` with that macro's own (recursively
    /// re-expanded) body, and registering any `Macro` definition reached
    /// along the way (via `defineMacro`) before continuing. `activeExpansion_`
    /// is the set of macro names currently being expanded along *this*
    /// expansion chain - oracle-verified real PB itself detects and rejects
    /// direct/indirect self-recursion ("Endless recursivity detected in the
    /// Macro.") rather than looping forever; this is the same guard,
    /// checked on entry to each nested expansion.
    std::vector<Token> expandTokens(const std::vector<Token>& input);

    /// Resolves `name` (already lowercased) as it would be seen from a
    /// *bare*, unqualified reference at the current scanning position -
    /// `Module::Name` qualified references go through `resolveQualifiedMacroName`
    /// instead (see its own doc comment for why the two need different
    /// rules, not just different lookup syntax). Returns the mangled
    /// `macros_` key to use, or an empty string if `name` doesn't resolve
    /// to any macro visible from here at all (not an error - just means
    /// this identifier isn't a macro invocation, handled as an ordinary
    /// token). Mirrors `Sema::resolveModuleQualifiedName`'s own resolution
    /// order and "sealed box" rule - oracle-verified directly (not assumed
    /// from that precedent alone) that both hold for `Macro` too: own-
    /// module access always sees its own macros first (public or private),
    /// then each `UseModule`'d import's own *public* macros only (a
    /// private import member is deliberately not even a candidate here -
    /// unlike a qualified reference, nothing asked to see a *specific*
    /// private member by name, so there's no access violation to report,
    /// only "not found"), and a top-level macro is only visible from
    /// top-level code - *never* as a fallback from inside a module, even
    /// when no better match exists (confirmed directly: a macro defined
    /// before any module, used unqualified inside one, is a real "not a
    /// function, array, list, map or macro" compile error in real PB).
    std::string resolveBareMacroName(const std::string& name) const;

    /// Resolves an explicit `Module::Name` qualified reference, enforcing
    /// the same public/private rule `Sema::checkModuleAccess` already does
    /// for every other declaration kind - oracle-verified this is a real,
    /// separate error (`"Module item 'Name' is not declared as public."`,
    /// attributed to the expanded macro itself, confirmed via real PB's
    /// own error text) when `moduleLower` exists but `nameLower` isn't one
    /// of its own `DeclareModule`-promised macros, not silently treated as
    /// "not a macro" the way an unresolved *bare* name is - a qualified
    /// reference explicitly asks for one specific member, so there's a
    /// real violation to report when it exists but isn't reachable, unlike
    /// `resolveBareMacroName`'s own "not found" case. Returns the mangled
    /// `macros_` key on success, or an empty string either when
    /// `moduleLower::nameLower` isn't a macro at all (not an error - could
    /// be a qualified variable/procedure/Structure/etc. reference instead,
    /// entirely legitimate) or after reporting the access violation (the
    /// caller treats both the same way: not a macro invocation to expand).
    std::string resolveQualifiedMacroName(const std::string& moduleLower, const std::string& nameLower,
                                           const std::string& nameSpelling, SourceLoc loc);

    DiagnosticEngine& diagnostics_;
    std::unordered_map<std::string, MacroDef> macros_;
    std::unordered_set<std::string> activeExpansion_;
    /// Mirrors `Sema::currentModule_`/`modulePublicMembers_`/`activeImports_`
    /// at the token-stream level - see `expandTokens`'s own doc comment on
    /// why this project's `MacroExpander` needs an independent copy of
    /// this bookkeeping rather than sharing Sema's (a completely separate
    /// pass, running before any AST exists at all).
    std::string currentModule_; ///< Empty at the top level.
    bool insideDeclareModuleSection_ = false; ///< Only meaningful while `currentModule_` is non-empty.
    std::unordered_map<std::string, std::unordered_set<std::string>> modulePublicMacros_;
    std::vector<std::string> activeImports_;
    /// `activeImports_`'s own value from just before entering the current
    /// `Module`'s body - restored on `EndModule`, the same save/restore
    /// `Sema::visitStmt`'s own `Module` case already does, so a `UseModule`
    /// inside one module's body doesn't leak into a sibling module's.
    /// `DeclareModule` needs no matching save/restore: `UseModule`/
    /// `UnuseModule` aren't legal inside one at all (not in this project's
    /// own supported subset), so entering one can never change
    /// `activeImports_` in the first place.
    std::vector<std::string> importsBeforeCurrentModule_;
};

} // namespace easybasic
