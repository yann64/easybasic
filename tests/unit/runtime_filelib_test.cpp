#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>

#include <easybasic/runtime/filelib.hpp>

using namespace easybasic::runtime;

namespace {
/// A path unique enough per test case not to collide if tests ever run in
/// parallel, cleaned up at the end of each TEST_CASE regardless of outcome.
/// Built from `std::filesystem::temp_directory_path()` rather than a
/// hardcoded `/tmp/...` - a real portability bug caught by this project's
/// own first-ever Windows CI run (M6): `/tmp` doesn't exist on Windows, so
/// every `fopen` here silently failed and every filelib test failed with
/// it, with no connection to the actual runtime code being tested.
struct ScratchPath {
    std::string path;
    explicit ScratchPath(const char* name)
        : path((std::filesystem::temp_directory_path() / (std::string("easybasic_filelib_unittest_") + name))
                    .string()) {
        std::remove(path.c_str());
    }
    ~ScratchPath() { std::remove(path.c_str()); }
};
} // namespace

TEST_CASE("pbCreateFile/pbWriteStringN/pbReadFile/pbReadString/pbEof round-trip lines", "[runtime][filelib]") {
    ScratchPath p("roundtrip");
    REQUIRE(pbCreateFile(0, PBString(p.path)) == 1);
    pbWriteStringN(0, PBString("line one"));
    pbWriteStringN(0, PBString("line two"));
    pbCloseFile(0);

    REQUIRE(pbReadFile(0, PBString(p.path)) == 1);
    CHECK(pbEof(0) == 0);
    CHECK(pbReadString(0).bytes() == "line one");
    CHECK(pbReadString(0).bytes() == "line two");
    CHECK(pbEof(0) == 1);
    pbCloseFile(0);
}

TEST_CASE("pbFileSize is -1 for a nonexistent file, else the byte count", "[runtime][filelib]") {
    ScratchPath p("size");
    CHECK(pbFileSize(PBString(p.path)) == -1);
    pbCreateFile(0, PBString(p.path));
    pbWriteString(0, PBString("0123456789"));
    pbCloseFile(0);
    CHECK(pbFileSize(PBString(p.path)) == 10);
}

TEST_CASE("pbOpenFile positions at the start and allows overwriting in place", "[runtime][filelib]") {
    ScratchPath p("openfile");
    pbCreateFile(0, PBString(p.path));
    pbWriteString(0, PBString("0123456789"));
    pbCloseFile(0);

    REQUIRE(pbOpenFile(0, PBString(p.path)) == 1);
    CHECK(pbLoc(0) == 0);
    pbFileSeek(0, 3);
    pbWriteString(0, PBString("X"));
    pbCloseFile(0);

    pbReadFile(0, PBString(p.path));
    CHECK(pbReadString(0).bytes() == "012X456789");
    pbCloseFile(0);
}

TEST_CASE("pbFileSeek/pbLoc/pbLof work on a write-only CreateFile handle", "[runtime][filelib]") {
    // Oracle-verified: Lof/Loc work even on a handle that looks write-only -
    // the reason this project uses FILE* rather than std::fstream.
    ScratchPath p("seek");
    pbCreateFile(0, PBString(p.path));
    pbWriteString(0, PBString("0123456789"));
    CHECK(pbLoc(0) == 10);
    pbFileSeek(0, 3);
    CHECK(pbLoc(0) == 3);
    pbWriteString(0, PBString("X"));
    CHECK(pbLof(0) == 10);
    pbCloseFile(0);
}

TEST_CASE("pbDeleteFile/pbRenameFile report success via FileSize afterward", "[runtime][filelib]") {
    ScratchPath a("rename_a");
    ScratchPath b("rename_b");
    pbCreateFile(0, PBString(a.path));
    pbWriteString(0, PBString("hi"));
    pbCloseFile(0);

    CHECK(pbRenameFile(PBString(a.path), PBString(b.path)) == 1);
    CHECK(pbFileSize(PBString(a.path)) == -1);
    CHECK(pbFileSize(PBString(b.path)) == 2);

    CHECK(pbDeleteFile(PBString(b.path)) == 1);
    CHECK(pbFileSize(PBString(b.path)) == -1);
}

TEST_CASE("A second CreateFile on the same handle closes the first", "[runtime][filelib]") {
    ScratchPath a("reuse_a");
    ScratchPath b("reuse_b");
    pbCreateFile(0, PBString(a.path));
    pbWriteString(0, PBString("first"));
    // No pbCloseFile(0) here - re-using the handle must still close it
    // cleanly (verified by the file actually containing "first" afterward).
    pbCreateFile(0, PBString(b.path));
    pbWriteString(0, PBString("second"));
    pbCloseFile(0);

    CHECK(pbFileSize(PBString(a.path)) == 5);
    CHECK(pbFileSize(PBString(b.path)) == 6);
}
