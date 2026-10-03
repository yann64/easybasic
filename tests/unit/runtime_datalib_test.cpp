#include <catch2/catch_test_macros.hpp>

#include <easybasic/runtime/datalib.hpp>

using namespace easybasic::runtime;

// The data pool/cursor are process-global state (see pbDataPoolSize's own
// doc comment), shared across every TEST_CASE in this binary - each test
// here snapshots the pool's size before adding its own items, then
// pbDataRestore()s back to that snapshot, so it reads back exactly what it
// added regardless of what any other test case already put in the pool.

TEST_CASE("pbReadDataInt/Double/String round-trip a same-type Data item", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddInt(42);
    pbDataAddDouble(3.5);
    pbDataAddString(PBString("hello"));
    pbDataRestore(start);
    CHECK(pbReadDataInt() == 42);
    CHECK(pbReadDataDouble() == 3.5);
    CHECK(pbReadDataString().bytes() == "hello");
}

TEST_CASE("pbReadDataInt coerces a String item via Val()-style parsing", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddString(PBString("123"));
    pbDataAddString(PBString("not a number"));
    pbDataRestore(start);
    CHECK(pbReadDataInt() == 123);
    CHECK(pbReadDataInt() == 0);
}

TEST_CASE("pbReadDataString coerces a numeric item via Str()-style formatting", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddInt(99);
    pbDataRestore(start);
    CHECK(pbReadDataString().bytes() == "99");
}

TEST_CASE("pbReadDataDouble coerces an Integer item", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddInt(7);
    pbDataRestore(start);
    CHECK(pbReadDataDouble() == 7.0);
}

TEST_CASE("Each read advances the shared cursor by exactly one item", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddInt(1);
    pbDataAddInt(2);
    pbDataAddInt(3);
    pbDataRestore(start);
    CHECK(pbDataHasMore());
    CHECK(pbReadDataInt() == 1);
    CHECK(pbReadDataInt() == 2);
    CHECK(pbReadDataInt() == 3);
}

TEST_CASE("pbDataHasMore reflects whether the cursor has reached the pool's end", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddInt(1);
    pbDataRestore(start);
    CHECK(pbDataHasMore());
    pbReadDataInt();
    CHECK_FALSE(pbDataHasMore());
}

TEST_CASE("pbDataRestore jumps the shared cursor back to an arbitrary index", "[runtime][datalib]") {
    std::size_t start = pbDataPoolSize();
    pbDataAddInt(10);
    pbDataAddInt(20);
    pbDataRestore(start);
    CHECK(pbReadDataInt() == 10);
    CHECK(pbReadDataInt() == 20);
    pbDataRestore(start); // Jump back and re-read the same items.
    CHECK(pbReadDataInt() == 10);
}

TEST_CASE("Reading past the pool's end returns a harmless default rather than crashing", "[runtime][datalib]") {
    // Oracle-verified: this is a release-mode-only fallback - a debug (-d)
    // build instead calls pbDataReadError() (fatal, via Codegen's own
    // explicit pbDataHasMore() check emitted before the read - not
    // exercised here, since it calls std::exit()). Restores to the pool's
    // *current* end (not a size captured earlier) so this is correct
    // regardless of what other test cases have already added, or what
    // order tests run in.
    pbDataRestore(pbDataPoolSize());
    CHECK(pbReadDataInt() == 0);
    CHECK(pbReadDataDouble() == 0.0);
    CHECK(pbReadDataString().bytes().empty());
}
