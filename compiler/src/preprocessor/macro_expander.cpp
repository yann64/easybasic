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
    // A Macro declared inside a Module's own body belongs to that module's
    // namespace, the identical mangled-key convention Sema's own
    // StructureDecl/InterfaceDecl/etc. cases already use - see
    // resolveBareMacroName's/resolveQualifiedMacroName's own doc comments
    // for the lookup side. Declared inside DeclareModule specifically
    // (not Module) makes it public, tracked in modulePublicMacros_ the
    // same way Sema's own modulePublicMembers_ is - captured here, before
    // mangling, exactly like Sema's own "capture the plain name just
    // before visitStmt mangles it in place" pattern.
    std::string key = name;
    if (!currentModule_.empty()) {
        if (insideDeclareModuleSection_) {
            modulePublicMacros_[currentModule_].insert(name);
        }
        key = currentModule_ + "::" + name;
    }
    macros_[key] = std::move(def); // Last definition for a given name wins - not oracle-verified, a pragmatic default.
    return i;
}

std::string MacroExpander::resolveBareMacroName(const std::string& name) const {
    if (!currentModule_.empty()) {
        std::string ownKey = currentModule_ + "::" + name;
        if (macros_.contains(ownKey)) {
            return ownKey;
        }
    }
    for (const std::string& imported : activeImports_) {
        auto publicIt = modulePublicMacros_.find(imported);
        if (publicIt != modulePublicMacros_.end() && publicIt->second.contains(name)) {
            return imported + "::" + name;
        }
    }
    if (currentModule_.empty() && macros_.contains(name)) {
        return name;
    }
    return "";
}

std::string MacroExpander::resolveQualifiedMacroName(const std::string& moduleLower, const std::string& nameLower,
                                                       const std::string& nameSpelling, SourceLoc loc) {
    std::string key = moduleLower + "::" + nameLower;
    if (!macros_.contains(key)) {
        return ""; // Not a macro at all - could be a qualified variable/procedure/Structure/etc. reference instead.
    }
    if (moduleLower == currentModule_) {
        return key; // A module's own code can always see its own macros, public or private.
    }
    auto publicIt = modulePublicMacros_.find(moduleLower);
    if (publicIt != modulePublicMacros_.end() && publicIt->second.contains(nameLower)) {
        return key;
    }
    diagnostics_.error(loc, "Module item '" + nameSpelling + "' is not declared as public.");
    return "";
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
        // Module/DeclareModule boundary tracking, purely for Macro's own
        // namespacing (see resolveBareMacroName's/resolveQualifiedMacroName's
        // own doc comments) - every one of these tokens is still passed
        // through to `output` unchanged below (this pass has no other
        // reason to care about them at all; the Parser handles the actual
        // Module/DeclareModule grammar itself, completely independently).
        if (input[i].kind == TokenKind::KwDeclareModule || input[i].kind == TokenKind::KwModule) {
            bool isDeclareModule = input[i].kind == TokenKind::KwDeclareModule;
            output.push_back(input[i]);
            ++i;
            if (i < input.size() && input[i].kind == TokenKind::Identifier) {
                currentModule_ = toLower(input[i].text);
                insideDeclareModuleSection_ = isDeclareModule;
                if (!isDeclareModule) {
                    // Saved/restored around Module's own body only -
                    // DeclareModule can't contain UseModule/UnuseModule at
                    // all in this project's own supported subset, so
                    // entering one never needs to save anything.
                    importsBeforeCurrentModule_ = activeImports_;
                }
                output.push_back(input[i]);
                ++i;
            }
            continue;
        }
        if (input[i].kind == TokenKind::KwEndDeclareModule || input[i].kind == TokenKind::KwEndModule) {
            if (input[i].kind == TokenKind::KwEndModule) {
                activeImports_ = importsBeforeCurrentModule_;
            }
            currentModule_.clear();
            insideDeclareModuleSection_ = false;
            output.push_back(input[i]);
            ++i;
            continue;
        }
        if (input[i].kind == TokenKind::KwUseModule || input[i].kind == TokenKind::KwUnuseModule) {
            bool isUse = input[i].kind == TokenKind::KwUseModule;
            output.push_back(input[i]);
            ++i;
            if (i < input.size() && input[i].kind == TokenKind::Identifier) {
                std::string importedLower = toLower(input[i].text);
                if (isUse) {
                    if (std::ranges::find(activeImports_, importedLower) == activeImports_.end()) {
                        activeImports_.push_back(importedLower);
                    }
                } else {
                    std::erase(activeImports_, importedLower);
                }
                output.push_back(input[i]);
                ++i;
            }
            continue;
        }
        const Token& tok = input[i];
        if (tok.kind != TokenKind::Identifier) {
            output.push_back(tok);
            ++i;
            continue;
        }
        std::string lower = toLower(tok.text);
        // A qualified `Module::Name` reference is tried first, exactly
        // like Sema's own resolveModuleQualifiedName does for every other
        // kind - an already-qualified reference is resolved as itself,
        // never re-interpreted as a bare name. `resolveQualifiedMacroName`
        // itself distinguishes "not a macro at all" (empty, silently - this
        // might be a qualified variable/procedure/Structure/etc. reference
        // instead) from "is a macro, but access denied" (empty, after
        // already reporting the real error) - both cases fall through to
        // the plain-token path below identically, since in neither case is
        // there a macro body to substitute.
        std::size_t headEnd = i + 1;
        std::string key;
        std::string invocationSpelling = tok.text; // Widened below for a qualified reference - used only in diagnostics.
        if (i + 2 < input.size() && input[i + 1].kind == TokenKind::ColonColon &&
            input[i + 2].kind == TokenKind::Identifier) {
            const Token& nameTok = input[i + 2];
            key = resolveQualifiedMacroName(lower, toLower(nameTok.text), nameTok.text, tok.loc);
            if (!key.empty()) {
                headEnd = i + 3;
                invocationSpelling = tok.text + "::" + nameTok.text;
            }
        }
        if (key.empty()) {
            key = resolveBareMacroName(lower);
        }
        if (key.empty()) {
            output.push_back(tok);
            ++i;
            continue;
        }
        auto it = macros_.find(key);
        const MacroDef& def = it->second;
        bool hasParens = (headEnd < input.size() && input[headEnd].kind == TokenKind::LParen);

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
        std::size_t afterInvocation = headEnd;
        if (isBareZeroArgInvocation) {
            substituted = def.body;
        } else {
            std::size_t j = headEnd + 1; // past the name (or Module::name) and '('
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
                diagnostics_.error(tok.loc, "Macro '" + invocationSpelling + "' call is missing a closing ')'");
                output.push_back(tok);
                ++i;
                continue;
            }
            afterInvocation = j;
            if (args.size() != def.params.size()) {
                diagnostics_.error(tok.loc, "Macro '" + invocationSpelling + "' expects " +
                                                 std::to_string(def.params.size()) + " argument(s), got " +
                                                 std::to_string(args.size()));
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
        // would otherwise never terminate. Keyed by `key` (the fully
        // mangled identity), not `lower` (just this invocation's own
        // spelling) - two different modules' own same-named macros are
        // genuinely different macros, and must not be confused for a
        // recursion cycle with each other just because they share a bare
        // name.
        if (!activeExpansion_.insert(key).second) {
            diagnostics_.error(tok.loc,
                                "Macro '" + invocationSpelling + "' cannot invoke itself, directly or indirectly");
            i = afterInvocation;
            continue;
        }
        std::vector<Token> expanded = expandTokens(substituted);
        activeExpansion_.erase(key);
        output.insert(output.end(), expanded.begin(), expanded.end());
        i = afterInvocation;
    }
    return output;
}

} // namespace easybasic
