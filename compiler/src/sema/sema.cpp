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

Sema::Sema(DiagnosticEngine& diagnostics) : diagnostics_(diagnostics) {}

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

bool Sema::analyze(ast::Module& module) {
    for (auto& stmt : module.statements) {
        visitStmt(*stmt);
    }
    return !diagnostics_.hasErrors();
}

void Sema::visitStmt(ast::Stmt& stmt) {
    switch (stmt.kind) {
        case ast::StmtKind::Define: {
            auto& def = static_cast<ast::DefineStmt&>(stmt);
            for (auto& decl : def.declarators) {
                TypeSuffix suffix = decl.suffix == TypeSuffix::None ? TypeSuffix::Integer : decl.suffix;
                declare(decl.name, decl.spelling, suffix, def.loc);
                if (decl.init) {
                    visitExpr(*decl.init);
                    checkAssignable(decl.spelling, suffix, *decl.init, def.loc);
                }
            }
            break;
        }
        case ast::StmtKind::Assign: {
            auto& assign = static_cast<ast::AssignStmt&>(stmt);
            // A plain assignment to a name Sema hasn't seen yet is PB's
            // implicit-declaration behavior (default type Integer, exactly
            // like real PureBasic with EnableExplicit off) - EnableExplicit
            // itself is a later milestone (M1).
            TypeSuffix suffix = assign.suffix == TypeSuffix::None ? typeOf(assign.name) : assign.suffix;
            declare(assign.name, assign.spelling, suffix, assign.loc);
            visitExpr(*assign.value);
            checkAssignable(assign.spelling, symbols_[assign.name], *assign.value, assign.loc);
            break;
        }
        case ast::StmtKind::Debug: {
            auto& dbg = static_cast<ast::DebugStmt&>(stmt);
            visitExpr(*dbg.value);
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
                // type Integer and value 0 rather than erroring (again,
                // EnableExplicit's stricter behavior is M1's job).
                declare(ref.name, ref.spelling, TypeSuffix::Integer, ref.loc);
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
        case ast::ExprKind::IntLiteral:
        case ast::ExprKind::FloatLiteral:
        case ast::ExprKind::StringLiteral:
            break;
    }
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

TypeSuffix Sema::typeOf(const std::string& lowerName) const {
    auto it = symbols_.find(lowerName);
    return it == symbols_.end() ? TypeSuffix::Integer : it->second;
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
        case ast::ExprKind::Unary: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            return classify(*un.operand, floatContext);
        }
        case ast::ExprKind::Binary: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
            if (bin.op == ast::BinaryOp::Div || bin.op == ast::BinaryOp::Mod) {
                // Target-typed: real (Div) / real-modulo (Mod) exactly when
                // floatContext is set OR either operand is *naturally*
                // (context-free) float-family - oracle-verified for Div,
                // see classify()'s own doc comment for the distinguishing
                // examples; Mod follows by consistent extrapolation only.
                bool lhsNaturallyFloat = classify(*bin.lhs, false) == ValueKind::FloatFamily;
                bool rhsNaturallyFloat = classify(*bin.rhs, false) == ValueKind::FloatFamily;
                return (floatContext || lhsNaturallyFloat || rhsNaturallyFloat) ? ValueKind::FloatFamily
                                                                                 : ValueKind::IntegerFamily;
            }
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
    }
    return ValueKind::IntegerFamily;
}

} // namespace easybasic
