#include <catch2/catch_test_macros.hpp>

#include <atomic>

#include <easybasic/runtime/threadlib.hpp>

using namespace easybasic::runtime;

namespace {
std::atomic<std::int64_t> g_lastParam{-1};
std::int64_t recordParam(std::int64_t param) {
    g_lastParam.store(param);
    return 0;
}

std::atomic<bool> g_spin{true};
std::int64_t spinUntilStopped(std::int64_t /*param*/) {
    while (g_spin.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}

// Loops calling pbDelay (rather than a raw sleep) so it is both a
// pthread_cancel cancellation point (for KillThread) and a cooperative
// pause point (for PauseThread/ResumeThread) - matching every real-PB
// oracle example for both, which always loops on Delay.
std::atomic<std::int64_t> g_counter{0};
std::int64_t countForever(std::int64_t /*param*/) {
    for (;;) {
        g_counter.fetch_add(1);
        pbDelay(5);
    }
    return 0;
}
} // namespace

TEST_CASE("pbCreateThread runs the entry procedure with the given parameter", "[runtime][threadlib]") {
    g_lastParam.store(-1);
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&recordParam), 42);
    pbWaitThread(tid);
    CHECK(g_lastParam.load() == 42);
}

TEST_CASE("pbIsThread reflects whether the thread is still running", "[runtime][threadlib]") {
    g_spin.store(true);
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&spinUntilStopped), 0);
    CHECK(pbIsThread(tid) == 1);
    g_spin.store(false);
    pbWaitThread(tid);
    CHECK(pbIsThread(tid) == 0);
}

TEST_CASE("pbWaitThread returns a plain success flag, not the procedure's own return value",
          "[runtime][threadlib]") {
    // Oracle-verified: a thread Procedure's `ProcedureReturn` value is never
    // observable through WaitThread - real PB threads communicate back via
    // shared state (globals/Mutex/Semaphore), not a return channel.
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&recordParam), 999);
    CHECK(pbWaitThread(tid) == 1);
}

TEST_CASE("pbIsThread/pbWaitThread return a harmless default for an unknown thread ID", "[runtime][threadlib]") {
    CHECK(pbIsThread(999999) == 0);
    CHECK(pbWaitThread(999999) == 0);
}

TEST_CASE("pbThreadID returns a stable, nonzero native handle while the thread is alive", "[runtime][threadlib]") {
    g_spin.store(true);
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&spinUntilStopped), 0);
    std::int64_t id1 = pbThreadID(tid);
    std::int64_t id2 = pbThreadID(tid);
    CHECK(id1 != 0);
    CHECK(id1 == id2);
    g_spin.store(false);
    pbWaitThread(tid);
}

TEST_CASE("pbThreadID returns a harmless default for an unknown thread ID", "[runtime][threadlib]") {
    CHECK(pbThreadID(999999) == 0);
}

TEST_CASE("pbKillThread terminates a genuinely still-running thread", "[runtime][threadlib]") {
    // Oracle-verified (carefully, in an isolated subprocess): KillThread on
    // a running thread stops it immediately - its own counter observably
    // stops advancing, and IsThread reports 0 right after. Its own return
    // value is always 0 (oracle-verified: real PB's docs list no return
    // value at all, and `Debug KillThread(...)` always prints 0), so only
    // the side effect is checked here, not the return value.
    g_counter.store(0);
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&countForever), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    CHECK(pbIsThread(tid) == 1);
    pbKillThread(tid);
    CHECK(pbIsThread(tid) == 0);
    std::int64_t frozen = g_counter.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(g_counter.load() == frozen);
}

TEST_CASE("pbKillThread on an already-finished thread is a harmless no-op", "[runtime][threadlib]") {
    // Oracle-verified: real PB raises a fatal debugger error here instead
    // ("The specified Thread does not exists.") - pbcxx deliberately
    // diverges by just doing nothing rather than replicating a debugger-
    // only abort path.
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&recordParam), 1);
    pbWaitThread(tid);
    CHECK(pbKillThread(tid) == 0);
    CHECK(pbIsThread(tid) == 0);
}

TEST_CASE("pbKillThread on an unknown handle is a harmless no-op", "[runtime][threadlib]") {
    CHECK(pbKillThread(999999) == 0);
}

TEST_CASE("pbPauseThread/pbResumeThread round-trip on a thread looping on Delay", "[runtime][threadlib]") {
    // Oracle-verified: matches the real PauseThread/ResumeThread example
    // exactly - the worker's own counter stops advancing while paused, and
    // advances again once resumed. Both always return 0 (same "no return
    // value" oracle finding as KillThread's own doc comment), so only the
    // side effect is checked here, not the return value.
    g_counter.store(0);
    std::int64_t tid = pbCreateThread(reinterpret_cast<std::int64_t>(&countForever), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    pbPauseThread(tid);
    std::int64_t paused1 = g_counter.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    std::int64_t paused2 = g_counter.load();
    CHECK(paused1 == paused2);
    pbResumeThread(tid);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    CHECK(g_counter.load() > paused2);
    pbKillThread(tid);
}

TEST_CASE("pbPauseThread/pbResumeThread on an unknown handle are a harmless no-op",
          "[runtime][threadlib]") {
    CHECK(pbPauseThread(999999) == 0);
    CHECK(pbResumeThread(999999) == 0);
}

TEST_CASE("Mutex is re-entrant (recursive) from the owning thread", "[runtime][threadlib]") {
    // Oracle-verified: a second TryLockMutex from the same thread that
    // already holds the lock succeeds too, rather than deadlocking.
    std::int64_t mutex = pbCreateMutex();
    CHECK(pbTryLockMutex(mutex) == 1);
    CHECK(pbTryLockMutex(mutex) == 1);
    pbUnlockMutex(mutex);
    CHECK(pbTryLockMutex(mutex) == 1);
    pbUnlockMutex(mutex);
    pbUnlockMutex(mutex);
    pbFreeMutex(mutex);
}

TEST_CASE("LockMutex/UnlockMutex round-trip", "[runtime][threadlib]") {
    std::int64_t mutex = pbCreateMutex();
    CHECK(pbLockMutex(mutex) == 1);
    CHECK(pbUnlockMutex(mutex) == 1);
    pbFreeMutex(mutex);
}

TEST_CASE("Mutex operations on an unknown handle return a harmless default", "[runtime][threadlib]") {
    CHECK(pbLockMutex(999999) == 0);
    CHECK(pbUnlockMutex(999999) == 0);
    CHECK(pbTryLockMutex(999999) == 0);
    CHECK(pbFreeMutex(999999) == 0);
}

TEST_CASE("CreateSemaphore defaults to an initial count of 0", "[runtime][threadlib]") {
    std::int64_t sem = pbCreateSemaphore();
    CHECK(pbTrySemaphore(sem) == 0);
    pbFreeSemaphore(sem);
}

TEST_CASE("CreateSemaphore honors an explicit initial count", "[runtime][threadlib]") {
    // Oracle-verified: CreateSemaphore(3) starts with 3 already available.
    std::int64_t sem = pbCreateSemaphore(3);
    CHECK(pbTrySemaphore(sem) == 1);
    CHECK(pbTrySemaphore(sem) == 1);
    CHECK(pbTrySemaphore(sem) == 1);
    CHECK(pbTrySemaphore(sem) == 0);
    pbFreeSemaphore(sem);
}

TEST_CASE("SignalSemaphore/TrySemaphore behave as a counting semaphore", "[runtime][threadlib]") {
    std::int64_t sem = pbCreateSemaphore();
    CHECK(pbTrySemaphore(sem) == 0);
    pbSignalSemaphore(sem);
    CHECK(pbTrySemaphore(sem) == 1);
    CHECK(pbTrySemaphore(sem) == 0);
    pbFreeSemaphore(sem);
}

TEST_CASE("WaitSemaphore blocks until signaled", "[runtime][threadlib]") {
    std::int64_t sem = pbCreateSemaphore();
    std::int64_t tid = pbCreateThread(
        reinterpret_cast<std::int64_t>(+[](std::int64_t s) -> std::int64_t {
            pbWaitSemaphore(s);
            return 0;
        }),
        sem);
    // Give the waiting thread a moment to actually block before signaling.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK(pbIsThread(tid) == 1);
    pbSignalSemaphore(sem);
    pbWaitThread(tid);
    CHECK(pbIsThread(tid) == 0);
    pbFreeSemaphore(sem);
}

TEST_CASE("Semaphore operations on an unknown handle return a harmless default", "[runtime][threadlib]") {
    CHECK(pbSignalSemaphore(999999) == 0);
    CHECK(pbTrySemaphore(999999) == 0);
    CHECK(pbFreeSemaphore(999999) == 0);
}

TEST_CASE("pbDelay sleeps for roughly the requested duration", "[runtime][threadlib]") {
    std::int64_t before = pbElapsedMilliseconds();
    pbDelay(50);
    std::int64_t after = pbElapsedMilliseconds();
    CHECK(after - before >= 40); // Loose lower bound to avoid flakiness; no upper bound (CI scheduling jitter).
}

TEST_CASE("pbElapsedMilliseconds is monotonically non-decreasing", "[runtime][threadlib]") {
    std::int64_t first = pbElapsedMilliseconds();
    std::int64_t second = pbElapsedMilliseconds();
    CHECK(second >= first);
}
