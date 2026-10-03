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
