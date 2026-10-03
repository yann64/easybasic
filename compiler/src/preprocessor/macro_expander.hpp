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

    DiagnosticEngine& diagnostics_;
    std::unordered_map<std::string, MacroDef> macros_;
    std::unordered_set<std::string> activeExpansion_;
};

} // namespace easybasic
