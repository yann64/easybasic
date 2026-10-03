#pragma once

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

#include "pbstring.hpp"

namespace easybasic::runtime {

namespace detail {
/// `std::localtime` (not the POSIX-only `localtime_r`/Windows-only
/// `localtime_s`) for maximum portability across every platform this
/// project targets (Linux/Haiku/Windows) - its shared-static-buffer
/// thread-safety caveat doesn't matter for single-threaded generated code
/// (threads are out of scope until M7), and the result is copied out
/// immediately regardless.
inline std::tm localTm(std::int64_t date) {
    auto t = static_cast<std::time_t>(date);
    std::tm* result = std::localtime(&t);
    return result != nullptr ? *result : std::tm{};
}
} // namespace detail

/// `Date()` with no arguments - the current local time as a Unix-epoch-
/// style Integer timestamp. Deliberately not oracle-matchable on exact
/// value (like `Random`) - only usable in a test as "is a plausible
/// timestamp", never a captured literal.
inline std::int64_t pbDateNow() { return static_cast<std::int64_t>(std::time(nullptr)); }

/// `Date(year, month, day, hour, minute, second)` - oracle-verified to
/// interpret its components as *local* time (matching `Year`/`Month`/etc.'s
/// own extraction, so the two round-trip consistently regardless of the
/// system's timezone) and reject any argument count other than exactly 0
/// or 6 (`Date(2024, 1, 1)` - 3 args - is a real PB compile error,
/// "Incorrect number of parameters.") - Sema only checks the *continuous*
/// range `[0, 6]`, a documented, minor imprecision versus that exact
/// bimodal rule; anything in between fails at the C++ backend-compile
/// stage instead of with a clean PB-style diagnostic, safely rejected
/// either way.
inline std::int64_t pbDate(std::int64_t year, std::int64_t month, std::int64_t day, std::int64_t hour,
                            std::int64_t minute, std::int64_t second) {
    std::tm value{};
    value.tm_year = static_cast<int>(year) - 1900;
    value.tm_mon = static_cast<int>(month) - 1;
    value.tm_mday = static_cast<int>(day);
    value.tm_hour = static_cast<int>(hour);
    value.tm_min = static_cast<int>(minute);
    value.tm_sec = static_cast<int>(second);
    value.tm_isdst = -1;
    return static_cast<std::int64_t>(std::mktime(&value));
}

inline std::int64_t pbYear(std::int64_t date) { return detail::localTm(date).tm_year + 1900; }
inline std::int64_t pbMonth(std::int64_t date) { return detail::localTm(date).tm_mon + 1; }
inline std::int64_t pbDay(std::int64_t date) { return detail::localTm(date).tm_mday; }
inline std::int64_t pbHour(std::int64_t date) { return detail::localTm(date).tm_hour; }
inline std::int64_t pbMinute(std::int64_t date) { return detail::localTm(date).tm_min; }
inline std::int64_t pbSecond(std::int64_t date) { return detail::localTm(date).tm_sec; }
/// Oracle-verified: `0` = Sunday through `6` = Saturday - `std::tm::tm_wday`
/// already uses this exact convention, so no translation is needed.
inline std::int64_t pbDayOfWeek(std::int64_t date) { return detail::localTm(date).tm_wday; }

/// Supports the oracle-verified placeholder set `%yyyy`/`%mm`/`%dd`/`%hh`/
/// `%ii`/`%ss` (4-digit year, 2-digit month/day/hour/minute/second) -
/// PB's own `FormatDate` supports more (short-year, month/weekday names,
/// AM/PM), not yet implemented.
inline PBString pbFormatDate(const PBString& mask, std::int64_t date) {
    std::tm value = detail::localTm(date);
    std::string result = mask.bytes();
    auto replaceAll = [&result](const std::string& from, const std::string& to) {
        std::size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    // Sized to safely fit any `int` (up to 11 digits plus sign plus the
    // null terminator), not just the small values a real `std::tm` ever
    // actually holds - avoids a `-Wformat-truncation` warning GCC raises
    // because it can't itself prove the smaller, merely-"big enough in
    // practice" size `snprintf` is writing into is never exceeded.
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d", value.tm_year + 1900);
    replaceAll("%yyyy", buf);
    std::snprintf(buf, sizeof(buf), "%02d", value.tm_mon + 1);
    replaceAll("%mm", buf);
    std::snprintf(buf, sizeof(buf), "%02d", value.tm_mday);
    replaceAll("%dd", buf);
    std::snprintf(buf, sizeof(buf), "%02d", value.tm_hour);
    replaceAll("%hh", buf);
    std::snprintf(buf, sizeof(buf), "%02d", value.tm_min);
    replaceAll("%ii", buf);
    std::snprintf(buf, sizeof(buf), "%02d", value.tm_sec);
    replaceAll("%ss", buf);
    return PBString(std::move(result));
}

/// `unit` is one of the oracle-verified `#PB_Date_*` constant values
/// (`Second`=6, `Minute`=5, `Hour`=4, `Day`=3, `Week`=2, `Month`=1,
/// `Year`=0 - `Sema::registerBuiltinConstants` pre-declares these).
/// `Month`/`Year` genuinely re-normalize through `mktime` (oracle-verified:
/// adding 1 month to March 15 lands on April 15, real calendar arithmetic,
/// not a fixed "+30 days" shortcut - confirmed by computing the exact
/// expected epoch value independently and matching the oracle's own
/// output, not just eyeballing a plausible-looking number).
inline std::int64_t pbAddDate(std::int64_t date, std::int64_t unit, std::int64_t value) {
    switch (unit) {
        case 6: return date + value;
        case 5: return date + (value * 60);
        case 4: return date + (value * 3600);
        case 3: return date + (value * 86400);
        case 2: return date + (value * 7 * 86400);
        case 1: {
            std::tm tmValue = detail::localTm(date);
            tmValue.tm_mon += static_cast<int>(value);
            tmValue.tm_isdst = -1;
            return static_cast<std::int64_t>(std::mktime(&tmValue));
        }
        case 0: {
            std::tm tmValue = detail::localTm(date);
            tmValue.tm_year += static_cast<int>(value);
            tmValue.tm_isdst = -1;
            return static_cast<std::int64_t>(std::mktime(&tmValue));
        }
        default: return date;
    }
}

} // namespace easybasic::runtime
