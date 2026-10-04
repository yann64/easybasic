#pragma once

#include <cstdint>
#include <optional>
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

    /// A resolved type: either one of PB's 11 primitive suffixes, or -
    /// when `suffix == TypeSuffix::Struct`/`Interface` - a named Structure/
    /// Interface (looked up via `structureInfo`/`interfaceInfo`). Every
    /// variable, array element, and Structure field ultimately resolves to
    /// one of these. Defined up here (ahead of its first real use further
    /// down in this class) because `ProcedureInfo`, just below, needs it
    /// complete - `std::unordered_map` can't hold an incomplete value type
    /// portably.
    struct ResolvedType {
        TypeSuffix suffix = TypeSuffix::Integer;
        std::string structName; ///< Lowercased; meaningful only if suffix == Struct or Interface.
    };

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
        /// Every pointer local's (among `locals`, by name) own pointee
        /// type, captured the same moment `locals` is. `pointerPointeeType_`
        /// itself is a single flat, never-scoped map (unlike `symbols_`/
        /// `order_`, it's never saved/restored per-procedure) - so two
        /// procedures that both name a pointer parameter `*this` (the
        /// natural, idiomatic style M7c's own Interface feature actively
        /// encourages: every implementing procedure using the same
        /// canonical parameter name) would otherwise silently clobber each
        /// other from Codegen's own point of view, since it reads
        /// `pointerPointeeType_` directly in a later, separate pass, well
        /// after `Sema::analyze()` has already visited every procedure -
        /// confirmed as a real, triggerable bug (not just the "not yet
        /// observed in practice" theoretical gap M3d's own notes first
        /// flagged), not a hypothetical. `Codegen::pointeeTypeOf` consults
        /// this (via the currently-generating procedure's own `locals`)
        /// before ever falling back to `Sema::pointeeTypeOf`'s global view.
        std::unordered_map<std::string, ResolvedType> pointerPointeeTypes;
    };

    /// Returns nullptr if `lowerName` was never declared as a procedure.
    const ProcedureInfo* procedureInfo(const std::string& lowerName) const;
    /// A Data label's flat index into the runtime data pool, or
    /// `std::nullopt` if `lowerName` isn't a declared label - `Codegen`
    /// uses this to emit `Restore label`'s own constant-index argument.
    std::optional<std::size_t> dataLabelIndex(const std::string& lowerName) const;

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

    /// True for the M4b Math-library functions (`Abs`, `Sqr`, `Pow`, `Sin`,
    /// `Cos`, `Tan`, `ASin`, `ACos`, `ATan`, `ATan2`, `Exp`, `Log`, `Log10`,
    /// `Round`, `Int`, `Random`, `RandomSeed`) - mirrors
    /// `isStringLibBuiltinName`'s own role and registration mechanism
    /// (`registerMathLibBuiltins`, called from the constructor alongside
    /// `registerStringLibBuiltins`).
    static bool isMathLibBuiltinName(const std::string& lowerName);

    /// True for the M4c Memory-library functions (`PeekB`/`PeekA`/`PeekC`/
    /// `PeekW`/`PeekU`/`PeekL`/`PeekQ`/`PeekF`/`PeekD`/`PeekS` and their
    /// `Poke*` counterparts) - mirrors `isMathLibBuiltinName`'s own role and
    /// registration mechanism (`registerMemoryLibBuiltins`). Closes the gap
    /// M3d's pointer work deliberately deferred: an *untyped* pointer
    /// (`Define *ptr`, no Structure type) was only ever useful as a plain
    /// address before this - these are what actually read/write through it.
    static bool isMemoryLibBuiltinName(const std::string& lowerName);

    /// True for the M4d File-library functions (`CreateFile`, `OpenFile`,
    /// `ReadFile`, `CloseFile`, `WriteString`, `WriteStringN`, `ReadString`,
    /// `Eof`, `FileSize`, `DeleteFile`, `RenameFile`, `FileSeek`, `Loc`,
    /// `Lof`) - mirrors `isMemoryLibBuiltinName`'s own role and registration
    /// mechanism (`registerFileLibBuiltins`). A file "number" is a plain
    /// Integer the PB program itself picks (oracle-verified: a literal, a
    /// variable, or any Integer expression), not a handle PB hands back -
    /// so, like the Memory library, no special argument-shape handling is
    /// needed here either.
    static bool isFileLibBuiltinName(const std::string& lowerName);

    /// True for the M4e Date-library functions (`Date`, `Year`, `Month`,
    /// `Day`, `Hour`, `Minute`, `Second`, `DayOfWeek`, `FormatDate`,
    /// `AddDate`) - mirrors `isFileLibBuiltinName`'s own role and
    /// registration mechanism (`registerDateLibBuiltins`). `Date` is the
    /// one exception to "no special argument-shape handling needed" every
    /// other M4 library enjoyed: it's oracle-verified to accept *exactly*
    /// 0 or 6 arguments, never 1-5, a genuinely bimodal arity `Sema`'s
    /// usual continuous `[required, total]` range can't express exactly -
    /// registered as `[0, 6]` anyway, a documented, minor imprecision (a
    /// 1-5-argument call fails at the C++ backend-compile stage instead of
    /// with a clean PB-style diagnostic, safely rejected either way).
    static bool isDateLibBuiltinName(const std::string& lowerName);

    /// True for the M7a thread-library functions (`CreateThread`,
    /// `IsThread`, `WaitThread`, `CreateMutex`, `LockMutex`, `UnlockMutex`,
    /// `TryLockMutex`, `FreeMutex`, `CreateSemaphore`, `SignalSemaphore`,
    /// `WaitSemaphore`, `TrySemaphore`, `FreeSemaphore`) - mirrors
    /// `isDateLibBuiltinName`'s own role and registration mechanism
    /// (`registerThreadLibBuiltins`). `CreateThread`'s first argument is
    /// `@Procedure()` (an `AddressOfExpr`, handled by its own dedicated
    /// Sema/Codegen logic - see `ast::AddressOfExpr`'s own notes), which
    /// already resolves to a plain Integer like any other address-of
    /// expression, so no special per-argument handling is needed here
    /// beyond the usual registration. `KillThread`/`PauseThread`/
    /// `ResumeThread`/`ThreadID` are deliberately not included - see the
    /// M7a roadmap notes for why. Also bundles `Delay`/`ElapsedMilliseconds`
    /// - not thread-specific commands in real PB, but needed immediately by
    /// any real thread-timing test, and not worth a separate single-purpose
    /// registration mechanism for just two functions.
    static bool isThreadLibBuiltinName(const std::string& lowerName);

    /// True for the M7b GUI-core functions (`OpenWindow`, `CloseWindow`,
    /// `IsWindow`, `ResizeWindow`, `HideWindow`, `WindowEvent`,
    /// `WaitWindowEvent`, `EventWindow`, `EventGadget`, `EventType`) -
    /// mirrors `isThreadLibBuiltinName`'s own role and registration
    /// mechanism (`registerGuiLibBuiltins`). `OpenWindow`'s own `Title`
    /// argument is the one String-typed parameter among these; every other
    /// argument/return value is a plain Integer (a window ID, a coordinate,
    /// an event code, or a native-handle-ish "truthy" value PB itself
    /// doesn't document the exact numeric meaning of - see the M7b roadmap
    /// notes on `OpenWindow`/`IsWindow`'s own oracle-observed return
    /// values).
    static bool isGuiLibBuiltinName(const std::string& lowerName);
    /// True once `visitCall` has resolved at least one real call to a GUI
    /// builtin (set in `visitCall` itself) - `main.cpp`'s own driver uses
    /// this after a successful `analyze()` to decide whether the backend-
    /// compiler invocation needs GTK3's own compile/link flags at all,
    /// keeping every non-GUI program's build completely unaffected (no
    /// GTK3 toolchain requirement, no `gtk_init()` call at runtime either -
    /// see `guilib.hpp`'s own lazy-init notes).
    bool usesGuiLibrary() const { return usesGui_; }

    /// The literal Integer value of one of the handful of built-in `#PB_*`
    /// constants Sema pre-declares (currently only `#PB_Round_Down` (0),
    /// `#PB_Round_Up` (1), `#PB_Round_Nearest` (2), for `Round`'s own mode
    /// argument - oracle-verified: a fourth, plausible-sounding
    /// `#PB_Round_Truncate`, does not actually exist in real PB).  Returns
    /// nullopt for any other name. Exposed so Codegen's `genExpr` can emit
    /// the literal value directly for a `ConstRefExpr` naming one of these,
    /// rather than a `k_<name>` global that (unlike a user's own `#Name`
    /// constant) was never actually emitted anywhere.
    static std::optional<std::int64_t> builtinConstantValue(const std::string& lowerName);

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

    /// One declared Interface method's signature (M7c) - name, declared
    /// return type (`TypeSuffix::None` normalized to `Integer`, the same
    /// rule a Procedure's own return type follows), and declared parameter
    /// types (likewise normalized; deliberately primitive-suffix-only, see
    /// ast::InterfaceDeclStmt's own doc comment). A method's vtable slot
    /// index is purely its position in `InterfaceInfo::methods`.
    struct InterfaceMethodInfo {
        std::string name;
        std::string spelling;
        TypeSuffix returnSuffix = TypeSuffix::Integer;
        std::vector<TypeSuffix> paramSuffixes;
    };
    struct InterfaceInfo {
        std::vector<InterfaceMethodInfo> methods;
    };
    /// Returns nullptr if `lowerName` was never declared as an Interface.
    const InterfaceInfo* interfaceInfo(const std::string& lowerName) const;
    /// The index of `methodLowerName` within `info.methods`, or nullopt if
    /// it isn't one of this Interface's own declared methods - the vtable
    /// slot a `MethodCallExpr` resolves to.
    static std::optional<std::size_t> interfaceMethodIndex(const InterfaceInfo& info,
                                                             const std::string& methodLowerName);
    /// True if `?labelLowerName` is legal - see its own doc comment in
    /// sema.cpp. Exposed so Codegen's own label-array emission pass can
    /// reuse the identical check, rather than re-deriving it.
    bool dataLabelAddressable(const std::string& labelLowerName) const;

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
    /// Same mechanism as `registerStringLibBuiltins`, for the M4b Math
    /// library - every one of these (bar `Int`) is oracle-verified to
    /// return a Double regardless of its argument's own type.
    void registerMathLibBuiltins();
    /// Same mechanism again, for the M4c Memory library (`Peek*`/`Poke*`) -
    /// every argument/return value here is a plain Integer address or
    /// primitive value, so - like the string/Math libraries, and unlike the
    /// M3d pointer builtins (`AllocateMemory` etc.) - no special-cased
    /// argument handling is needed in Sema at all.
    void registerMemoryLibBuiltins();
    /// Same mechanism again, for the M4d File library.
    void registerFileLibBuiltins();
    /// Same mechanism again, for the M4e Date library - see
    /// `isDateLibBuiltinName`'s own doc comment for the one wrinkle
    /// (`Date`'s bimodal arity) this mechanism doesn't capture exactly.
    void registerDateLibBuiltins();
    /// Same mechanism again, for the M7a thread library (`CreateThread` and
    /// friends) - see `isThreadLibBuiltinName`'s own doc comment.
    void registerThreadLibBuiltins();
    /// Same mechanism again, for the M7b GUI-core library (`OpenWindow` and
    /// friends) - see `isGuiLibBuiltinName`'s own doc comment.
    void registerGuiLibBuiltins();
    /// Pre-populates `constants_` (but deliberately NOT `constOrder_` - see
    /// `Codegen::genExpr`'s `ConstRef` case) with the handful of built-in
    /// `#PB_*` constants Sema recognizes (currently just `Round`'s own
    /// three mode constants).
    void registerBuiltinConstants();
    void visitStmt(ast::Stmt& stmt);
    /// Visits every statement in `block` in order - except a
    /// `CompilerIf`/`CompilerSelect` is never passed to `visitStmt` at all.
    /// Oracle-verified that the *other* branches of one of these aren't
    /// even type-checked (a bogus call in a non-taken branch raises no
    /// error), so this resolves the condition/selector right here with
    /// `evalConstExpr` and *splices the selected branch's own statements
    /// into `block` in its place* - a genuine, permanent AST rewrite (this
    /// method takes `block` by non-const reference specifically to allow
    /// it), after which the `CompilerIf`/`CompilerSelect` node itself no
    /// longer exists anywhere in the tree. `Codegen` runs an entirely
    /// separate pass afterward over this same, already-rewritten `Module`,
    /// so it never needs its own logic for either `StmtKind` beyond a
    /// `-Wswitch` exhaustiveness placeholder - exactly mirroring how a real
    /// preprocessor's textual substitution would behave, just done at the
    /// AST level instead of the token level.
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
    /// Evaluates `expr` as a compile-time Integer constant expression -
    /// literals, `#Name` references (both built-in `#PB_*` ones and a
    /// user's own, looked up in `constantIntValues_`), and the same
    /// arithmetic/comparison/`And`/`Or`/`Not` operators `genCondition`
    /// lowers for a *runtime* condition, but computed here instead of
    /// generated as C++. Returns `std::nullopt` for anything not constant-
    /// foldable (a variable reference, `LogicalXOr` - already untrusted
    /// elsewhere, see `BinaryOp::LogicalXOr`'s own doc comment - or a
    /// String/Float-valued sub-expression), which `resolveCompilerIf`/
    /// `resolveCompilerSelect` turn into a diagnostic.
    std::optional<std::int64_t> evalConstExpr(const ast::Expr& expr) const;
    /// The two `visitBlock` helpers that do the actual splicing for a
    /// `CompilerIf`/`CompilerSelect` found at `block[index]` - replacing
    /// that single element with its selected branch's own statements
    /// (moved, not copied), or removing it outright if nothing matched and
    /// there was no `CompilerElse`/`CompilerDefault`.
    void resolveCompilerIf(ast::Block& block, std::size_t index);
    void resolveCompilerSelect(ast::Block& block, std::size_t index);
    /// Runs once, recursively, over the *entire* Module before any other
    /// pass - resolves every CompilerIf/CompilerSelect node everywhere in
    /// the tree (via resolveCompilerIf/resolveCompilerSelect), and along
    /// the way does a light preview of ConstDecl/Enumeration's own value
    /// computation (just enough to populate constantIntValues_, not the
    /// full declareConst/symbols_ bookkeeping the real ConstDecl/
    /// Enumeration case in visitStmt still does later) - needed so a
    /// CompilerIf can fold a reference to a #Constant declared earlier in
    /// the same file, exactly as it could before this was split out of
    /// visitBlock into its own pre-pass.
    void preResolveCompilerDirectives(ast::Block& block);
    /// Runs once, recursively, over the entire (by now CompilerIf/
    /// CompilerSelect-free) Module, after preResolveCompilerDirectives but
    /// before visitBlock's own main walk - assigns each Data label
    /// (dataLabels_) its flat index into the eventual runtime data pool,
    /// and counts the pool's total size (dataCount_). Must run as its own
    /// pass, not interleaved with the main walk, because `Restore` can
    /// forward-reference a label defined later in the file (oracle-
    /// verified) - the label table needs to be complete before any
    /// Restore statement is resolved against it.
    void collectDataSections(const ast::Block& block);
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
    /// Resolves `call`'s own method against whatever Interface its base
    /// pointer points at (M7c) - reports a diagnostic and returns nullptr
    /// for every way this can be invalid (the base isn't a `*ptr`-named
    /// VarRef, its pointee isn't Interface-typed, or the name isn't one of
    /// that Interface's own declared methods). Called from both visitExpr
    /// (arity/type-checks the call) and classify (the method's own return
    /// type) - safe to call twice for the same node despite emitting
    /// diagnostics, since Codegen (classify's other caller) never runs
    /// unless Sema::analyze() already finished with zero errors.
    const InterfaceMethodInfo* resolveInterfaceMethod(const ast::MethodCallExpr& call) const;
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
    /// Every Integer-family constant's own compile-time VALUE (as opposed
    /// to `constants_`, which only tracks each constant's TYPE) - every
    /// other milestone's constant handling never needed the actual value,
    /// only the type, since `Codegen` re-evaluates a constant's own init
    /// expression directly rather than asking `Sema` for a precomputed
    /// number. `CompilerIf`/`CompilerSelect` (M5a) are the first thing that
    /// needs real compile-time arithmetic on a `#Name` reference, via
    /// `evalConstExpr`. Populated for the built-in `#PB_*` constants (see
    /// `registerBuiltinConstants`) and for every user `#Name = expr`/
    /// `Enumeration` member whose value is itself constant-foldable;
    /// String/Float constants are deliberately not tracked here (not
    /// needed for any oracle-verified `CompilerIf` usage).
    std::unordered_map<std::string, std::int64_t> constantIntValues_;
    /// Every Data label's flat index into the eventual runtime data pool,
    /// and the pool's total item count - both computed once by
    /// collectDataSections(). `Codegen` reads these via dataLabelIndex()
    /// when emitting a `Restore label`'s own constant-index argument.
    std::unordered_map<std::string, std::size_t> dataLabels_;
    std::size_t dataCount_ = 0;
    /// Every label's own run of `Data` item suffixes (up to the next label
    /// or `EndDataSection`), in order - also populated by
    /// collectDataSections(), used only to validate a `?Label` use (see
    /// dataLabelAddressable's own doc comment). Not needed at all for plain
    /// Read/Restore access, which goes through dataLabels_/dataCount_ alone.
    std::unordered_map<std::string, std::vector<TypeSuffix>> dataLabelItemSuffixes_;
    bool explicitEnabled_ = false;
    bool usesGui_ = false; ///< Set by visitCall - see usesGuiLibrary()'s own doc comment.
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
    std::unordered_map<std::string, InterfaceInfo> interfaces_;
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
