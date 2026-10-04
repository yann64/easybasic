#include "codegen.hpp"

#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace easybasic {

namespace {
/// M7d: every Sema-internal module-qualified key uses a literal "::"
/// separator (e.g. "ferrari::createferrari") - never a legal C++ identifier
/// substring, so the few places that actually emit a name as C++ source
/// text (`cppTypeFor`'s own two-arg overload just below, and `Codegen`'s own
/// `cppVarName`/`cppProcName`) replace it with something that is. Defined
/// here, ahead of its own first use, since `cppTypeFor` is a free function
/// (not a `Codegen` member) needing it too.
std::string sanitizeModuleQualifier(const std::string& name) {
    std::string result = name;
    std::size_t pos = 0;
    while ((pos = result.find("::", pos)) != std::string::npos) {
        result.replace(pos, 2, "_M_");
        pos += 3;
    }
    return result;
}
} // namespace

const char* cppTypeFor(TypeSuffix suffix) {
    switch (suffix) {
        case TypeSuffix::Byte: return "std::int8_t";
        case TypeSuffix::Ascii: return "std::uint8_t";
        case TypeSuffix::Character: return "std::uint16_t"; // storage-identical to Unicode (oracle-verified)
        case TypeSuffix::Word: return "std::int16_t";
        case TypeSuffix::Unicode: return "std::uint16_t";
        case TypeSuffix::Long: return "std::int32_t";
        case TypeSuffix::Quad: return "std::int64_t";
        case TypeSuffix::Float: return "float";
        case TypeSuffix::Double: return "double";
        case TypeSuffix::String: return "easybasic::runtime::PBString";
        case TypeSuffix::Integer:
        case TypeSuffix::None:
        default:
            // PB's own `integer` is platform-width, but every target this
            // project ships for (Linux/Haiku/Windows desktops) is 64-bit,
            // so a fixed int64_t is a deliberate simplification, not a bug.
            return "std::int64_t";
    }
}

std::string cppTypeFor(TypeSuffix suffix, const std::string& structName) {
    if (suffix == TypeSuffix::Struct) {
        return "s_" + sanitizeModuleQualifier(structName);
    }
    return cppTypeFor(suffix);
}

namespace {

std::string escapeCppString(const std::string& raw) {
    std::string out = "\"";
    for (char c : raw) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    out += "\"";
    return out;
}

std::string formatFloatLiteral(double value) {
    std::ostringstream oss;
    oss << std::setprecision(17) << value;
    return oss.str();
}

/// Maps a String-library builtin's lowercased PB name to its
/// `easybasic::runtime::` implementation, e.g. `"left"` -> `"easybasic::
/// runtime::pbLeft"` - the one place that needs to know each function's
/// exact capitalization, since `Sema::isStringLibBuiltinName`'s own table
/// only needs the lowercased form for lookup.
std::string stringLibRuntimeName(const std::string& lowerName) {
    static const std::unordered_map<std::string, std::string> names = {
        {"len", "pbLen"},     {"left", "pbLeft"},   {"right", "pbRight"}, {"mid", "pbMid"},
        {"ucase", "pbUCase"}, {"lcase", "pbLCase"}, {"trim", "pbTrim"},   {"ltrim", "pbLTrim"},
        {"rtrim", "pbRTrim"}, {"str", "pbStr"},     {"val", "pbVal"},     {"strf", "pbStrF"},
        {"valf", "pbValF"},   {"chr", "pbChr"},     {"asc", "pbAsc"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

/// As `stringLibRuntimeName`, for the M4b Math library.
std::string mathLibRuntimeName(const std::string& lowerName) {
    static const std::unordered_map<std::string, std::string> names = {
        {"abs", "pbAbs"},     {"sqr", "pbSqr"},   {"pow", "pbPow"},     {"sin", "pbSin"},
        {"cos", "pbCos"},     {"tan", "pbTan"},   {"asin", "pbASin"},   {"acos", "pbACos"},
        {"atan", "pbATan"},   {"atan2", "pbATan2"}, {"exp", "pbExp"},   {"log", "pbLog"},
        {"log10", "pbLog10"}, {"round", "pbRound"}, {"int", "pbInt"},
        {"random", "pbRandom"}, {"randomseed", "pbRandomSeed"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

/// As `stringLibRuntimeName`, for the M4c Memory library.
std::string memoryLibRuntimeName(const std::string& lowerName) {
    static const std::unordered_map<std::string, std::string> names = {
        {"peekb", "pbPeekB"}, {"peeka", "pbPeekA"}, {"peekc", "pbPeekC"}, {"peekw", "pbPeekW"},
        {"peeku", "pbPeekU"}, {"peekl", "pbPeekL"}, {"peekq", "pbPeekQ"}, {"peekf", "pbPeekF"},
        {"peekd", "pbPeekD"}, {"peeks", "pbPeekS"}, {"pokeb", "pbPokeB"}, {"pokea", "pbPokeA"},
        {"pokec", "pbPokeC"}, {"pokew", "pbPokeW"}, {"pokeu", "pbPokeU"}, {"pokel", "pbPokeL"},
        {"pokeq", "pbPokeQ"}, {"pokef", "pbPokeF"}, {"poked", "pbPokeD"}, {"pokes", "pbPokeS"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

/// As `stringLibRuntimeName`, for the M4d File library.
std::string fileLibRuntimeName(const std::string& lowerName) {
    static const std::unordered_map<std::string, std::string> names = {
        {"createfile", "pbCreateFile"}, {"openfile", "pbOpenFile"},   {"readfile", "pbReadFile"},
        {"closefile", "pbCloseFile"},   {"writestring", "pbWriteString"},
        {"writestringn", "pbWriteStringN"}, {"readstring", "pbReadString"}, {"eof", "pbEof"},
        {"filesize", "pbFileSize"},     {"deletefile", "pbDeleteFile"}, {"renamefile", "pbRenameFile"},
        {"fileseek", "pbFileSeek"},     {"loc", "pbLoc"},              {"lof", "pbLof"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

/// As `stringLibRuntimeName`, for the M4e Date library. `argCount` is
/// needed specifically for `Date`: its zero-arg ("current time") and
/// six-arg ("construct from components") forms map to two entirely
/// different C++ functions (`pbDateNow`/`pbDate`), unlike every other
/// optional-argument case elsewhere in this project - a C++ default
/// parameter can't express "omit all six arguments at once, or none of
/// them" (see `Sema::isDateLibBuiltinName`'s own doc comment).
std::string dateLibRuntimeName(const std::string& lowerName, std::size_t argCount) {
    if (lowerName == "date") {
        return argCount == 0 ? "easybasic::runtime::pbDateNow" : "easybasic::runtime::pbDate";
    }
    static const std::unordered_map<std::string, std::string> names = {
        {"year", "pbYear"},     {"month", "pbMonth"},         {"day", "pbDay"},
        {"hour", "pbHour"},     {"minute", "pbMinute"},       {"second", "pbSecond"},
        {"dayofweek", "pbDayOfWeek"}, {"formatdate", "pbFormatDate"}, {"adddate", "pbAddDate"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

std::string threadLibRuntimeName(const std::string& lowerName) {
    static const std::unordered_map<std::string, std::string> names = {
        {"createthread", "pbCreateThread"},   {"isthread", "pbIsThread"},
        {"waitthread", "pbWaitThread"},       {"createmutex", "pbCreateMutex"},
        {"lockmutex", "pbLockMutex"},         {"unlockmutex", "pbUnlockMutex"},
        {"trylockmutex", "pbTryLockMutex"},   {"freemutex", "pbFreeMutex"},
        {"createsemaphore", "pbCreateSemaphore"}, {"signalsemaphore", "pbSignalSemaphore"},
        {"waitsemaphore", "pbWaitSemaphore"}, {"trysemaphore", "pbTrySemaphore"},
        {"freesemaphore", "pbFreeSemaphore"}, {"delay", "pbDelay"},
        {"elapsedmilliseconds", "pbElapsedMilliseconds"},
        {"killthread", "pbKillThread"},       {"pausethread", "pbPauseThread"},
        {"resumethread", "pbResumeThread"},   {"threadid", "pbThreadID"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

std::string guiLibRuntimeName(const std::string& lowerName) {
    static const std::unordered_map<std::string, std::string> names = {
        {"openwindow", "pbOpenWindow"},   {"closewindow", "pbCloseWindow"},
        {"iswindow", "pbIsWindow"},       {"resizewindow", "pbResizeWindow"},
        {"hidewindow", "pbHideWindow"},   {"windowevent", "pbWindowEvent"},
        {"waitwindowevent", "pbWaitWindowEvent"}, {"eventwindow", "pbEventWindow"},
        {"eventgadget", "pbEventGadget"}, {"eventtype", "pbEventType"},
        {"buttongadget", "pbButtonGadget"},     {"textgadget", "pbTextGadget"},
        {"stringgadget", "pbStringGadget"},     {"checkboxgadget", "pbCheckBoxGadget"},
        {"framegadget", "pbFrameGadget"},       {"isgadget", "pbIsGadget"},
        {"freegadget", "pbFreeGadget"},         {"resizegadget", "pbResizeGadget"},
        {"hidegadget", "pbHideGadget"},         {"disablegadget", "pbDisableGadget"},
        {"setgadgettext", "pbSetGadgetText"},   {"getgadgettext", "pbGetGadgetText"},
        {"setgadgetstate", "pbSetGadgetState"}, {"getgadgetstate", "pbGetGadgetState"},
        {"messagerequester", "pbMessageRequester"},
        {"windowid", "pbWindowID"},
        {"createmenu", "pbCreateMenu"},     {"menutitle", "pbMenuTitle"},
        {"menuitem", "pbMenuItem"},         {"menubar", "pbMenuBar"},
        {"opensubmenu", "pbOpenSubMenu"},   {"closesubmenu", "pbCloseSubMenu"},
        {"ismenu", "pbIsMenu"},             {"freemenu", "pbFreeMenu"},
        {"hidemenu", "pbHideMenu"},         {"disablemenuitem", "pbDisableMenuItem"},
        {"getmenuitemstate", "pbGetMenuItemState"}, {"setmenuitemstate", "pbSetMenuItemState"},
        {"getmenuitemtext", "pbGetMenuItemText"},   {"setmenuitemtext", "pbSetMenuItemText"},
        {"getmenutitletext", "pbGetMenuTitleText"}, {"setmenutitletext", "pbSetMenuTitleText"},
        {"menuheight", "pbMenuHeight"},     {"menuid", "pbMenuID"},
        {"eventmenu", "pbEventMenu"},
        {"createstatusbar", "pbCreateStatusBar"},   {"addstatusbarfield", "pbAddStatusBarField"},
        {"statusbartext", "pbStatusBarText"},       {"isstatusbar", "pbIsStatusBar"},
        {"freestatusbar", "pbFreeStatusBar"},       {"statusbarheight", "pbStatusBarHeight"},
        {"statusbarid", "pbStatusBarID"},
        {"createimage", "pbCreateImage"},   {"loadimage", "pbLoadImage"},
        {"isimage", "pbIsImage"},           {"freeimage", "pbFreeImage"},
        {"imageid", "pbImageID"},           {"imagewidth", "pbImageWidth"},
        {"imageheight", "pbImageHeight"},   {"createimagemenu", "pbCreateImageMenu"},
    };
    return "easybasic::runtime::" + names.at(lowerName);
}

} // namespace

std::string defaultValueLiteral(TypeSuffix suffix, const std::string& structName) {
    if (suffix == TypeSuffix::Struct) {
        return cppTypeFor(suffix, structName) + "{}";
    }
    if (familyOf(suffix) == ValueKind::StringFamily) {
        return "easybasic::runtime::PBString()";
    }
    return std::string("static_cast<") + cppTypeFor(suffix) + ">(0)";
}

Codegen::Codegen(const ast::Module& module, const Sema& sema, bool debugMode)
    : module_(module), sema_(sema), debugMode_(debugMode) {}

std::string Codegen::cppVarName(const std::string& name) {
    if (!name.empty() && name.front() == '*') {
        return "vp_" + sanitizeModuleQualifier(name.substr(1));
    }
    return "v_" + sanitizeModuleQualifier(name);
}

std::string Codegen::cppProcName(const std::string& name) { return "f_" + sanitizeModuleQualifier(name); }

std::string Codegen::cppConstName(const std::string& name) { return "k_" + sanitizeModuleQualifier(name); }

Sema::ResolvedType Codegen::pointeeTypeOf(const std::string& pointerKey) const {
    if (currentProcInfo_ != nullptr) {
        auto it = currentProcInfo_->pointerPointeeTypes.find(pointerKey);
        if (it != currentProcInfo_->pointerPointeeTypes.end()) {
            return it->second;
        }
    }
    return sema_.pointeeTypeOf(pointerKey);
}

std::string Codegen::convert(const std::string& exprCode, ValueKind fromFamily, TypeSuffix toSuffix) {
    ValueKind toFamily = familyOf(toSuffix);
    const char* cppType = cppTypeFor(toSuffix);

    if (toFamily == ValueKind::StringFamily) {
        // Sema::checkAssignable already rejected String<->numeric mixes, so
        // by the time Codegen sees this, exprCode is already a PBString.
        return exprCode;
    }
    if (toFamily == ValueKind::IntegerFamily && fromFamily == ValueKind::FloatFamily) {
        // PB rounds float->integer conversions half-to-even rather than
        // truncating (oracle-verified: 2.5->2, 3.5->4, -2.5->-2) - `llrint`
        // under the default IEEE rounding mode matches this exactly.
        return std::string("static_cast<") + cppType + ">(llrint(" + exprCode + "))";
    }
    return std::string("static_cast<") + cppType + ">(" + exprCode + ")";
}

std::string Codegen::convertReadValue(const std::string& readCall, ValueKind dataFamily, TypeSuffix targetSuffix) {
    ValueKind targetFamily = familyOf(targetSuffix);
    if (dataFamily == targetFamily) {
        return convert(readCall, dataFamily, targetSuffix);
    }
    if (targetFamily == ValueKind::StringFamily) {
        // dataFamily is Integer or Float here.
        return dataFamily == ValueKind::IntegerFamily
                   ? "easybasic::runtime::pbStr(" + readCall + ")"
                   : "easybasic::runtime::PBString(std::to_string(" + readCall + "))";
    }
    if (dataFamily == ValueKind::StringFamily) {
        // targetFamily is Integer or Float here.
        return targetFamily == ValueKind::IntegerFamily ? "easybasic::runtime::pbVal(" + readCall + ")"
                                                          : "std::strtod((" + readCall + ").bytes().c_str(), nullptr)";
    }
    return convert(readCall, dataFamily, targetSuffix); // Integer<->Float, not through String.
}

std::string Codegen::genExpr(const ast::Expr& expr, bool floatContext) {
    switch (expr.kind) {
        case ast::ExprKind::IntLiteral: {
            const auto& lit = static_cast<const ast::IntLiteralExpr&>(expr);
            return std::to_string(lit.value);
        }
        case ast::ExprKind::FloatLiteral: {
            const auto& lit = static_cast<const ast::FloatLiteralExpr&>(expr);
            return formatFloatLiteral(lit.value);
        }
        case ast::ExprKind::StringLiteral: {
            const auto& lit = static_cast<const ast::StringLiteralExpr&>(expr);
            return "easybasic::runtime::PBString(" + escapeCppString(lit.value) + ")";
        }
        case ast::ExprKind::VarRef: {
            const auto& ref = static_cast<const ast::VarRefExpr&>(expr);
            return cppVarName(ref.name);
        }
        case ast::ExprKind::ConstRef: {
            const auto& ref = static_cast<const ast::ConstRefExpr&>(expr);
            // A handful of `#PB_*` constants (currently just `Round`'s own
            // mode constants) are recognized by Sema but never actually
            // emitted as a `k_<name>` global - unlike a user's own
            // `#Name = expr`, there is no ConstDeclStmt/EnumerationStmt
            // anywhere in the AST for Codegen to hoist, so the literal
            // value is emitted directly here instead.
            if (auto builtin = Sema::builtinConstantValue(ref.name)) {
                return std::to_string(*builtin);
            }
            return cppConstName(ref.name);
        }
        case ast::ExprKind::FieldAccess: {
            const auto& access = static_cast<const ast::FieldAccessExpr&>(expr);
            // `*ptr\field` - a dereference, not a plain Structure field
            // access: the base is a bare pointer-value VarRef (its name
            // always starts with '*') whose C++ storage is a plain
            // int64_t address, so it's `reinterpret_cast` at exactly this
            // point rather than a `.f_field` member access (see
            // Sema::resolveType's identical special-case for why).
            if (access.base->kind == ast::ExprKind::VarRef) {
                const auto& baseRef = static_cast<const ast::VarRefExpr&>(*access.base);
                if (!baseRef.name.empty() && baseRef.name.front() == '*') {
                    // Sema already rejected any pointer whose pointee isn't
                    // a Structure (see Sema::resolveType's identical
                    // pointer-dereference branch), and the pipeline stops
                    // before Codegen runs whenever Sema reports an error -
                    // so `pointee.suffix == Struct` always holds here.
                    Sema::ResolvedType pointee = pointeeTypeOf(baseRef.name);
                    std::string addrCode = cppVarName(baseRef.name);
                    return "(reinterpret_cast<" + cppTypeFor(pointee.suffix, pointee.structName) + "*>(" +
                           addrCode + ")->f_" + access.field + ")";
                }
            }
            return "(" + genExpr(*access.base, false) + ").f_" + access.field;
        }
        case ast::ExprKind::AddressOf: {
            const auto& addr = static_cast<const ast::AddressOfExpr&>(expr);
            if (addr.operand->kind == ast::ExprKind::Call) {
                const auto& call = static_cast<const ast::CallExpr&>(*addr.operand);
                if (sema_.procedureInfo(call.name) != nullptr) {
                    // `@ProcedureName()` - the function's own address, not a
                    // call followed by address-of its result (see Sema's
                    // matching visitExpr case for why this can't just fall
                    // through to the generic path below).
                    return "reinterpret_cast<std::int64_t>(&" + cppProcName(call.name) + ")";
                }
            }
            return "reinterpret_cast<std::int64_t>(&(" + genExpr(*addr.operand, false) + "))";
        }
        case ast::ExprKind::DataLabelAddress: {
            const auto& addr = static_cast<const ast::DataLabelAddressExpr&>(expr);
            // Sema already rejected any label that isn't dataLabelAddressable
            // (so genDataLabelArrays() always emitted this array) - the
            // pipeline stops before Codegen runs whenever Sema reports an
            // error.
            return "reinterpret_cast<std::int64_t>(pb_label_" + sanitizeModuleQualifier(addr.labelName) +
                   ".data())";
        }
        case ast::ExprKind::MethodCall: {
            const auto& call = static_cast<const ast::MethodCallExpr&>(expr);
            // Sema already rejected anything but a `*ptr`-named VarRef base
            // pointing at a real Interface method (see resolveInterfaceMethod).
            const auto& baseRef = static_cast<const ast::VarRefExpr&>(*call.base);
            Sema::ResolvedType pointee = pointeeTypeOf(baseRef.name);
            const Sema::InterfaceInfo* info = sema_.interfaceInfo(pointee.structName);
            std::size_t slot = *Sema::interfaceMethodIndex(*info, call.method);
            const Sema::InterfaceMethodInfo& methodInfo = info->methods[slot];

            // Real PB's own C backend builds a genuine manual vtable this
            // same way (confirmed by reading pbcompilerc's own `-c` output -
            // see the M7c roadmap notes): the pointer variable's own value
            // IS the implementing Structure's address; its first 8 bytes
            // (read via one dereference) are the vtable base `?Label`
            // produced; indexing that by the method's declared position and
            // reinterpreting the stored address as a function pointer of
            // the *Interface's own declared signature* gives a call that's
            // exactly as well-defined as this project's own pointer
            // parameters already are: every implementing procedure's own
            // "this" parameter is already a plain `std::int64_t` (Sema
            // forces every pointer parameter's C++ storage type to Integer,
            // never a real typed struct pointer - see
            // Sema::visitStmt's ProcedureDecl case), so this reinterpret_cast
            // targets the *exact* real underlying function pointer type,
            // not merely a same-size-and-hope-for-the-best stand-in the way
            // real PB's own `integer`-typed vtable slots are.
            std::string objectAddr = cppVarName(baseRef.name);
            std::string fnPtrType = std::string(cppTypeFor(methodInfo.returnSuffix)) + "(*)(std::int64_t";
            for (TypeSuffix paramSuffix : methodInfo.paramSuffixes) {
                fnPtrType += ", " + std::string(cppTypeFor(paramSuffix));
            }
            fnPtrType += ")";

            std::string code = "reinterpret_cast<" + fnPtrType + ">(*reinterpret_cast<std::int64_t*>(" +
                                "*reinterpret_cast<std::int64_t*>(" + objectAddr + ") + " +
                                std::to_string(slot * 8) + "))(" + objectAddr;
            for (std::size_t i = 0; i < call.args.size(); ++i) {
                TypeSuffix paramSuffix = methodInfo.paramSuffixes[i];
                bool paramFloatCtx = familyOf(paramSuffix) == ValueKind::FloatFamily;
                code += ", " + convert(genExpr(*call.args[i], paramFloatCtx),
                                        sema_.classify(*call.args[i], paramFloatCtx), paramSuffix);
            }
            code += ")";
            return code;
        }
        case ast::ExprKind::Call: {
            const auto& call = static_cast<const ast::CallExpr&>(expr);
            if (Sema::isPointerBuiltinName(call.name)) {
                if (call.name == "allocatememory") {
                    return "reinterpret_cast<std::int64_t>(std::malloc(static_cast<std::size_t>(" +
                           genExpr(*call.args.front(), false) + ")))";
                }
                if (call.name == "allocatestructure") {
                    const auto& typeRef = static_cast<const ast::VarRefExpr&>(*call.args.front());
                    return "reinterpret_cast<std::int64_t>(new " + cppTypeFor(TypeSuffix::Struct, typeRef.name) +
                           "())";
                }
                if (call.name == "freememory") {
                    return "(std::free(reinterpret_cast<void*>(" + genExpr(*call.args.front(), false) +
                           ")), static_cast<std::int64_t>(0))";
                }
                // freestructure
                const ast::Expr& ptrArg = *call.args.front();
                Sema::ResolvedType pointee{};
                if (ptrArg.kind == ast::ExprKind::VarRef) {
                    pointee = pointeeTypeOf(static_cast<const ast::VarRefExpr&>(ptrArg).name);
                }
                std::string typeName = pointee.suffix == TypeSuffix::Struct
                                            ? cppTypeFor(pointee.suffix, pointee.structName)
                                            : std::string(cppTypeFor(TypeSuffix::Integer));
                return "(delete reinterpret_cast<" + typeName + "*>(" + genExpr(ptrArg, false) +
                       "), static_cast<std::int64_t>(0))";
            }
            if (Sema::isListBuiltinName(call.name)) {
                const auto& listCall = static_cast<const ast::CallExpr&>(*call.args.front());
                std::string listVar = cppVarName(listCall.name);
                if (call.name == "addelement") {
                    return "(" + listVar + ".addElement(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "insertelement") {
                    return "(" + listVar + ".insertElement(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "deleteelement") {
                    return "(" + listVar + ".deleteElement(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "clearlist") {
                    return "(" + listVar + ".clear(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "firstelement") {
                    return "static_cast<std::int64_t>(" + listVar + ".firstElement())";
                }
                if (call.name == "lastelement") {
                    return "static_cast<std::int64_t>(" + listVar + ".lastElement())";
                }
                if (call.name == "nextelement") {
                    return "static_cast<std::int64_t>(" + listVar + ".nextElement())";
                }
                if (call.name == "previouselement") {
                    return "static_cast<std::int64_t>(" + listVar + ".previousElement())";
                }
                if (call.name == "listsize") {
                    return listVar + ".size()";
                }
                if (call.name == "listindex") {
                    return listVar + ".listIndex()";
                }
                // selectelement
                return "static_cast<std::int64_t>(" + listVar + ".selectElement(" +
                       genExpr(*call.args[1], false) + "))";
            }
            if (sema_.listInfo(call.name) != nullptr) {
                // `name()` reads the List's current element (M3e).
                return cppVarName(call.name) + ".current()";
            }
            if (Sema::isMapBuiltinName(call.name)) {
                const auto& mapCall = static_cast<const ast::CallExpr&>(*call.args.front());
                std::string mapVar = cppVarName(mapCall.name);
                if (call.name == "addmapelement") {
                    return "(" + mapVar + ".addMapElement(" + genExpr(*call.args[1], false) +
                           "), static_cast<std::int64_t>(0))";
                }
                if (call.name == "deletemapelement") {
                    if (call.args.size() == 2) {
                        return "(" + mapVar + ".deleteKey(" + genExpr(*call.args[1], false) +
                               "), static_cast<std::int64_t>(0))";
                    }
                    return "(" + mapVar + ".deleteCurrent(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "clearmap") {
                    return "(" + mapVar + ".clear(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "mapsize") {
                    return mapVar + ".size()";
                }
                if (call.name == "mapkey") {
                    return mapVar + ".mapKey()";
                }
                if (call.name == "resetmap") {
                    return "(" + mapVar + ".resetForEach(), static_cast<std::int64_t>(0))";
                }
                if (call.name == "nextmapelement") {
                    return "static_cast<std::int64_t>(" + mapVar + ".nextElement())";
                }
                // findmapelement
                return "static_cast<std::int64_t>(" + mapVar + ".findMapElement(" +
                       genExpr(*call.args[1], false) + "))";
            }
            if (sema_.mapInfo(call.name) != nullptr) {
                // `name()` reads the Map's current element; `name(key)`
                // reads/auto-creates by key (M3f).
                std::string mapVar = cppVarName(call.name);
                if (call.args.empty()) {
                    return mapVar + ".current()";
                }
                return mapVar + ".access(" + genExpr(*call.args.front(), false) + ")";
            }
            if (sema_.arrayInfo(call.name) != nullptr) {
                return cppVarName(call.name) + ".at(" + genArrayIndexCode(call.name, call.args) + ")";
            }
            const Sema::ProcedureInfo* info = sema_.procedureInfo(call.name);
            std::string calleeName;
            if (Sema::isStringLibBuiltinName(call.name)) {
                calleeName = stringLibRuntimeName(call.name);
            } else if (Sema::isMathLibBuiltinName(call.name)) {
                calleeName = mathLibRuntimeName(call.name);
            } else if (Sema::isMemoryLibBuiltinName(call.name)) {
                calleeName = memoryLibRuntimeName(call.name);
            } else if (Sema::isFileLibBuiltinName(call.name)) {
                calleeName = fileLibRuntimeName(call.name);
            } else if (Sema::isDateLibBuiltinName(call.name)) {
                calleeName = dateLibRuntimeName(call.name, call.args.size());
            } else if (Sema::isThreadLibBuiltinName(call.name)) {
                calleeName = threadLibRuntimeName(call.name);
            } else if (Sema::isGuiLibBuiltinName(call.name)) {
                calleeName = guiLibRuntimeName(call.name);
            } else {
                calleeName = cppProcName(call.name);
            }
            std::string code = calleeName + "(";
            for (std::size_t i = 0; i < call.args.size(); ++i) {
                if (i != 0) {
                    code += ", ";
                }
                // Each argument is converted to its parameter's declared
                // type at the call site, mirroring Define/Assign's own
                // target-typed conversion (same banker's-rounding rule
                // applies: passing a Float where an Integer parameter is
                // declared rounds, it doesn't truncate).
                TypeSuffix paramSuffix =
                    (info != nullptr && i < info->paramSuffixes.size()) ? info->paramSuffixes[i] : TypeSuffix::Integer;
                bool paramFloatCtx = familyOf(paramSuffix) == ValueKind::FloatFamily;
                code += convert(genExpr(*call.args[i], paramFloatCtx), sema_.classify(*call.args[i], paramFloatCtx),
                                 paramSuffix);
            }
            code += ")";
            return code;
        }
        case ast::ExprKind::Unary: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            switch (un.op) {
                case ast::UnaryOp::Negate:
                    return "(-" + genExpr(*un.operand, floatContext) + ")";
                case ast::UnaryOp::BitNot:
                    // Integer-only, regardless of any enclosing Float
                    // destination (see BinaryOp::BitAnd's identical
                    // rationale just below).
                    return "(~" + genExpr(*un.operand, false) + ")";
                case ast::UnaryOp::LogicalNot:
                    // Only ever appears inside a condition tree, lowered by
                    // genCondition instead - unreachable in practice.
                    return "(!" + genExpr(*un.operand, false) + ")";
            }
            break;
        }
        case ast::ExprKind::Binary: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
            switch (bin.op) {
                case ast::BinaryOp::BitAnd:
                case ast::BinaryOp::BitOr:
                case ast::BinaryOp::BitXor:
                case ast::BinaryOp::ShiftLeft:
                case ast::BinaryOp::ShiftRight: {
                    // Integer-only (see Sema::classify's identical
                    // rationale) - operands generated without floatContext
                    // even if the destination is Float, since these ops
                    // themselves are always evaluated as integers first.
                    std::string lhs = genExpr(*bin.lhs, false);
                    std::string rhs = genExpr(*bin.rhs, false);
                    switch (bin.op) {
                        case ast::BinaryOp::BitAnd: return "(" + lhs + " & " + rhs + ")";
                        case ast::BinaryOp::BitOr: return "(" + lhs + " | " + rhs + ")";
                        // PB's `!` is bitwise XOR, not logical negation.
                        case ast::BinaryOp::BitXor: return "(" + lhs + " ^ " + rhs + ")";
                        case ast::BinaryOp::ShiftLeft: return "(" + lhs + " << " + rhs + ")";
                        case ast::BinaryOp::ShiftRight: return "(" + lhs + " >> " + rhs + ")";
                        default: break;
                    }
                    break;
                }
                default:
                    break;
            }
            std::string lhs = genExpr(*bin.lhs, floatContext);
            std::string rhs = genExpr(*bin.rhs, floatContext);
            switch (bin.op) {
                case ast::BinaryOp::Add: return "(" + lhs + " + " + rhs + ")";
                case ast::BinaryOp::Sub: return "(" + lhs + " - " + rhs + ")";
                case ast::BinaryOp::Mul: return "(" + lhs + " * " + rhs + ")";
                case ast::BinaryOp::Div:
                    // Target-typed (see Sema::classify's doc comment):
                    // real division only when this specific node classifies
                    // as float-family under the propagated context, else
                    // plain C++ integer division - never unconditionally
                    // one or the other.
                    if (sema_.classify(bin, floatContext) == ValueKind::FloatFamily) {
                        return "(static_cast<double>(" + lhs + ") / static_cast<double>(" + rhs + "))";
                    }
                    return "(" + lhs + " / " + rhs + ")";
                case ast::BinaryOp::Mod:
                    if (sema_.classify(bin, floatContext) == ValueKind::FloatFamily) {
                        return "std::fmod(static_cast<double>(" + lhs + "), static_cast<double>(" + rhs + "))";
                    }
                    return "(" + lhs + " % " + rhs + ")";
                default:
                    // Eq/Ne/Lt/Gt/Le/Ge/LogicalAnd/LogicalOr/LogicalXOr only
                    // ever appear inside a condition tree, lowered by
                    // genCondition instead - unreachable in practice.
                    return "0";
            }
        }
    }
    return "0"; // unreachable for a well-formed AST
}

std::string Codegen::genArrayIndexCode(const std::string& name,
                                        const std::vector<std::unique_ptr<ast::Expr>>& indices) {
    if (indices.size() == 1) {
        return genExpr(*indices[0], false); // Array indices are always Integer-family.
    }
    // 2D: row-major flattening via the hidden `v_name_dim1` companion
    // variable the Dim statement itself sets (see this method's own header
    // comment) - referencing the stored variable rather than re-emitting
    // the dimension-1 size expression a second time here.
    std::string idx0 = genExpr(*indices[0], false);
    std::string idx1 = genExpr(*indices[1], false);
    return "((" + idx0 + ") * (" + cppVarName(name) + "_dim1 + 1) + (" + idx1 + "))";
}

std::string Codegen::genCondition(const ast::Expr& expr) {
    if (expr.kind == ast::ExprKind::Binary) {
        const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
        switch (bin.op) {
            case ast::BinaryOp::LogicalAnd:
                return "(" + genCondition(*bin.lhs) + " && " + genCondition(*bin.rhs) + ")";
            case ast::BinaryOp::LogicalOr:
                return "(" + genCondition(*bin.lhs) + " || " + genCondition(*bin.rhs) + ")";
            case ast::BinaryOp::LogicalXOr:
                // Sema already reports an error for this (see
                // Sema::visitCondition), so main.cpp never reaches Codegen
                // for this input - a harmless placeholder is enough.
                return "false";
            case ast::BinaryOp::Eq: return "(" + genExpr(*bin.lhs, false) + " == " + genExpr(*bin.rhs, false) + ")";
            case ast::BinaryOp::Ne: return "(" + genExpr(*bin.lhs, false) + " != " + genExpr(*bin.rhs, false) + ")";
            case ast::BinaryOp::Lt: return "(" + genExpr(*bin.lhs, false) + " < " + genExpr(*bin.rhs, false) + ")";
            case ast::BinaryOp::Gt: return "(" + genExpr(*bin.lhs, false) + " > " + genExpr(*bin.rhs, false) + ")";
            case ast::BinaryOp::Le: return "(" + genExpr(*bin.lhs, false) + " <= " + genExpr(*bin.rhs, false) + ")";
            case ast::BinaryOp::Ge: return "(" + genExpr(*bin.lhs, false) + " >= " + genExpr(*bin.rhs, false) + ")";
            default:
                break; // A bare arithmetic expression used as a condition - fall through below.
        }
    } else if (expr.kind == ast::ExprKind::Unary) {
        const auto& un = static_cast<const ast::UnaryExpr&>(expr);
        if (un.op == ast::UnaryOp::LogicalNot) {
            return "(!" + genCondition(*un.operand) + ")";
        }
    }
    // PB truthiness: a bare (non-comparison) expression is true iff nonzero.
    return "(" + genExpr(expr, false) + " != 0)";
}

void Codegen::genStructures() {
    for (const auto& [name, info] : sema_.structureDeclarationOrder()) {
        out_ += "struct " + cppTypeFor(TypeSuffix::Struct, name) + " {\n";
        for (const auto& field : info.fields) {
            std::string fieldType = field.suffix == TypeSuffix::Struct
                                         ? cppTypeFor(field.suffix, field.structTypeName)
                                         : cppTypeFor(field.suffix);
            out_ += "    " + fieldType + " f_" + field.name + "{};\n";
        }
        out_ += "};\n\n";
    }
}

void Codegen::genGlobalConstants() {
    // Constants are compile-time (their initializer can only reference
    // literals and other already-declared constants, never variables), so -
    // unlike Define/Assign, whose values are computed inside main() - they
    // are hoisted out to real global `static const`s here, in source order,
    // before main() even starts. See codegen.hpp's own doc comment for why
    // this has to recurse into every nested block, not just scan
    // `module_.statements` directly.
    genConstantsIn(module_.statements);
}

void Codegen::genConstantsIn(const ast::Block& block) {
    for (const auto& stmt : block) {
        switch (stmt->kind) {
            case ast::StmtKind::ConstDecl: {
                const auto& constDecl = static_cast<const ast::ConstDeclStmt&>(*stmt);
                TypeSuffix suffix = sema_.constTypeOf(constDecl.name);
                std::string valueCode =
                    convert(genExpr(*constDecl.value, false), sema_.classify(*constDecl.value, false), suffix);
                out_ += std::string("static const ") + cppTypeFor(suffix) + " " + cppConstName(constDecl.name) +
                        " = " + valueCode + ";\n";
                break;
            }
            case ast::StmtKind::Enumeration: {
                const auto& enumStmt = static_cast<const ast::EnumerationStmt&>(*stmt);
                std::string previousName;
                for (const auto& member : enumStmt.members) {
                    std::string valueCode;
                    if (member.explicitValue) {
                        valueCode = genExpr(*member.explicitValue, false);
                    } else if (previousName.empty()) {
                        valueCode = "0";
                    } else {
                        // Chains off the *previous* member's own generated
                        // C++ constant rather than trying to fold the value
                        // ourselves - correct for any explicit value, not
                        // just literal ones, and lets the C++ compiler do
                        // the actual arithmetic.
                        valueCode = "(" + cppConstName(previousName) + " + 1)";
                    }
                    out_ += "static const std::int64_t " + cppConstName(member.name) + " = " + valueCode + ";\n";
                    previousName = member.name;
                }
                break;
            }
            case ast::StmtKind::If: {
                const auto& ifStmt = static_cast<const ast::IfStmt&>(*stmt);
                for (const auto& branch : ifStmt.branches) {
                    genConstantsIn(branch.body);
                }
                break;
            }
            case ast::StmtKind::Select: {
                const auto& sel = static_cast<const ast::SelectStmt&>(*stmt);
                for (const auto& branch : sel.cases) {
                    genConstantsIn(branch.body);
                }
                break;
            }
            case ast::StmtKind::For:
                genConstantsIn(static_cast<const ast::ForStmt&>(*stmt).body);
                break;
            case ast::StmtKind::While:
                genConstantsIn(static_cast<const ast::WhileStmt&>(*stmt).body);
                break;
            case ast::StmtKind::Repeat:
                genConstantsIn(static_cast<const ast::RepeatStmt&>(*stmt).body);
                break;
            case ast::StmtKind::ForEach:
                genConstantsIn(static_cast<const ast::ForEachStmt&>(*stmt).body);
                break;
            case ast::StmtKind::ProcedureDecl:
                // Oracle-verified legal (`Procedure Foo() : #X = 5 : ...`) -
                // a Procedure itself can never be nested (Sema now rejects
                // that, see its own ProcedureDecl notes), so recursing here
                // can't double-visit anything genProcedures() also walks.
                genConstantsIn(static_cast<const ast::ProcedureDeclStmt&>(*stmt).body);
                break;
            case ast::StmtKind::DeclareModule:
                // M7d's second slice: a module's own public constants/
                // Enumerations live in its DeclareModule section.
                genConstantsIn(static_cast<const ast::DeclareModuleStmt&>(*stmt).body);
                break;
            case ast::StmtKind::Module:
                // ...and its private ones in the matching Module section.
                genConstantsIn(static_cast<const ast::ModuleStmt&>(*stmt).body);
                break;
            default:
                break; // Nothing to collect from any other statement kind.
        }
    }
}

void Codegen::genDataPool(const ast::Block& block) {
    for (const auto& stmt : block) {
        switch (stmt->kind) {
            case ast::StmtKind::DataSection: {
                const auto& dataSection = static_cast<const ast::DataSectionStmt&>(*stmt);
                for (const auto& child : dataSection.body) {
                    if (child->kind != ast::StmtKind::Data) {
                        continue; // DataLabelStmt - no runtime code of its own; its index already came from Sema::collectDataSections.
                    }
                    const auto& data = static_cast<const ast::DataStmt&>(*child);
                    bool floatContext = familyOf(data.suffix) == ValueKind::FloatFamily;
                    for (const auto& value : data.values) {
                        std::string code =
                            convert(genExpr(*value, floatContext), sema_.classify(*value, floatContext), data.suffix);
                        switch (familyOf(data.suffix)) {
                            case ValueKind::StringFamily:
                                out_ += "    easybasic::runtime::pbDataAddString(" + code + ");\n";
                                break;
                            case ValueKind::FloatFamily:
                                out_ += "    easybasic::runtime::pbDataAddDouble(" + code + ");\n";
                                break;
                            case ValueKind::IntegerFamily:
                                out_ += "    easybasic::runtime::pbDataAddInt(" + code + ");\n";
                                break;
                        }
                    }
                }
                break;
            }
            case ast::StmtKind::If: {
                const auto& ifStmt = static_cast<const ast::IfStmt&>(*stmt);
                for (const auto& branch : ifStmt.branches) {
                    genDataPool(branch.body);
                }
                break;
            }
            case ast::StmtKind::Select: {
                const auto& sel = static_cast<const ast::SelectStmt&>(*stmt);
                for (const auto& branch : sel.cases) {
                    genDataPool(branch.body);
                }
                break;
            }
            case ast::StmtKind::For:
                genDataPool(static_cast<const ast::ForStmt&>(*stmt).body);
                break;
            case ast::StmtKind::While:
                genDataPool(static_cast<const ast::WhileStmt&>(*stmt).body);
                break;
            case ast::StmtKind::Repeat:
                genDataPool(static_cast<const ast::RepeatStmt&>(*stmt).body);
                break;
            case ast::StmtKind::ForEach:
                genDataPool(static_cast<const ast::ForEachStmt&>(*stmt).body);
                break;
            case ast::StmtKind::ProcedureDecl:
                genDataPool(static_cast<const ast::ProcedureDeclStmt&>(*stmt).body);
                break;
            case ast::StmtKind::DeclareModule:
                genDataPool(static_cast<const ast::DeclareModuleStmt&>(*stmt).body);
                break;
            case ast::StmtKind::Module:
                genDataPool(static_cast<const ast::ModuleStmt&>(*stmt).body);
                break;
            default:
                break;
        }
    }
}

void Codegen::collectDataLabelArrays(const ast::Block& block,
                                      std::vector<std::pair<std::string, std::vector<std::string>>>& out) {
    for (const auto& stmt : block) {
        switch (stmt->kind) {
            case ast::StmtKind::DataSection: {
                const auto& dataSection = static_cast<const ast::DataSectionStmt&>(*stmt);
                // Mirrors Sema::collectDataSections's own identically-named
                // local - reset per DataSection, tracking whichever label
                // this run of Data items follows.
                std::string currentLabel;
                std::vector<std::string>* currentItems = nullptr;
                for (const auto& child : dataSection.body) {
                    if (child->kind == ast::StmtKind::DataLabel) {
                        const auto& label = static_cast<const ast::DataLabelStmt&>(*child);
                        currentLabel = label.name;
                        currentItems = nullptr;
                        if (sema_.dataLabelAddressable(currentLabel)) {
                            out.emplace_back(currentLabel, std::vector<std::string>{});
                            currentItems = &out.back().second;
                        }
                        continue;
                    }
                    if (currentItems == nullptr) {
                        continue; // Not under an addressable label - genDataPool() still emits its pbDataAdd* calls.
                    }
                    const auto& data = static_cast<const ast::DataStmt&>(*child);
                    // dataLabelAddressable() already guarantees every item
                    // in this run is '.i' - genExpr'd the same way an
                    // ordinary Integer-context value would be.
                    for (const auto& value : data.values) {
                        currentItems->push_back(
                            convert(genExpr(*value, false), sema_.classify(*value, false), TypeSuffix::Integer));
                    }
                }
                break;
            }
            case ast::StmtKind::If: {
                const auto& ifStmt = static_cast<const ast::IfStmt&>(*stmt);
                for (const auto& branch : ifStmt.branches) {
                    collectDataLabelArrays(branch.body, out);
                }
                break;
            }
            case ast::StmtKind::Select: {
                const auto& sel = static_cast<const ast::SelectStmt&>(*stmt);
                for (const auto& branch : sel.cases) {
                    collectDataLabelArrays(branch.body, out);
                }
                break;
            }
            case ast::StmtKind::For:
                collectDataLabelArrays(static_cast<const ast::ForStmt&>(*stmt).body, out);
                break;
            case ast::StmtKind::While:
                collectDataLabelArrays(static_cast<const ast::WhileStmt&>(*stmt).body, out);
                break;
            case ast::StmtKind::Repeat:
                collectDataLabelArrays(static_cast<const ast::RepeatStmt&>(*stmt).body, out);
                break;
            case ast::StmtKind::ForEach:
                collectDataLabelArrays(static_cast<const ast::ForEachStmt&>(*stmt).body, out);
                break;
            case ast::StmtKind::ProcedureDecl:
                collectDataLabelArrays(static_cast<const ast::ProcedureDeclStmt&>(*stmt).body, out);
                break;
            case ast::StmtKind::DeclareModule:
                collectDataLabelArrays(static_cast<const ast::DeclareModuleStmt&>(*stmt).body, out);
                break;
            case ast::StmtKind::Module:
                collectDataLabelArrays(static_cast<const ast::ModuleStmt&>(*stmt).body, out);
                break;
            default:
                break;
        }
    }
}

void Codegen::genDataLabelArrays() {
    std::vector<std::pair<std::string, std::vector<std::string>>> labels;
    collectDataLabelArrays(module_.statements, labels);
    for (const auto& [name, items] : labels) {
        out_ += "static const std::array<std::int64_t, " + std::to_string(items.size()) + "> pb_label_" +
                sanitizeModuleQualifier(name) + " = {";
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i != 0) {
                out_ += ", ";
            }
            out_ += items[i];
        }
        out_ += "};\n";
    }
}

void Codegen::genDeclarePrototypes() {
    // A plain, top-level-only loop (PB doesn't nest procedures, so a
    // `Declare` was never found anywhere else) - extended for M7d: a
    // DeclareModuleStmt's own body is the *only* other place a `Declare`
    // can appear (a module's own public procedure signatures), one level
    // of nesting, never more (modules don't nest).
    auto emitPrototypesIn = [this](const ast::Block& block) {
        for (const auto& stmt : block) {
            if (stmt->kind != ast::StmtKind::Declare) {
                continue;
            }
            const auto& decl = static_cast<const ast::DeclareStmt&>(*stmt);
            const Sema::ProcedureInfo* info = sema_.procedureInfo(decl.name);
            TypeSuffix returnSuffix = info != nullptr ? info->returnSuffix : TypeSuffix::Integer;
            out_ += std::string(cppTypeFor(returnSuffix)) + " " + cppProcName(decl.name) + "(";
            for (std::size_t i = 0; i < decl.params.size(); ++i) {
                if (i != 0) {
                    out_ += ", ";
                }
                TypeSuffix paramSuffix = info != nullptr && i < info->paramSuffixes.size() ? info->paramSuffixes[i]
                                                                                            : TypeSuffix::Integer;
                out_ += cppTypeFor(paramSuffix);
            }
            out_ += ");\n";
        }
    };
    emitPrototypesIn(module_.statements);
    for (const auto& stmt : module_.statements) {
        if (stmt->kind == ast::StmtKind::DeclareModule) {
            emitPrototypesIn(static_cast<const ast::DeclareModuleStmt&>(*stmt).body);
        }
    }
}

void Codegen::genProcedures() {
    // Same top-level-only-plus-one-level-of-Module-nesting shape as
    // genDeclarePrototypes's own (M7d): a ModuleStmt's own body is the
    // *only* other place a real `Procedure` definition can appear.
    auto emitProceduresIn = [this](const ast::Block& block) {
        for (const auto& stmt : block) {
            if (stmt->kind == ast::StmtKind::ProcedureDecl) {
                genProcedureDecl(static_cast<const ast::ProcedureDeclStmt&>(*stmt));
            }
        }
    };
    emitProceduresIn(module_.statements);
    for (const auto& stmt : module_.statements) {
        if (stmt->kind == ast::StmtKind::Module) {
            emitProceduresIn(static_cast<const ast::ModuleStmt&>(*stmt).body);
        }
    }
}

void Codegen::genProcedureDecl(const ast::ProcedureDeclStmt& proc) {
    const Sema::ProcedureInfo* info = sema_.procedureInfo(proc.name);
    TypeSuffix returnSuffix = info != nullptr ? info->returnSuffix : TypeSuffix::Integer;

    out_ += std::string(cppTypeFor(returnSuffix)) + " " + cppProcName(proc.name) + "(";
    for (std::size_t i = 0; i < proc.params.size(); ++i) {
        if (i != 0) {
            out_ += ", ";
        }
        const auto& param = proc.params[i];
        TypeSuffix paramSuffix = info != nullptr && i < info->paramSuffixes.size() ? info->paramSuffixes[i] : TypeSuffix::Integer;
        out_ += std::string(cppTypeFor(paramSuffix)) + " " + cppVarName(param.name);
        if (param.defaultValue) {
            // A real C++ default parameter - the callee's own declaration
            // supplies it natively, so a call site omitting the argument
            // needs no special handling at all (see genExpr's Call case).
            bool floatCtx = familyOf(paramSuffix) == ValueKind::FloatFamily;
            out_ += " = " + convert(genExpr(*param.defaultValue, floatCtx),
                                     sema_.classify(*param.defaultValue, floatCtx), paramSuffix);
        }
    }
    out_ += ") {\n";

    // Non-parameter locals (params are already real C++ parameters, so
    // they're skipped here - see Sema::ProcedureInfo::locals's own comment).
    if (info != nullptr) {
        for (std::size_t i = info->paramSuffixes.size(); i < info->locals.size(); ++i) {
            const auto& [name, suffix] = info->locals[i];
            std::string cppType =
                suffix == TypeSuffix::Struct ? cppTypeFor(suffix, sema_.structTypeOfVar(name)) : cppTypeFor(suffix);
            out_ += "    " + cppType + " " + cppVarName(name) + "{};\n";
        }
    }

    TypeSuffix savedReturnSuffix = currentProcReturnSuffix_;
    currentProcReturnSuffix_ = returnSuffix;
    const Sema::ProcedureInfo* savedProcInfo = currentProcInfo_;
    currentProcInfo_ = info;
    genBlock(proc.body);
    currentProcInfo_ = savedProcInfo;
    currentProcReturnSuffix_ = savedReturnSuffix;

    // Fallthrough safety net (mirrors eBasic's own identical pattern):
    // oracle-verified that falling off the end of a PB procedure returns
    // the declared return type's zero value, which is undefined behavior
    // for a non-void C++ function without this - never rely on every
    // control-flow path having hit an explicit ProcedureReturn.
    out_ += "    return " + defaultValueLiteral(returnSuffix) + ";\n";
    out_ += "}\n\n";
}

void Codegen::genBlock(const ast::Block& block) {
    for (const auto& stmt : block) {
        genStmt(*stmt);
    }
}

void Codegen::genStmt(const ast::Stmt& stmt) {
    switch (stmt.kind) {
        case ast::StmtKind::Define: {
            const auto& def = static_cast<const ast::DefineStmt&>(stmt);
            for (const auto& decl : def.declarators) {
                if (!decl.init) {
                    continue; // global already default-initialized to zero/empty
                }
                TypeSuffix targetSuffix = sema_.typeOf(decl.name);
                bool floatContext = familyOf(targetSuffix) == ValueKind::FloatFamily;
                std::string rhs =
                    convert(genExpr(*decl.init, floatContext), sema_.classify(*decl.init, floatContext), targetSuffix);
                out_ += "    " + cppVarName(decl.name) + " = " + rhs + ";\n";
            }
            break;
        }
        case ast::StmtKind::Assign: {
            const auto& assign = static_cast<const ast::AssignStmt&>(stmt);
            TypeSuffix targetSuffix = sema_.typeOf(assign.name);
            bool floatContext = familyOf(targetSuffix) == ValueKind::FloatFamily;
            std::string rhs =
                convert(genExpr(*assign.value, floatContext), sema_.classify(*assign.value, floatContext), targetSuffix);
            out_ += "    " + cppVarName(assign.name) + " = " + rhs + ";\n";
            break;
        }
        case ast::StmtKind::Debug: {
            if (!debugMode_) {
                break; // Stripped entirely, exactly like a plain (non `-d`) pbcompilerc build.
            }
            const auto& dbg = static_cast<const ast::DebugStmt&>(stmt);
            // A bare Debug'd expression has no destination at all, so it
            // uses PB's un-forced, bottom-up classification (see the
            // `Debug 7/2` -> `3` oracle example in Sema::classify's doc).
            ValueKind family = sema_.familyOfExpr(*dbg.value);
            std::string valueCode = genExpr(*dbg.value, false);
            std::string textCode;
            switch (family) {
                case ValueKind::StringFamily:
                    textCode = valueCode + ".bytes()";
                    break;
                case ValueKind::FloatFamily: {
                    // Oracle-verified: a raw Float and a raw Double each
                    // print via their own, different format when Debug'd
                    // directly (see pbDebugFormatFloat/Double's own doc
                    // comments) - resolveType() correctly distinguishes
                    // them for a VarRef/Call/FieldAccess (which covers
                    // every M4b Math builtin, all Double-returning); an
                    // arbitrary Binary/Unary expression it can't resolve
                    // defaults to Integer, so falling back to the Double
                    // format here (Double, not Integer's own format) is
                    // the correct interpretation of that fallback given
                    // `family` already says this is FloatFamily.
                    TypeSuffix suffix = sema_.resolveType(*dbg.value).suffix;
                    const char* formatter =
                        suffix == TypeSuffix::Float ? "pbDebugFormatFloat" : "pbDebugFormatDouble";
                    textCode = std::string("easybasic::runtime::") + formatter + "(" + valueCode + ")";
                    break;
                }
                case ValueKind::IntegerFamily:
                    textCode = "std::to_string(" + valueCode + ")";
                    break;
            }
            out_ += "    easybasic::runtime::debugPrint(" + textCode + ");\n";
            break;
        }
        case ast::StmtKind::If: {
            const auto& ifStmt = static_cast<const ast::IfStmt&>(stmt);
            for (std::size_t i = 0; i < ifStmt.branches.size(); ++i) {
                const auto& branch = ifStmt.branches[i];
                if (branch.condition) {
                    out_ += std::string(i == 0 ? "    if (" : "    else if (") + genCondition(*branch.condition) + ") {\n";
                } else {
                    out_ += "    else {\n";
                }
                genBlock(branch.body);
                out_ += "    }\n";
            }
            break;
        }
        case ast::StmtKind::Select: {
            // Lowered to an if/else-if chain of equality checks rather than
            // a C++ switch: PB's Select works on arbitrary runtime values
            // (including PBString), not just integral compile-time
            // constants, which a real `switch` requires.
            const auto& sel = static_cast<const ast::SelectStmt&>(stmt);
            std::string tempName = "sel" + std::to_string(tempCounter_++);
            out_ += "    { auto " + tempName + " = " + genExpr(*sel.selector, false) + ";\n";
            bool first = true;
            for (const auto& branch : sel.cases) {
                if (branch.values.empty()) {
                    out_ += "    else {\n"; // Default - PB has no CaseElse (oracle-verified rejected).
                } else {
                    std::string cond;
                    for (std::size_t i = 0; i < branch.values.size(); ++i) {
                        if (i != 0) {
                            cond += " || ";
                        }
                        cond += "(" + tempName + " == " + genExpr(*branch.values[i], false) + ")";
                    }
                    out_ += std::string(first ? "    if (" : "    else if (") + cond + ") {\n";
                }
                first = false;
                genBlock(branch.body);
                out_ += "    }\n";
            }
            out_ += "    }\n";
            break;
        }
        case ast::StmtKind::CompilerIf:
        case ast::StmtKind::CompilerSelect:
            // Unreachable: Sema::preResolveCompilerDirectives() splices
            // every CompilerIf/CompilerSelect node out of the tree
            // (replacing it with its selected branch's own statements)
            // before Codegen ever runs - see its own doc comment.
        case ast::StmtKind::DataSection:
        case ast::StmtKind::DataLabel:
        case ast::StmtKind::Data:
            // No code at this position at all: genDataPool() (called once,
            // up front, at the very start of generated main()) already
            // emitted every Data value's pbDataAddX() call, independent of
            // where the DataSection textually appears - see its own doc
            // comment. Grouped with the CompilerIf/CompilerSelect case just
            // above (rather than its own identical `break;`) since clang-
            // tidy's bugprone-branch-clone flags two adjacent switch cases
            // with the same body - both exist purely so -Wswitch stays an
            // exhaustiveness net for this enum.
            break;
        case ast::StmtKind::Read: {
            const auto& read = static_cast<const ast::ReadStmt&>(stmt);
            TypeSuffix dataSuffix = read.suffix == TypeSuffix::None ? TypeSuffix::Integer : read.suffix;
            std::string readCall;
            switch (familyOf(dataSuffix)) {
                case ValueKind::StringFamily:
                    readCall = "easybasic::runtime::pbReadDataString()";
                    break;
                case ValueKind::FloatFamily:
                    readCall = "easybasic::runtime::pbReadDataDouble()";
                    break;
                case ValueKind::IntegerFamily:
                    readCall = "easybasic::runtime::pbReadDataInt()";
                    break;
            }
            if (debugMode_) {
                // Oracle-verified: exhausting the data pool is a fatal
                // error only in a debug (`-d`) build - see
                // pbDataReadError's own doc comment.
                out_ += "    if (!easybasic::runtime::pbDataHasMore()) { easybasic::runtime::pbDataReadError(); }\n";
            }
            TypeSuffix targetSuffix = sema_.typeOf(read.varName);
            // `readCall` already returns a value whose C++ type genuinely
            // matches `dataSuffix`'s own family (pbReadDataInt/Double/
            // String each coerce internally from whatever the pool item's
            // *actual* stored kind is - see datalib.hpp's own notes);
            // convertReadValue then handles the second stage - `dataSuffix`
            // (Read's own declared suffix) to `targetSuffix` (the
            // destination variable's own type), which oracle-verified can
            // itself be a legal cross-family String<->numeric coercion.
            out_ += "    " + cppVarName(read.varName) + " = " +
                    convertReadValue(readCall, familyOf(dataSuffix), targetSuffix) + ";\n";
            break;
        }
        case ast::StmtKind::Restore: {
            const auto& restore = static_cast<const ast::RestoreStmt&>(stmt);
            auto index = sema_.dataLabelIndex(restore.labelName);
            // Sema already rejected an unknown label with a diagnostic (the
            // driver stops before Codegen once any error is recorded - see
            // main.cpp), so reaching here with no index at all shouldn't
            // happen; falling back to 0 keeps this defensive rather than UB.
            out_ += "    easybasic::runtime::pbDataRestore(" + std::to_string(index.value_or(0)) + ");\n";
            break;
        }
        case ast::StmtKind::For: {
            const auto& forStmt = static_cast<const ast::ForStmt&>(stmt);
            TypeSuffix suffix = sema_.typeOf(forStmt.varName);
            bool floatContext = familyOf(suffix) == ValueKind::FloatFamily;
            std::string varType = cppTypeFor(suffix);
            std::string fromCode =
                convert(genExpr(*forStmt.from, floatContext), sema_.classify(*forStmt.from, floatContext), suffix);
            std::string toCode =
                convert(genExpr(*forStmt.to, floatContext), sema_.classify(*forStmt.to, floatContext), suffix);
            std::string stepCode =
                forStmt.step
                    ? convert(genExpr(*forStmt.step, floatContext), sema_.classify(*forStmt.step, floatContext), suffix)
                    : "1";
            std::string tmp = std::to_string(tempCounter_++);
            // `to`/`step` are evaluated once at loop entry, not re-evaluated
            // per iteration (matches PB semantics); the step's sign (also
            // captured once) picks the loop direction at runtime, since PB
            // allows a negative Step to count down.
            out_ += "    { " + varType + " forTo" + tmp + " = " + toCode + "; " + varType + " forStep" + tmp +
                    " = " + stepCode + ";\n";
            out_ += "    for (v_" + forStmt.varName + " = " + fromCode + "; (forStep" + tmp + " >= 0) ? (v_" +
                    forStmt.varName + " <= forTo" + tmp + ") : (v_" + forStmt.varName + " >= forTo" + tmp +
                    "); v_" + forStmt.varName + " += forStep" + tmp + ") {\n";
            genBlock(forStmt.body);
            out_ += "    }\n    }\n";
            break;
        }
        case ast::StmtKind::While: {
            const auto& whileStmt = static_cast<const ast::WhileStmt&>(stmt);
            out_ += "    while (" + genCondition(*whileStmt.condition) + ") {\n";
            genBlock(whileStmt.body);
            out_ += "    }\n";
            break;
        }
        case ast::StmtKind::Repeat: {
            const auto& repeatStmt = static_cast<const ast::RepeatStmt&>(stmt);
            if (repeatStmt.untilCondition) {
                out_ += "    do {\n";
                genBlock(repeatStmt.body);
                // PB's Repeat/Until loops until the condition becomes true
                // (i.e. continues while it's false) - the opposite sense of
                // C++'s do/while, hence the negation.
                out_ += "    } while (!(" + genCondition(*repeatStmt.untilCondition) + "));\n";
            } else {
                out_ += "    for (;;) {\n"; // Repeat/ForEver: unconditional.
                genBlock(repeatStmt.body);
                out_ += "    }\n";
            }
            break;
        }
        case ast::StmtKind::Break:
            out_ += "    break;\n";
            break;
        case ast::StmtKind::Continue:
            out_ += "    continue;\n";
            break;
        case ast::StmtKind::EnableExplicit: // NOLINT(bugprone-branch-clone) - a Sema-only directive; nothing to generate.
            break;
        case ast::StmtKind::ConstDecl:
        case ast::StmtKind::Enumeration:
        case ast::StmtKind::ProcedureDecl: // NOLINT(bugprone-branch-clone) - already emitted by genProcedures().
            break; // Already emitted as globals/functions by genGlobalConstants()/genProcedures().
        case ast::StmtKind::Declare:
            // Sema-only (forward-declares a signature in `procedures_` for
            // mutual recursion) - nothing to generate; the real `Procedure`
            // it promises is what genProcedures() actually emits.
            break;
        case ast::StmtKind::ProcedureReturn: {
            const auto& ret = static_cast<const ast::ProcedureReturnStmt&>(stmt);
            if (ret.value) {
                bool floatCtx = familyOf(currentProcReturnSuffix_) == ValueKind::FloatFamily;
                std::string code = convert(genExpr(*ret.value, floatCtx), sema_.classify(*ret.value, floatCtx),
                                            currentProcReturnSuffix_);
                out_ += "    return " + code + ";\n";
            } else {
                out_ += "    return " + defaultValueLiteral(currentProcReturnSuffix_) + ";\n";
            }
            break;
        }
        case ast::StmtKind::ExprStmt: {
            const auto& exprStmt = static_cast<const ast::ExprStmt&>(stmt);
            out_ += "    " + genExpr(*exprStmt.expr, false) + ";\n";
            break;
        }
        case ast::StmtKind::Shared:
            // Nothing to generate: Sema already resolved each shared name
            // to the real outer `v_name` (see its own notes) - by the time
            // Codegen sees a VarRef inside this body, it just emits `v_name`
            // like any other reference, which already means the right thing.
            break;
        case ast::StmtKind::Dim: {
            const auto& dim = static_cast<const ast::DimStmt&>(stmt);
            const Sema::ArrayInfo* info = sema_.arrayInfo(dim.name);
            TypeSuffix elemSuffix = info != nullptr ? info->elementSuffix : TypeSuffix::Integer;
            std::string elemStructName = info != nullptr ? info->elementStructName : "";
            std::string sizeExpr0 = genExpr(*dim.dimensionSizes[0], false);
            if (dim.dimensionSizes.size() == 1) {
                // `Dim` (re-)sizes and resets the array at this exact
                // statement position, not at global-init time - the size
                // expression may depend on runtime values computed earlier
                // (oracle-verified: PB allows an arbitrary expression here,
                // not just a compile-time constant).
                out_ += "    " + cppVarName(dim.name) + ".assign(static_cast<std::size_t>(" + sizeExpr0 + ") + 1, " +
                        defaultValueLiteral(elemSuffix, elemStructName) + ");\n";
            } else {
                std::string sizeExpr1 = genExpr(*dim.dimensionSizes[1], false);
                out_ += "    " + cppVarName(dim.name) + "_dim1 = " + sizeExpr1 + ";\n";
                out_ += "    " + cppVarName(dim.name) + ".assign(static_cast<std::size_t>(" + sizeExpr0 +
                        " + 1) * static_cast<std::size_t>(" + cppVarName(dim.name) + "_dim1 + 1), " +
                        defaultValueLiteral(elemSuffix, elemStructName) + ");\n";
            }
            break;
        }
        case ast::StmtKind::IndexAssign: {
            const auto& indexAssign = static_cast<const ast::IndexAssignStmt&>(stmt);
            if (indexAssign.indices.empty()) {
                if (const Sema::ListInfo* listInfo = sema_.listInfo(indexAssign.name)) {
                    // `name() = expr` - sets the List's current element (M3e).
                    bool floatCtx = familyOf(listInfo->elementSuffix) == ValueKind::FloatFamily;
                    std::string valueCode = convert(genExpr(*indexAssign.value, floatCtx),
                                                     sema_.classify(*indexAssign.value, floatCtx),
                                                     listInfo->elementSuffix);
                    out_ += "    " + cppVarName(indexAssign.name) + ".current() = " + valueCode + ";\n";
                    break;
                }
                // `name() = expr` also sets a Map's *current* element (M3f).
                if (const Sema::MapInfo* mapInfo = sema_.mapInfo(indexAssign.name)) {
                    bool floatCtx = familyOf(mapInfo->elementSuffix) == ValueKind::FloatFamily;
                    std::string valueCode = convert(genExpr(*indexAssign.value, floatCtx),
                                                     sema_.classify(*indexAssign.value, floatCtx),
                                                     mapInfo->elementSuffix);
                    out_ += "    " + cppVarName(indexAssign.name) + ".current() = " + valueCode + ";\n";
                    break;
                }
            }
            if (indexAssign.indices.size() == 1) {
                if (const Sema::MapInfo* mapInfo = sema_.mapInfo(indexAssign.name)) {
                    // `name(key) = expr` - writes (auto-creating if absent)
                    // the Map's element at `key` (M3f).
                    bool floatCtx = familyOf(mapInfo->elementSuffix) == ValueKind::FloatFamily;
                    std::string valueCode = convert(genExpr(*indexAssign.value, floatCtx),
                                                     sema_.classify(*indexAssign.value, floatCtx),
                                                     mapInfo->elementSuffix);
                    out_ += "    " + cppVarName(indexAssign.name) + ".access(" +
                            genExpr(*indexAssign.indices.front(), false) + ") = " + valueCode + ";\n";
                    break;
                }
            }
            const Sema::ArrayInfo* info = sema_.arrayInfo(indexAssign.name);
            TypeSuffix elemSuffix = info != nullptr ? info->elementSuffix : TypeSuffix::Integer;
            bool floatCtx = familyOf(elemSuffix) == ValueKind::FloatFamily;
            std::string valueCode = convert(genExpr(*indexAssign.value, floatCtx),
                                             sema_.classify(*indexAssign.value, floatCtx), elemSuffix);
            out_ += "    " + cppVarName(indexAssign.name) + ".at(" +
                    genArrayIndexCode(indexAssign.name, indexAssign.indices) + ") = " + valueCode + ";\n";
            break;
        }
        case ast::StmtKind::StructureDecl: // NOLINT(bugprone-branch-clone) - already emitted by genStructures().
            break;
        case ast::StmtKind::InterfaceDecl: // NOLINT(bugprone-branch-clone) - pure metadata, consumed by Sema only.
            break;
        case ast::StmtKind::DeclareModule: {
            // Oracle-verified: a DeclareModule section's own code runs at
            // its own textual position, same as a Module section's (see
            // ModuleStmt's own doc comment) - a `Global` declarator's own
            // initializer and a `Dim`'s own runtime resize/assign both
            // need real emission here; every other kind DeclareModule's own
            // body can contain (`Declare`/`Structure`/`Enumeration`/a
            // `#Constant`/`NewList`/`NewMap`/`DataSection`) already has its
            // own no-op case elsewhere in this very switch (its C++ side
            // effects, if any, already emitted by genStructures()/
            // genConstantsIn()/genDataPool()/genDeclarePrototypes(), each
            // already extended to recurse into a DeclareModule's own body -
            // see their own doc comments) - so, like Module's own case just
            // below, every statement is simply genStmt'd here with no
            // separate kind-filtering needed.
            const auto& decl = static_cast<const ast::DeclareModuleStmt&>(stmt);
            for (const auto& bodyStmt : decl.body) {
                genStmt(*bodyStmt);
            }
            break;
        }
        case ast::StmtKind::Module: {
            // Oracle-verified: a Module's own top-level body isn't just
            // declarations - ordinary executable code runs right there, at
            // its own textual position (see Sema's identical note on its
            // own Module case) - so every statement is genStmt'd here,
            // unlike DeclareModule's own Define-only case just above.
            // ProcedureDecl/Declare/UseModule/UnuseModule already have
            // their own no-op cases in this very switch (already emitted
            // elsewhere, or carry no runtime code at all), so this needs
            // no separate statement-kind filtering of its own.
            const auto& mod = static_cast<const ast::ModuleStmt&>(stmt);
            for (const auto& bodyStmt : mod.body) {
                genStmt(*bodyStmt);
            }
            break;
        }
        case ast::StmtKind::UseModule: // NOLINT(bugprone-branch-clone) - compile-time only, consumed by Sema.
            break;
        case ast::StmtKind::UnuseModule: // NOLINT(bugprone-branch-clone) - compile-time only, consumed by Sema.
            break;
        case ast::StmtKind::NewList: // NOLINT(bugprone-branch-clone) - already emitted by generate() itself.
            break;
        case ast::StmtKind::NewMap: // NOLINT(bugprone-branch-clone) - already emitted by generate() itself.
            break;
        case ast::StmtKind::ForEach: {
            const auto& forEach = static_cast<const ast::ForEachStmt&>(stmt);
            std::string listVar = cppVarName(forEach.name);
            // Mirrors real PB's own generated code exactly:
            // `PB_ResetList(...); while (PB_NextElement(...)) { ... }`.
            out_ += "    " + listVar + ".resetForEach();\n";
            out_ += "    while (" + listVar + ".nextElement()) {\n";
            genBlock(forEach.body);
            out_ += "    }\n";
            break;
        }
        case ast::StmtKind::FieldAssign: {
            const auto& fieldAssign = static_cast<const ast::FieldAssignStmt&>(stmt);
            Sema::ResolvedType targetType = sema_.resolveType(*fieldAssign.target);
            bool floatCtx = familyOf(targetType.suffix) == ValueKind::FloatFamily;
            std::string valueCode = convert(genExpr(*fieldAssign.value, floatCtx),
                                             sema_.classify(*fieldAssign.value, floatCtx), targetType.suffix);
            out_ += "    " + genExpr(*fieldAssign.target, false) + " = " + valueCode + ";\n";
            break;
        }
    }
}

std::string Codegen::generate() {
    out_.clear();
    out_ += "// Generated by pbcxx - do not edit.\n";
    out_ += "#include <array>\n"; // std::array - DataSection labels addressable via ?Label (M7c).
    out_ += "#include <cstdint>\n";
    out_ += "#include <cstdlib>\n";
    out_ += "#include <cmath>\n";
    out_ += "#include <string>\n";
    out_ += "#include <vector>\n";
    out_ += "#include <easybasic/runtime/runtime.hpp>\n";
    // guilib.hpp is deliberately *not* part of the plain runtime.hpp
    // umbrella (unlike every other runtime library) - it transitively
    // includes <gtk/gtk.h>, which a program that never touches the GUI
    // library shouldn't need installed at all just to compile. Only
    // included here when Sema has already confirmed the program actually
    // calls a GUI builtin - see Sema::usesGuiLibrary()'s own doc comment,
    // and main.cpp's matching conditional GTK3 compile/link flags.
    if (sema_.usesGuiLibrary()) {
        out_ += "#include <easybasic/runtime/guilib.hpp>\n";
    }
    out_ += "\n";

    genStructures(); // must precede any variable/array/procedure that might be an instance of one
    genGlobalConstants();
    for (const auto& [name, suffix] : sema_.declarationOrder()) {
        std::string cppType = suffix == TypeSuffix::Struct ? cppTypeFor(suffix, sema_.structTypeOfVar(name))
                                                             : cppTypeFor(suffix);
        out_ += "static " + cppType + " " + cppVarName(name) + "{};\n";
    }
    for (const auto& [name, info] : sema_.arrayDeclarationOrder()) {
        std::string elemType = info.elementSuffix == TypeSuffix::Struct
                                    ? cppTypeFor(info.elementSuffix, info.elementStructName)
                                    : cppTypeFor(info.elementSuffix);
        out_ += "static std::vector<" + elemType + "> " + cppVarName(name) + ";\n";
        if (info.dimensionCount == 2) {
            out_ += "static std::int64_t " + cppVarName(name) + "_dim1 = 0;\n";
        }
    }
    for (const auto& [name, info] : sema_.listDeclarationOrder()) {
        std::string elemType = info.elementSuffix == TypeSuffix::Struct
                                    ? cppTypeFor(info.elementSuffix, info.elementStructName)
                                    : cppTypeFor(info.elementSuffix);
        out_ += "static easybasic::runtime::PBList<" + elemType + "> " + cppVarName(name) + ";\n";
    }
    for (const auto& [name, info] : sema_.mapDeclarationOrder()) {
        std::string elemType = info.elementSuffix == TypeSuffix::Struct
                                    ? cppTypeFor(info.elementSuffix, info.elementStructName)
                                    : cppTypeFor(info.elementSuffix);
        out_ += "static easybasic::runtime::PBMap<" + elemType + "> " + cppVarName(name) + ";\n";
    }
    out_ += "\n";
    // Global variable declarations MUST come before procedure definitions:
    // a `Global`/`Shared`-accessed name is emitted by a procedure body as a
    // direct reference to this same file-scope `v_name` (see Sema's own
    // notes on how Global/Shared pre-populate a procedure's local scope
    // without adding to its locals list) - C++ needs the declaration
    // visible first, unlike PB itself which has no such ordering concern.
    genDeclarePrototypes();
    genProcedures();
    // Must come after genProcedures(): a label's own array element can be
    // `@Procedure()` (M7c's own Interface vtable use case), which needs the
    // real function already declared - see genDataLabelArrays's own doc
    // comment.
    genDataLabelArrays();
    out_ += "\nint main() {\n";
    genDataPool(module_.statements);
    for (const auto& stmt : module_.statements) {
        genStmt(*stmt);
    }
    out_ += "    return 0;\n}\n";
    return out_;
}

} // namespace easybasic
