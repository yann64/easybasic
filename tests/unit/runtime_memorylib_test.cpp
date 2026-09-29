#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include <easybasic/runtime/memorylib.hpp>

using namespace easybasic::runtime;

namespace {
/// RAII wrapper so a failed CHECK doesn't leak the scratch buffer.
struct ScratchBuffer {
    void* ptr = std::malloc(64);
    std::int64_t address = reinterpret_cast<std::int64_t>(ptr);
    ~ScratchBuffer() { std::free(ptr); }
};
} // namespace

TEST_CASE("pbPeek*/pbPoke* round-trip every primitive type", "[runtime][memorylib]") {
    ScratchBuffer buf;
    pbPokeB(buf.address, -5);
    CHECK(pbPeekB(buf.address) == -5);
    pbPokeA(buf.address, 200);
    CHECK(pbPeekA(buf.address) == 200);
    pbPokeW(buf.address, -1000);
    CHECK(pbPeekW(buf.address) == -1000);
    pbPokeU(buf.address, 5000);
    CHECK(pbPeekU(buf.address) == 5000);
    pbPokeL(buf.address, -100000);
    CHECK(pbPeekL(buf.address) == -100000);
    pbPokeQ(buf.address, 123456789012LL);
    CHECK(pbPeekQ(buf.address) == 123456789012LL);
    pbPokeF(buf.address, 3.5F);
    CHECK(pbPeekF(buf.address) == 3.5F);
    pbPokeD(buf.address, 2.71828);
    CHECK(pbPeekD(buf.address) == 2.71828);
}

TEST_CASE("pbPeek*/pbPoke* work at a deliberately misaligned address", "[runtime][memorylib]") {
    // Regression: an earlier implementation dereferenced a raw
    // reinterpret_cast'd pointer directly, undefined behavior a targeted
    // UBSan run caught on exactly this pattern (misaligned load/store)
    // even though it happened to produce the right numeric answer on x86 -
    // fixed with a memcpy'd unaligned load/store instead.
    ScratchBuffer buf;
    pbPokeL(buf.address + 1, 123456);
    CHECK(pbPeekL(buf.address + 1) == 123456);
    pbPokeD(buf.address + 3, 2.5);
    CHECK(pbPeekD(buf.address + 3) == 2.5);
}

TEST_CASE("pbPokeS/pbPeekS round-trip a String as UTF-16LE bytes", "[runtime][memorylib]") {
    // Oracle-verified: real PB compiles in Unicode mode by default, so
    // PokeS writes 2 bytes per character (e.g. "Hi" -> [72,0,105,0,0,0]),
    // not PBString's own UTF-8 internal layout.
    ScratchBuffer buf;
    pbPokeS(buf.address, PBString("Hi"));
    const auto* bytes = static_cast<const unsigned char*>(buf.ptr);
    CHECK(bytes[0] == 'H');
    CHECK(bytes[1] == 0);
    CHECK(bytes[2] == 'i');
    CHECK(bytes[3] == 0);
    CHECK(bytes[4] == 0); // null terminator, low byte
    CHECK(bytes[5] == 0); // null terminator, high byte
    CHECK(pbPeekS(buf.address).bytes() == "Hi");
}

TEST_CASE("pbPeekS with an explicit length stops before the null terminator", "[runtime][memorylib]") {
    ScratchBuffer buf;
    pbPokeS(buf.address, PBString("Hello World"));
    CHECK(pbPeekS(buf.address).bytes() == "Hello World");
    CHECK(pbPeekS(buf.address, 5).bytes() == "Hello");
}
