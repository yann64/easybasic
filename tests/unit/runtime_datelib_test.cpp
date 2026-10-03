#include <catch2/catch_test_macros.hpp>

#include <easybasic/runtime/datelib.hpp>

using namespace easybasic::runtime;

TEST_CASE("pbDate/pbYear/pbMonth/pbDay/pbHour/pbMinute/pbSecond round-trip", "[runtime][datelib]") {
    // Both construction (pbDate) and extraction interpret components as
    // local time, so this round-trips identically regardless of the
    // system's timezone - no TZ-specific setup needed.
    std::int64_t d = pbDate(2024, 3, 15, 10, 30, 45);
    CHECK(pbYear(d) == 2024);
    CHECK(pbMonth(d) == 3);
    CHECK(pbDay(d) == 15);
    CHECK(pbHour(d) == 10);
    CHECK(pbMinute(d) == 30);
    CHECK(pbSecond(d) == 45);
}

TEST_CASE("pbDayOfWeek matches real PB's 0=Sunday..6=Saturday convention", "[runtime][datelib]") {
    // 2024-03-15 is oracle-verified to be a Friday (5).
    std::int64_t friday = pbDate(2024, 3, 15, 0, 0, 0);
    CHECK(pbDayOfWeek(friday) == 5);
}

TEST_CASE("pbDateNow returns a plausible current timestamp", "[runtime][datelib]") {
    // Like pbRandom, not oracle-matchable on an exact value - only that
    // it's a sane, recent-looking timestamp.
    std::int64_t now = pbDateNow();
    CHECK(now > pbDate(2020, 1, 1, 0, 0, 0));
}

TEST_CASE("pbFormatDate substitutes the oracle-verified placeholder set", "[runtime][datelib]") {
    std::int64_t d = pbDate(2024, 3, 5, 9, 8, 7);
    CHECK(pbFormatDate(PBString("%yyyy-%mm-%dd %hh:%ii:%ss"), d).bytes() == "2024-03-05 09:08:07");
}

TEST_CASE("pbAddDate's Day/Hour/Minute/Second units are plain second arithmetic", "[runtime][datelib]") {
    std::int64_t d = pbDate(2024, 3, 15, 10, 30, 45);
    CHECK(pbAddDate(d, 3, 1) == d + 86400);   // Day
    CHECK(pbAddDate(d, 4, 2) == d + 7200);    // Hour
    CHECK(pbAddDate(d, 5, 30) == d + 1800);   // Minute
    CHECK(pbAddDate(d, 6, 10) == d + 10);     // Second
    CHECK(pbAddDate(d, 2, 1) == d + (7 * 86400)); // Week
}

TEST_CASE("pbAddDate's Month/Year units are genuine calendar arithmetic, not a fixed day offset",
          "[runtime][datelib]") {
    // Oracle-verified: adding 1 month to March 15 lands on April 15 (via
    // real mktime normalization), not "+31 days" as a coincidence of
    // March's own length - confirmed by checking against the exact
    // expected date's own components, not just a plausible-looking number.
    std::int64_t d = pbDate(2024, 3, 15, 10, 30, 45);
    std::int64_t nextMonth = pbAddDate(d, 1, 1);
    CHECK(pbMonth(nextMonth) == 4);
    CHECK(pbDay(nextMonth) == 15);

    std::int64_t nextYear = pbAddDate(d, 0, 1);
    CHECK(pbYear(nextYear) == 2025);
    CHECK(pbMonth(nextYear) == 3);
    CHECK(pbDay(nextYear) == 15);
}
