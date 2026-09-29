#pragma once

#include <cstdint>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "pbstring.hpp"

namespace easybasic::runtime {

/// PureBasic's `NewMap`: a String-keyed hash map with an explicit single
/// cursor, mirroring `AddMapElement`/`DeleteMapElement`/`FindMapElement`/
/// `MapKey`/`ClearMap`/`MapSize`/`ResetMap`/`NextMapElement` one-to-one -
/// see docs/architecture/roadmap.md's M3f notes for the oracle-verified
/// semantics this implements. Backed by an insertion-ordered `std::list`
/// (so `ForEach`/`NextMapElement` iteration is deterministic) plus an
/// `unordered_map` index for O(1) key lookup - real PB's own Map iteration
/// order is hash-bucket-dependent and not something a well-formed PB
/// program should rely on anyway, so matching it exactly isn't a goal;
/// giving *our own* iteration a stable, deterministic order instead is
/// strictly more testable.
template <typename T>
class PBMap {
public:
    /// `name(key)` - direct access by key: if `key` already exists, returns
    /// its existing value and moves the cursor to it (oracle-verified: an
    /// existing value is preserved, e.g. reading a key set to 2 still reads
    /// 2); if absent, creates it with a zero value first (oracle-verified:
    /// reading or writing a never-seen key auto-creates it - `MapSize`
    /// increases). Either way, the cursor ends up on `key`.
    T& access(const PBString& key) {
        auto it = index_.find(key.bytes());
        if (it != index_.end()) {
            cursor_ = it->second;
            hasCursor_ = true;
            return cursor_->value;
        }
        items_.push_back(Entry{key, T{}});
        cursor_ = std::prev(items_.end());
        index_.emplace(key.bytes(), cursor_);
        hasCursor_ = true;
        return cursor_->value;
    }

    /// `AddMapElement(map(), key)` - oracle-verified to be subtly different
    /// from plain `access()`: it *always* resets the value to zero, even
    /// when `key` already exists (verified: adding an already-`= 1`-valued
    /// key back via `AddMapElement` and reading it afterward gives 0, not
    /// 1) - not a documentation guess, a genuinely surprising real-PB
    /// behavior this project caught by testing rather than assuming.
    void addMapElement(const PBString& key) {
        auto it = index_.find(key.bytes());
        if (it != index_.end()) {
            it->second->value = T{};
            cursor_ = it->second;
        } else {
            items_.push_back(Entry{key, T{}});
            cursor_ = std::prev(items_.end());
            index_.emplace(key.bytes(), cursor_);
        }
        hasCursor_ = true;
    }

    /// Moves the cursor to `key` and returns true if it exists; returns
    /// false, leaving the cursor untouched, otherwise (assumed symmetric
    /// with `PBList::firstElement`'s own oracle-verified "failed move never
    /// disturbs the cursor" rule - not independently re-verified for Map,
    /// but there is no reason to expect it differs).
    bool findMapElement(const PBString& key) {
        auto it = index_.find(key.bytes());
        if (it == index_.end()) {
            return false;
        }
        cursor_ = it->second;
        hasCursor_ = true;
        return true;
    }

    /// `DeleteMapElement(map())` - the 1-arg form: deletes the element at
    /// the cursor. A no-op if the cursor is invalid.
    void deleteCurrent() {
        if (!hasCursor_) {
            return;
        }
        index_.erase(cursor_->key.bytes());
        items_.erase(cursor_);
        hasCursor_ = false;
    }

    /// `DeleteMapElement(map(), key)` - the 2-arg form: deletes by key
    /// regardless of the current cursor; invalidates the cursor only if it
    /// happened to be sitting on the deleted key.
    void deleteKey(const PBString& key) {
        auto it = index_.find(key.bytes());
        if (it == index_.end()) {
            return;
        }
        bool wasCursor = hasCursor_ && it->second == cursor_;
        items_.erase(it->second);
        index_.erase(it);
        if (wasCursor) {
            hasCursor_ = false;
        }
    }

    void clear() {
        items_.clear();
        index_.clear();
        hasCursor_ = false;
    }

    std::int64_t size() const { return static_cast<std::int64_t>(items_.size()); }

    /// The current element's key. Throws if the cursor is invalid - the
    /// same memory-safety-over-UB choice as `PBList::current`.
    PBString mapKey() const {
        if (!hasCursor_) {
            throw std::out_of_range("PBMap: no current element");
        }
        return cursor_->key;
    }

    /// Invalidates the cursor without touching the map's contents - what
    /// `ForEach name() ... Next` lowers to, identically to `PBList`'s own
    /// `resetForEach`/`nextElement` pair.
    void resetForEach() { hasCursor_ = false; }

    bool nextElement() {
        if (!hasCursor_) {
            if (items_.empty()) {
                return false;
            }
            cursor_ = items_.begin();
            hasCursor_ = true;
            return true;
        }
        auto next = std::next(cursor_);
        if (next == items_.end()) {
            return false;
        }
        cursor_ = next;
        return true;
    }

    T& current() {
        if (!hasCursor_) {
            throw std::out_of_range("PBMap: no current element");
        }
        return cursor_->value;
    }
    const T& current() const {
        if (!hasCursor_) {
            throw std::out_of_range("PBMap: no current element");
        }
        return cursor_->value;
    }

private:
    struct Entry {
        PBString key;
        T value;
    };
    std::list<Entry> items_;
    std::unordered_map<std::string, typename std::list<Entry>::iterator> index_;
    typename std::list<Entry>::iterator cursor_{};
    bool hasCursor_ = false;
};

} // namespace easybasic::runtime
