#pragma once

#include <cstdio>
#include <iostream>
#include <string>

namespace easybasic::runtime {

/// Emits one `Debug` line, matching real PureBasic's debugger console
/// format exactly (oracle-verified byte-for-byte via `pbcompilerc -d`:
/// `"[Debugger]  " + value + '\n'`, two spaces after the closing bracket).
///
/// Only called from generated code when `pbcxx` was invoked in debug mode
/// (mirroring `pbcompilerc -d`) - a plain build emits no call at all for a
/// `Debug` statement, exactly like a plain (non-`-d`) `pbcompilerc` build
/// strips it to nothing. See docs/architecture/roadmap.md's M0 notes.
inline void debugPrint(const std::string& text) {
    std::cout << "[Debugger]  " << text << '\n';
}

/// `Debug`'ing a raw Double directly (not through `Str`/`StrF`) uses a
/// different format from either of those - oracle-verified: a 16-character-
/// wide, right-justified `%g`-style field (6 significant digits, switching
/// to scientific notation for very large/small magnitudes, e.g. `Debug
/// 1000000.0` prints `"           1e+06"`). This is what every M4b Math
/// builtin (`Sqr`, `Pow`, `Round`, ...) needs, since they all return Double
/// regardless of their argument's own type (also oracle-verified: `Abs(-5)`
/// with an Integer argument still prints via this Double format, not a
/// plain Integer one).
inline std::string pbDebugFormatDouble(double value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%16g", value);
    return buf;
}

/// `Debug`'ing a raw Float (single precision) directly uses yet another,
/// simpler format - oracle-verified: a plain `%f`-style six-decimal-place
/// rendering, no field width, no scientific notation (e.g. `Debug f` for
/// `f.f = 3.14159` prints `"3.141590"`, not the Double form's padded
/// `%g` rendering).
inline std::string pbDebugFormatFloat(float value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%f", static_cast<double>(value));
    return buf;
}

} // namespace easybasic::runtime
