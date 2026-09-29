#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../ast/ast.hpp"
#include "../diagnostics/diagnostics.hpp"

namespace easybasic {

/// The three type "families" Sema distinguishes for M0's numeric/string
/// subset. This is deliberately coarser than the full 11-suffix type system
/// (that distinction matters for codegen's chosen C++ type, but not for the
/// handful of family-level rules Sema itself needs to enforce - e.g. "is
/// this side of `/` promoted to floating-point").
enum class ValueKind { IntegerFamily, FloatFamily, StringFamily };

ValueKind familyOf(TypeSuffix suffix);

/// Resolves variable types (via `Define` or PB's own implicit-declaration
/// behavior - a plain assignment to an unseen name declares it, defaulting
/// to Integer, exactly like real PureBasic with `EnableExplicit` off) and
/// annotates enough about each expression for Codegen to get PB's numeric
/// conversion rules right without re-deriving them itself.
class Sema {
public:
    explicit Sema(DiagnosticEngine& diagnostics);

    /// Walks the whole module, populating the symbol table in place.
    /// Returns false if any error was reported (mirrors DiagnosticEngine's
    /// own hasErrors(), exposed here for driver convenience).
    bool analyze(ast::Module& module);

    /// The resolved type-suffix for a declared/implicitly-declared name, or
    /// TypeSuffix::Integer if never seen (should not happen after a
    /// successful analyze() on a well-formed module).
    TypeSuffix typeOf(const std::string& lowerName) const;

    /// The Structure type name for a variable whose typeOf() is
    /// TypeSuffix::Struct, or an empty string otherwise. Used by Codegen to
    /// pick the real C++ type (`s_<name>`) for a struct-typed variable's
    /// own declaration.
    const std::string& structTypeOfVar(const std::string& lowerName) const;

    /// Bottom-up family classification with no destination context - what an
    /// expression "naturally" is when nothing forces it otherwise. Used for
    /// a `Debug`'d expression (which has no destination type at all) and for
    /// checkAssignable's String-vs-numeric check (context never turns a
    /// String into a numeric family or vice versa, so the distinction below
    /// doesn't matter for that check).
    ValueKind familyOfExpr(const ast::Expr& expr) const;

    /// PB's `/` (and, by consistent extrapolation - not yet independently
    /// oracle-disambiguated, see docs/architecture/roadmap.md's M0 notes -
    /// `%` too) is *target-typed*: oracle-verified that `Define g.d = 1 +
    /// 7/2` evaluates to `4.5`, not `4` - a Double destination forces real
    /// division through the *entire* initializer expression tree, not just
    /// a shallow top-level check, whereas the exact same expression with no
    /// float destination at all (a bare `Debug 7/2`, or an Integer
    /// destination) performs plain integer division and yields `3`.
    /// `floatContext` is true exactly when some enclosing destination is
    /// Float-family; Codegen threads the same flag through genExpr so the
    /// generated code matches this classification node-for-node.
    ValueKind classify(const ast::Expr& expr, bool floatContext) const;

    /// Every declared (explicit `Define` or implicitly-declared) variable,
    /// in first-seen order, for Codegen to emit as C++ globals.
    const std::vector<std::pair<std::string, TypeSuffix>>& declarationOrder() const {
        return order_;
    }

    /// The resolved type for a declared `#Name` constant, or
    /// TypeSuffix::Integer if never seen.
    TypeSuffix constTypeOf(const std::string& lowerName) const;

    /// Every declared constant (`#Name = expr` or an `Enumeration` member),
    /// in first-seen order, for Codegen to emit as C++ globals. Constants
    /// live in their own namespace from variables (`#Foo` and `Foo` never
    /// collide), mirrored here by a separate table from `declarationOrder`.
    const std::vector<std::pair<std::string, TypeSuffix>>& constDeclarationOrder() const {
        return constOrder_;
    }

    /// A declared procedure's resolved signature, for Codegen to emit a
    /// matching C++ function and to convert call-site arguments/return
    /// values with the same banker's-rounding rules as everything else.
    struct ProcedureInfo {
        TypeSuffix returnSuffix = TypeSuffix::Integer;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount = 0; ///< Params before the first one with a default.
        /// Every local (parameters first, in declaration order, then any
        /// body-internal `Define`/implicit-declare) - Codegen skips the
        /// first `paramSuffixes.size()` entries when emitting a function's
        /// *non-parameter* locals, since those are already real C++
        /// parameters.
        std::vector<std::pair<std::string, TypeSuffix>> locals;
    };

    /// Returns nullptr if `lowerName` was never declared as a procedure.
    const ProcedureInfo* procedureInfo(const std::string& lowerName) const;

    /// A declared array's element type and dimension count (1 or 2 - see
    /// ast::DimStmt's own doc comment on the current dimension limit).
    /// `name(args)` is ambiguous with a procedure call at parse time (both
    /// are `Name(args)`) - Sema disambiguates by checking this table
    /// first: a name declared via `Dim` is always an array reference, never
    /// a call, even if a same-named procedure somehow also existed (PB's
    /// own namespace rules for this collision aren't modeled - not
    /// expected to come up in practice).
    struct ArrayInfo {
        TypeSuffix elementSuffix = TypeSuffix::Integer; ///< Struct means look at elementStructName instead.
        std::string elementStructName; ///< Lowercased; meaningful only if elementSuffix == Struct.
        int dimensionCount = 1;
    };
    const ArrayInfo* arrayInfo(const std::string& lowerName) const;
    /// Every declared array, in first-seen order, for Codegen to emit as a
    /// global `std::vector` (plus a hidden dimension-size companion
    /// variable for 2D arrays) - mirrors `declarationOrder`'s role for
    /// plain scalar variables.
    const std::vector<std::pair<std::string, ArrayInfo>>& arrayDeclarationOrder() const {
        return arrayOrder_;
    }

    /// A declared List's element type (M3e). `name()` - a zero-arg
    /// `Name(args)` - is ambiguous at parse time between an array read
    /// (M3b), a procedure call, and (new here) reading/writing a List's
    /// *current element*, or naming the list itself as an argument to a
    /// list built-in (`AddElement(name())` etc.) - Sema disambiguates by
    /// checking this table, exactly like `ArrayInfo` does for arrays.
    struct ListInfo {
        TypeSuffix elementSuffix = TypeSuffix::Integer; ///< Struct means look at elementStructName instead.
        std::string elementStructName; ///< Lowercased; meaningful only if elementSuffix == Struct.
    };
    const ListInfo* listInfo(const std::string& lowerName) const;
    /// Every declared List, in first-seen order, for Codegen to emit as a
    /// global `easybasic::runtime::PBList<T>` - mirrors
    /// `arrayDeclarationOrder`'s role for arrays.
    const std::vector<std::pair<std::string, ListInfo>>& listDeclarationOrder() const { return listOrder_; }

    /// True for the List built-ins recognized by name (`AddElement`,
    /// `InsertElement`, `DeleteElement`, `ClearList`, `FirstElement`,
    /// `LastElement`, `NextElement`, `PreviousElement`, `ListSize`,
    /// `SelectElement`, `ListIndex`) - mirrors `isPointerBuiltinName`'s own
    /// role, exposed so Codegen's `genExpr` can special-case them the same
    /// way Sema's own `visitExpr` does.
    static bool isListBuiltinName(const std::string& lowerName);

    /// A declared Map's element type (M3f). Unlike a List, a Map supports
    /// *two* element-access shapes: `name()` (the current cursor's value,
    /// identical to a List) and `name(key)` (direct access by a String key -
    /// oracle-verified: auto-creates the key with a zero value if absent,
    /// else keeps its existing value; either way moves the cursor to it,
    /// same as `FindMapElement`). Both are still just a `CallExpr` (zero or
    /// one arg respectively) - the one-arg form is genuinely ambiguous with
    /// a 1D array read at parse time, disambiguated by checking this table
    /// before `ArrayInfo`.
    struct MapInfo {
        TypeSuffix elementSuffix = TypeSuffix::Integer; ///< Struct means look at elementStructName instead.
        std::string elementStructName; ///< Lowercased; meaningful only if elementSuffix == Struct.
    };
    const MapInfo* mapInfo(const std::string& lowerName) const;
    /// Every declared Map, in first-seen order, for Codegen to emit as a
    /// global `easybasic::runtime::PBMap<T>` - mirrors `listDeclarationOrder`'s
    /// role for Lists.
    const std::vector<std::pair<std::string, MapInfo>>& mapDeclarationOrder() const { return mapOrder_; }

    /// True for the Map built-ins recognized by name (`AddMapElement`,
    /// `DeleteMapElement`, `ClearMap`, `MapSize`, `MapKey`, `ResetMap`,
    /// `NextMapElement`, `FindMapElement`) - mirrors `isListBuiltinName`'s
    /// own role.
    static bool isMapBuiltinName(const std::string& lowerName);

    /// True for the M4 String-library functions (`Len`, `Left`, `Right`,
    /// `Mid`, `UCase`, `LCase`, `Trim`, `LTrim`, `RTrim`, `Str`, `Val`,
    /// `StrF`, `ValF`, `Chr`, `Asc`) - exposed so Codegen's `genExpr` knows
    /// to route a call to one of these to its `easybasic::runtime::pb*`
    /// implementation instead of the generic `f_<name>(...)` a user-defined
    /// procedure call lowers to. Unlike the pointer/List/Map built-ins,
    /// these need no special-cased argument handling in Sema at all - see
    /// this constructor's own registration into `procedures_`.
    static bool isStringLibBuiltinName(const std::string& lowerName);

    /// A resolved type: either one of PB's 11 primitive suffixes, or -
    /// when `suffix == TypeSuffix::Struct` - a named Structure (looked up
    /// via `structureInfo(structName)`). Every variable, array element,
    /// and Structure field ultimately resolves to one of these.
    struct ResolvedType {
        TypeSuffix suffix = TypeSuffix::Integer;
        std::string structName; ///< Lowercased; meaningful only if suffix == Struct.
    };

    /// One declared Structure's field list, in declaration order.
    struct FieldInfo {
        std::string name;
        std::string spelling;
        TypeSuffix suffix = TypeSuffix::Integer;
        std::string structTypeName; ///< Meaningful only if suffix == Struct.
    };
    struct StructureInfo {
        std::vector<FieldInfo> fields;
    };
    /// Returns nullptr if `lowerName` was never declared as a Structure.
    const StructureInfo* structureInfo(const std::string& lowerName) const;
    /// Every declared Structure, in declaration order (a field naming
    /// another Structure can only refer to one already declared earlier -
    /// PB's usual declare-before-use rule - so source order is always a
    /// valid Codegen emission order too, with no separate dependency sort
    /// needed).
    const std::vector<std::pair<std::string, StructureInfo>>& structureDeclarationOrder() const {
        return structureOrder_;
    }

    /// Resolves the type of any expression that denotes a storage location
    /// - a plain variable, an array element (`arr(i)`), or a field-access
    /// chain (`base\field`, however deeply nested) - by walking down to the
    /// root and then re-resolving each field lookup on the way back up.
    /// This is the single place that understands how a Structure's field
    /// types chain together; `classify()` and Codegen both go through it
    /// for anything that might be Structure-typed.
    ResolvedType resolveType(const ast::Expr& expr) const;

    /// The pointee type a pointer variable (keyed with its leading '*', see
    /// ast::DefineStmt::Declarator's own doc comment) was declared to point
    /// at, or a safe Integer fallback if `pointerKey` names no known
    /// pointer. Used by Codegen to pick the right `reinterpret_cast` target
    /// when lowering a dereference (`*ptr\field`).
    ResolvedType pointeeTypeOf(const std::string& pointerKey) const;

    /// True for the handful of pointer/memory built-ins (`AllocateMemory`,
    /// `FreeMemory`, `AllocateStructure`, `FreeStructure`) that Sema
    /// recognizes by name rather than through the normal user-declared-
    /// procedure table - these exist only because pointers are essentially
    /// unusable without *some* allocation mechanism, bundled into M3d
    /// rather than waiting for the general standard library (M4). Exposed
    /// so Codegen's `genExpr`/`ExprStmt` handling can special-case them the
    /// same way Sema's own `visitExpr` does.
    static bool isPointerBuiltinName(const std::string& lowerName);

private:
    /// Pre-populates `procedures_` with each String-library function's
    /// signature, called once from the constructor - after this, a call to
    /// `Len`/`Left`/etc. is resolved, arity/type-checked, and argument-
    /// converted (including `Str`'s own banker's-rounding of a Float
    /// argument, oracle-verified: `Str(2.5)` is `2`, `Str(3.5)` is `4`) by
    /// exactly the same code path as a call to a real user-defined
    /// procedure - `Codegen` is the only place that needs to know these
    /// names are special, routing them to `easybasic::runtime::pb*` instead
    /// of emitting/calling an `f_<name>` function that was never declared.
    void registerStringLibBuiltins();
    void visitStmt(ast::Stmt& stmt);
    void visitBlock(ast::Block& block);
    /// Like `visitBlock`, but for a body that is genuinely nested inside a
    /// control-flow construct (`If`/`Select`/`For`/`While`/`Repeat`/
    /// `ForEach`) - tracked via `controlFlowDepth_` purely so a
    /// `ProcedureDecl` reached through it can be rejected the same way real
    /// PB itself rejects one (oracle-verified: "A procedure can't be
    /// declared inside an If, Repeat, While or For." - the same wording
    /// PB's own compiler uses even for `Select`/`ForEach`, which its own
    /// error text doesn't actually name). Not used for a `ProcedureDecl`'s
    /// own body (see `insideProcedure_` instead, which covers that case with
    /// its own distinct oracle-verified wording).
    void visitNestedBlock(ast::Block& block);
    void declare(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                 SourceLoc loc);
    /// Like declare(), but for a name reached through PB's implicit-
    /// declaration behavior (a bare assignment, a For-loop variable, or
    /// reading a never-assigned name) rather than an explicit `Define`.
    /// Errors under `EnableExplicit` instead of silently declaring, exactly
    /// like real PB (oracle-verified error text: "With 'EnableExplicit',
    /// variables have to be declared: <name>.") - but still declares
    /// afterwards regardless, purely so Codegen never sees a symbol with no
    /// recorded type (the pipeline already stops before Codegen runs once
    /// any error is recorded, so this is a defensive fallback, not a way to
    /// let EnableExplicit violations silently through).
    void declareImplicit(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                          SourceLoc loc);
    void declareConst(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                       SourceLoc loc);
    void visitExpr(ast::Expr& expr);
    /// Rejects String<->numeric assignments, which real PB also rejects
    /// without an explicit Str()/Val() conversion (not modeled yet - M4).
    void checkAssignable(const std::string& targetSpelling, TypeSuffix targetSuffix,
                          const ast::Expr& value, SourceLoc loc);
    /// A Map's key must be a String expression (oracle-verified error text:
    /// "A string expression is expected"). `keyExpr` is assumed to already
    /// have been visited (for implicit-declaration purposes) by the caller.
    void checkMapKey(const ast::Expr& keyExpr, SourceLoc loc) const;
    /// Walks a condition expression (If/While/Until), flagging logical
    /// `XOr` - see BinaryOp::LogicalXOr's own doc comment for why it's not
    /// yet trusted - and resolving/declaring any variables it references.
    void visitCondition(ast::Expr& expr);
    /// Validates a call's argument count against the callee's signature and
    /// visits each argument expression; reports an "undeclared procedure"
    /// error (with a recovery fallback so later statements still resolve
    /// sensibly) if `name` was never declared.
    void visitCall(ast::CallExpr& call);
    /// Pulls an outer-scope (`outerScope`) name into the *current* (already
    /// swapped-in, procedure-local) `symbols_` for type resolution, without
    /// adding it to `order_` - see ast::DefineStmt::isGlobal's and
    /// ast::SharedStmt's own doc comments for why this is exactly what both
    /// `Global` auto-visibility and `Shared` need, and why deliberately
    /// *not* adding it to `order_` is what makes Codegen reference the real
    /// outer C++ variable instead of declaring a shadowing procedure-local
    /// copy (see Codegen::genProcedureDecl's own notes).
    void bringIntoScope(const std::unordered_map<std::string, TypeSuffix>& outerScope,
                         const std::string& lowerName, const std::string& spelling, SourceLoc loc,
                         const char* directiveNameForError);
    /// Looks up `field` on whatever Structure `baseType` names, reporting a
    /// diagnostic and returning a safe Integer fallback if `baseType` isn't
    /// a Structure at all or has no such field.
    ResolvedType resolveField(const ResolvedType& baseType, const std::string& fieldLowerName,
                               const std::string& fieldSpelling, SourceLoc loc) const;
    /// Handles one of the four names `isPointerBuiltinName` recognizes,
    /// returning true if `call.name` was one of them (and hence fully
    /// handled here - the caller must not also treat it as an array read or
    /// a normal procedure call). `AllocateStructure`'s sole argument is a
    /// bare Structure type name, not a variable read (oracle-verified
    /// syntax: `AllocateStructure(Point)`), so it's deliberately *not*
    /// visited as an expression - doing so would implicitly declare a
    /// bogus variable named after the type.
    bool visitPointerBuiltinCall(ast::CallExpr& call);
    /// Handles one of the names `isListBuiltinName` recognizes, returning
    /// true if `call.name` was one of them. Every one of these takes a bare
    /// `name()` (a declared List) as its first argument - naming the list
    /// itself, not reading its current element - so that argument is
    /// validated (must be a zero-arg `CallExpr` naming a declared List) but
    /// deliberately not visited as an ordinary expression; `SelectElement`'s
    /// second argument (an index) *is* visited normally.
    bool visitListBuiltinCall(ast::CallExpr& call);
    /// Looks up `lowerName` in `lists_`, reporting a diagnostic and
    /// returning nullptr if it doesn't name a declared List - shared by
    /// `visitListBuiltinCall` (after checking its argument is a bare,
    /// zero-arg `name()`) and by `ForEachStmt`'s own handling.
    const ListInfo* requireList(const std::string& lowerName, const std::string& spelling, SourceLoc loc) const;
    /// Handles one of the names `isMapBuiltinName` recognizes, returning
    /// true if `call.name` was one of them. Every one of these takes a bare
    /// `name()` as its first argument (naming the Map itself, exactly like
    /// `visitListBuiltinCall`'s own first argument); `AddMapElement`/
    /// `FindMapElement`/the 2-arg form of `DeleteMapElement` additionally
    /// take a key argument, visited normally and required to classify as
    /// String-family (oracle-verified: `m(5) = 1` is rejected with "A
    /// string expression is expected").
    bool visitMapBuiltinCall(ast::CallExpr& call);
    /// Validates and builds the `ResolvedType` a pointer declaration's own
    /// pointee-describing fields (`suffix`/`structTypeName`/
    /// `structTypeSpelling`, shared field names between
    /// ast::DefineStmt::Declarator and ast::ProcedureDeclStmt::Param)
    /// resolve to. Oracle-verified: only an *untyped* pointer (`Define
    /// *pa`, `suffix == TypeSuffix::None` - dereferenced via the Memory
    /// library's Peek*/Poke* functions, not yet implemented - M4) or a
    /// *Structure-typed* one (`Define *pp.Point`, dereferenced with
    /// `\field`) is legal; a primitive-typed pointer (`Define *pa.i`) is
    /// rejected by real PB itself ("Native types can't be used with
    /// pointers."), reproduced here verbatim.
    ResolvedType resolvePointeeType(TypeSuffix suffix, const std::string& structTypeName,
                                     const std::string& structTypeSpelling, SourceLoc loc);

    DiagnosticEngine& diagnostics_;
    std::unordered_map<std::string, TypeSuffix> symbols_;
    std::vector<std::pair<std::string, TypeSuffix>> order_; ///< First-seen declaration order.
    std::unordered_map<std::string, TypeSuffix> constants_;
    std::vector<std::pair<std::string, TypeSuffix>> constOrder_;
    bool explicitEnabled_ = false;
    std::unordered_map<std::string, ProcedureInfo> procedures_;
    /// Names `Declare`d but not yet fulfilled by a matching `Procedure`,
    /// mapping the lowercased name to its original spelling and the
    /// `Declare` statement's own location (for the final error message,
    /// oracle-verified wording: "The procedure 'name()' has been declared
    /// but not defined."). Populated by a `Declare`, erased once the real
    /// `Procedure` with the same name is seen (after its signature is
    /// checked to match); anything still present once `analyze()` finishes
    /// the whole module is reported as an error.
    std::unordered_map<std::string, std::pair<std::string, SourceLoc>> declaredNotDefined_;
    std::unordered_map<std::string, ArrayInfo> arrays_;
    std::vector<std::pair<std::string, ArrayInfo>> arrayOrder_;
    std::unordered_map<std::string, ListInfo> lists_;
    std::vector<std::pair<std::string, ListInfo>> listOrder_;
    std::unordered_map<std::string, MapInfo> maps_;
    std::vector<std::pair<std::string, MapInfo>> mapOrder_;
    /// A declared variable's Structure type name, keyed by the variable's
    /// lowercased name, present only when that variable's entry in
    /// `symbols_` is `TypeSuffix::Struct`.
    std::unordered_map<std::string, std::string> varStructType_;
    std::unordered_map<std::string, StructureInfo> structures_;
    std::vector<std::pair<std::string, StructureInfo>> structureOrder_;
    /// The return suffix of the procedure whose body is currently being
    /// visited, used by a nested `ProcedureReturn`'s own type checking; only
    /// meaningful while `insideProcedure_` is true (PB procedures don't
    /// nest, so a single flag - not a stack - is enough).
    TypeSuffix currentProcedureReturnSuffix_ = TypeSuffix::Integer;
    bool insideProcedure_ = false;
    /// How many `If`/`Select`/`For`/`While`/`Repeat`/`ForEach` bodies deep
    /// the statement currently being visited is nested (0 at true top level,
    /// or directly inside a `ProcedureDecl`'s own body) - see
    /// `visitNestedBlock`'s own doc comment for why this exists.
    int controlFlowDepth_ = 0;
    /// Every `Global`-declared name seen so far (module scope only - PB
    /// requires declare-before-use even for procedures, oracle-verified, so
    /// "seen so far" is the right set to pre-populate a procedure's scope
    /// with). Names only; the type itself lives in the outer `symbols_`
    /// captured at the time a `ProcedureDecl` is processed.
    std::unordered_set<std::string> globalNames_;
    /// The module-level scope, valid only while a `ProcedureDecl`'s body is
    /// being visited (i.e. while `symbols_`/`order_` hold the swapped-in
    /// local scope) - what a `Shared` statement inside that body resolves
    /// names against. Null outside that window.
    const std::unordered_map<std::string, TypeSuffix>* outerScopeForShared_ = nullptr;
    /// The pointee type each declared pointer variable/parameter was
    /// declared to point at, keyed by the variable's own `*`-prefixed name
    /// (same key as `symbols_`). See ast::DefineStmt::Declarator's doc
    /// comment: the pointer's own `symbols_` entry is always forced to
    /// TypeSuffix::Integer (its C++ storage is a plain int64_t address), so
    /// the *pointee* type has to live somewhere else - this is that
    /// somewhere else. Like `varStructType_`, deliberately NOT saved/
    /// restored around a `ProcedureDecl`'s scope swap - Codegen queries this
    /// map for a procedure's own params/locals only *after* Sema::analyze()
    /// has finished entirely, by which point the swap has already unwound,
    /// so an entry added while visiting a procedure body must outlive that
    /// visit (unlike `symbols_` itself, which Codegen never reads directly).
    std::unordered_map<std::string, ResolvedType> pointerPointeeType_;
};

} // namespace easybasic
