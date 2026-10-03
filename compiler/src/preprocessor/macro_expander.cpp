#include "macro_expander.hpp"

#include <algorithm>
#include <cctype>

namespace easybasic {

namespace {
std::string toLower(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}
} // namespace

MacroExpander::MacroExpander(DiagnosticEngine& diagnostics) : diagnostics_(diagnostics) {}

std::vector<Token> MacroExpander::expand(const std::vector<Token>& tokens) { return expandTokens(tokens); }

std::size_t MacroExpander::defineMacro(const std::vector<Token>& input, std::size_t macroKeywordIndex) {
    std::size_t i = macroKeywordIndex;
    SourceLoc defLoc = input[i].loc;
    ++i; // 'Macro'
    if (i >= input.size() || input[i].kind != TokenKind::Identifier) {
        diagnostics_.error(defLoc, "expected a name after 'Macro'");
        return i;
    }
    std::string spelling = input[i].text;
    std::string name = toLower(spelling);
    ++i; // name

    std::vector<std::string> params;
    if (i < input.size() && input[i].kind == TokenKind::LParen) {
        ++i; // '('
        if (i < input.size() && input[i].kind != TokenKind::RParen) {
            while (true) {
                if (i >= input.size() || input[i].kind != TokenKind::Identifier) {
                    diagnostics_.error(defLoc, "expected a parameter name in Macro '" + spelling + "'");
                    break;
                }
                params.push_back(toLower(input[i].text));
                ++i;
                if (i < input.size() && input[i].kind == TokenKind::Comma) {
                    ++i;
                    continue;
                }
                break;
            }
        }
        if (i < input.size() && input[i].kind == TokenKind::RParen) {
            ++i; // ')'
        } else {
            diagnostics_.error(defLoc, "expected ')' to close Macro '" + spelling + "' parameter list");
        }
    }

    std::vector<Token> body;
    while (i < input.size() && input[i].kind != TokenKind::KwEndMacro) {
        body.push_back(input[i]);
        ++i;
    }
    if (i >= input.size()) {
        diagnostics_.error(defLoc, "Macro '" + spelling + "' is missing a matching 'EndMacro'");
    } else {
        ++i; // 'EndMacro'
    }
    // Trim the leading separator (the newline ending the `Macro name(...)`
    // header line itself) and trailing separator (the newline right before
    // `EndMacro`) - these are formatting artifacts of how the *definition*
    // happens to be laid out, not meaningful content, and pasting the
    // leading one verbatim into an invocation site would terminate the
    // invoking statement immediately (e.g. `Debug Double(5)` expanding to
    // `Debug` <newline> `(5) * 2` as two separate, broken statements
    // instead of one). A NewLine/Colon *between* two body statements (as in
    // a multi-statement macro) is left untouched - only the outermost one
    // on each end is trimmed.
    while (!body.empty() && (body.front().kind == TokenKind::NewLine || body.front().kind == TokenKind::Colon)) {
        body.erase(body.begin());
    }
    while (!body.empty() && (body.back().kind == TokenKind::NewLine || body.back().kind == TokenKind::Colon)) {
        body.pop_back();
    }

    MacroDef def;
    def.params = std::move(params);
    def.body = std::move(body);
    def.loc = defLoc;
    def.spelling = spelling;
    macros_[name] = std::move(def); // Last definition for a given name wins - not oracle-verified, a pragmatic default.
    return i;
}

std::vector<Token> MacroExpander::expandTokens(const std::vector<Token>& input) {
    std::vector<Token> output;
    std::size_t i = 0;
    while (i < input.size()) {
        if (input[i].kind == TokenKind::KwMacro) {
            // Registers into macros_ immediately - available to any
            // invocation from this point in the scan onward only (see
            // expand's own doc comment on why this can't be a separate,
            // whole-file collection pass the way Sema::collectDataSections
            // is for DataSection labels). Produces no output tokens of its
            // own at this position.
            i = defineMacro(input, i);
            continue;
        }
        const Token& tok = input[i];
        if (tok.kind != TokenKind::Identifier) {
            output.push_back(tok);
            ++i;
            continue;
        }
        std::string lower = toLower(tok.text);
        auto it = macros_.find(lower);
        if (it == macros_.end()) {
            output.push_back(tok);
            ++i;
            continue;
        }
        const MacroDef& def = it->second;
        bool hasParens = (i + 1 < input.size() && input[i + 1].kind == TokenKind::LParen);

        // Oracle-verified: a zero-parameter macro is invoked *bare*, with no
        // parens at all - `Greet()` is a syntax error, not a zero-arg call
        // the way it would be for a Procedure. A macro WITH parameters not
        // immediately followed by '(' (and a zero-param macro that IS
        // followed by '(') both fall through to the plain-token path below,
        // left for the Parser/Sema to react to as an ordinary identifier -
        // not byte-for-byte identical to real PB's own diagnostic wording,
        // but a clean compile error either way.
        bool isBareZeroArgInvocation = def.params.empty() && !hasParens;
        bool isCallStyleInvocation = !def.params.empty() && hasParens;
        if (!isBareZeroArgInvocation && !isCallStyleInvocation) {
            output.push_back(tok);
            ++i;
            continue;
        }

        std::vector<Token> substituted;
        std::size_t afterInvocation = i + 1;
        if (isBareZeroArgInvocation) {
            substituted = def.body;
        } else {
            std::size_t j = i + 2; // past the name and '('
            std::vector<std::vector<Token>> args;
            std::vector<Token> currentArg;
            int depth = 1;
            while (j < input.size() && depth > 0) {
                const Token& t = input[j];
                if (t.kind == TokenKind::LParen) {
                    ++depth;
                    currentArg.push_back(t);
                } else if (t.kind == TokenKind::RParen) {
                    --depth;
                    if (depth > 0) {
                        currentArg.push_back(t);
                    }
                } else if (t.kind == TokenKind::Comma && depth == 1) {
                    args.push_back(std::move(currentArg));
                    currentArg.clear();
                } else {
                    currentArg.push_back(t);
                }
                ++j;
            }
            args.push_back(std::move(currentArg));
            if (depth != 0) {
                diagnostics_.error(tok.loc, "Macro '" + tok.text + "' call is missing a closing ')'");
                output.push_back(tok);
                ++i;
                continue;
            }
            afterInvocation = j;
            if (args.size() != def.params.size()) {
                diagnostics_.error(tok.loc, "Macro '" + tok.text + "' expects " + std::to_string(def.params.size()) +
                                                 " argument(s), got " + std::to_string(args.size()));
                i = afterInvocation;
                continue;
            }
            for (const Token& bodyTok : def.body) {
                if (bodyTok.kind == TokenKind::Identifier) {
                    std::string bodyLower = toLower(bodyTok.text);
                    auto paramIt = std::ranges::find(def.params, bodyLower);
                    if (paramIt != def.params.end()) {
                        auto paramIndex = static_cast<std::size_t>(std::distance(def.params.begin(), paramIt));
                        const std::vector<Token>& argTokens = args[paramIndex];
                        substituted.insert(substituted.end(), argTokens.begin(), argTokens.end());
                        continue;
                    }
                }
                substituted.push_back(bodyTok);
            }
        }

        // Oracle-verified: real PB itself detects and rejects direct/
        // indirect self-recursion ("Endless recursivity detected in the
        // Macro.") rather than expanding forever - `activeExpansion_` is
        // the set of macro names currently being expanded along *this*
        // chain, so a cycle of any length (not just immediate self-
        // recursion) is caught here, on entry to the nested expansion that
        // would otherwise never terminate.
        if (!activeExpansion_.insert(lower).second) {
            diagnostics_.error(tok.loc,
                                "Macro '" + tok.text + "' cannot invoke itself, directly or indirectly");
            i = afterInvocation;
            continue;
        }
        std::vector<Token> expanded = expandTokens(substituted);
        activeExpansion_.erase(lower);
        output.insert(output.end(), expanded.begin(), expanded.end());
        i = afterInvocation;
    }
    return output;
}

} // namespace easybasic
