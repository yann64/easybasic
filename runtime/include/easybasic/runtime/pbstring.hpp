#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace easybasic::runtime {

/// PureBasic's STRING type, backed by UTF-8 rather than PB's own internal
/// length-prefixed-UTF-16 layout (a deliberate choice, not an oversight -
/// see docs/architecture/roadmap.md's M0 notes for the full rationale: this
/// gets idiomatic, sanitizer-friendly C++ instead of reimplementing PB's
/// undocumented C-backend ABI for no user-visible benefit).
///
/// Ref-counted/copy-on-write: copying a PBString is O(1) (shares the same
/// backing buffer); no mutating operation exists yet in M0, so there is
/// currently no write path that needs to actually perform the "write" side
/// of copy-on-write - that lands with the String library (M4) mutating
/// operations.
class PBString {
public:
    PBString() : data_(std::make_shared<const std::string>()) {}
    PBString(const char* text) : data_(std::make_shared<const std::string>(text)) {}
    PBString(std::string text) : data_(std::make_shared<const std::string>(std::move(text))) {}
    PBString(std::string_view text) : data_(std::make_shared<const std::string>(text)) {}

    /// The raw UTF-8 bytes. Named `bytes()` rather than `str()`/`data()` to
    /// keep it obviously distinct from PB's own `Len()` (a UTF-16-code-unit
    /// count, not a byte count - see the String library's M4 notes) once
    /// that lands.
    const std::string& bytes() const { return *data_; }

    PBString operator+(const PBString& other) const {
        return PBString(*data_ + *other.data_);
    }

    bool operator==(const PBString& other) const { return *data_ == *other.data_; }

private:
    std::shared_ptr<const std::string> data_;
};

} // namespace easybasic::runtime
