#include "codegen.hpp"

#include <iomanip>
#include <sstream>

namespace easybasic {

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

} // namespace

Codegen::Codegen(const ast::Module& module, const Sema& sema, bool debugMode)
    : module_(module), sema_(sema), debugMode_(debugMode) {}

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
            return "v_" + ref.name;
        }
        case ast::ExprKind::Unary: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            return "(-" + genExpr(*un.operand, floatContext) + ")";
        }
        case ast::ExprKind::Binary: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
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
            }
            break;
        }
    }
    return "0"; // unreachable for a well-formed AST
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
                out_ += "    v_" + decl.name + " = " + rhs + ";\n";
            }
            break;
        }
        case ast::StmtKind::Assign: {
            const auto& assign = static_cast<const ast::AssignStmt&>(stmt);
            TypeSuffix targetSuffix = sema_.typeOf(assign.name);
            bool floatContext = familyOf(targetSuffix) == ValueKind::FloatFamily;
            std::string rhs =
                convert(genExpr(*assign.value, floatContext), sema_.classify(*assign.value, floatContext), targetSuffix);
            out_ += "    v_" + assign.name + " = " + rhs + ";\n";
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
                case ValueKind::FloatFamily:
                    // A reasonable default, not yet PB's exact Str()
                    // formatting rule for floats - that lands with the
                    // String library (M4).
                case ValueKind::IntegerFamily:
                    textCode = "std::to_string(" + valueCode + ")";
                    break;
            }
            out_ += "    easybasic::runtime::debugPrint(" + textCode + ");\n";
            break;
        }
    }
}

std::string Codegen::generate() {
    out_.clear();
    out_ += "// Generated by pbcxx - do not edit.\n";
    out_ += "#include <cstdint>\n";
    out_ += "#include <cmath>\n";
    out_ += "#include <string>\n";
    out_ += "#include <easybasic/runtime/runtime.hpp>\n\n";

    for (const auto& [name, suffix] : sema_.declarationOrder()) {
        out_ += std::string("static ") + cppTypeFor(suffix) + " v_" + name + "{};\n";
    }
    out_ += "\nint main() {\n";
    for (const auto& stmt : module_.statements) {
        genStmt(*stmt);
    }
    out_ += "    return 0;\n}\n";
    return out_;
}

} // namespace easybasic
