#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "pbstring.hpp"

namespace easybasic::runtime {

namespace detail {

/// Decodes one UTF-8 code point starting at byte index `i` in `s`,
/// advancing `i` past it. A malformed/truncated leading byte is treated as
/// a single Latin-1 code point and `i` advances by one, so callers always
/// make forward progress - PB source is well-formed UTF-8 in practice, so
/// this lenient fallback is a safety net, not a path exercised by real
/// programs.
inline std::uint32_t decodeUtf8At(const std::string& s, std::size_t& i) {
    auto byteAt = [&s](std::size_t idx) { return static_cast<unsigned char>(s[idx]); };
    unsigned char b0 = byteAt(i);
    if (b0 < 0x80) {
        ++i;
        return b0;
    }
    if ((b0 & 0xE0U) == 0xC0U && i + 1 < s.size()) {
        std::uint32_t cp = (static_cast<std::uint32_t>(b0 & 0x1FU) << 6) | (byteAt(i + 1) & 0x3FU);
        i += 2;
        return cp;
    }
    if ((b0 & 0xF0U) == 0xE0U && i + 2 < s.size()) {
        std::uint32_t cp = (static_cast<std::uint32_t>(b0 & 0x0FU) << 12) |
                            (static_cast<std::uint32_t>(byteAt(i + 1) & 0x3FU) << 6) | (byteAt(i + 2) & 0x3FU);
        i += 3;
        return cp;
    }
    if ((b0 & 0xF8U) == 0xF0U && i + 3 < s.size()) {
        std::uint32_t cp = (static_cast<std::uint32_t>(b0 & 0x07U) << 18) |
                            (static_cast<std::uint32_t>(byteAt(i + 1) & 0x3FU) << 12) |
                            (static_cast<std::uint32_t>(byteAt(i + 2) & 0x3FU) << 6) | (byteAt(i + 3) & 0x3FU);
        i += 4;
        return cp;
    }
    ++i;
    return b0;
}

} // namespace detail

/// The number of UTF-16 code units `utf8` would occupy in PB's own internal
/// representation (a code point above the Basic Multilingual Plane counts
/// as 2, a surrogate pair) - this, not a byte count, is what PB's `Len`
/// actually measures (see docs/architecture/roadmap.md's M0 notes on why
/// `PBString` is UTF-8-backed rather than replicating PB's own layout, and
/// the mitigation this function is part of).
inline std::size_t pbUtf16Length(const std::string& utf8) {
    std::size_t count = 0;
    std::size_t i = 0;
    while (i < utf8.size()) {
        std::uint32_t cp = detail::decodeUtf8At(utf8, i);
        count += (cp > 0xFFFFU) ? 2 : 1;
    }
    return count;
}

/// The UTF-8 bytes corresponding to the UTF-16 code-unit range
/// `[startCodeUnit, startCodeUnit + codeUnitCount)` of `utf8` - the shared
/// worker behind `Left`/`Right`/`Mid`. Clamps gracefully at both ends
/// (oracle-verified: `Left("Hi", 100)` returns `"Hi"`, `Mid("Hi", 5, 2)`
/// returns `""`) rather than throwing, matching real PB's own forgiving
/// behavior for out-of-range lengths. A code-unit boundary that falls
/// inside a surrogate pair rounds outward to the nearest whole code point
/// (an extreme edge case with no independently oracle-verified behavior to
/// match).
inline std::string pbUtf16Slice(const std::string& utf8, std::int64_t startCodeUnit, std::int64_t codeUnitCount) {
    if (startCodeUnit < 0) {
        startCodeUnit = 0;
    }
    if (codeUnitCount < 0) {
        codeUnitCount = 0;
    }
    auto start = static_cast<std::uint64_t>(startCodeUnit);
    std::uint64_t endExclusive = start + static_cast<std::uint64_t>(codeUnitCount);
    if (endExclusive <= start) {
        // A zero (or, after clamping above, an originally-negative) count
        // requests an empty slice - must be handled before the loop below,
        // which otherwise only discovers "already past the end" after
        // having consumed one whole code point too many (oracle-verified
        // regression caught by testing: `Left("Hi", 0)` must be `""`, not
        // `"H"`).
        return {};
    }

    std::size_t i = 0;
    std::uint64_t codeUnitPos = 0;
    std::size_t byteStart = utf8.size();
    std::size_t byteEnd = utf8.size();
    bool started = false;
    while (i < utf8.size()) {
        std::size_t byteBefore = i;
        std::uint32_t cp = detail::decodeUtf8At(utf8, i);
        std::uint64_t units = (cp > 0xFFFFU) ? 2 : 1;
        if (!started && codeUnitPos >= start) {
            byteStart = byteBefore;
            started = true;
        }
        codeUnitPos += units;
        if (started && codeUnitPos >= endExclusive) {
            byteEnd = i;
            break;
        }
    }
    if (!started) {
        return {};
    }
    return utf8.substr(byteStart, byteEnd - byteStart);
}

inline std::int64_t pbLen(const PBString& s) { return static_cast<std::int64_t>(pbUtf16Length(s.bytes())); }

inline PBString pbLeft(const PBString& s, std::int64_t n) { return PBString(pbUtf16Slice(s.bytes(), 0, n)); }

inline PBString pbRight(const PBString& s, std::int64_t n) {
    auto total = static_cast<std::int64_t>(pbUtf16Length(s.bytes()));
    std::int64_t start = total - n;
    if (start < 0) {
        start = 0;
    }
    return PBString(pbUtf16Slice(s.bytes(), start, n));
}

/// `start` is 1-based, matching PB's own `Mid` convention. `count`
/// defaulting to (effectively) "the rest of the string" relies on
/// `pbUtf16Slice`'s own end-clamping rather than computing the string's
/// actual remaining length here, since a real C++ default parameter must be
/// a compile-time constant.
inline PBString pbMid(const PBString& s, std::int64_t start, std::int64_t count = INT64_MAX) {
    std::int64_t zeroBased = start - 1;
    if (zeroBased < 0) {
        zeroBased = 0;
    }
    return PBString(pbUtf16Slice(s.bytes(), zeroBased, count));
}

/// ASCII-only case conversion - ASCII covers the overwhelming majority of
/// real PB source's string literals, and leaving every byte `>= 0x80`
/// untouched is what keeps this safe on arbitrary UTF-8 (a multi-byte
/// sequence's continuation bytes are always in that range, so this can
/// never corrupt one) rather than a correctness guarantee for accented or
/// non-Latin scripts, which would need real Unicode case-folding tables.
inline PBString pbUCase(const PBString& s) {
    std::string out = s.bytes();
    for (char& c : out) {
        if (static_cast<unsigned char>(c) < 0x80) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    return PBString(std::move(out));
}

inline PBString pbLCase(const PBString& s) {
    std::string out = s.bytes();
    for (char& c : out) {
        if (static_cast<unsigned char>(c) < 0x80) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return PBString(std::move(out));
}

/// Trims plain ASCII spaces only (PB's own default, un-verified for other
/// whitespace like tabs - real PB's `Trim`/`LTrim`/`RTrim` accept an
/// optional second argument naming a different character to trim instead,
/// not yet implemented).
inline PBString pbLTrim(const PBString& s) {
    const std::string& b = s.bytes();
    std::size_t i = 0;
    while (i < b.size() && b[i] == ' ') {
        ++i;
    }
    return PBString(b.substr(i));
}

inline PBString pbRTrim(const PBString& s) {
    const std::string& b = s.bytes();
    std::size_t end = b.size();
    while (end > 0 && b[end - 1] == ' ') {
        --end;
    }
    return PBString(b.substr(0, end));
}

inline PBString pbTrim(const PBString& s) { return pbRTrim(pbLTrim(s)); }

inline PBString pbStr(std::int64_t n) { return PBString(std::to_string(n)); }

/// Oracle-verified: `Val` parses like `strtoll` - optional leading
/// whitespace and sign, stops at the first non-digit character (so a
/// decimal point truncates rather than erroring, e.g. `Val("3.14")` is `3`)
/// and yields `0` for a string with no valid leading number at all.
inline std::int64_t pbVal(const PBString& s) { return std::strtoll(s.bytes().c_str(), nullptr, 10); }

/// Oracle-verified: `ValF` returns a Float (single precision), not a
/// Double - confirmed by `StrF`'s own default-precision output showing
/// single-precision rounding artifacts on a literal argument.
inline float pbValF(const PBString& s) { return std::strtof(s.bytes().c_str(), nullptr); }

/// `decimals` defaulting to 10 is oracle-verified (`StrF(3.14159)` with no
/// second argument prints 10 decimal digits, including the literal's own
/// single-precision rounding artifacts: `3.1415901184`).
inline PBString pbStrF(float n, std::int64_t decimals = 10) {
    if (decimals < 0) {
        decimals = 0;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", static_cast<int>(decimals), static_cast<double>(n));
    return PBString(buf);
}

inline PBString pbChr(std::int64_t codepoint) {
    std::string out;
    auto cp = static_cast<std::uint32_t>(codepoint);
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
    return PBString(std::move(out));
}

/// The empty-string case (`0`, oracle-verified) is the one input
/// `detail::decodeUtf8At` can't itself handle (it assumes at least one
/// byte to look at).
inline std::int64_t pbAsc(const PBString& s) {
    const std::string& b = s.bytes();
    if (b.empty()) {
        return 0;
    }
    std::size_t i = 0;
    return static_cast<std::int64_t>(detail::decodeUtf8At(b, i));
}

} // namespace easybasic::runtime
