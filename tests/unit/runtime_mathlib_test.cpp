#include <catch2/catch_test_macros.hpp>

#include <easybasic/runtime/mathlib.hpp>

using namespace easybasic::runtime;

TEST_CASE("pbAbs/pbSqr/pbPow compute the expected Double results", "[runtime][mathlib]") {
    CHECK(pbAbs(-5.0) == 5.0);
    CHECK(pbAbs(5.0) == 5.0);
    CHECK(pbSqr(16.0) == 4.0);
    CHECK(pbPow(2.0, 10.0) == 1024.0);
}

TEST_CASE("pbInt truncates toward zero, distinct from floor", "[runtime][mathlib]") {
    CHECK(pbInt(3.7) == 3);
    CHECK(pbInt(-3.7) == -3); // not -4, which a real floor would give
}

TEST_CASE("pbRound's Nearest mode rounds half away from zero", "[runtime][mathlib]") {
    // Oracle-verified: distinct from this project's usual banker's-rounding
    // rule for an implicit Float-to-Integer conversion elsewhere.
    CHECK(pbRound(3.5, 2) == 4.0);
    CHECK(pbRound(2.5, 2) == 3.0);
    CHECK(pbRound(-2.5, 2) == -3.0);
}

TEST_CASE("pbRound's Down/Up modes are real floor/ceiling", "[runtime][mathlib]") {
    // Oracle-verified: "Down" means floor (toward negative infinity), not
    // truncation toward zero - Round(-3.1, Down) is -4, not -3.
    CHECK(pbRound(3.9, 0) == 3.0);
    CHECK(pbRound(-3.1, 0) == -4.0);
    CHECK(pbRound(3.1, 1) == 4.0);
}

TEST_CASE("pbSin/pbCos/pbATan2 work in radians", "[runtime][mathlib]") {
    CHECK(pbSin(0.0) == 0.0);
    CHECK(pbCos(0.0) == 1.0);
    CHECK(pbATan2(1.0, 1.0) > 0.785); // pi/4
    CHECK(pbATan2(1.0, 1.0) < 0.786);
}

TEST_CASE("pbRandomSeed makes pbRandom reproducible", "[runtime][mathlib]") {
    pbRandomSeed(42);
    std::int64_t first = pbRandom(1000);
    pbRandomSeed(42);
    std::int64_t second = pbRandom(1000);
    CHECK(first == second);
}

TEST_CASE("pbRandom respects an inclusive [min, max] range", "[runtime][mathlib]") {
    pbRandomSeed(1);
    for (int i = 0; i < 50; ++i) {
        std::int64_t v = pbRandom(10, 5);
        CHECK(v >= 5);
        CHECK(v <= 10);
    }
}
