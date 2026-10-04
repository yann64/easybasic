; Exercises M7a: CreateThread/IsThread/WaitThread, Mutex (CreateMutex/
; LockMutex/UnlockMutex/TryLockMutex/FreeMutex), Semaphore (CreateSemaphore/
; SignalSemaphore/WaitSemaphore/TrySemaphore/FreeSemaphore), and @Procedure()
; (the procedure's own address, oracle-verified to need no arguments at the
; call site regardless of the named procedure's own parameter count).
;
; Deliberately avoids asserting on anything timing-sensitive (no raw
; Delay/ElapsedMilliseconds values printed) - every assertion here is a
; join-synchronized boolean/count check, so the test is deterministic
; regardless of how the underlying OS actually schedules the threads.
Procedure Worker(n)
  ProcedureReturn n * 2
EndProcedure

tid = CreateThread(@Worker(), 21)
Debug IsThread(tid)
result = WaitThread(tid)
Debug result
Debug IsThread(tid)

; Oracle-verified: PB's own Mutex is re-entrant (recursive) - a second
; TryLockMutex from the same thread that already holds the lock succeeds
; too, rather than deadlocking.
mut = CreateMutex()
Debug TryLockMutex(mut)
Debug TryLockMutex(mut)
UnlockMutex(mut)
Debug TryLockMutex(mut)
UnlockMutex(mut)
FreeMutex(mut)

; Oracle-verified: CreateSemaphore() defaults its initial count to 0;
; CreateSemaphore(3) starts with 3 already available.
sem1 = CreateSemaphore()
Debug TrySemaphore(sem1)
sem2 = CreateSemaphore(3)
Debug TrySemaphore(sem2)
Debug TrySemaphore(sem2)
Debug TrySemaphore(sem2)
Debug TrySemaphore(sem2)
SignalSemaphore(sem2)
Debug TrySemaphore(sem2)
FreeSemaphore(sem1)
FreeSemaphore(sem2)

; M7a's own deferred KillThread/PauseThread/ResumeThread/ThreadID.
; Oracle-verified (carefully, in an isolated/sandboxed subprocess, since
; real PB's own KillThread is genuinely dangerous when misused - see the
; roadmap's own M7a notes): with ThreadSafe mode enabled, KillThread on a
; genuinely still-running thread terminates it cleanly; a thread calling
; KillThread on an *already-finished* thread instead gets a fatal debugger
; error that aborts the whole program, so that case is deliberately never
; exercised here (it would make this very diff test non-comparable).
; KillThread/PauseThread/ResumeThread also all have no documented return
; value - oracle-verified directly, `Debug KillThread(...)` always prints
; 0 regardless of success - so their own return values are never printed
; below either, only their side effects.
;
; Printed as derived booleans/comparisons rather than raw values
; (ThreadID's own native handle, or an exact loop-iteration count) since
; those aren't expected to match bit-for-bit between the oracle's own
; binary and pbcxx's - only the *shape* of the behavior is. Resuming is
; checked only for "didn't advance while paused", not "advances again
; soon after resuming" - oracle-verified real PB's own ResumeThread has a
; large, apparently non-deterministic latency before a paused thread
; actually resumes (seen taking over half a second), so a short post-
; resume window isn't reliably comparable between the two binaries.
idSem = CreateSemaphore()
Procedure Ticker(sem)
  SignalSemaphore(sem)
  Repeat
    Delay(20)
  ForEver
EndProcedure
tid2 = CreateThread(@Ticker(), idSem)
WaitSemaphore(idSem) ; blocks until the worker has genuinely started running
id1 = ThreadID(tid2)
id2 = ThreadID(tid2)
If id1 = id2
  Debug 1
Else
  Debug 0
EndIf
If id1 <> 0
  Debug 1
Else
  Debug 0
EndIf
KillThread(tid2)
Delay(60)
Debug IsThread(tid2)
FreeSemaphore(idSem)

counterSem = CreateSemaphore()
Global counter.i = 0
Procedure Counter(sem)
  SignalSemaphore(sem)
  Repeat
    counter = counter + 1
    Delay(10)
  ForEver
EndProcedure
tid3 = CreateThread(@Counter(), counterSem)
WaitSemaphore(counterSem)
Delay(60)
PauseThread(tid3)
beforePause.i = counter
Delay(80)
afterPause.i = counter
If beforePause = afterPause ; should not have advanced while paused
  Debug 1
Else
  Debug 0
EndIf
ResumeThread(tid3)
KillThread(tid3)
FreeSemaphore(counterSem)
