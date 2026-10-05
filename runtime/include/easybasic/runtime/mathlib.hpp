#pragma once

#include <cmath>
#include <cstdint>
#include <random>

namespace easybasic::runtime {

/// Every one of these (except `Int`) is oracle-verified to always return a
/// Double, regardless of its argument's own type - `Abs(-5)` with an
/// Integer argument still prints via `pbDebugFormatDouble`'s own field-
/// width format, not a plain Integer one. Each is a thin wrapper over the
/// matching `<cmath>` function; PB's own trig functions work in radians,
/// exactly like C's (oracle-verified: `ATan2(1, 1)` is `0.785398`, i.e.
/// pi/4 radians, not 45 degrees).
inline double pbAbs(double n) { return std::fabs(n); }
inline double pbSqr(double n) { return std::sqrt(n); }
inline double pbPow(double base, double exponent) { return std::pow(base, exponent); }
inline double pbSin(double n) { return std::sin(n); }
inline double pbCos(double n) { return std::cos(n); }
inline double pbTan(double n) { return std::tan(n); }
inline double pbASin(double n) { return std::asin(n); }
inline double pbACos(double n) { return std::acos(n); }
inline double pbATan(double n) { return std::atan(n); }
inline double pbATan2(double y, double x) { return std::atan2(y, x); }
inline double pbExp(double n) { return std::exp(n); }
inline double pbLog(double n) { return std::log(n); }
inline double pbLog10(double n) { return std::log10(n); }

/// `Round(n, mode)` - `mode` is one of the three oracle-verified
/// `#PB_Round_*` constants pre-declared by `Sema::registerBuiltinConstants`
/// (`Down` = 0, `Up` = 1, `Nearest` = 2; a fourth, `#PB_Round_Truncate`,
/// does not actually exist in real PB - oracle-verified: "Constant not
/// found"). `Nearest` rounds half away from zero (oracle-verified:
/// `Round(2.5, ...)` is `3`, `Round(-2.5, ...)` is `-3`) - `std::round`'s
/// own behavior exactly, genuinely different from this project's usual
/// banker's-rounding rule for an *implicit* Float-to-Integer conversion
/// elsewhere. `Down`/`Up` are real mathematical floor/ceiling (oracle-
/// verified: `Round(-3.1, Down)` is `-4`, not `-3` - rounding toward zero
/// is not what "Down" means here).
inline double pbRound(double n, std::int64_t mode) {
    switch (mode) {
        case 0: return std::floor(n);
        case 1: return std::ceil(n);
        default: return std::round(n);
    }
}

/// `Int(n)` truncates toward zero (oracle-verified: `Int(-3.7)` is `-3`,
/// not `-4` - distinct from `Round(n, #PB_Round_Down)`'s real floor) and,
/// unlike every other function here, genuinely returns an Integer.
inline std::int64_t pbInt(double n) { return static_cast<std::int64_t>(std::trunc(n)); }

namespace detail {
/// A process-wide PRNG state for `Random`/`RandomSeed` - a plain
/// `std::mt19937`, deliberately not an attempt to replicate real PB's own
/// (undocumented) generator algorithm bit-for-bit. `RandomSeed` makes this
/// project's own output reproducible for a given seed, exactly like real
/// PB's does for its own sequence, but the two will never agree on the
/// *specific* values produced - not a goal a differential test against the
/// oracle can even meaningfully verify, so no e2e_diff test asserts an
/// exact `Random` value.
inline std::mt19937& pbRandomEngine() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
} // namespace detail

inline void pbRandomSeed(std::int64_t seed) {
    detail::pbRandomEngine().seed(static_cast<std::mt19937::result_type>(seed));
}

/// `Random(max [, min])` - an inclusive `[min, max]` range (`min` defaults
/// to 0), matching real PB's own documented range.
inline std::int64_t pbRandom(std::int64_t maxValue, std::int64_t minValue = 0) {
    if (maxValue < minValue) {
        std::swap(maxValue, minValue);
    }
    std::uniform_int_distribution<std::int64_t> dist(minValue, maxValue);
    return dist(detail::pbRandomEngine());
}

/// `RGB`/`RGBA`/`Red`/`Green`/`Blue`/`Alpha` - a prerequisite for the
/// Requester family's own `ColorRequester`/`FontRequester` (M7b), neither
/// of which existed in this project before. Oracle-verified packing (a
/// `Debug RGB(10,20,30)` probe, decoded): `0x00BBGGRR` - the classic Win32
/// `COLORREF` byte order (red in the lowest byte), not `0x00RRGGBB` as the
/// argument order alone might suggest. `RGBA` is the same layout with
/// alpha in the highest byte (`0xAABBGGRR`), confirmed the same way.
inline std::int64_t pbRGB(std::int64_t r, std::int64_t g, std::int64_t b) {
    return (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16);
}

inline std::int64_t pbRGBA(std::int64_t r, std::int64_t g, std::int64_t b, std::int64_t a) {
    return pbRGB(r, g, b) | ((a & 0xff) << 24);
}

inline std::int64_t pbRed(std::int64_t color) { return color & 0xff; }
inline std::int64_t pbGreen(std::int64_t color) { return (color >> 8) & 0xff; }
inline std::int64_t pbBlue(std::int64_t color) { return (color >> 16) & 0xff; }
inline std::int64_t pbAlpha(std::int64_t color) { return (color >> 24) & 0xff; }

} // namespace easybasic::runtime
