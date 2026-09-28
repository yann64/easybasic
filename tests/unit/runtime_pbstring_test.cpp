#include <catch2/catch_test_macros.hpp>

#include <easybasic/runtime/pbstring.hpp>

using easybasic::runtime::PBString;

TEST_CASE("PBString concatenates", "[runtime][pbstring]") {
    PBString a("Hello, ");
    PBString b("World!");
    CHECK((a + b).bytes() == "Hello, World!");
}

TEST_CASE("PBString copies are cheap and compare by value", "[runtime][pbstring]") {
    PBString a("same");
    PBString b = a; // shares the backing buffer (ref-counted)
    CHECK(a == b);
    CHECK(a.bytes() == "same");
}

TEST_CASE("PBString default-constructs empty", "[runtime][pbstring]") {
    PBString empty;
    CHECK(empty.bytes().empty());
}
