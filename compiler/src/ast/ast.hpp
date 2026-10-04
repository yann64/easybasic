#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../diagnostics/diagnostics.hpp"
#include "../lexer/token.hpp"

namespace easybasic::ast {

/// Oracle-verified precedence groups (docs/architecture/roadmap.md's M1
/// notes have the full derivation and the two-directional reversal-test
/// methodology used to nail them down after an initial, buggier attempt
/// mis-merged two of these into one tier - caught by the differential e2e
/// suite). Tightest to loosest: unary `-`/`~` > flat tier {`%`, `!`(xor),
/// `<<`, `>>`} > flat tier {`&`, `|`} > `*`/`/` > binary `+`/`-`. Comparisons
/// sit looser still and are restricted to boolean contexts (If/While/Until
/// conditions, or `Bool()`). `And`/`Or` are likewise ONE FLAT left-to-right
/// tier, not nested (And does *not* bind tighter than Or, unlike virtually
/// every other language).
/// `LogicalXOr` is parsed but deliberately not lowered by Codegen yet - its
/// runtime truth table showed a genuine, unexplained anomaly under oracle
/// testing that needs more investigation before being trusted.
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    BitAnd, BitOr, BitXor, ShiftLeft, ShiftRight,
    Eq, Ne, Lt, Gt, Le, Ge,
    LogicalAnd, LogicalOr, LogicalXOr,
};
enum class UnaryOp { Negate, BitNot, LogicalNot };

/// Cheap dispatch tag, one per concrete Expr subclass - avoids RTTI
/// (dynamic_cast) so Sema/Codegen visitors work identically whether or not
/// the target toolchain builds with RTTI enabled (Haiku's GCC does by
/// default, but there's no reason to depend on it for a simple closed set
/// of node types known entirely at compile time).
enum class ExprKind {
    IntLiteral, FloatLiteral, StringLiteral, VarRef, ConstRef, Binary, Unary, Call, FieldAccess, AddressOf,
    DataLabelAddress, MethodCall
};

/// Base of every expression node. Untyped: Sema annotates/validates types in
/// place over this same tree rather than building a second, typed tree - the
/// pipeline only ever has one AST.
struct Expr {
    SourceLoc loc;
    ExprKind kind;
    explicit Expr(ExprKind k) : kind(k) {}
    virtual ~Expr() = default;
    // Every concrete node lives behind a unique_ptr<Expr> and is downcast via
    // its `kind` tag (see Sema/Codegen) - copying/moving through the base
    // class would slice the derived fields, so both are deleted rather than
    // left to the (dangerous) implicitly-defined versions.
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;
};

struct IntLiteralExpr : Expr {
    IntLiteralExpr() : Expr(ExprKind::IntLiteral) {}
    long long value = 0;
    TypeSuffix suffix = TypeSuffix::None;
};

struct FloatLiteralExpr : Expr {
    FloatLiteralExpr() : Expr(ExprKind::FloatLiteral) {}
    double value = 0.0;
};

struct StringLiteralExpr : Expr {
    StringLiteralExpr() : Expr(ExprKind::StringLiteral) {}
    std::string value;
};

struct VarRefExpr : Expr {
    VarRefExpr() : Expr(ExprKind::VarRef) {}
    std::string name;      ///< Canonical (lowercased) for lookup.
    std::string spelling;  ///< Original source casing, for diagnostics/codegen.
    TypeSuffix suffix = TypeSuffix::None;
};

/// A `#Name` reference to a compile-time constant (own namespace, separate
/// from variables - `#Foo` and `Foo` never collide).
struct ConstRefExpr : Expr {
    ConstRefExpr() : Expr(ExprKind::ConstRef) {}
    std::string name;
    std::string spelling;
};

struct BinaryExpr : Expr {
    BinaryExpr() : Expr(ExprKind::Binary) {}
    BinaryOp op = BinaryOp::Add;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct UnaryExpr : Expr {
    UnaryExpr() : Expr(ExprKind::Unary) {}
    UnaryOp op = UnaryOp::Negate;
    std::unique_ptr<Expr> operand;
};

/// `Name(arg1, arg2, ...)` - a procedure call used as a value.
struct CallExpr : Expr {
    CallExpr() : Expr(ExprKind::Call) {}
    std::string name;
    std::string spelling;
    std::vector<std::unique_ptr<Expr>> args;
};

/// `base\field` - a Structure field access, read as a value. Chains left-
/// associate: `r\topLeft\x` parses as `FieldAccessExpr{ base:
/// FieldAccessExpr{ base: VarRef(r), field: topLeft }, field: x }` (see
/// Sema's own notes on how the base's structure type is resolved
/// recursively down such a chain, including through an array-of-Structure
/// element, e.g. `points(0)\x`).
struct FieldAccessExpr : Expr {
    FieldAccessExpr() : Expr(ExprKind::FieldAccess) {}
    std::unique_ptr<Expr> base;
    std::string field;
    std::string fieldSpelling;
};

/// `@operand` - address-of, yielding an Integer address (oracle-verified:
/// `Define *ptr.Point = @p` initializes a pointer with a plain variable's
/// address; `@p\field` and `@arr(i)` - address of a field or array element
/// - work the same way, since `operand` can be any location-denoting
/// expression the existing grammar already parses).
struct AddressOfExpr : Expr {
    AddressOfExpr() : Expr(ExprKind::AddressOf) {}
    std::unique_ptr<Expr> operand;
};

/// `?Label` - the compile-time address of a `DataSection` label (M7c's own
/// prerequisite, needed for `Interface`'s manually-built vtables). Unlike
/// `Data`/`Read`/`Restore`'s sequential cursor, this hands back a raw,
/// directly-indexable pointer into the data pool - oracle-verified
/// (`PeekI(?Label)`/`PeekI(?Label + 8)` read successive `.i` items 8 bytes
/// apart, confirming real PB's own DataSection is laid out as genuine
/// contiguous process memory). `pbcxx` deliberately scopes this to a label
/// whose own run of `Data` items (up to the next label or `EndDataSection`)
/// are all `.i`-suffix - the only oracle-verified real use case (an
/// Interface's vtable) - see `Sema::dataLabelAddressable`'s own doc comment
/// for the validation this implies.
struct DataLabelAddressExpr : Expr {
    DataLabelAddressExpr() : Expr(ExprKind::DataLabelAddress) {}
    std::string labelName; ///< Lowercased.
    std::string labelSpelling;
};

/// `base\Method(args)` - a call through an Interface-typed pointer's own
/// manually-built vtable (M7c). Syntactically this is `FieldAccessExpr`'s
/// own `\identifier` immediately followed by `(args)` - the Parser
/// disambiguates the two shapes the same moment it would otherwise build a
/// plain `FieldAccessExpr` (see `Parser::parsePostfixFieldAccess`), since a
/// trailing `(` can never start a legal field name. `base` is always an
/// Interface-typed pointer's own `VarRef` in every case this project
/// supports - no pointer-to-pointer-to-Interface chaining, matching real
/// PB's own documented usage.
struct MethodCallExpr : Expr {
    MethodCallExpr() : Expr(ExprKind::MethodCall) {}
    std::unique_ptr<Expr> base;
    std::string method; ///< Lowercased.
    std::string methodSpelling;
    std::vector<std::unique_ptr<Expr>> args;
};

enum class StmtKind {
    Define, Assign, Debug,
    If, Select, For, While, Repeat,
    Break, Continue, EnableExplicit, ConstDecl, Enumeration,
    ProcedureDecl, ProcedureReturn, ExprStmt, Shared,
    Dim, IndexAssign, StructureDecl, FieldAssign,
    NewList, ForEach, NewMap, Declare,
    CompilerIf, CompilerSelect,
    DataSection, DataLabel, Data, Read, Restore,
    InterfaceDecl,
    DeclareModule, Module, UseModule, UnuseModule,
};

/// Base of every statement node.
struct Stmt {
    SourceLoc loc;
    StmtKind kind;
    explicit Stmt(StmtKind k) : kind(k) {}
    virtual ~Stmt() = default;
    // See Expr's identical rationale just above.
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    Stmt(Stmt&&) = delete;
    Stmt& operator=(Stmt&&) = delete;
};

/// A statement sequence, reused for every construct with a nested body
/// (If/Select/For/While/Repeat).
using Block = std::vector<std::unique_ptr<Stmt>>;

/// One `Define a.i[, b.s = "x", ...]` statement. PB allows several
/// comma-separated declarators per Define, each with its own optional
/// initializer. `Protected` parses to this same node (`isGlobal` stays
/// false) - oracle-verified to behave just like an ordinary local `Define`
/// in every case actually tested (see docs/architecture/roadmap.md's M3
/// notes for the one, deliberately-unexplored edge case: a `Protected`
/// meant to shadow an existing same-named `Global`).
struct DefineStmt : Stmt {
    DefineStmt() : Stmt(StmtKind::Define) {}
    struct Declarator {
        std::string name;     ///< For a pointer, this already includes the leading '*' (see below).
        std::string spelling; ///< Likewise includes a leading '*' for a pointer, for diagnostics.
        /// `suffix`/`structTypeName` describe the *pointee* type when
        /// `isPointer` is set (`Define *ptr.Point` - oracle-verified), not
        /// the variable's own type (a pointer's own value is always a plain
        /// address, represented as Integer - see Sema's own notes). Meaning
        /// depends entirely on `isPointer`; there's no separate "primitive
        /// pointer" vs "struct pointer" tag here.
        TypeSuffix suffix = TypeSuffix::None; ///< TypeSuffix::Struct means look at structTypeName instead.
        std::string structTypeName;    ///< Lowercased; meaningful only if suffix == Struct.
        std::string structTypeSpelling;
        /// `Define *ptr.Type = ...` - oracle-verified: a pointer variable's
        /// name is in a genuinely separate namespace from a plain variable
        /// of the same base name (reading a same-named non-pointer variable
        /// gives an unrelated value) - modeled simply by giving `name`/
        /// `spelling` a literal leading `*` wherever a pointer is declared
        /// or referenced, which every existing name-keyed Sema table
        /// already keeps distinct for free, with zero additional plumbing.
        bool isPointer = false;
        std::unique_ptr<Expr> init; ///< May be null (default-initialized).
    };
    std::vector<Declarator> declarators;
    /// `Global a.i[, ...]` - oracle-verified: unlike a plain top-level
    /// `Define`, a `Global` variable is automatically visible and writable
    /// from *any* procedure with no `Shared` needed at all (see Sema's own
    /// notes on how this is modeled without any Codegen-level distinction
    /// between Global and plain top-level variables).
    bool isGlobal = false;
};

/// `Shared name[, name2, ...]` inside a procedure body - opts that specific
/// procedure into accessing an otherwise-invisible top-level `Define`d (or
/// `Global`) variable by its real name, aliasing the same underlying
/// storage (oracle-verified: mutations through the shared name are visible
/// to the caller afterward).
struct SharedStmt : Stmt {
    SharedStmt() : Stmt(StmtKind::Shared) {}
    struct Name {
        std::string name;
        std::string spelling;
    };
    std::vector<Name> names;
};

/// `Dim name.suffix(size0[, size1])` - a static array declaration. Limited
/// to 1 or 2 dimensions for now (higher dimensions deferred - see
/// docs/architecture/roadmap.md's M3b notes); each size is the highest
/// valid index (oracle-verified: `Dim arr.i(4)` makes a 5-element array,
/// indices 0..4), and can be an arbitrary runtime expression, not just a
/// compile-time constant - `pbcxx` backs this with a real `std::vector`
/// sized at the Dim statement's own position, not a fixed-size C array.
struct DimStmt : Stmt {
    DimStmt() : Stmt(StmtKind::Dim) {}
    std::string name;
    std::string spelling;
    TypeSuffix suffix = TypeSuffix::None; ///< TypeSuffix::Struct means look at structTypeName instead.
    std::string structTypeName;    ///< Lowercased; meaningful only if suffix == Struct.
    std::string structTypeSpelling;
    std::vector<std::unique_ptr<Expr>> dimensionSizes; ///< 1 or 2 entries.
};

/// `NewList name.type()` - declares a List (M3e). Oracle-verified: unlike
/// `Define`, only a *single* list can be declared per statement (`NewList
/// a.i(), b.i()` is rejected as "Garbage at the end of the line"), so this
/// carries one declaration directly rather than a `declarators` vector.
/// `name()` used elsewhere (as an expression, an assignment target, or an
/// argument to a list built-in like `AddElement`) is a plain, already-
/// existing `CallExpr` with zero args - Sema disambiguates it from an array
/// read/procedure call the same way it already disambiguates those two (see
/// `Sema::ListInfo`'s own doc comment), so no new expression-level AST node
/// is needed for that part.
struct NewListStmt : Stmt {
    NewListStmt() : Stmt(StmtKind::NewList) {}
    std::string name;
    std::string spelling;
    TypeSuffix suffix = TypeSuffix::None; ///< TypeSuffix::Struct means look at structTypeName instead.
    std::string structTypeName;    ///< Lowercased; meaningful only if suffix == Struct.
    std::string structTypeSpelling;
};

/// `name(index0[, index1]) = expr` - an array element assignment. Syntax is
/// otherwise indistinguishable from a procedure call at parse time
/// (`Name(args)`) - see Sema's own notes on how `name(args)` used as a
/// plain *expression* (read, not assigned-to) is disambiguated instead.
struct IndexAssignStmt : Stmt {
    IndexAssignStmt() : Stmt(StmtKind::IndexAssign) {}
    std::string name;
    std::string spelling;
    std::vector<std::unique_ptr<Expr>> indices;
    std::unique_ptr<Expr> value;
};

/// `Structure Name \n field.suffix \n ... \n EndStructure`. A field's own
/// type follows the identical suffix-or-named-type rule as a variable
/// (`suffix == Struct` means `structTypeName` names another, previously
/// declared Structure - oracle-verified nesting, e.g. `topLeft.Point`
/// inside `Rect`). No array fields, no `StructureUnion` yet.
struct StructureDeclStmt : Stmt {
    StructureDeclStmt() : Stmt(StmtKind::StructureDecl) {}
    struct Field {
        std::string name;
        std::string spelling;
        TypeSuffix suffix = TypeSuffix::None;
        std::string structTypeName;
        std::string structTypeSpelling;
    };
    std::string name;
    std::string spelling;
    std::vector<Field> fields;
};

/// `Interface Name \n Method[.suffix](params) \n ... \n EndInterface`
/// (M7c) - a pure method-name-and-signature list, nothing more. Oracle-
/// verified: real PB's own `Interface` is a thin syntactic layer over a
/// manually-built, programmer-visible vtable, not an automatic OOP feature -
/// there is no structural conformance checking at all between a Structure
/// and the Interfaces it's later used through (a worked example compiled
/// and ran correctly with the implementing procedures' own parameter types
/// never even mentioned here), so this node only needs to record each
/// method's declared signature - its vtable slot index is purely its
/// position in `methods`. A method's own return-suffix-defaulting rule is
/// identical to a Procedure's (no suffix means Integer, M2); a method
/// parameter is deliberately primitive-suffix-only (no pointer/Structure
/// params), the same documented scope limit `Declare`'s own parameter list
/// already has (M2-closure) and for the identical reason - not yet oracle-
/// verified and judged rare enough not to hold up this slice.
struct InterfaceDeclStmt : Stmt {
    InterfaceDeclStmt() : Stmt(StmtKind::InterfaceDecl) {}
    struct Param {
        std::string name;
        std::string spelling;
        TypeSuffix suffix = TypeSuffix::Integer;
    };
    struct Method {
        std::string name;
        std::string spelling;
        TypeSuffix returnSuffix = TypeSuffix::Integer;
        std::vector<Param> params;
    };
    std::string name;
    std::string spelling;
    std::vector<Method> methods;
};

/// `target\field = expr` where `target` is itself a field-access chain
/// (possibly of length 1, e.g. `p\x = 3`) - the write counterpart to
/// FieldAccessExpr, needed because PB's field access has no single
/// "lvalue expression" form shared with plain assignment (see
/// Parser::parseIdentifierStatement's own notes on why the parser builds
/// the whole base+chain once and decides what kind of statement it is only
/// after seeing whether `=` follows).
struct FieldAssignStmt : Stmt {
    FieldAssignStmt() : Stmt(StmtKind::FieldAssign) {}
    std::unique_ptr<Expr> target; ///< Always a FieldAccessExpr.
    std::unique_ptr<Expr> value;
};

/// `name[.suffix] = expr`.
struct AssignStmt : Stmt {
    AssignStmt() : Stmt(StmtKind::Assign) {}
    std::string name;
    std::string spelling;
    TypeSuffix suffix = TypeSuffix::None;
    std::unique_ptr<Expr> value;
};

/// `Debug expr`. Mirrors real PureBasic: compiled out entirely unless the
/// program is built in debug mode (verified against pbcompilerc - a plain,
/// non-`-d` build emits no code at all for Debug statements; `pbcompilerc
/// -d` links successfully and prints the value prefixed with
/// "[Debugger]  "). `pbcxx` reproduces this rather than always printing, so
/// differential testing against the real compiler stays meaningful.
struct DebugStmt : Stmt {
    DebugStmt() : Stmt(StmtKind::Debug) {}
    std::unique_ptr<Expr> value;
};

/// `If cond ... [ElseIf cond ...]* [Else ...] EndIf`. Each branch's
/// `condition` is null exactly for a trailing `Else` (never for `If`/
/// `ElseIf`, which always have one).
struct IfStmt : Stmt {
    IfStmt() : Stmt(StmtKind::If) {}
    struct Branch {
        std::unique_ptr<Expr> condition; ///< Null only for the `Else` branch.
        Block body;
    };
    std::vector<Branch> branches;
};

/// `Select selector ... [Case v1[, v2, ...] ...]* [Default ...] EndSelect`.
/// An empty `values` list marks the `Default` branch (PB has no `CaseElse` -
/// oracle-verified rejected as a syntax error; `Default` is the only form).
struct SelectStmt : Stmt {
    SelectStmt() : Stmt(StmtKind::Select) {}
    struct CaseBranch {
        std::vector<std::unique_ptr<Expr>> values; ///< Empty means `Default`.
        Block body;
    };
    std::unique_ptr<Expr> selector;
    std::vector<CaseBranch> cases;
};

/// `CompilerIf cond ... [CompilerElseIf cond ...]* [CompilerElse ...]
/// CompilerEndIf` - resolved entirely at compile time (oracle-verified: the
/// *other* branches aren't even type-checked - a bogus call in a non-taken
/// branch raises no error at all), unlike the structurally-identical
/// `IfStmt`. `condition` is parsed the same way a runtime `If`'s is (the
/// same comparison/`And`/`Or`/`Not` grammar - oracle-verified: a bare `=`
/// comparison works directly, the same boolean-context rule as `If`), but
/// `Sema` evaluates it with its own compile-time constant folder
/// (`evalConstExpr`) rather than lowering it to a runtime check - see
/// `Sema::visitBlock`'s own notes on how this node is *spliced out of the
/// tree entirely* (replaced by its selected branch's own statements)
/// before `Codegen` ever runs, so `Codegen` never needs its own case for
/// this `StmtKind` beyond an exhaustiveness placeholder.
struct CompilerIfStmt : Stmt {
    CompilerIfStmt() : Stmt(StmtKind::CompilerIf) {}
    struct Branch {
        std::unique_ptr<Expr> condition; ///< Null only for the `CompilerElse` branch.
        Block body;
    };
    std::vector<Branch> branches;
};

/// `CompilerSelect selector [CompilerCase v1[, v2, ...] ...]*
/// [CompilerDefault ...] CompilerEndSelect` - the `CompilerIf`-family
/// counterpart to `SelectStmt`, resolved away the same way.
struct CompilerSelectStmt : Stmt {
    CompilerSelectStmt() : Stmt(StmtKind::CompilerSelect) {}
    struct CaseBranch {
        std::vector<std::unique_ptr<Expr>> values; ///< Empty means `CompilerDefault`.
        Block body;
    };
    std::unique_ptr<Expr> selector;
    std::vector<CaseBranch> cases;
};

/// `DataSection ... EndDataSection` (M5b) - a flat, ordered sequence of
/// `DataLabelStmt`/`DataStmt` entries (no other statement kind is legal
/// inside one - enforced by the parser, which has its own dedicated
/// DataSection-body loop rather than reusing the general statement
/// dispatch). Oracle-verified: multiple `DataSection`s anywhere in the
/// module - even inside a `Procedure` - are concatenated into one single,
/// global, sequential data pool; `Read` continues seamlessly from one into
/// the next, with no `Restore` needed at the boundary.
struct DataSectionStmt : Stmt {
    DataSectionStmt() : Stmt(StmtKind::DataSection) {}
    Block body;
};

/// `name:` inside a `DataSection` - a `Restore` target. Oracle-verified:
/// `Restore` can forward-reference a label defined *later* in the file (see
/// `Sema::collectDataSections`'s own notes on why this needs a dedicated
/// pre-pass rather than being resolved during the normal sequential walk).
struct DataLabelStmt : Stmt {
    DataLabelStmt() : Stmt(StmtKind::DataLabel) {}
    std::string name;
    std::string spelling;
};

/// `Data.<suffix> value[, value, ...]` - oracle-verified: the suffix is
/// *required* ("A type or structure must be specified after 'Data'.",
/// unlike most other suffixed constructs in this language, which default
/// to Integer when omitted). Each value is a general expression (oracle-
/// verified: `Data.l #Two + 1` - a `#Constant` arithmetic expression -
/// compiles and reads back correctly), not just a bare literal.
struct DataStmt : Stmt {
    DataStmt() : Stmt(StmtKind::Data) {}
    TypeSuffix suffix = TypeSuffix::None;
    std::vector<std::unique_ptr<Expr>> values;
};

/// `Read[.suffix] varname` - oracle-verified to behave like a plain
/// assignment (`varname = <next data pool value>`) for every purpose
/// *except* what the destination variable's own type defaults to: an
/// undeclared `varname` is auto-declared as Integer regardless of `Read`'s
/// own suffix (oracle-verified: `Read.s s1` into a fresh `s1` leaves `s1`
/// an Integer, not a String - printing `Val("hello")` = `0`, not
/// `"hello"`), unlike e.g. a suffixed `Define`. `suffix` instead tells
/// Codegen which runtime pool-reading function to call (`pbReadDataInt`/
/// `Double`/`String`) - `TypeSuffix::None` (a bare `Read x`) defaults to
/// Integer, matching the project's general suffix-optional convention
/// (documented divergence: real PB's own bare `Read` appears to default to
/// its platform-width `.i` rather than `.l`, which can desync the pool's
/// byte-level cursor against mismatched-width `Data.l` items in real PB -
/// not something this project's own pool model, which tracks one *logical
/// item* per cursor step rather than raw bytes, can reproduce or needs to).
struct ReadStmt : Stmt {
    ReadStmt() : Stmt(StmtKind::Read) {}
    TypeSuffix suffix = TypeSuffix::None;
    std::string varName;
    std::string varSpelling;
};

/// `Restore label` - oracle-verified: the label is *required* (a bare
/// `Restore` with no argument is a syntax error, unlike `Return`-family
/// statements elsewhere in the language).
struct RestoreStmt : Stmt {
    RestoreStmt() : Stmt(StmtKind::Restore) {}
    std::string labelName;
    std::string labelSpelling;
};

/// `For var = from To to [Step step] ... Next [var]`.
struct ForStmt : Stmt {
    ForStmt() : Stmt(StmtKind::For) {}
    std::string varName;
    std::string varSpelling;
    TypeSuffix suffix = TypeSuffix::None;
    std::unique_ptr<Expr> from;
    std::unique_ptr<Expr> to;
    std::unique_ptr<Expr> step; ///< Null means the default step of 1.
    Block body;
};

/// `While cond ... Wend`.
struct WhileStmt : Stmt {
    WhileStmt() : Stmt(StmtKind::While) {}
    std::unique_ptr<Expr> condition;
    Block body;
};

/// `Repeat ... Until cond` or `Repeat ... ForEver` (`untilCondition` null
/// means the latter - an unconditional loop).
struct RepeatStmt : Stmt {
    RepeatStmt() : Stmt(StmtKind::Repeat) {}
    Block body;
    std::unique_ptr<Expr> untilCondition;
};

/// `ForEach name() ... Next` - iterates every element of a List (M3e; a Map
/// is a future increment). Oracle-verified: the header is always a bare
/// `name()` naming a declared List, never an arbitrary expression, so this
/// stores the name directly rather than a generic `Expr` - matching
/// `ForStmt`'s own name/spelling fields.
/// `ForEach name() ... Next` where `name` was declared with `ForEach`'s own
/// doc comment covers a List; `name` may equally be a Map (M3f) - both
/// iterate their elements in the same cursor-based way, so Sema's own
/// handling (not this node) is what tells them apart.
struct ForEachStmt : Stmt {
    ForEachStmt() : Stmt(StmtKind::ForEach) {}
    std::string name;
    std::string spelling;
    Block body;
};

/// `NewMap name.type()` - declares a Map (M3f). Structurally identical to
/// `NewListStmt` (see its own doc comment for why only one declaration is
/// allowed per statement); kept as its own node/StmtKind rather than a
/// shared one with a `isMap` flag purely so Codegen's dispatch stays
/// symmetric and searchable, matching how every other declaration form in
/// this compiler (`Dim`, `Structure`, `NewList`) gets its own kind despite
/// some structural overlap.
struct NewMapStmt : Stmt {
    NewMapStmt() : Stmt(StmtKind::NewMap) {}
    std::string name;
    std::string spelling;
    TypeSuffix suffix = TypeSuffix::None; ///< TypeSuffix::Struct means look at structTypeName instead.
    std::string structTypeName;    ///< Lowercased; meaningful only if suffix == Struct.
    std::string structTypeSpelling;
};

struct BreakStmt : Stmt {
    BreakStmt() : Stmt(StmtKind::Break) {}
};

struct ContinueStmt : Stmt {
    ContinueStmt() : Stmt(StmtKind::Continue) {}
};

/// The `EnableExplicit` directive: sets a Sema-wide flag rather than
/// generating any code of its own.
struct EnableExplicitStmt : Stmt {
    EnableExplicitStmt() : Stmt(StmtKind::EnableExplicit) {}
};

/// `#Name = expr`, a compile-time constant (own namespace from variables).
struct ConstDeclStmt : Stmt {
    ConstDeclStmt() : Stmt(StmtKind::ConstDecl) {}
    std::string name;
    std::string spelling;
    std::unique_ptr<Expr> value;
};

/// `Enumeration [#First[=v]] ... EndEnumeration`: each `#Name` becomes an
/// integer ConstDecl, auto-incrementing from the previous member's value
/// (or 0 for the very first member) unless given its own `= expr`.
struct EnumerationStmt : Stmt {
    EnumerationStmt() : Stmt(StmtKind::Enumeration) {}
    struct Member {
        std::string name;
        std::string spelling;
        std::unique_ptr<Expr> explicitValue; ///< Null means auto-increment.
    };
    std::vector<Member> members;
};

/// `Procedure[.suffix|$] Name(param1[.suffix][ = default], ...) ... EndProcedure`.
/// PB requires procedures to be fully defined before any call to them
/// (oracle-verified: calling one declared later in the file is a compile
/// error - no forward declarations/hoisting) and gives each one its own,
/// completely isolated local scope (oracle-verified: a procedure body
/// reading a same-named outer variable gets a fresh local defaulting to 0,
/// not the outer value) - see Sema's own notes for how this is modeled.
struct ProcedureDeclStmt : Stmt {
    ProcedureDeclStmt() : Stmt(StmtKind::ProcedureDecl) {}
    std::string name;
    std::string spelling;
    TypeSuffix returnSuffix = TypeSuffix::None; ///< None defaults to Integer, like everywhere else.
    struct Param {
        std::string name;     ///< Includes a leading '*' for a pointer param (see DefineStmt::Declarator).
        std::string spelling;
        /// Describes the *pointee* type when `isPointer` is set, exactly
        /// like DefineStmt::Declarator's identically-named fields (oracle-
        /// verified pointer parameters, e.g. `Procedure SetX(*p.Point,
        /// v.i)` mutating the caller's Structure through `*p\x = v`).
        TypeSuffix suffix = TypeSuffix::None;
        std::string structTypeName;
        std::string structTypeSpelling;
        bool isPointer = false;
        std::unique_ptr<Expr> defaultValue; ///< Null means required.
    };
    std::vector<Param> params;
    Block body;
};

/// `DeclareModule Name \n ... \n EndDeclareModule` (M7d) - a Module's own
/// public interface: `Declare`'d procedure signatures and `Global`
/// variables (with optional initializers) that become visible from outside
/// the module, either qualified (`Name::Member`, works everywhere
/// unconditionally) or unqualified after `UseModule Name`. Oracle-verified
/// real PB also accepts Structures/Macros/Enumerations/constants/arrays/
/// Lists/Maps/labels here - this project deliberately scopes its own first
/// Module slice to just procedures and Global variables (the only two
/// kinds either oracle worked example actually exercises), rejecting
/// anything else inside this body with a real diagnostic rather than
/// silently mishandling it; `Sema` enforces this, not the Parser, which
/// reuses the ordinary statement grammar for this body unchanged.
struct DeclareModuleStmt : Stmt {
    DeclareModuleStmt() : Stmt(StmtKind::DeclareModule) {}
    std::string name;
    std::string spelling;
    Block body;
};

/// `Module Name \n ... \n EndModule` (M7d) - a Module's own private
/// implementation: every `Procedure`/`Global` declared here is private
/// (inaccessible from outside the module at all, oracle-verified: "Module
/// item 'X' is not declared as public.") *unless* the same name was also
/// declared in the matching `DeclareModuleStmt` (a `Procedure` fulfilling a
/// `Declare`, or a `Global` with the same name) - see `Sema`'s own module
/// name-resolution notes for exactly how a module's contents are kept in a
/// separate, mangled namespace from the main program's and every other
/// module's.
struct ModuleStmt : Stmt {
    ModuleStmt() : Stmt(StmtKind::Module) {}
    std::string name;
    std::string spelling;
    Block body;
};

/// `UseModule Name` (M7d) - imports `Name`'s own public members into the
/// current scope's unqualified name lookup (oracle-verified: *not* required
/// to use a module at all - `Name::Member` always works regardless - purely
/// a convenience for dropping the `Name::` prefix). Scoped to wherever it's
/// encountered: a top-level `UseModule` affects the rest of top-level code;
/// one inside another `Module`'s own body (oracle-verified legal, e.g. a
/// "common" module's Globals shared by several other modules) is saved/
/// restored around that body, not leaked past its own `EndModule`.
struct UseModuleStmt : Stmt {
    UseModuleStmt() : Stmt(StmtKind::UseModule) {}
    std::string name;
    std::string spelling;
};

/// `UnuseModule Name` (M7d) - the inverse of `UseModuleStmt`: removes
/// `Name` from the active-imports list (a no-op if it wasn't imported at
/// all - not independently oracle-verified but a reasonable, low-risk
/// assumption).
struct UnuseModuleStmt : Stmt {
    UnuseModuleStmt() : Stmt(StmtKind::UnuseModule) {}
    std::string name;
    std::string spelling;
};

/// `Declare[.suffix] Name(params)` - a forward declaration enabling mutual
/// recursion (or simply a forward call from earlier in the file), the one
/// legitimate way around PB's usual declare-before-use rule for procedures
/// (oracle-verified: `Declare IsOdd(n.i)` before `Procedure IsEven` lets
/// `IsEven`'s body call the not-yet-defined `IsOdd`). The later real
/// `Procedure` with the same name must match this signature exactly -
/// oracle-verified error: "Declare doesn't match with real Procedure." for
/// *any* parameter or return type mismatch, not just an arity mismatch -
/// and if no matching `Procedure` ever follows, real PB rejects the whole
/// program ("The procedure 'name()' has been declared but not defined.").
/// Deliberately scoped to primitive parameter types only - a `Declare` for
/// a pointer- or Structure-typed parameter is not independently oracle-
/// verified and judged rare enough not to hold up closing this gap.
struct DeclareStmt : Stmt {
    DeclareStmt() : Stmt(StmtKind::Declare) {}
    std::string name;
    std::string spelling;
    TypeSuffix returnSuffix = TypeSuffix::None;
    struct Param {
        std::string name;
        std::string spelling;
        TypeSuffix suffix = TypeSuffix::None;
        /// Oracle-verified: `Declare` params can carry a default value too
        /// (`Declare Foo(x.i = 5)`), matching the real `Procedure`'s own -
        /// only *whether* one is present matters here (for computing how
        /// many arguments a call is required to supply before the real
        /// `Procedure` is even seen), so the expression itself is parsed
        /// and discarded rather than stored.
        bool hasDefault = false;
    };
    std::vector<Param> params;
};

/// `ProcedureReturn [expr]` - `value` is null for a bare return with no
/// value (a Sub-like procedure). Falling off the end of a procedure body
/// without ever executing one behaves like `ProcedureReturn` with no value
/// (oracle-verified: returns the declared return type's zero value) -
/// Codegen's fallthrough safety net (mirroring eBasic's own identical
/// pattern) reproduces this rather than relying on C++'s undefined
/// behavior for falling off the end of a non-void function.
struct ProcedureReturnStmt : Stmt {
    ProcedureReturnStmt() : Stmt(StmtKind::ProcedureReturn) {}
    std::unique_ptr<Expr> value;
};

/// A call used as a whole statement (its return value, if any, discarded) -
/// e.g. `DoSomething(1, 2)` on its own line.
struct ExprStmt : Stmt {
    ExprStmt() : Stmt(StmtKind::ExprStmt) {}
    std::unique_ptr<Expr> expr;
};

/// A whole compiled translation unit.
struct Module {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace easybasic::ast
