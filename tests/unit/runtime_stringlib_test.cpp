#include <catch2/catch_test_macros.hpp>

#include <easybasic/runtime/stringlib.hpp>

using namespace easybasic::runtime;

TEST_CASE("pbLen counts UTF-16 code units, not bytes", "[runtime][stringlib]") {
    CHECK(pbLen(PBString("Hello")) == 5);
    CHECK(pbLen(PBString("")) == 0);
    // "caf" + Chr(233) ('e' with acute accent) - one 2-byte UTF-8 sequence,
    // one UTF-16 code unit (oracle-verified: matches real PB's own Len()
    // for a string built via Chr(), as opposed to a raw non-ASCII byte
    // embedded directly in a source file, which real PB's own compiler
    // does NOT UTF-8-decode - see docs/architecture/roadmap.md's M4a notes).
    PBString cafe = PBString("caf") + pbChr(233);
    CHECK(pbLen(cafe) == 4);
}

TEST_CASE("pbLeft/pbRight clamp gracefully past the string's length", "[runtime][stringlib]") {
    PBString s("Hi");
    CHECK(pbLeft(s, 100).bytes() == "Hi");
    CHECK(pbRight(s, 100).bytes() == "Hi");
}

TEST_CASE("pbLeft/pbRight/pbMid with a zero count return an empty string", "[runtime][stringlib]") {
    // Regression: an earlier implementation of pbUtf16Slice only detected
    // "already past the end" after consuming one whole extra code point,
    // so Left("Hi", 0) came out "H" instead of "" - caught by testing
    // against the oracle before it shipped.
    PBString s("Hi");
    CHECK(pbLeft(s, 0).bytes().empty());
    CHECK(pbRight(s, 0).bytes().empty());
    CHECK(pbMid(s, 1, 0).bytes().empty());
}

TEST_CASE("pbMid is 1-based and clamps past the end", "[runtime][stringlib]") {
    PBString s("Hello, World!");
    CHECK(pbMid(s, 8, 5).bytes() == "World");
    CHECK(pbMid(s, 8).bytes() == "World!"); // omitted count defaults to "the rest"
    CHECK(pbMid(s, 100, 5).bytes().empty());
}

TEST_CASE("pbUCase/pbLCase convert ASCII only", "[runtime][stringlib]") {
    CHECK(pbUCase(PBString("Hello!")).bytes() == "HELLO!");
    CHECK(pbLCase(PBString("Hello!")).bytes() == "hello!");
}

TEST_CASE("pbTrim/pbLTrim/pbRTrim strip only leading/trailing ASCII spaces", "[runtime][stringlib]") {
    PBString s("  hi there  ");
    CHECK(pbTrim(s).bytes() == "hi there");
    CHECK(pbLTrim(s).bytes() == "hi there  ");
    CHECK(pbRTrim(s).bytes() == "  hi there");
}

TEST_CASE("pbStr formats a plain Integer", "[runtime][stringlib]") {
    CHECK(pbStr(42).bytes() == "42");
    CHECK(pbStr(-17).bytes() == "-17");
}

TEST_CASE("pbVal parses like strtoll, truncating at the first invalid character", "[runtime][stringlib]") {
    CHECK(pbVal(PBString("123")) == 123);
    CHECK(pbVal(PBString("  45abc")) == 45);
    CHECK(pbVal(PBString("notanumber")) == 0);
    CHECK(pbVal(PBString("-42")) == -42);
    CHECK(pbVal(PBString("3.14")) == 3); // oracle-verified: Val truncates at the decimal point
    CHECK(pbVal(PBString("")) == 0);
}

TEST_CASE("pbStrF formats with the given decimal count, defaulting to 10", "[runtime][stringlib]") {
    CHECK(pbStrF(3.14159F, 2).bytes() == "3.14");
    CHECK(pbStrF(-2.5F, 3).bytes() == "-2.500");
    CHECK(pbStrF(3.14159F, 0).bytes() == "3");
}

TEST_CASE("pbChr/pbAsc round-trip a code point", "[runtime][stringlib]") {
    CHECK(pbChr(65).bytes() == "A");
    CHECK(pbAsc(PBString("A")) == 65);
    CHECK(pbAsc(PBString("")) == 0); // oracle-verified
    // A codepoint above ASCII round-trips through UTF-8 correctly.
    PBString e = pbChr(233);
    CHECK(pbAsc(e) == 233);
}

TEST_CASE("pbUtf16Length/pbUtf16Slice handle an astral code point as two UTF-16 code units",
          "[runtime][stringlib]") {
    // Real PB's own Chr() actually rejects code points outside the BMP
    // ("Invalid value for Chr(), should be between 0 and $D7FF or between
    // $E000 and $FFFF", oracle-verified) - meaning a genuine surrogate pair
    // is essentially unreachable through legitimate PB code, so this is
    // this project's own generalization for internal consistency, not
    // independently oracle-verified for an actual PB program.
    std::string emoji = pbChr(128512).bytes(); // U+1F600, a 4-byte UTF-8 sequence
    CHECK(pbUtf16Length(emoji) == 2);
}
