#include "sema.hpp"

namespace easybasic {

ValueKind familyOf(TypeSuffix suffix) {
    switch (suffix) {
        case TypeSuffix::Float:
        case TypeSuffix::Double:
            return ValueKind::FloatFamily;
        case TypeSuffix::String:
            return ValueKind::StringFamily;
        default:
            return ValueKind::IntegerFamily;
    }
}

Sema::Sema(DiagnosticEngine& diagnostics) : diagnostics_(diagnostics) {
    registerStringLibBuiltins();
    registerMathLibBuiltins();
    registerMemoryLibBuiltins();
    registerBuiltinConstants();
}

void Sema::registerStringLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // `Mid`'s optional `count` and `StrF`'s optional `decimals` reuse the
    // exact same "fewer args than paramSuffixes.size() means the trailing
    // ones were defaulted" mechanism a real Procedure's own default-valued
    // parameters already use - Codegen relies on the runtime function
    // itself supplying the C++-level default (see stringlib.hpp).
    static const std::vector<Signature> signatures = {
        {"len", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        {"left", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer}, 2},
        {"right", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer}, 2},
        {"mid", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"ucase", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"lcase", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"trim", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"ltrim", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"rtrim", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"str", TypeSuffix::String, {TypeSuffix::Integer}, 1},
        {"val", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        {"strf", TypeSuffix::String, {TypeSuffix::Float, TypeSuffix::Integer}, 1},
        {"valf", TypeSuffix::Float, {TypeSuffix::String}, 1},
        {"chr", TypeSuffix::String, {TypeSuffix::Integer}, 1},
        {"asc", TypeSuffix::Integer, {TypeSuffix::String}, 1},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isStringLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "len", "left", "right", "mid", "ucase", "lcase", "trim", "ltrim",
        "rtrim", "str", "val", "strf", "valf", "chr", "asc",
    };
    return names.contains(lowerName);
}

void Sema::registerMathLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // `Round`'s `mode` and `Random`'s optional `min` both reuse the same
    // "fewer call-site args than paramSuffixes.size()" mechanism as a real
    // Procedure's own defaulted trailing parameters (see
    // registerStringLibBuiltins's identical note on `Mid`/`StrF`) - except
    // `Round` has no real default of its own (its mode argument is always
    // required); only `Random`'s `min` is genuinely optional.
    static const std::vector<Signature> signatures = {
        {"abs", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"sqr", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"pow", TypeSuffix::Double, {TypeSuffix::Double, TypeSuffix::Double}, 2},
        {"sin", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"cos", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"tan", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"asin", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"acos", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"atan", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"atan2", TypeSuffix::Double, {TypeSuffix::Double, TypeSuffix::Double}, 2},
        {"exp", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"log", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"log10", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"round", TypeSuffix::Double, {TypeSuffix::Double, TypeSuffix::Integer}, 2},
        {"int", TypeSuffix::Integer, {TypeSuffix::Double}, 1},
        {"random", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 1},
        {"randomseed", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isMathLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "abs", "sqr", "pow", "sin", "cos", "tan", "asin", "acos", "atan",
        "atan2", "exp", "log", "log10", "round", "int", "random", "randomseed",
    };
    return names.contains(lowerName);
}

void Sema::registerMemoryLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // Every Peek* takes one Integer address and returns the matching
    // primitive type; every Poke* takes that same address plus the value to
    // write, returning an Integer (see registerMemoryLibBuiltins's own
    // runtime-side doc comment on why that return value is a placeholder,
    // not an oracle-verified one). `PeekS`'s optional length argument reuses
    // the same "fewer call-site args than paramSuffixes.size()" default-
    // parameter mechanism the String/Math libraries already use.
    static const std::vector<Signature> signatures = {
        {"peekb", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peeka", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekc", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekw", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peeku", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekl", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekq", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekf", TypeSuffix::Float, {TypeSuffix::Integer}, 1},
        {"peekd", TypeSuffix::Double, {TypeSuffix::Integer}, 1},
        {"peeks", TypeSuffix::String, {TypeSuffix::Integer, TypeSuffix::Integer}, 1},
        {"pokeb", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokea", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokec", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokew", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokeu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokel", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokeq", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokef", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Float}, 2},
        {"poked", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Double}, 2},
        {"pokes", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isMemoryLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "peekb", "peeka", "peekc", "peekw", "peeku", "peekl", "peekq", "peekf", "peekd", "peeks",
        "pokeb", "pokea", "pokec", "pokew", "pokeu", "pokel", "pokeq", "pokef", "poked", "pokes",
    };
    return names.contains(lowerName);
}

namespace {
const std::unordered_map<std::string, std::int64_t>& builtinConstantTable() {
    // Oracle-verified values (`pbcompilerc`): `#PB_Round_Down` = 0,
    // `#PB_Round_Up` = 1, `#PB_Round_Nearest` = 2. A fourth, plausible-
    // sounding `#PB_Round_Truncate` does NOT exist in real PB ("Constant
    // not found").
    static const std::unordered_map<std::string, std::int64_t> table = {
        {"pb_round_down", 0},
        {"pb_round_up", 1},
        {"pb_round_nearest", 2},
    };
    return table;
}
} // namespace

void Sema::registerBuiltinConstants() {
    for (const auto& [name, value] : builtinConstantTable()) {
        constants_[name] = TypeSuffix::Integer;
    }
}

std::optional<std::int64_t> Sema::builtinConstantValue(const std::string& lowerName) {
    const auto& table = builtinConstantTable();
    auto it = table.find(lowerName);
    if (it == table.end()) {
        return std::nullopt;
    }
    return it->second;
}

void Sema::declare(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                    SourceLoc loc) {
    auto it = symbols_.find(lowerName);
    if (it == symbols_.end()) {
        symbols_.emplace(lowerName, suffix);
        order_.emplace_back(lowerName, suffix);
        return;
    }
    // Re-Define'ing (or implicitly re-touching) the same name with a
    // different suffix is a real PB error ("Redefinition of ..."); M0 keeps
    // this simple and just flags a mismatch rather than modeling PB's exact
    // diagnostic wording yet.
    if (it->second != suffix && suffix != TypeSuffix::None) {
        diagnostics_.error(loc, "'" + spelling + "' redeclared with a different type suffix");
    }
}

void Sema::declareImplicit(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                            SourceLoc loc) {
    // Oracle-verified error text (pbcompilerc, `EnableExplicit` + an
    // undeclared variable): "With 'EnableExplicit', variables have to be
    // declared: x." `declare()` still runs regardless so Codegen never sees
    // a symbol with no recorded type - the driver already stops before
    // Codegen once any error is recorded (see main.cpp), so this is a
    // defensive fallback, not a way to let the violation silently through.
    if (explicitEnabled_ && !symbols_.contains(lowerName)) {
        diagnostics_.error(loc, "With 'EnableExplicit', variables have to be declared: " + spelling + ".");
    }
    declare(lowerName, spelling, suffix, loc);
}

void Sema::declareConst(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                         SourceLoc loc) {
    if (constants_.contains(lowerName)) {
        diagnostics_.error(loc, "'#" + spelling + "' is already declared as a constant");
        return;
    }
    constants_.emplace(lowerName, suffix);
    constOrder_.emplace_back(lowerName, suffix);
}

bool Sema::analyze(ast::Module& module) {
    visitBlock(module.statements);
    // Every `Declare` must eventually be fulfilled by a matching
    // `Procedure` (oracle-verified: "The procedure 'name()' has been
    // declared but not defined." otherwise) - checked only after the whole
    // module has been walked, since the fulfilling `Procedure` may appear
    // anywhere later in the file.
    for (const auto& [lowerName, spellingAndLoc] : declaredNotDefined_) {
        diagnostics_.error(spellingAndLoc.second,
                            "The procedure '" + spellingAndLoc.first + "()' has been declared but not defined.");
    }
    return !diagnostics_.hasErrors();
}

void Sema::visitBlock(ast::Block& block) {
    for (auto& stmt : block) {
        visitStmt(*stmt);
    }
}

void Sema::visitNestedBlock(ast::Block& block) {
    ++controlFlowDepth_;
    visitBlock(block);
    --controlFlowDepth_;
}

void Sema::visitStmt(ast::Stmt& stmt) {
    switch (stmt.kind) {
        case ast::StmtKind::Define: {
            auto& def = static_cast<ast::DefineStmt&>(stmt);
            for (auto& decl : def.declarators) {
                if (decl.isPointer) {
                    // A pointer variable's own storage is always a plain
                    // Integer address (see ast::DefineStmt::Declarator's own
                    // doc comment); `decl.suffix`/`structTypeName` describe
                    // what it *points at*, recorded separately below.
                    declare(decl.name, decl.spelling, TypeSuffix::Integer, def.loc);
                    pointerPointeeType_[decl.name] = resolvePointeeType(decl.suffix, decl.structTypeName,
                                                                         decl.structTypeSpelling, def.loc);
                    if (decl.init) {
                        visitExpr(*decl.init);
                        checkAssignable(decl.spelling, TypeSuffix::Integer, *decl.init, def.loc);
                    }
                    if (def.isGlobal) {
                        globalNames_.insert(decl.name);
                    }
                    continue;
                }
                TypeSuffix suffix = decl.suffix == TypeSuffix::None ? TypeSuffix::Integer : decl.suffix;
                declare(decl.name, decl.spelling, suffix, def.loc);
                if (suffix == TypeSuffix::Struct) {
                    if (!structures_.contains(decl.structTypeName)) {
                        diagnostics_.error(def.loc, "'" + decl.structTypeSpelling + "' is not a declared Structure");
                    }
                    varStructType_[decl.name] = decl.structTypeName;
                    if (decl.init) {
                        diagnostics_.error(def.loc, "Structure-typed variables cannot have an initializer");
                    }
                } else if (decl.init) {
                    visitExpr(*decl.init);
                    checkAssignable(decl.spelling, suffix, *decl.init, def.loc);
                }
                if (def.isGlobal) {
                    globalNames_.insert(decl.name);
                }
            }
            break;
        }
        case ast::StmtKind::Shared: {
            auto& shared = static_cast<ast::SharedStmt&>(stmt);
            for (auto& name : shared.names) {
                if (outerScopeForShared_ != nullptr) {
                    bringIntoScope(*outerScopeForShared_, name.name, name.spelling, shared.loc, "Shared");
                } else {
                    diagnostics_.error(shared.loc, "'Shared' is only valid inside a procedure");
                }
            }
            break;
        }
        case ast::StmtKind::Dim: {
            auto& dim = static_cast<ast::DimStmt&>(stmt);
            for (auto& size : dim.dimensionSizes) {
                visitExpr(*size);
            }
            if (arrays_.contains(dim.name)) {
                diagnostics_.error(dim.loc, "'" + dim.spelling + "' is already declared as an array");
                break;
            }
            ArrayInfo info;
            info.elementSuffix = dim.suffix == TypeSuffix::None ? TypeSuffix::Integer : dim.suffix;
            info.dimensionCount = static_cast<int>(dim.dimensionSizes.size());
            if (info.elementSuffix == TypeSuffix::Struct) {
                if (!structures_.contains(dim.structTypeName)) {
                    diagnostics_.error(dim.loc, "'" + dim.structTypeSpelling + "' is not a declared Structure");
                }
                info.elementStructName = dim.structTypeName;
            }
            arrays_.emplace(dim.name, info);
            arrayOrder_.emplace_back(dim.name, info);
            break;
        }
        case ast::StmtKind::NewList: {
            auto& newList = static_cast<ast::NewListStmt&>(stmt);
            if (lists_.contains(newList.name)) {
                diagnostics_.error(newList.loc, "'" + newList.spelling + "' is already declared as a List");
                break;
            }
            ListInfo info;
            info.elementSuffix = newList.suffix == TypeSuffix::None ? TypeSuffix::Integer : newList.suffix;
            if (info.elementSuffix == TypeSuffix::Struct) {
                if (!structures_.contains(newList.structTypeName)) {
                    diagnostics_.error(newList.loc, "'" + newList.structTypeSpelling + "' is not a declared Structure");
                }
                info.elementStructName = newList.structTypeName;
            }
            lists_.emplace(newList.name, info);
            listOrder_.emplace_back(newList.name, info);
            break;
        }
        case ast::StmtKind::NewMap: {
            auto& newMap = static_cast<ast::NewMapStmt&>(stmt);
            if (maps_.contains(newMap.name)) {
                diagnostics_.error(newMap.loc, "'" + newMap.spelling + "' is already declared as a Map");
                break;
            }
            MapInfo info;
            info.elementSuffix = newMap.suffix == TypeSuffix::None ? TypeSuffix::Integer : newMap.suffix;
            if (info.elementSuffix == TypeSuffix::Struct) {
                if (!structures_.contains(newMap.structTypeName)) {
                    diagnostics_.error(newMap.loc, "'" + newMap.structTypeSpelling + "' is not a declared Structure");
                }
                info.elementStructName = newMap.structTypeName;
            }
            maps_.emplace(newMap.name, info);
            mapOrder_.emplace_back(newMap.name, info);
            break;
        }
        case ast::StmtKind::ForEach: {
            auto& forEach = static_cast<ast::ForEachStmt&>(stmt);
            if (listInfo(forEach.name) == nullptr && mapInfo(forEach.name) == nullptr) {
                diagnostics_.error(forEach.loc, "'" + forEach.spelling + "' is not a declared List or Map");
            }
            visitNestedBlock(forEach.body);
            break;
        }
        case ast::StmtKind::IndexAssign: {
            auto& indexAssign = static_cast<ast::IndexAssignStmt&>(stmt);
            for (auto& idx : indexAssign.indices) {
                visitExpr(*idx);
            }
            visitExpr(*indexAssign.value);
            if (indexAssign.indices.empty()) {
                if (const ListInfo* listInfoPtr = listInfo(indexAssign.name)) {
                    if (listInfoPtr->elementSuffix == TypeSuffix::Struct) {
                        diagnostics_.error(indexAssign.loc,
                                            "cannot assign directly to '" + indexAssign.spelling +
                                                "()', a Structure element - assign to one of its own fields instead");
                    } else {
                        checkAssignable(indexAssign.spelling, listInfoPtr->elementSuffix, *indexAssign.value,
                                         indexAssign.loc);
                    }
                    break;
                }
                // `name() = expr` is also how a Map's *current* element is
                // set (M3f) - identical shape to a List's own zero-arg form.
                if (const MapInfo* mapInfoPtr = mapInfo(indexAssign.name)) {
                    if (mapInfoPtr->elementSuffix == TypeSuffix::Struct) {
                        diagnostics_.error(indexAssign.loc,
                                            "cannot assign directly to '" + indexAssign.spelling +
                                                "()', a Structure element - assign to one of its own fields instead");
                    } else {
                        checkAssignable(indexAssign.spelling, mapInfoPtr->elementSuffix, *indexAssign.value,
                                         indexAssign.loc);
                    }
                    break;
                }
            }
            if (indexAssign.indices.size() == 1) {
                if (const MapInfo* mapInfoPtr = mapInfo(indexAssign.name)) {
                    checkMapKey(*indexAssign.indices.front(), indexAssign.loc);
                    if (mapInfoPtr->elementSuffix == TypeSuffix::Struct) {
                        diagnostics_.error(indexAssign.loc,
                                            "cannot assign directly to '" + indexAssign.spelling +
                                                "(...)', a Structure element - assign to one of its own fields instead");
                    } else {
                        checkAssignable(indexAssign.spelling, mapInfoPtr->elementSuffix, *indexAssign.value,
                                         indexAssign.loc);
                    }
                    break;
                }
            }
            const ArrayInfo* info = arrayInfo(indexAssign.name);
            if (info == nullptr) {
                diagnostics_.error(indexAssign.loc, "'" + indexAssign.spelling + "' is not a declared array");
                break;
            }
            if (std::cmp_not_equal(indexAssign.indices.size(), info->dimensionCount)) {
                diagnostics_.error(indexAssign.loc, "'" + indexAssign.spelling +
                                                         "' indexed with the wrong number of dimensions");
            }
            if (info->elementSuffix == TypeSuffix::Struct) {
                diagnostics_.error(indexAssign.loc, "cannot assign directly to '" + indexAssign.spelling +
                                                         "(...)', a Structure element - assign to one of its "
                                                         "own fields instead");
            } else {
                checkAssignable(indexAssign.spelling, info->elementSuffix, *indexAssign.value, indexAssign.loc);
            }
            break;
        }
        case ast::StmtKind::Assign: {
            auto& assign = static_cast<ast::AssignStmt&>(stmt);
            TypeSuffix suffix = assign.suffix == TypeSuffix::None ? typeOf(assign.name) : assign.suffix;
            declareImplicit(assign.name, assign.spelling, suffix, assign.loc);
            visitExpr(*assign.value);
            checkAssignable(assign.spelling, symbols_[assign.name], *assign.value, assign.loc);
            break;
        }
        case ast::StmtKind::Debug: {
            auto& dbg = static_cast<ast::DebugStmt&>(stmt);
            visitExpr(*dbg.value);
            break;
        }
        case ast::StmtKind::If: {
            auto& ifStmt = static_cast<ast::IfStmt&>(stmt);
            for (auto& branch : ifStmt.branches) {
                if (branch.condition) {
                    visitCondition(*branch.condition);
                }
                visitNestedBlock(branch.body);
            }
            break;
        }
        case ast::StmtKind::Select: {
            auto& sel = static_cast<ast::SelectStmt&>(stmt);
            visitExpr(*sel.selector);
            for (auto& branch : sel.cases) {
                for (auto& value : branch.values) {
                    visitExpr(*value);
                }
                visitNestedBlock(branch.body);
            }
            break;
        }
        case ast::StmtKind::For: {
            auto& forStmt = static_cast<ast::ForStmt&>(stmt);
            TypeSuffix suffix = forStmt.suffix == TypeSuffix::None ? typeOf(forStmt.varName) : forStmt.suffix;
            visitExpr(*forStmt.from);
            visitExpr(*forStmt.to);
            if (forStmt.step) {
                visitExpr(*forStmt.step);
            }
            declareImplicit(forStmt.varName, forStmt.varSpelling, suffix, forStmt.loc);
            visitNestedBlock(forStmt.body);
            break;
        }
        case ast::StmtKind::While: {
            auto& whileStmt = static_cast<ast::WhileStmt&>(stmt);
            visitCondition(*whileStmt.condition);
            visitNestedBlock(whileStmt.body);
            break;
        }
        case ast::StmtKind::Repeat: {
            auto& repeatStmt = static_cast<ast::RepeatStmt&>(stmt);
            visitNestedBlock(repeatStmt.body);
            if (repeatStmt.untilCondition) {
                visitCondition(*repeatStmt.untilCondition);
            }
            break;
        }
        case ast::StmtKind::Break:
        case ast::StmtKind::Continue:
            break; // Nothing to resolve; "outside any loop" checking is a follow-up, not M1-blocking.
        case ast::StmtKind::EnableExplicit:
            explicitEnabled_ = true;
            break;
        case ast::StmtKind::ConstDecl: {
            auto& constDecl = static_cast<ast::ConstDeclStmt&>(stmt);
            visitExpr(*constDecl.value);
            ValueKind family = familyOfExpr(*constDecl.value);
            TypeSuffix suffix = TypeSuffix::Integer;
            if (family == ValueKind::FloatFamily) {
                suffix = TypeSuffix::Double;
            } else if (family == ValueKind::StringFamily) {
                suffix = TypeSuffix::String;
            }
            declareConst(constDecl.name, constDecl.spelling, suffix, constDecl.loc);
            break;
        }
        case ast::StmtKind::Enumeration: {
            auto& enumStmt = static_cast<ast::EnumerationStmt&>(stmt);
            for (auto& member : enumStmt.members) {
                if (member.explicitValue) {
                    visitExpr(*member.explicitValue);
                }
                declareConst(member.name, member.spelling, TypeSuffix::Integer, enumStmt.loc);
            }
            break;
        }
        case ast::StmtKind::StructureDecl: {
            auto& structDecl = static_cast<ast::StructureDeclStmt&>(stmt);
            if (structures_.contains(structDecl.name)) {
                diagnostics_.error(structDecl.loc,
                                    "'" + structDecl.spelling + "' is already declared as a Structure");
                break;
            }
            StructureInfo info;
            for (auto& field : structDecl.fields) {
                FieldInfo fieldInfo;
                fieldInfo.name = field.name;
                fieldInfo.spelling = field.spelling;
                fieldInfo.suffix = field.suffix == TypeSuffix::None ? TypeSuffix::Integer : field.suffix;
                if (fieldInfo.suffix == TypeSuffix::Struct) {
                    // Oracle-verified: a field can name another, previously
                    // declared Structure (nesting) - `structures_` only
                    // contains Structures already fully processed by this
                    // point in source order, so this naturally enforces
                    // PB's own declare-before-use rule for free.
                    if (!structures_.contains(field.structTypeName)) {
                        diagnostics_.error(structDecl.loc,
                                            "'" + field.structTypeSpelling + "' is not a declared Structure");
                    }
                    fieldInfo.structTypeName = field.structTypeName;
                }
                info.fields.push_back(std::move(fieldInfo));
            }
            structures_.emplace(structDecl.name, info);
            structureOrder_.emplace_back(structDecl.name, info);
            break;
        }
        case ast::StmtKind::FieldAssign: {
            auto& fieldAssign = static_cast<ast::FieldAssignStmt&>(stmt);
            visitExpr(*fieldAssign.target);
            visitExpr(*fieldAssign.value);
            ResolvedType targetType = resolveType(*fieldAssign.target);
            const std::string& targetSpelling = static_cast<ast::FieldAccessExpr&>(*fieldAssign.target).fieldSpelling;
            if (targetType.suffix == TypeSuffix::Struct) {
                // Whole-Structure assignment (`r\topLeft = ...` where
                // topLeft is itself a nested Structure, not a leaf
                // primitive field) isn't supported - only leaf primitive
                // fields can be assigned to (`r\topLeft\x = ...` instead).
                diagnostics_.error(fieldAssign.loc,
                                    "cannot assign directly to '" + targetSpelling +
                                        "', which is a Structure - assign to one of its own fields instead");
            } else {
                checkAssignable(targetSpelling, targetType.suffix, *fieldAssign.value, fieldAssign.loc);
            }
            break;
        }
        case ast::StmtKind::Declare: {
            auto& decl = static_cast<ast::DeclareStmt&>(stmt);
            if (procedures_.contains(decl.name)) {
                // Either a genuine duplicate `Declare`, or one appearing
                // after the real `Procedure` already fully defined it -
                // neither is the intended "forward-declare, define later"
                // usage this exists for, so it's simplest to just flag it
                // the same way any other redeclaration is flagged rather
                // than modeling PB's own (unverified) behavior for this
                // unusual ordering.
                diagnostics_.error(decl.loc, "'" + decl.spelling + "' is already declared as a procedure");
                break;
            }
            ProcedureInfo info;
            info.returnSuffix = decl.returnSuffix == TypeSuffix::None ? TypeSuffix::Integer : decl.returnSuffix;
            for (auto& param : decl.params) {
                info.paramSuffixes.push_back(param.suffix == TypeSuffix::None ? TypeSuffix::Integer : param.suffix);
                if (!param.hasDefault) {
                    ++info.requiredParamCount;
                }
            }
            procedures_[decl.name] = info;
            declaredNotDefined_[decl.name] = {decl.spelling, decl.loc};
            break;
        }
        case ast::StmtKind::ProcedureDecl: {
            auto& proc = static_cast<ast::ProcedureDeclStmt&>(stmt);
            // Oracle-verified: PB rejects a Procedure declared in either of
            // these positions outright (distinct wording for each), rather
            // than accepting it and doing something PB-specific with it - so
            // this reports the error and stops, matching that (unlike a
            // recoverable issue like a redeclaration, just below, there is
            // no sensible "continue processing anyway" for a Procedure whose
            // very presence here is illegal).
            if (insideProcedure_) {
                diagnostics_.error(proc.loc, "Can't define a procedure inside another procedure.");
                break;
            }
            if (controlFlowDepth_ > 0) {
                diagnostics_.error(proc.loc,
                                    "A procedure can't be declared inside an If, Repeat, While or For.");
                break;
            }
            TypeSuffix returnSuffix = proc.returnSuffix == TypeSuffix::None ? TypeSuffix::Integer : proc.returnSuffix;

            // Fulfilling an earlier `Declare` looks identical to a plain
            // redeclaration (`procedures_` already contains the name
            // either way) - `declaredNotDefined_` is what tells the two
            // apart, so only a genuine second full definition is flagged
            // here.
            bool fulfillsDeclare = declaredNotDefined_.contains(proc.name);
            if (procedures_.contains(proc.name) && !fulfillsDeclare) {
                diagnostics_.error(proc.loc, "'" + proc.spelling + "' is already declared as a procedure");
            }

            ProcedureInfo info;
            info.returnSuffix = returnSuffix;
            bool sawDefault = false;
            for (auto& param : proc.params) {
                // A pointer parameter's own runtime representation is always
                // a plain Integer address, exactly like a pointer Define
                // (see the Define case above) - `param.suffix` here
                // describes the pointee, not the parameter's own storage.
                TypeSuffix paramSuffix = TypeSuffix::Integer;
                if (!param.isPointer && param.suffix != TypeSuffix::None) {
                    paramSuffix = param.suffix;
                }
                info.paramSuffixes.push_back(paramSuffix);
                if (param.defaultValue) {
                    sawDefault = true;
                } else if (sawDefault) {
                    diagnostics_.error(proc.loc,
                                        "'" + param.spelling + "': a required parameter cannot follow one with a default value");
                } else {
                    ++info.requiredParamCount;
                }
            }
            if (fulfillsDeclare) {
                // Oracle-verified: the real Procedure must match the
                // Declare's promised signature exactly (return type AND
                // every parameter type, not just arity) - "Declare doesn't
                // match with real Procedure." otherwise.
                const ProcedureInfo& declared = procedures_[proc.name];
                if (declared.returnSuffix != info.returnSuffix || declared.paramSuffixes != info.paramSuffixes) {
                    diagnostics_.error(proc.loc, "Declare doesn't match with real Procedure.");
                }
                declaredNotDefined_.erase(proc.name);
            }
            // Registered BEFORE the body is visited - oracle-verified PB
            // requires this for self-recursion to resolve at all (see the
            // struct's own doc comment), so this mirrors real PB exactly.
            procedures_[proc.name] = info;

            // Default values are evaluated in the OUTER scope, not against
            // the procedure's own (not-yet-entered) locals.
            for (auto& param : proc.params) {
                if (param.defaultValue) {
                    visitExpr(*param.defaultValue);
                }
            }

            // Swap to a fresh, ISOLATED local scope - oracle-verified: a
            // procedure body cannot see outer variables at all by default
            // (see ast::ProcedureDeclStmt's own doc comment). PB procedures
            // never nest, so a simple save/restore is enough, not a stack.
            auto savedSymbols = std::move(symbols_);
            auto savedOrder = std::move(order_);
            symbols_ = {};
            order_ = {};

            for (auto& param : proc.params) {
                if (param.isPointer) {
                    declare(param.name, param.spelling, TypeSuffix::Integer, proc.loc);
                    pointerPointeeType_[param.name] = resolvePointeeType(param.suffix, param.structTypeName,
                                                                         param.structTypeSpelling, proc.loc);
                    continue;
                }
                TypeSuffix paramSuffix = param.suffix == TypeSuffix::None ? TypeSuffix::Integer : param.suffix;
                declare(param.name, param.spelling, paramSuffix, proc.loc);
            }

            // `Global` variables are auto-visible in every procedure with no
            // `Shared` needed (oracle-verified) - pre-populate for type
            // resolution, but never overwrite a same-named parameter, and
            // deliberately without adding to `order_` (see
            // Sema::bringIntoScope's own doc comment for why).
            for (const auto& globalName : globalNames_) {
                if (symbols_.contains(globalName)) {
                    continue;
                }
                auto globalIt = savedSymbols.find(globalName);
                if (globalIt != savedSymbols.end()) {
                    symbols_[globalName] = globalIt->second;
                }
            }

            bool savedInside = insideProcedure_;
            TypeSuffix savedReturnSuffix = currentProcedureReturnSuffix_;
            const std::unordered_map<std::string, TypeSuffix>* savedOuterForShared = outerScopeForShared_;
            insideProcedure_ = true;
            currentProcedureReturnSuffix_ = returnSuffix;
            outerScopeForShared_ = &savedSymbols;

            visitBlock(proc.body);

            insideProcedure_ = savedInside;
            currentProcedureReturnSuffix_ = savedReturnSuffix;
            outerScopeForShared_ = savedOuterForShared;

            procedures_[proc.name].locals = order_; // params first, then any body-internal locals

            symbols_ = std::move(savedSymbols);
            order_ = std::move(savedOrder);
            break;
        }
        case ast::StmtKind::ProcedureReturn: {
            auto& ret = static_cast<ast::ProcedureReturnStmt&>(stmt);
            if (ret.value) {
                visitExpr(*ret.value);
                checkAssignable("return value", currentProcedureReturnSuffix_, *ret.value, ret.loc);
            }
            break;
        }
        case ast::StmtKind::ExprStmt: {
            auto& exprStmt = static_cast<ast::ExprStmt&>(stmt);
            visitExpr(*exprStmt.expr);
            break;
        }
    }
}

void Sema::visitExpr(ast::Expr& expr) {
    switch (expr.kind) {
        case ast::ExprKind::VarRef: {
            auto& ref = static_cast<ast::VarRefExpr&>(expr);
            if (!symbols_.contains(ref.name)) {
                // Implicit read of a never-assigned name: real PB gives it
                // type Integer and value 0 (or errors under EnableExplicit).
                declareImplicit(ref.name, ref.spelling, TypeSuffix::Integer, ref.loc);
            }
            break;
        }
        case ast::ExprKind::ConstRef: {
            auto& ref = static_cast<ast::ConstRefExpr&>(expr);
            if (!constants_.contains(ref.name)) {
                diagnostics_.error(ref.loc, "'#" + ref.spelling + "' is not declared");
                declareConst(ref.name, ref.spelling, TypeSuffix::Integer, ref.loc); // recovery fallback
            }
            break;
        }
        case ast::ExprKind::Binary: {
            auto& bin = static_cast<ast::BinaryExpr&>(expr);
            visitExpr(*bin.lhs);
            visitExpr(*bin.rhs);
            break;
        }
        case ast::ExprKind::Unary: {
            auto& un = static_cast<ast::UnaryExpr&>(expr);
            visitExpr(*un.operand);
            break;
        }
        case ast::ExprKind::Call: {
            auto& call = static_cast<ast::CallExpr&>(expr);
            // The handful of pointer/memory built-ins are recognized by name
            // before anything else - `AllocateStructure`'s sole argument in
            // particular must NOT fall through to the ordinary call/array
            // handling below (see visitPointerBuiltinCall's own doc comment).
            if (visitPointerBuiltinCall(call)) {
                break;
            }
            if (visitListBuiltinCall(call)) {
                break;
            }
            if (visitMapBuiltinCall(call)) {
                break;
            }
            if (listInfo(call.name) != nullptr) {
                // `name()` reads the List's current element (M3e) - no args
                // to visit (see Sema::ListInfo's own doc comment).
                if (!call.args.empty()) {
                    diagnostics_.error(call.loc, "'" + call.spelling + "' is a List and takes no arguments here");
                }
                break;
            }
            if (mapInfo(call.name) != nullptr) {
                // `name()` reads the Map's current element; `name(key)`
                // reads (and auto-creates) by key (M3f - see Sema::MapInfo's
                // own doc comment).
                if (call.args.size() == 1) {
                    visitExpr(*call.args.front());
                    checkMapKey(*call.args.front(), call.loc);
                } else if (!call.args.empty()) {
                    diagnostics_.error(call.loc, "'" + call.spelling + "' takes at most one key argument");
                }
                break;
            }
            // `Name(args)` is ambiguous with a call at parse time - a
            // `Dim`'d name always means an array read here (see ArrayInfo's
            // own doc comment).
            if (const ArrayInfo* info = arrayInfo(call.name)) {
                if (std::cmp_not_equal(call.args.size(), info->dimensionCount)) {
                    diagnostics_.error(call.loc,
                                        "'" + call.spelling + "' indexed with the wrong number of dimensions");
                }
                for (auto& idx : call.args) {
                    visitExpr(*idx);
                }
            } else {
                visitCall(call);
            }
            break;
        }
        case ast::ExprKind::FieldAccess: {
            auto& access = static_cast<ast::FieldAccessExpr&>(expr);
            visitExpr(*access.base);
            resolveType(expr); // reported errors only; handles the pointer-dereference case too.
            break;
        }
        case ast::ExprKind::AddressOf: {
            auto& addr = static_cast<ast::AddressOfExpr&>(expr);
            visitExpr(*addr.operand);
            break;
        }
        case ast::ExprKind::IntLiteral:
        case ast::ExprKind::FloatLiteral:
        case ast::ExprKind::StringLiteral:
            break;
    }
}

void Sema::bringIntoScope(const std::unordered_map<std::string, TypeSuffix>& outerScope,
                           const std::string& lowerName, const std::string& spelling, SourceLoc loc,
                           const char* directiveNameForError) {
    auto it = outerScope.find(lowerName);
    TypeSuffix suffix = TypeSuffix::Integer;
    if (it == outerScope.end()) {
        diagnostics_.error(loc, "'" + spelling + "' is not declared, cannot be used with '" +
                                     directiveNameForError + "'");
    } else {
        suffix = it->second;
    }
    symbols_[lowerName] = suffix; // Deliberately NOT added to order_ - see this method's own doc comment.
}

void Sema::visitCall(ast::CallExpr& call) {
    auto it = procedures_.find(call.name);
    if (it == procedures_.end()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' is not a declared procedure");
        for (auto& arg : call.args) {
            visitExpr(*arg);
        }
        return;
    }
    const ProcedureInfo& info = it->second;
    if (call.args.size() < info.requiredParamCount || call.args.size() > info.paramSuffixes.size()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' called with the wrong number of arguments");
    }
    for (std::size_t i = 0; i < call.args.size(); ++i) {
        visitExpr(*call.args[i]);
        if (i >= info.paramSuffixes.size()) {
            continue; // Already flagged above as a wrong-argument-count error.
        }
        // Oracle-verified: real PB rejects a String<->numeric mismatch here
        // too (with its own distinct wording from `checkAssignable`'s, and -
        // unlike an arity mismatch - only caught by a full compile, not by
        // `-k`'s syntax-only check, a general lesson recorded in this
        // project's oracle-testing methodology doc): "Bad parameter type,
        // number expected instead of string." when a String argument is
        // passed for a numeric parameter, or "Bad parameter type: a string
        // is expected." for the reverse.
        bool paramIsString = familyOf(info.paramSuffixes[i]) == ValueKind::StringFamily;
        bool argIsString = familyOfExpr(*call.args[i]) == ValueKind::StringFamily;
        if (paramIsString != argIsString) {
            diagnostics_.error(call.loc, paramIsString ? "Bad parameter type: a string is expected."
                                                        : "Bad parameter type, number expected instead of string.");
        }
    }
}

void Sema::visitCondition(ast::Expr& expr) {
    if (expr.kind == ast::ExprKind::Binary) {
        auto& bin = static_cast<ast::BinaryExpr&>(expr);
        switch (bin.op) {
            case ast::BinaryOp::LogicalAnd:
            case ast::BinaryOp::LogicalOr:
                visitCondition(*bin.lhs);
                visitCondition(*bin.rhs);
                return;
            case ast::BinaryOp::LogicalXOr:
                // Real PB's logical XOr showed a runtime truth table that
                // didn't match ANY consistent interpretation under oracle
                // testing (see docs/architecture/roadmap.md's M1 notes) -
                // rather than guess, this is flagged as unsupported.
                diagnostics_.error(bin.loc,
                                    "logical 'XOr' is not yet supported (see "
                                    "docs/architecture/roadmap.md's M1 notes)");
                visitCondition(*bin.lhs);
                visitCondition(*bin.rhs);
                return;
            case ast::BinaryOp::Eq:
            case ast::BinaryOp::Ne:
            case ast::BinaryOp::Lt:
            case ast::BinaryOp::Gt:
            case ast::BinaryOp::Le:
            case ast::BinaryOp::Ge:
                visitExpr(*bin.lhs);
                visitExpr(*bin.rhs);
                return;
            default:
                break; // A bare arithmetic expression used as a condition - fall through.
        }
    } else if (expr.kind == ast::ExprKind::Unary) {
        auto& un = static_cast<ast::UnaryExpr&>(expr);
        if (un.op == ast::UnaryOp::LogicalNot) {
            visitCondition(*un.operand);
            return;
        }
    }
    visitExpr(expr);
}

void Sema::checkAssignable(const std::string& targetSpelling, TypeSuffix targetSuffix,
                            const ast::Expr& value, SourceLoc loc) {
    ValueKind targetFamily = familyOf(targetSuffix);
    ValueKind valueFamily = familyOfExpr(value);
    bool targetIsString = targetFamily == ValueKind::StringFamily;
    bool valueIsString = valueFamily == ValueKind::StringFamily;
    if (targetIsString != valueIsString) {
        diagnostics_.error(loc, "cannot assign " +
                                     std::string(valueIsString ? "a String" : "a numeric value") +
                                     " to '" + targetSpelling + "', which is " +
                                     (targetIsString ? "a String" : "numeric") +
                                     " (use Str()/Val() to convert explicitly)");
    }
}

void Sema::checkMapKey(const ast::Expr& keyExpr, SourceLoc loc) const {
    if (familyOfExpr(keyExpr) != ValueKind::StringFamily) {
        diagnostics_.error(loc, "A string expression is expected");
    }
}

TypeSuffix Sema::typeOf(const std::string& lowerName) const {
    auto it = symbols_.find(lowerName);
    return it == symbols_.end() ? TypeSuffix::Integer : it->second;
}

const std::string& Sema::structTypeOfVar(const std::string& lowerName) const {
    static const std::string empty;
    auto it = varStructType_.find(lowerName);
    return it == varStructType_.end() ? empty : it->second;
}

ValueKind Sema::familyOfExpr(const ast::Expr& expr) const { return classify(expr, false); }

ValueKind Sema::classify(const ast::Expr& expr, bool floatContext) const {
    switch (expr.kind) {
        case ast::ExprKind::IntLiteral:
            return floatContext ? ValueKind::FloatFamily : ValueKind::IntegerFamily;
        case ast::ExprKind::FloatLiteral:
            return ValueKind::FloatFamily;
        case ast::ExprKind::StringLiteral:
            return ValueKind::StringFamily; // Context never turns a String into a number.
        case ast::ExprKind::VarRef: {
            const auto& ref = static_cast<const ast::VarRefExpr&>(expr);
            ValueKind natural = familyOf(typeOf(ref.name));
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::ConstRef: {
            const auto& ref = static_cast<const ast::ConstRefExpr&>(expr);
            ValueKind natural = familyOf(constTypeOf(ref.name));
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::Call: {
            const auto& call = static_cast<const ast::CallExpr&>(expr);
            if (call.name == "mapkey") {
                // The one List/Map/pointer built-in that returns something
                // other than a plain Integer status code - MapKey(map())
                // yields the current element's String key (oracle-verified:
                // it's used directly as a String value, e.g. `Debug
                // MapKey(m())`) - so unlike every other builtin here, it
                // can't rely on the generic "unrecognized Call defaults to
                // Integer" fallback below.
                return ValueKind::StringFamily;
            }
            const ListInfo* list = listInfo(call.name);
            const MapInfo* map = list == nullptr ? mapInfo(call.name) : nullptr;
            const ArrayInfo* array = (list == nullptr && map == nullptr) ? arrayInfo(call.name) : nullptr;
            const ProcedureInfo* proc =
                (list == nullptr && map == nullptr && array == nullptr) ? procedureInfo(call.name) : nullptr;
            ValueKind natural = ValueKind::IntegerFamily;
            if (list != nullptr) {
                natural = familyOf(list->elementSuffix);
            } else if (map != nullptr) {
                natural = familyOf(map->elementSuffix);
            } else if (array != nullptr) {
                natural = familyOf(array->elementSuffix);
            } else if (proc != nullptr) {
                natural = familyOf(proc->returnSuffix);
            }
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::FieldAccess: {
            ValueKind natural = familyOf(resolveType(expr).suffix);
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::Unary: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            return classify(*un.operand, floatContext);
        }
        case ast::ExprKind::AddressOf:
            // `@operand` always yields a plain Integer address, regardless
            // of any enclosing Float destination.
            return ValueKind::IntegerFamily;
        case ast::ExprKind::Binary: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
            switch (bin.op) {
                case ast::BinaryOp::Div:
                case ast::BinaryOp::Mod: {
                    // Target-typed: real (Div) / real-modulo (Mod) exactly
                    // when floatContext is set OR either operand is
                    // *naturally* (context-free) float-family - oracle-
                    // verified for Div; Mod follows by consistent
                    // extrapolation only (see classify()'s own doc comment).
                    bool lhsFloat = classify(*bin.lhs, false) == ValueKind::FloatFamily;
                    bool rhsFloat = classify(*bin.rhs, false) == ValueKind::FloatFamily;
                    return (floatContext || lhsFloat || rhsFloat) ? ValueKind::FloatFamily
                                                                   : ValueKind::IntegerFamily;
                }
                case ast::BinaryOp::BitAnd:
                case ast::BinaryOp::BitOr:
                case ast::BinaryOp::BitXor:
                case ast::BinaryOp::ShiftLeft:
                case ast::BinaryOp::ShiftRight:
                    // Bitwise ops are integer-only regardless of any
                    // enclosing Float destination - unlike Div/Mod, a
                    // "real-valued shift/and/or/xor" has no natural meaning,
                    // so (unlike Div/Mod) floatContext is deliberately NOT
                    // propagated here. Not independently oracle-verified
                    // (every real PB program uses these on integers anyway);
                    // flagged as an assumption in docs/architecture/roadmap.md.
                    return ValueKind::IntegerFamily;
                case ast::BinaryOp::Add:
                case ast::BinaryOp::Sub:
                case ast::BinaryOp::Mul: {
                    ValueKind lhs = classify(*bin.lhs, floatContext);
                    ValueKind rhs = classify(*bin.rhs, floatContext);
                    if (lhs == ValueKind::StringFamily || rhs == ValueKind::StringFamily) {
                        return ValueKind::StringFamily; // `+` as string concatenation.
                    }
                    if (floatContext || lhs == ValueKind::FloatFamily || rhs == ValueKind::FloatFamily) {
                        return ValueKind::FloatFamily;
                    }
                    return ValueKind::IntegerFamily;
                }
                default:
                    // Eq/Ne/Lt/Gt/Le/Ge/LogicalAnd/LogicalOr/LogicalXOr only
                    // ever appear inside a condition tree, which Codegen
                    // lowers via genCondition (not genExpr/classify) - this
                    // is an unreachable-in-practice, safe fallback only.
                    return ValueKind::IntegerFamily;
            }
        }
    }
    return ValueKind::IntegerFamily;
}

TypeSuffix Sema::constTypeOf(const std::string& lowerName) const {
    auto it = constants_.find(lowerName);
    return it == constants_.end() ? TypeSuffix::Integer : it->second;
}

const Sema::ProcedureInfo* Sema::procedureInfo(const std::string& lowerName) const {
    auto it = procedures_.find(lowerName);
    return it == procedures_.end() ? nullptr : &it->second;
}

const Sema::ArrayInfo* Sema::arrayInfo(const std::string& lowerName) const {
    auto it = arrays_.find(lowerName);
    return it == arrays_.end() ? nullptr : &it->second;
}

const Sema::ListInfo* Sema::listInfo(const std::string& lowerName) const {
    auto it = lists_.find(lowerName);
    return it == lists_.end() ? nullptr : &it->second;
}

const Sema::ListInfo* Sema::requireList(const std::string& lowerName, const std::string& spelling,
                                        SourceLoc loc) const {
    const ListInfo* info = listInfo(lowerName);
    if (info == nullptr) {
        diagnostics_.error(loc, "'" + spelling + "' is not a declared List");
    }
    return info;
}

bool Sema::isListBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "addelement",  "insertelement", "deleteelement", "clearlist",     "firstelement",
        "lastelement", "nextelement",   "previouselement", "listsize",   "selectelement", "listindex",
    };
    return names.contains(lowerName);
}

bool Sema::visitListBuiltinCall(ast::CallExpr& call) {
    if (!isListBuiltinName(call.name)) {
        return false;
    }
    if (call.args.empty()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a List argument");
        return true;
    }
    // Every one of these takes a bare `name()` as its first argument -
    // naming the List itself, not reading its current element - so it's
    // validated directly rather than visited as an ordinary expression (see
    // this method's own doc comment).
    const ast::Expr& listArg = *call.args.front();
    if (listArg.kind == ast::ExprKind::Call) {
        const auto& listCall = static_cast<const ast::CallExpr&>(listArg);
        if (listCall.args.empty()) {
            requireList(listCall.name, listCall.spelling, call.loc);
        } else {
            diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' List argument");
        }
    } else {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' List argument");
    }
    if (call.name == "selectelement") {
        if (call.args.size() != 2) {
            diagnostics_.error(call.loc, "'SelectElement' expects a List and an index argument");
        } else {
            visitExpr(*call.args[1]);
        }
    } else if (call.args.size() != 1) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects exactly one List argument");
    }
    return true;
}

const Sema::MapInfo* Sema::mapInfo(const std::string& lowerName) const {
    auto it = maps_.find(lowerName);
    return it == maps_.end() ? nullptr : &it->second;
}

bool Sema::isMapBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "addmapelement", "deletemapelement", "clearmap", "mapsize",
        "mapkey",        "resetmap",         "nextmapelement", "findmapelement",
    };
    return names.contains(lowerName);
}

bool Sema::visitMapBuiltinCall(ast::CallExpr& call) {
    if (!isMapBuiltinName(call.name)) {
        return false;
    }
    if (call.args.empty()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a Map argument");
        return true;
    }
    // As with visitListBuiltinCall, the first argument names the Map itself
    // (a bare `name()`) rather than being visited as an ordinary expression.
    const ast::Expr& mapArg = *call.args.front();
    if (mapArg.kind == ast::ExprKind::Call) {
        const auto& mapCall = static_cast<const ast::CallExpr&>(mapArg);
        if (mapCall.args.empty()) {
            if (mapInfo(mapCall.name) == nullptr) {
                diagnostics_.error(call.loc, "'" + mapCall.spelling + "' is not a declared Map");
            }
        } else {
            diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' Map argument");
        }
    } else {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' Map argument");
    }
    if (call.name == "addmapelement" || call.name == "findmapelement") {
        if (call.args.size() != 2) {
            diagnostics_.error(call.loc, "'" + call.spelling + "' expects a Map and a key argument");
        } else {
            visitExpr(*call.args[1]);
            checkMapKey(*call.args[1], call.loc);
        }
    } else if (call.name == "deletemapelement") {
        // Oracle-verified: DeleteMapElement accepts either 1 arg (deletes
        // the current cursor element) or 2 (deletes by key).
        if (call.args.size() == 2) {
            visitExpr(*call.args[1]);
            checkMapKey(*call.args[1], call.loc);
        } else if (call.args.size() != 1) {
            diagnostics_.error(call.loc, "'DeleteMapElement' expects a Map, and optionally a key, argument");
        }
    } else if (call.args.size() != 1) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects exactly one Map argument");
    }
    return true;
}

const Sema::StructureInfo* Sema::structureInfo(const std::string& lowerName) const {
    auto it = structures_.find(lowerName);
    return it == structures_.end() ? nullptr : &it->second;
}

Sema::ResolvedType Sema::resolveField(const ResolvedType& baseType, const std::string& fieldLowerName,
                                      const std::string& fieldSpelling, SourceLoc loc) const {
    if (baseType.suffix != TypeSuffix::Struct) {
        diagnostics_.error(loc, "'\\" + fieldSpelling + "' used on a value that isn't a Structure");
        return ResolvedType{};
    }
    const StructureInfo* info = structureInfo(baseType.structName);
    if (info == nullptr) {
        diagnostics_.error(loc, "'\\" + fieldSpelling + "' used on an unknown Structure type");
        return ResolvedType{};
    }
    for (const auto& field : info->fields) {
        if (field.name == fieldLowerName) {
            ResolvedType result;
            result.suffix = field.suffix;
            if (field.suffix == TypeSuffix::Struct) {
                result.structName = field.structTypeName;
            }
            return result;
        }
    }
    diagnostics_.error(loc, "'" + fieldSpelling + "' is not a field of this Structure");
    return ResolvedType{};
}

Sema::ResolvedType Sema::resolveType(const ast::Expr& expr) const {
    switch (expr.kind) {
        case ast::ExprKind::VarRef: {
            const auto& ref = static_cast<const ast::VarRefExpr&>(expr);
            TypeSuffix suffix = typeOf(ref.name);
            ResolvedType result;
            result.suffix = suffix;
            if (suffix == TypeSuffix::Struct) {
                auto it = varStructType_.find(ref.name);
                result.structName = it != varStructType_.end() ? it->second : "";
            }
            return result;
        }
        case ast::ExprKind::Call: {
            const auto& call = static_cast<const ast::CallExpr&>(expr);
            if (const ListInfo* lst = listInfo(call.name)) {
                ResolvedType result;
                result.suffix = lst->elementSuffix;
                if (lst->elementSuffix == TypeSuffix::Struct) {
                    result.structName = lst->elementStructName;
                }
                return result;
            }
            if (const MapInfo* map = mapInfo(call.name)) {
                ResolvedType result;
                result.suffix = map->elementSuffix;
                if (map->elementSuffix == TypeSuffix::Struct) {
                    result.structName = map->elementStructName;
                }
                return result;
            }
            if (const ArrayInfo* arr = arrayInfo(call.name)) {
                ResolvedType result;
                result.suffix = arr->elementSuffix;
                if (arr->elementSuffix == TypeSuffix::Struct) {
                    result.structName = arr->elementStructName;
                }
                return result;
            }
            if (const ProcedureInfo* proc = procedureInfo(call.name)) {
                return ResolvedType{proc->returnSuffix, ""};
            }
            return ResolvedType{};
        }
        case ast::ExprKind::FieldAccess: {
            const auto& access = static_cast<const ast::FieldAccessExpr&>(expr);
            // `*ptr\field` - the base is a bare pointer-value VarRef (its
            // name always starts with '*', see DefineStmt::Declarator's own
            // doc comment), whose *own* symbols_ entry is forced to plain
            // Integer (the address itself) - so resolving the dereference's
            // type has to go through pointerPointeeType_ instead of
            // recursing into resolveType(*access.base) normally, which
            // would just see "Integer" and reject the field access outright.
            if (access.base->kind == ast::ExprKind::VarRef) {
                const auto& baseRef = static_cast<const ast::VarRefExpr&>(*access.base);
                if (!baseRef.name.empty() && baseRef.name.front() == '*') {
                    ResolvedType pointee = pointeeTypeOf(baseRef.name);
                    if (pointee.suffix == TypeSuffix::Struct) {
                        return resolveField(pointee, access.field, access.fieldSpelling, access.loc);
                    }
                    // An untyped pointer (`Define *pa`, no Structure type)
                    // has no fields to dereference - real dereferencing for
                    // one of these goes through the Memory library's
                    // Peek*/Poke* functions instead (not yet implemented -
                    // M4), so `\field` on one is rejected outright here.
                    diagnostics_.error(access.loc, "'\\" + access.fieldSpelling + "' used on '" + baseRef.spelling +
                                                        "', which is not a Structure-typed pointer");
                    return ResolvedType{};
                }
            }
            ResolvedType baseType = resolveType(*access.base);
            return resolveField(baseType, access.field, access.fieldSpelling, access.loc);
        }
        default:
            return ResolvedType{};
    }
}

Sema::ResolvedType Sema::pointeeTypeOf(const std::string& pointerKey) const {
    auto it = pointerPointeeType_.find(pointerKey);
    return it == pointerPointeeType_.end() ? ResolvedType{} : it->second;
}

bool Sema::isPointerBuiltinName(const std::string& lowerName) {
    return lowerName == "allocatememory" || lowerName == "freememory" ||
           lowerName == "allocatestructure" || lowerName == "freestructure";
}

bool Sema::visitPointerBuiltinCall(ast::CallExpr& call) {
    if (!isPointerBuiltinName(call.name)) {
        return false;
    }
    if (call.name == "allocatestructure") {
        // Oracle-verified syntax: `AllocateStructure(Point)` - the sole
        // argument is a bare Structure type name, not a variable read, so
        // it's deliberately not visited as an expression (doing so would
        // implicitly declare a bogus variable named after the type).
        if (call.args.size() != 1 || call.args.front()->kind != ast::ExprKind::VarRef) {
            diagnostics_.error(call.loc, "'AllocateStructure' expects a single Structure type name");
            return true;
        }
        const auto& typeRef = static_cast<const ast::VarRefExpr&>(*call.args.front());
        if (!structures_.contains(typeRef.name)) {
            diagnostics_.error(call.loc, "'" + typeRef.spelling + "' is not a declared Structure");
        }
        return true;
    }
    // AllocateMemory(size)/FreeMemory(ptr)/FreeStructure(ptr) all take one
    // ordinary expression argument (a size or a pointer value) - visited
    // normally like any other call argument.
    if (call.args.size() != 1) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a single argument");
    }
    for (auto& arg : call.args) {
        visitExpr(*arg);
    }
    return true;
}

Sema::ResolvedType Sema::resolvePointeeType(TypeSuffix suffix, const std::string& structTypeName,
                                             const std::string& structTypeSpelling, SourceLoc loc) {
    ResolvedType pointee;
    if (suffix == TypeSuffix::Struct) {
        if (!structures_.contains(structTypeName)) {
            diagnostics_.error(loc, "'" + structTypeSpelling + "' is not a declared Structure");
        }
        pointee.suffix = TypeSuffix::Struct;
        pointee.structName = structTypeName;
    } else if (suffix != TypeSuffix::None) {
        // Oracle-verified (`pbcompilerc`, `Define *pa.i`): a pointer can
        // only be untyped or Structure-typed, never a primitive.
        diagnostics_.error(loc, "Native types can't be used with pointers.");
        pointee.suffix = TypeSuffix::None;
    } else {
        pointee.suffix = TypeSuffix::None; // Untyped pointer - see resolveType()'s own FieldAccess notes.
    }
    return pointee;
}

} // namespace easybasic
