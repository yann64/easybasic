#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>

#include "pbstring.hpp"

namespace easybasic::runtime {

/// Every file operation is keyed by a plain Integer "file number" the PB
/// program itself picks (oracle-verified: it can be a literal, a variable,
/// or any Integer expression - not a handle PB hands back), exactly
/// mirroring classic BASIC's own numbered-file model. `std::FILE*` (not
/// `std::fstream`) is used specifically because `ftell`/`fseek` behave
/// uniformly across read-only/write-only/read-write modes on every
/// platform this project targets, unlike `std::iostream`'s own get/put
/// positioning, which the standard only fully guarantees for a stream
/// opened with `ios::in` - and `Lof`/`Loc` are oracle-verified to work even
/// on a `CreateFile` (write-only-looking) handle.
namespace detail {

struct FileCloser {
    void operator()(std::FILE* f) const {
        if (f != nullptr) {
            std::fclose(f);
        }
    }
};
using FileHandle = std::unique_ptr<std::FILE, FileCloser>;

/// A process-wide table, so every open handle is still reachable (and, on
/// program exit, still automatically closed via `FileHandle`'s own
/// destructor - the memory-safety-relevant reason this isn't just a raw
/// `FILE*` map) regardless of which generated function touches it.
inline std::unordered_map<std::int64_t, FileHandle>& fileTable() {
    static std::unordered_map<std::int64_t, FileHandle> table;
    return table;
}

inline std::int64_t openAs(std::int64_t handle, const PBString& path, const char* mode) {
    auto& table = fileTable();
    table.erase(handle); // re-using a handle silently closes whatever it pointed to before.
    std::FILE* f = std::fopen(path.bytes().c_str(), mode);
    if (f == nullptr) {
        return 0;
    }
    table[handle] = FileHandle(f);
    return 1;
}

} // namespace detail

/// `wb+` (not plain `wb`): oracle-verified that `Lof`/`Loc` work on a
/// `CreateFile` handle, which needs read-positioning (`ftell`/`fseek`) to
/// work even though the handle is conceptually "for writing" - still
/// truncates/creates exactly like `wb` would.
inline std::int64_t pbCreateFile(std::int64_t handle, const PBString& path) {
    return detail::openAs(handle, path, "wb+");
}

/// `rb+`: read+write, no truncation, positioned at the start - oracle-
/// verified: `OpenFile` requires the file to already exist and lets a
/// subsequent `WriteString` overwrite bytes in place rather than append.
inline std::int64_t pbOpenFile(std::int64_t handle, const PBString& path) {
    return detail::openAs(handle, path, "rb+");
}

inline std::int64_t pbReadFile(std::int64_t handle, const PBString& path) {
    return detail::openAs(handle, path, "rb");
}

inline std::int64_t pbCloseFile(std::int64_t handle) {
    detail::fileTable().erase(handle);
    return 0;
}

inline std::int64_t pbWriteString(std::int64_t handle, const PBString& value) {
    auto it = detail::fileTable().find(handle);
    if (it == detail::fileTable().end()) {
        return 0;
    }
    const std::string& bytes = value.bytes();
    std::fwrite(bytes.data(), 1, bytes.size(), it->second.get());
    return 1;
}

/// Oracle-verified: the line terminator is a plain `\n`, not `\r\n` -
/// confirmed by measuring `FileSize` after writing two lines, not assumed
/// from the host platform's own text-mode convention (every handle here is
/// opened in binary mode specifically so no C library text-mode newline
/// translation could silently change this on a non-Linux target).
inline std::int64_t pbWriteStringN(std::int64_t handle, const PBString& value) {
    if (pbWriteString(handle, value) == 0) {
        return 0;
    }
    std::fputc('\n', detail::fileTable().at(handle).get());
    return 1;
}

/// Reads one line, stopping at `\n` or EOF; a trailing `\r` just before the
/// `\n` is stripped too (not independently oracle-verified - a reasonable,
/// documented universal-newline accommodation for a file that happened to
/// be written by something CRLF-based, since this project's own
/// `WriteStringN` never produces one to round-trip against).
inline PBString pbReadString(std::int64_t handle) {
    auto it = detail::fileTable().find(handle);
    if (it == detail::fileTable().end()) {
        return PBString();
    }
    std::string line;
    int c = 0;
    while ((c = std::fgetc(it->second.get())) != EOF && c != '\n') {
        line += static_cast<char>(c);
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return PBString(std::move(line));
}

inline std::int64_t pbEof(std::int64_t handle) {
    auto it = detail::fileTable().find(handle);
    if (it == detail::fileTable().end()) {
        return 1;
    }
    std::FILE* f = it->second.get();
    int c = std::fgetc(f);
    if (c == EOF) {
        return 1;
    }
    std::ungetc(c, f);
    return 0;
}

inline std::int64_t pbFileSeek(std::int64_t handle, std::int64_t position) {
    auto it = detail::fileTable().find(handle);
    if (it == detail::fileTable().end()) {
        return 0;
    }
    std::fseek(it->second.get(), static_cast<long>(position), SEEK_SET);
    return 0;
}

inline std::int64_t pbLoc(std::int64_t handle) {
    auto it = detail::fileTable().find(handle);
    if (it == detail::fileTable().end()) {
        return 0;
    }
    return static_cast<std::int64_t>(std::ftell(it->second.get()));
}

inline std::int64_t pbLof(std::int64_t handle) {
    auto it = detail::fileTable().find(handle);
    if (it == detail::fileTable().end()) {
        return 0;
    }
    std::FILE* f = it->second.get();
    long saved = std::ftell(f);
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, saved, SEEK_SET);
    return static_cast<std::int64_t>(size);
}

/// Oracle-verified: `-1` for a file that doesn't exist, not an error/crash.
inline std::int64_t pbFileSize(const PBString& path) {
    std::FILE* f = std::fopen(path.bytes().c_str(), "rb");
    if (f == nullptr) {
        return -1;
    }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fclose(f);
    return static_cast<std::int64_t>(size);
}

inline std::int64_t pbDeleteFile(const PBString& path) { return std::remove(path.bytes().c_str()) == 0 ? 1 : 0; }

inline std::int64_t pbRenameFile(const PBString& oldPath, const PBString& newPath) {
    return std::rename(oldPath.bytes().c_str(), newPath.bytes().c_str()) == 0 ? 1 : 0;
}

} // namespace easybasic::runtime
