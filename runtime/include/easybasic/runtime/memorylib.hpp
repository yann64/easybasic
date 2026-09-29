#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "pbstring.hpp"
#include "stringlib.hpp"

namespace easybasic::runtime {

namespace detail {
/// A real PB program can legally `Poke`/`Peek` at any byte offset, with no
/// guarantee it lands on a naturally-aligned address for the type being
/// read/written (oracle-verified via a targeted UBSan run of this project's
/// own generated code: `PokeL(*blk + 1, ...)` - deliberately misaligned -
/// crashed with "store to misaligned address ... requires 4 byte
/// alignment" when this used a plain `*reinterpret_cast<int32_t*>(address)`
/// dereference, undefined behavior even though it happens to work on x86 in
/// practice). `memcpy`'d unaligned load/store is what every `Peek*`/`Poke*`
/// below actually uses instead - exactly the kind of bug this project's own
/// "special emphasis on memory safety" testing requirement exists to catch,
/// caught here before it ever shipped.
template <typename T>
T unalignedLoad(std::int64_t address) {
    T value;
    std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(T));
    return value;
}

template <typename T>
void unalignedStore(std::int64_t address, T value) {
    std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
}
} // namespace detail

/// Every `PeekX`/`PokeX` reads/writes raw memory at a plain Integer address
/// - PB's own pointer variables are themselves just addresses (see M3d's
/// own notes on why `pbcxx`'s pointer variables are plain `int64_t`), so
/// these take/return that same representation, with no special "is this
/// really a pointer" plumbing needed anywhere in Sema (registered exactly
/// like the M4a/M4b library functions - a fake `ProcedureInfo` entry is all
/// that's needed).
///
/// `PeekC`/`PeekU` share PB's own oracle-verified storage-identical layout
/// (both 16-bit) with `Character`/`Unicode` elsewhere in this project - see
/// `TypeSuffix::Character`'s own doc comment.
inline std::int64_t pbPeekB(std::int64_t address) { return detail::unalignedLoad<std::int8_t>(address); }
inline std::int64_t pbPeekA(std::int64_t address) { return detail::unalignedLoad<std::uint8_t>(address); }
inline std::int64_t pbPeekC(std::int64_t address) { return detail::unalignedLoad<std::uint16_t>(address); }
inline std::int64_t pbPeekW(std::int64_t address) { return detail::unalignedLoad<std::int16_t>(address); }
inline std::int64_t pbPeekU(std::int64_t address) { return detail::unalignedLoad<std::uint16_t>(address); }
inline std::int64_t pbPeekL(std::int64_t address) { return detail::unalignedLoad<std::int32_t>(address); }
inline std::int64_t pbPeekQ(std::int64_t address) { return detail::unalignedLoad<std::int64_t>(address); }
inline float pbPeekF(std::int64_t address) { return detail::unalignedLoad<float>(address); }
inline double pbPeekD(std::int64_t address) { return detail::unalignedLoad<double>(address); }

/// A `Poke*` call's own return value is not independently oracle-verified
/// (a real one appears to be some address-derived value, truncated to
/// whatever numeric width the specific `Poke*` function returns, and is
/// essentially never used in practice - every real PB example calls
/// `Poke*` as a bare statement) - `0` is a safe, documented placeholder
/// rather than a value this project claims matches real PB's own.
inline std::int64_t pbPokeB(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, static_cast<std::int8_t>(value));
    return 0;
}
inline std::int64_t pbPokeA(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, static_cast<std::uint8_t>(value));
    return 0;
}
inline std::int64_t pbPokeC(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, static_cast<std::uint16_t>(value));
    return 0;
}
inline std::int64_t pbPokeW(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, static_cast<std::int16_t>(value));
    return 0;
}
inline std::int64_t pbPokeU(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, static_cast<std::uint16_t>(value));
    return 0;
}
inline std::int64_t pbPokeL(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, static_cast<std::int32_t>(value));
    return 0;
}
inline std::int64_t pbPokeQ(std::int64_t address, std::int64_t value) {
    detail::unalignedStore(address, value);
    return 0;
}
inline std::int64_t pbPokeF(std::int64_t address, float value) {
    detail::unalignedStore(address, value);
    return 0;
}
inline std::int64_t pbPokeD(std::int64_t address, double value) {
    detail::unalignedStore(address, value);
    return 0;
}

namespace detail {

/// Encodes `utf8` (a `PBString`'s own internal representation) as raw
/// UTF-16LE bytes, astral code points becoming a surrogate pair - the
/// layout `PokeS` actually writes to memory. Oracle-verified: real PB
/// compiles in Unicode mode by default, so `PokeS(*mem, "Hi")` writes the
/// bytes `[72, 0, 105, 0, 0, 0]` (`'H'`, `'i'`, then a 2-byte null
/// terminator) - genuinely 2 bytes per character, not `PBString`'s own
/// UTF-8 layout, so this conversion (not a raw byte copy) is required for
/// `PeekS`/`PokeS` to interoperate correctly with anything that reads/
/// writes memory at the byte level (which is `Peek*`/`Poke*`'s whole
/// reason to exist).
inline std::string utf8ToUtf16LEBytes(const std::string& utf8) {
    std::string out;
    std::size_t i = 0;
    while (i < utf8.size()) {
        std::uint32_t cp = decodeUtf8At(utf8, i);
        if (cp <= 0xFFFFU) {
            out += static_cast<char>(cp & 0xFFU);
            out += static_cast<char>((cp >> 8) & 0xFFU);
        } else {
            std::uint32_t v = cp - 0x10000U;
            auto hi = static_cast<std::uint16_t>(0xD800U + (v >> 10));
            auto lo = static_cast<std::uint16_t>(0xDC00U + (v & 0x3FFU));
            out += static_cast<char>(hi & 0xFFU);
            out += static_cast<char>((hi >> 8) & 0xFFU);
            out += static_cast<char>(lo & 0xFFU);
            out += static_cast<char>((lo >> 8) & 0xFFU);
        }
    }
    return out;
}

/// Appends `cp`'s UTF-8 encoding to `out` - the same logic `pbChr` uses,
/// factored out so `utf16LEBytesToUtf8` (which decodes a whole run of code
/// points, not just one) can reuse it directly.
inline void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp <= 0x7FU) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FFU) {
        out += static_cast<char>(0xC0U | (cp >> 6));
        out += static_cast<char>(0x80U | (cp & 0x3FU));
    } else if (cp <= 0xFFFFU) {
        out += static_cast<char>(0xE0U | (cp >> 12));
        out += static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        out += static_cast<char>(0x80U | (cp & 0x3FU));
    } else {
        out += static_cast<char>(0xF0U | (cp >> 18));
        out += static_cast<char>(0x80U | ((cp >> 12) & 0x3FU));
        out += static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        out += static_cast<char>(0x80U | (cp & 0x3FU));
    }
}

/// Decodes UTF-16LE code units starting at `address`, either exactly
/// `codeUnitCount` of them (when `>= 0`) or until a null code unit
/// (`codeUnitCount < 0`, `PeekS`'s own default) - the reverse of
/// `utf8ToUtf16LEBytes`, returning ordinary UTF-8 bytes for a `PBString`.
inline std::string utf16LEBytesToUtf8(std::int64_t address, std::int64_t codeUnitCount) {
    std::string out;
    const auto* bytes = reinterpret_cast<const unsigned char*>(address);
    std::int64_t i = 0;
    while (codeUnitCount < 0 || i < codeUnitCount) {
        auto unit = static_cast<std::uint16_t>(bytes[i * 2] | (static_cast<std::uint16_t>(bytes[(i * 2) + 1]) << 8));
        if (codeUnitCount < 0 && unit == 0) {
            break;
        }
        ++i;
        std::uint32_t cp = unit;
        if (unit >= 0xD800U && unit <= 0xDBFFU) {
            auto lo = static_cast<std::uint16_t>(bytes[i * 2] | (static_cast<std::uint16_t>(bytes[(i * 2) + 1]) << 8));
            cp = 0x10000U + ((static_cast<std::uint32_t>(unit) - 0xD800U) << 10) + (lo - 0xDC00U);
            ++i;
        }
        appendUtf8(out, cp);
    }
    return out;
}

} // namespace detail

inline std::int64_t pbPokeS(std::int64_t address, const PBString& value) {
    std::string bytes = detail::utf8ToUtf16LEBytes(value.bytes());
    std::memcpy(reinterpret_cast<void*>(address), bytes.data(), bytes.size());
    // Null-terminate (a 2-byte UTF-16 code unit), matching real PB's own
    // PokeS - oracle-verified via the resulting bytes being readable back
    // with PeekS's own no-length, null-terminated form.
    detail::unalignedStore(address + static_cast<std::int64_t>(bytes.size()), std::uint16_t{0});
    return 0;
}

/// `length` defaults to reading until a null terminator (oracle-verified:
/// `PeekS(*blk)` reads the whole string); a non-negative `length` instead
/// reads exactly that many UTF-16 code units (oracle-verified:
/// `PeekS(*blk, 5)` on a longer buffer reads only the first 5 characters).
inline PBString pbPeekS(std::int64_t address, std::int64_t length = -1) {
    return PBString(detail::utf16LEBytesToUtf8(address, length));
}

} // namespace easybasic::runtime
