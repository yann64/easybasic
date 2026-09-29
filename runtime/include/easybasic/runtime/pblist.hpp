#pragma once

#include <cstdint>
#include <list>
#include <stdexcept>

namespace easybasic::runtime {

/// PureBasic's `NewList`, backed by `std::list<T>` (PB itself calls this a
/// "LinkedList") with an explicit single cursor baked into the container
/// itself, exactly mirroring `AddElement`/`InsertElement`/`DeleteElement`/
/// `FirstElement`/`LastElement`/`NextElement`/`PreviousElement`/
/// `SelectElement`/`ListIndex`/`ClearList`/`ListSize` one-to-one - see
/// docs/architecture/roadmap.md's M3e notes for the oracle-verified cursor
/// semantics this implements (in particular: a failed move leaves the
/// cursor exactly where it was, and `AddElement`/`InsertElement` insert
/// after/before the cursor respectively).
///
/// Deliberately NOT a thin wrapper exposing raw STL iterators to generated
/// code: PB's single-cursor-per-list model is forgiving in ways a stricter
/// STL-iterator model isn't (in particular, no operation here can leave a
/// generated program holding a genuinely dangling iterator the way raw STL
/// iterator use could), so keeping the API PB-shaped avoids a whole class of
/// iterator-invalidation bugs in generated code that ASan would otherwise
/// need to catch after the fact instead.
template <typename T>
class PBList {
public:
    /// Inserts a new (zero-valued) element immediately after the cursor and
    /// moves the cursor to it; on an empty list, the new element becomes the
    /// list's only element (oracle-verified).
    void addElement() {
        auto pos = hasCursor_ ? std::next(cursor_) : items_.end();
        cursor_ = items_.insert(pos, T{});
        hasCursor_ = true;
    }

    /// Inserts a new (zero-valued) element immediately before the cursor and
    /// moves the cursor to it; on an empty list, behaves like `addElement`
    /// (oracle-verified).
    void insertElement() {
        auto pos = hasCursor_ ? cursor_ : items_.end();
        cursor_ = items_.insert(pos, T{});
        hasCursor_ = true;
    }

    /// Removes the element at the cursor. The cursor then moves to the
    /// *previous* element if one exists, else the *next* one (oracle-
    /// verified: deleting the first element of a longer list moves the
    /// cursor to the new first element); if neither exists (the list is now
    /// empty), the cursor becomes invalid. Real PB's debugger raises "The
    /// LinkedList has no current element" if this ever empties the list and
    /// a later operation is attempted without re-establishing the cursor -
    /// not reproduced here (that's a debug-build-only diagnostic, not a
    /// language semantic); this simply leaves `hasCursor_` false, a
    /// documented, deliberate simplification. A no-op if the cursor is
    /// already invalid.
    void deleteElement() {
        if (!hasCursor_) {
            return;
        }
        auto current = cursor_;
        bool hadPrevious = current != items_.begin();
        auto previous = hadPrevious ? std::prev(current) : items_.end();
        auto next = std::next(current);
        items_.erase(current);
        if (hadPrevious) {
            cursor_ = previous;
            hasCursor_ = true;
        } else if (next != items_.end()) {
            cursor_ = next;
            hasCursor_ = true;
        } else {
            hasCursor_ = false;
        }
    }

    void clear() {
        items_.clear();
        hasCursor_ = false;
    }

    /// Moves the cursor to the first element and returns true, or returns
    /// false (leaving the cursor untouched) if the list is empty (oracle-
    /// verified: a failed move never disturbs the current cursor).
    bool firstElement() {
        if (items_.empty()) {
            return false;
        }
        cursor_ = items_.begin();
        hasCursor_ = true;
        return true;
    }

    bool lastElement() {
        if (items_.empty()) {
            return false;
        }
        cursor_ = std::prev(items_.end());
        hasCursor_ = true;
        return true;
    }

    /// Advances the cursor by one and returns true; returns false (cursor
    /// unchanged) if already at the last element. From an invalid cursor
    /// (an empty list, or `ForEach`'s own reset - see `resetForEach`), moves
    /// to the first element instead, exactly matching the
    /// `PB_ResetList`+`while(PB_NextElement(...))` pattern real PB's own
    /// generated code uses for `ForEach`.
    bool nextElement() {
        if (!hasCursor_) {
            return firstElement();
        }
        auto next = std::next(cursor_);
        if (next == items_.end()) {
            return false;
        }
        cursor_ = next;
        return true;
    }

    /// Symmetric with `nextElement`. The "from invalid, move to the last
    /// element" case is not independently oracle-verified (no real PB
    /// program was found exercising `PreviousElement` on a freshly reset
    /// cursor) but is a reasonable, documented assumption mirroring
    /// `nextElement`'s own verified behavior.
    bool previousElement() {
        if (!hasCursor_) {
            return lastElement();
        }
        if (cursor_ == items_.begin()) {
            return false;
        }
        cursor_ = std::prev(cursor_);
        return true;
    }

    /// Moves the cursor to the 0-based `index`, returning true on success;
    /// returns false (cursor unchanged) if out of range.
    bool selectElement(std::int64_t index) {
        if (index < 0 || static_cast<std::size_t>(index) >= items_.size()) {
            return false;
        }
        cursor_ = std::next(items_.begin(), static_cast<std::ptrdiff_t>(index));
        hasCursor_ = true;
        return true;
    }

    /// The cursor's current 0-based index, or -1 if invalid.
    std::int64_t listIndex() const {
        if (!hasCursor_) {
            return -1;
        }
        // `cursor_` is a non-const iterator even here (there is no separate
        // const/non-const cursor) - std::distance needs both arguments to
        // be the exact same iterator type, so `items_.cbegin()` (a
        // const_iterator, since `items_` is const in a const method) won't
        // deduce against it directly; converting explicitly (a standard
        // iterator -> const_iterator conversion) resolves that.
        typename std::list<T>::const_iterator constCursor = cursor_;
        return static_cast<std::int64_t>(std::distance(items_.cbegin(), constCursor));
    }

    std::int64_t size() const { return static_cast<std::int64_t>(items_.size()); }

    /// Invalidates the cursor without touching the list's contents, so the
    /// very next `nextElement()` call moves to the first element - exactly
    /// what `ForEach name() ... Next` lowers to
    /// (`v_name.resetForEach(); while (v_name.nextElement()) { ... }`).
    void resetForEach() { hasCursor_ = false; }

    /// The current element, read or written via `name()` in generated code.
    /// Throws (rather than invoking undefined behavior) if the cursor is
    /// invalid - the same memory-safety-over-raw-indexing choice this
    /// project makes for `PBArray`'s own `.at()`-based element access.
    T& current() {
        if (!hasCursor_) {
            throw std::out_of_range("PBList: no current element");
        }
        return *cursor_;
    }
    const T& current() const {
        if (!hasCursor_) {
            throw std::out_of_range("PBList: no current element");
        }
        return *cursor_;
    }

private:
    std::list<T> items_;
    typename std::list<T>::iterator cursor_{};
    bool hasCursor_ = false;
};

} // namespace easybasic::runtime
