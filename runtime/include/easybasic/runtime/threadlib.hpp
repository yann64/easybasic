#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>
#include <unordered_map>

namespace easybasic::runtime {

/// `Delay`/`ElapsedMilliseconds` aren't thread-specific commands in real PB
/// - they're general timing primitives - but are bundled into this same
/// library since M7a's own oracle verification needed them immediately
/// (any real test of thread timing/synchronization needs a way to sleep
/// and measure elapsed time) and a whole separate "syslib" for just two
/// functions isn't warranted yet.
inline std::int64_t pbDelay(std::int64_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return 0; // Matches the project's existing convention of every builtin returning an Integer, even when real PB's own return value is unused/void.
}

inline std::int64_t pbElapsedMilliseconds() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

/// A PB thread entry procedure's own C++ signature - every `Procedure` ever
/// passed to `CreateThread` via `@Name()` takes exactly one Integer
/// parameter and returns Integer (oracle-verified: `Procedure Worker(n)`,
/// the real `Thread.pb` example's own shape), matching this project's
/// existing codegen for an Integer-parameter, Integer-returning Procedure
/// exactly - no special-casing needed on the Codegen side beyond emitting
/// `@Worker()` as a plain function-pointer value (see `Codegen::genExpr`'s
/// own `AddressOf` case).
using PbThreadEntry = std::int64_t (*)(std::int64_t);

namespace detail {
struct ThreadHandle {
    ThreadHandle() = default; // Declaring the deleted special members below suppresses this implicitly otherwise.
    std::thread thread;
    std::atomic<bool> finished{false};
    bool joined = false;
    // A `std::thread` still joinable at destruction calls std::terminate()
    // - but real PB code routinely never joins a thread at all (the actual
    // `Thread.pb` example starts one with an infinite `Repeat/ForEver`
    // loop and simply lets the whole process exit once a blocking
    // `MessageRequester` returns, with no explicit cleanup). Detaching
    // instead matches that real behavior: the OS tears the thread down
    // along with the rest of the process at exit, rather than crashing
    // pbcxx's own generated program first.
    ~ThreadHandle() {
        if (thread.joinable()) {
            thread.detach();
        }
    }
    // Always managed through a shared_ptr (see pbCreateThread) - never
    // copied or moved directly, so these are just deleted explicitly
    // rather than left implicit (std::thread's own non-copyability would
    // delete the copy operations implicitly anyway, but not the move ones).
    ThreadHandle(const ThreadHandle&) = delete;
    ThreadHandle& operator=(const ThreadHandle&) = delete;
    ThreadHandle(ThreadHandle&&) = delete;
    ThreadHandle& operator=(ThreadHandle&&) = delete;
};

inline std::mutex& threadTableMutex() {
    static std::mutex m;
    return m;
}
inline std::unordered_map<std::int64_t, std::shared_ptr<ThreadHandle>>& threadTable() {
    static std::unordered_map<std::int64_t, std::shared_ptr<ThreadHandle>> table;
    return table;
}
inline std::int64_t nextThreadId() {
    static std::int64_t id = 0;
    return ++id;
}

inline std::mutex& mutexTableMutex() {
    static std::mutex m;
    return m;
}
// Oracle-verified: a second `TryLockMutex` from the same thread that
// already holds the lock succeeds too, rather than deadlocking or failing
// - PB's Mutex is re-entrant, matching `std::recursive_mutex`
// (`std::mutex` itself is explicitly undefined behavior to re-lock from
// the owning thread).
inline std::unordered_map<std::int64_t, std::unique_ptr<std::recursive_mutex>>& mutexTable() {
    static std::unordered_map<std::int64_t, std::unique_ptr<std::recursive_mutex>> table;
    return table;
}
inline std::int64_t nextMutexId() {
    static std::int64_t id = 0;
    return ++id;
}

inline std::mutex& semaphoreTableMutex() {
    static std::mutex m;
    return m;
}
// `std::counting_semaphore<>`'s default max count (`LEAST_MAX_VALUE`, at
// least 2^31-1 on every real implementation) comfortably covers any
// realistic PB initial-count argument.
inline std::unordered_map<std::int64_t, std::unique_ptr<std::counting_semaphore<>>>& semaphoreTable() {
    static std::unordered_map<std::int64_t, std::unique_ptr<std::counting_semaphore<>>> table;
    return table;
}
inline std::int64_t nextSemaphoreId() {
    static std::int64_t id = 0;
    return ++id;
}
} // namespace detail

/// `CreateThread(@Procedure(), Parameter)` - oracle-verified: the launched
/// procedure's own `ProcedureReturn` value is never observable afterward
/// (confirmed directly: `WaitThread`'s own return is a plain success flag,
/// not the procedure's result - PB threads communicate back via shared
/// globals/Mutex/Semaphore, not a return channel), so the entry function's
/// return value is simply discarded here too.
inline std::int64_t pbCreateThread(std::int64_t funcAddr, std::int64_t param) {
    auto handle = std::make_shared<detail::ThreadHandle>();
    auto entry = reinterpret_cast<PbThreadEntry>(funcAddr);
    // Constructs the real std::thread (and so starts it) *before* this
    // handle is published into the table - otherwise a WaitThread racing
    // in right after CreateThread returns could see a still-default-
    // constructed (non-joinable) thread object and skip joining entirely.
    // Captures `handle` by value (a shared_ptr) so the ThreadHandle stays
    // alive for the thread's own lifetime regardless of what happens to
    // the table entry.
    handle->thread = std::thread([entry, param, handle]() {
        entry(param);
        handle->finished.store(true);
    });
    std::int64_t id = 0;
    {
        std::scoped_lock<std::mutex> lock(detail::threadTableMutex());
        id = detail::nextThreadId();
        detail::threadTable()[id] = handle;
    }
    return id;
}

/// Oracle-verified: `1` while the thread is still running, `0` once it has
/// finished (checked after a blocking `WaitThread`) or for an unknown
/// handle - a non-blocking query, unlike `WaitThread`.
inline std::int64_t pbIsThread(std::int64_t threadId) {
    std::shared_ptr<detail::ThreadHandle> handle;
    {
        std::scoped_lock<std::mutex> lock(detail::threadTableMutex());
        auto it = detail::threadTable().find(threadId);
        if (it == detail::threadTable().end()) {
            return 0;
        }
        handle = it->second;
    }
    return handle->finished.load() ? 0 : 1;
}

/// Oracle-verified: blocks until the thread finishes, then returns `1`
/// (a plain success flag - oracle-verified it is *not* the thread
/// procedure's own `ProcedureReturn` value, which is unobservable from
/// here, see `pbCreateThread`'s own notes). Safe to call more than once
/// (guarded against a double `std::thread::join`, which is itself
/// undefined behavior) - not separately oracle-verified, but a reasonable
/// safety margin since nothing about repeated `WaitThread` calls was ruled
/// out.
inline std::int64_t pbWaitThread(std::int64_t threadId) {
    std::shared_ptr<detail::ThreadHandle> handle;
    {
        std::scoped_lock<std::mutex> lock(detail::threadTableMutex());
        auto it = detail::threadTable().find(threadId);
        if (it == detail::threadTable().end()) {
            return 0;
        }
        handle = it->second;
    }
    if (!handle->joined && handle->thread.joinable()) {
        handle->thread.join();
        handle->joined = true;
    }
    return 1;
}

inline std::int64_t pbCreateMutex() {
    auto mutex = std::make_unique<std::recursive_mutex>();
    std::scoped_lock<std::mutex> lock(detail::mutexTableMutex());
    std::int64_t id = detail::nextMutexId();
    detail::mutexTable()[id] = std::move(mutex);
    return id;
}

inline std::recursive_mutex* pbFindMutex(std::int64_t mutexId) {
    std::scoped_lock<std::mutex> lock(detail::mutexTableMutex());
    auto it = detail::mutexTable().find(mutexId);
    return it == detail::mutexTable().end() ? nullptr : it->second.get();
}

inline std::int64_t pbLockMutex(std::int64_t mutexId) {
    std::recursive_mutex* mutex = pbFindMutex(mutexId);
    if (mutex == nullptr) {
        return 0;
    }
    mutex->lock();
    return 1;
}

inline std::int64_t pbUnlockMutex(std::int64_t mutexId) {
    std::recursive_mutex* mutex = pbFindMutex(mutexId);
    if (mutex == nullptr) {
        return 0;
    }
    mutex->unlock();
    return 1;
}

inline std::int64_t pbTryLockMutex(std::int64_t mutexId) {
    std::recursive_mutex* mutex = pbFindMutex(mutexId);
    if (mutex == nullptr) {
        return 0;
    }
    return mutex->try_lock() ? 1 : 0;
}

inline std::int64_t pbFreeMutex(std::int64_t mutexId) {
    std::scoped_lock<std::mutex> lock(detail::mutexTableMutex());
    return detail::mutexTable().erase(mutexId) > 0 ? 1 : 0;
}

/// Oracle-verified: `CreateSemaphore()` (no argument) defaults the initial
/// count to 0; `CreateSemaphore(3)` starts with 3 already available.
inline std::int64_t pbCreateSemaphore(std::int64_t initialCount = 0) {
    auto semaphore = std::make_unique<std::counting_semaphore<>>(initialCount);
    std::scoped_lock<std::mutex> lock(detail::semaphoreTableMutex());
    std::int64_t id = detail::nextSemaphoreId();
    detail::semaphoreTable()[id] = std::move(semaphore);
    return id;
}

inline std::counting_semaphore<>* pbFindSemaphore(std::int64_t semaphoreId) {
    std::scoped_lock<std::mutex> lock(detail::semaphoreTableMutex());
    auto it = detail::semaphoreTable().find(semaphoreId);
    return it == detail::semaphoreTable().end() ? nullptr : it->second.get();
}

inline std::int64_t pbSignalSemaphore(std::int64_t semaphoreId) {
    std::counting_semaphore<>* semaphore = pbFindSemaphore(semaphoreId);
    if (semaphore == nullptr) {
        return 0;
    }
    semaphore->release();
    return 1;
}

inline std::int64_t pbWaitSemaphore(std::int64_t semaphoreId) {
    std::counting_semaphore<>* semaphore = pbFindSemaphore(semaphoreId);
    if (semaphore == nullptr) {
        return 0;
    }
    semaphore->acquire();
    return 1;
}

inline std::int64_t pbTrySemaphore(std::int64_t semaphoreId) {
    std::counting_semaphore<>* semaphore = pbFindSemaphore(semaphoreId);
    if (semaphore == nullptr) {
        return 0;
    }
    return semaphore->try_acquire() ? 1 : 0;
}

inline std::int64_t pbFreeSemaphore(std::int64_t semaphoreId) {
    std::scoped_lock<std::mutex> lock(detail::semaphoreTableMutex());
    return detail::semaphoreTable().erase(semaphoreId) > 0 ? 1 : 0;
}

} // namespace easybasic::runtime
