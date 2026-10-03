#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "pbstring.hpp"
#include "stringlib.hpp"

namespace easybasic::runtime {

/// One `Data` item's own value, tagged by the `TypeSuffix` family its
/// `Data.<suffix>` declaration gave it. Oracle-verified (`Read.s` into an
/// Integer-typed destination silently yields `Val("hello")` = `0`, not a
/// type error - "Data/Read is a plain-assignment-coercion contract, not a
/// type-checked one") that a `Read`/`Data` mismatch is legal and coerces
/// rather than erroring - `pbReadDataInt`/`Double`/`String` below do that
/// coercion themselves, by the item's own *stored* kind, so by the time
/// Codegen's own target-typed `convert()` sees the result, it's already a
/// genuinely well-typed `std::int64_t`/`double`/`PBString` matching
/// `Read`'s own declared suffix, and ordinary same-family conversion rules
/// apply from there. This is a deliberate simplification, not a byte-for-
/// byte replica of real PB's own raw-memory-blob model (where a mismatched
/// `Read` instead desyncs a *byte-level* cursor, producing outright
/// garbage on every later read too) - see docs/architecture/roadmap.md's
/// M5b notes for the oracle exploration behind this choice.
struct PBDataValue {
    enum class Kind : std::uint8_t { Integer, Double, String };
    Kind kind = Kind::Integer;
    std::int64_t intValue = 0;
    double doubleValue = 0.0;
    PBString stringValue;
};

namespace detail {
inline std::vector<PBDataValue>& dataPool() {
    static std::vector<PBDataValue> pool;
    return pool;
}
inline std::size_t& dataCursor() {
    static std::size_t cursor = 0;
    return cursor;
}
} // namespace detail

/// Registers one literal `Data` value into the global pool - Codegen emits
/// a call to one of these, in file order, as the very first statements in
/// generated `main()`, for every `Data.<suffix>` value anywhere in the
/// module (oracle-verified: multiple `DataSection`s - even one nested
/// inside a `Procedure` - are concatenated into a single flat sequence,
/// not kept separate).
inline void pbDataAddInt(std::int64_t v) {
    PBDataValue item;
    item.kind = PBDataValue::Kind::Integer;
    item.intValue = v;
    detail::dataPool().push_back(item);
}
inline void pbDataAddDouble(double v) {
    PBDataValue item;
    item.kind = PBDataValue::Kind::Double;
    item.doubleValue = v;
    detail::dataPool().push_back(item);
}
inline void pbDataAddString(const PBString& v) {
    PBDataValue item;
    item.kind = PBDataValue::Kind::String;
    item.stringValue = v;
    detail::dataPool().push_back(item);
}

inline bool pbDataHasMore() { return detail::dataCursor() < detail::dataPool().size(); }

/// The pool's current size - not something generated code ever needs (a
/// `Restore label`'s own index is always a compile-time constant), but
/// useful for tests: since the pool/cursor are process-global state shared
/// across every test case in the same binary, a test can snapshot this
/// *before* adding its own items, then `pbDataRestore` back to it, to read
/// back exactly what it added regardless of what other test cases already
/// put in the pool or what order tests run in.
inline std::size_t pbDataPoolSize() { return detail::dataPool().size(); }

/// Oracle-verified: reading past the last `Data` value is a fatal error
/// ("Read data error: no more data.", exit code 1) - but *only* in a debug
/// (`-d`) build; a plain build has no such check at all, exactly like
/// `Debug` statements themselves vanishing in a release build. Codegen
/// emits a call to this only under `-d`, immediately before the actual
/// read, mirroring that same convention (see `Codegen::debugMode_`'s own
/// notes). This doesn't attempt to replicate PB's exact message wording or
/// its file/line reference - the oracle-verified *contract* (debug-only
/// fatal error on exhaustion) is what matters, not byte-for-byte text.
[[noreturn]] inline void pbDataReadError() {
    std::cerr << "Read data error: no more data.\n";
    std::exit(1);
}

inline std::int64_t pbReadDataInt() {
    if (!pbDataHasMore()) {
        return 0; // Release-mode fallback - real PB's own release-mode behavior here was never observed (its debug-only check hides whatever it is), so this is a safe, harmless default rather than an attempt to match it.
    }
    const PBDataValue& v = detail::dataPool()[detail::dataCursor()++];
    switch (v.kind) {
        case PBDataValue::Kind::Integer:
            return v.intValue;
        case PBDataValue::Kind::Double:
            return static_cast<std::int64_t>(v.doubleValue);
        case PBDataValue::Kind::String:
            return pbVal(v.stringValue);
    }
    return 0;
}

inline double pbReadDataDouble() {
    if (!pbDataHasMore()) {
        return 0.0;
    }
    const PBDataValue& v = detail::dataPool()[detail::dataCursor()++];
    switch (v.kind) {
        case PBDataValue::Kind::Integer:
            return static_cast<double>(v.intValue);
        case PBDataValue::Kind::Double:
            return v.doubleValue;
        case PBDataValue::Kind::String:
            return std::strtod(v.stringValue.bytes().c_str(), nullptr);
    }
    return 0.0;
}

inline PBString pbReadDataString() {
    if (!pbDataHasMore()) {
        return {};
    }
    const PBDataValue& v = detail::dataPool()[detail::dataCursor()++];
    switch (v.kind) {
        case PBDataValue::Kind::Integer:
            return pbStr(v.intValue);
        case PBDataValue::Kind::Double:
            return PBString(std::to_string(v.doubleValue));
        case PBDataValue::Kind::String:
            return v.stringValue;
    }
    return {};
}

/// `Restore label` - Codegen emits `pbDataRestore(<N>)` with `label`'s own
/// compile-time-known flat index (see `Sema::dataLabelIndex`).
inline void pbDataRestore(std::size_t index) { detail::dataCursor() = index; }

} // namespace easybasic::runtime
