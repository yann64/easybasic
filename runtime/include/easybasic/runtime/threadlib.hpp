#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>
#include <unordered_map>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace easybasic::runtime {

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
    /// M7a's deferred PauseThread/ResumeThread (second slice): cooperative,
    /// not an OS-level suspend - see pbPauseThread's own doc comment for
    /// why. Guarded by `pauseMutex`; `pauseCv` is what a paused thread's own
    /// `pbDelay` call blocks on.
    std::mutex pauseMutex;
    std::condition_variable pauseCv;
    bool pauseRequested = false;
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

/// Shared by every thread-handle lookup (`IsThread`/`WaitThread`/
/// `KillThread`/`PauseThread`/`ResumeThread`/`ThreadID`) - a null result
/// means `threadId` names no known thread.
inline std::shared_ptr<ThreadHandle> findThread(std::int64_t threadId) {
    std::scoped_lock<std::mutex> lock(threadTableMutex());
    auto it = threadTable().find(threadId);
    return it == threadTable().end() ? nullptr : it->second;
}

/// Set once, at the very start of a `CreateThread`-launched thread's own
/// body (see `pbCreateThread`) - lets `pbDelay`, called *from inside* that
/// same thread, find its own `ThreadHandle` to check for a pending
/// `PauseThread` request. Empty/null on the main thread (which was never
/// `CreateThread`'d, so can never legally be a `PauseThread` target anyway -
/// oracle-verified `Thread` is always a value `CreateThread` returned).
inline std::shared_ptr<ThreadHandle>& currentThreadHandle() {
    thread_local std::shared_ptr<ThreadHandle> handle;
    return handle;
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

/// `Delay`/`ElapsedMilliseconds` aren't thread-specific commands in real PB
/// - they're general timing primitives - but are bundled into this same
/// library since M7a's own oracle verification needed them immediately
/// (any real test of thread timing/synchronization needs a way to sleep
/// and measure elapsed time) and a whole separate "syslib" for just two
/// functions isn't warranted yet.
///
/// M7a's own deferred `PauseThread`/`ResumeThread` (second slice) hook in
/// here too: oracle-verified real PB's own worker-thread examples for both
/// always call `Delay` repeatedly in their own loop (never a single long
/// sleep or a pure compute loop), so `Delay` is the natural, already-
/// idiomatic cooperative yield point - checked only when called from
/// *inside* a `CreateThread`-launched thread (`detail::currentThreadHandle()`
/// is null on the main thread, which can never legally be a `PauseThread`
/// target anyway). See `pbPauseThread`'s own doc comment for why this
/// cooperative design was chosen over an OS-level forced suspend.
inline std::int64_t pbDelay(std::int64_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    auto& handle = detail::currentThreadHandle();
    if (handle) {
        std::unique_lock<std::mutex> lock(handle->pauseMutex);
        handle->pauseCv.wait(lock, [&handle] { return !handle->pauseRequested; });
    }
    return 0; // Matches the project's existing convention of every builtin returning an Integer, even when real PB's own return value is unused/void.
}

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
        // Published *before* the entry procedure runs, so even its very
        // first Delay() call can already see its own PauseThread state.
        detail::currentThreadHandle() = handle;
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
    auto handle = detail::findThread(threadId);
    if (!handle) {
        return 0;
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
    auto handle = detail::findThread(threadId);
    if (!handle) {
        return 0;
    }
    if (!handle->joined && handle->thread.joinable()) {
        handle->thread.join();
        handle->joined = true;
    }
    return 1;
}

/// Oracle-verified (carefully, in an isolated/sandboxed subprocess - see
/// the roadmap's own M7a notes on why that caution mattered here):
/// `KillThread` on a *genuinely still-running* thread, built with `-t`
/// (ThreadSafe mode), terminates it immediately - its own counter variable
/// observably stopped advancing, `IsThread` reported `0` right after, and
/// the whole process stayed alive and exited cleanly. On an *already-
/// finished* thread it is instead a fatal debugger error ("The specified
/// Thread does not exists.") that halts the whole program; pbcxx
/// deliberately diverges there by just doing nothing rather than
/// replicating a debugger-only abort path. Real PB's own docs list its
/// return value as "Aucune" (none) - oracle-verified directly: `Debug
/// KillThread(...)` always prints `0`, success or not - so this always
/// returns `0` too, rather than inventing its own success/failure signal.
///
/// Implementation: real PB's own forced termination has no safe, portable
/// C++ equivalent (PB's own docs call `KillThread` "very dangerous" for
/// exactly this reason - a killed thread never gets a chance to release
/// its own resources), so this uses each platform's own native forced-stop
/// primitive directly - `pthread_cancel` (POSIX: Linux, Haiku) or
/// `TerminateThread` (Windows). Deferred cancellation (glibc's default)
/// only takes effect at a cancellation point - `pbDelay`'s own
/// `sleep_for` is one on every target platform, matching every oracle-
/// verified example, which always loops on `Delay`. A thread with no
/// cancellation point (a pure compute loop, never oracle-tested) might not
/// honor this promptly, or at all; the call still returns immediately
/// either way - *not* joined/waited-for here, matching the oracle's own
/// apparently-asynchronous, non-blocking return, and avoiding a risk real
/// PB's own OS-level kill doesn't have: hanging the *caller* on a target
/// that never reaches one.
inline std::int64_t pbKillThread(std::int64_t threadId) {
    auto handle = detail::findThread(threadId);
    if (!handle || handle->finished.load()) {
        return 0;
    }
#if defined(_WIN32)
    TerminateThread(handle->thread.native_handle(), 0);
#else
    pthread_cancel(handle->thread.native_handle());
#endif
    handle->finished.store(true);
    handle->joined = true;
    if (handle->thread.joinable()) {
        handle->thread.detach();
    }
    return 0;
}

/// Oracle-verified (same isolated-subprocess caution as `KillThread`, see
/// its own doc comment): matches the real `PauseThread`/`ResumeThread`
/// example exactly - a worker thread looping on `Delay` visibly stops
/// advancing its own counter while paused, and resumes advancing once
/// `ResumeThread` is called... eventually: oracle-tested, real PB's own
/// `ResumeThread` has a large, apparently non-deterministic latency before
/// a paused thread actually resumes (observed: still paused at +600ms,
/// clearly running again by +2600ms) - not replicated here, since there's
/// no documented contract to replicate and no way to pick a "right" delay
/// (this cooperative implementation simply resumes as soon as it can). Both
/// functions return `0` unconditionally, matching their own docs' "Aucune"
/// (no return value) and oracle-verified directly the same way as
/// `KillThread`'s own doc comment describes.
///
/// Implementation: no portable POSIX primitive exists to suspend an
/// arbitrary *other* thread (`SIGSTOP` targeted at one thread via
/// `pthread_kill` stops the whole process on Linux, not just that thread;
/// Haiku has its own native `suspend_thread`/`resume_thread`, but that's
/// Haiku-only). Rather than a signal-handler-based suspend (real but
/// async-signal-safety-fragile, and still not portable to Haiku/Windows
/// uniformly), this is cooperative: flips a flag `pbDelay` itself checks
/// (see its own doc comment) - fully portable, pure `std::` synchronization,
/// no signal handling at all. Diverges from real PB for a thread with no
/// `Delay` call in its own loop (never oracle-tested; such a thread can't
/// be paused here), the same honest limitation `KillThread` has for a
/// thread with no cancellation point.
inline std::int64_t pbPauseThread(std::int64_t threadId) {
    auto handle = detail::findThread(threadId);
    if (handle) {
        std::scoped_lock<std::mutex> lock(handle->pauseMutex);
        handle->pauseRequested = true;
    }
    return 0;
}

inline std::int64_t pbResumeThread(std::int64_t threadId) {
    auto handle = detail::findThread(threadId);
    if (handle) {
        {
            std::scoped_lock<std::mutex> lock(handle->pauseMutex);
            handle->pauseRequested = false;
        }
        handle->pauseCv.notify_all();
    }
    return 0;
}

/// Oracle-verified: returns the thread's own native system identifier (PB's
/// docs call it a "Handle") - an opaque, platform-specific value, not
/// anything meaningful to do arithmetic on. `std::thread::native_handle()`
/// is `pthread_t` on POSIX or `HANDLE` on Windows; both fit in 8 bytes on
/// every target platform, so a raw byte copy (rather than a cast, which
/// isn't well-defined when the native type isn't itself an integer) is
/// the one truly portable way to reinterpret it as an Integer.
inline std::int64_t pbThreadID(std::int64_t threadId) {
    auto handle = detail::findThread(threadId);
    if (!handle) {
        return 0;
    }
    auto native = handle->thread.native_handle();
    std::int64_t id = 0;
    std::memcpy(&id, &native, std::min(sizeof(native), sizeof(id)));
    return id;
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
