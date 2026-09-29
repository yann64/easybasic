; Exercises the M4c Memory library: every Peek*/Poke* primitive variant
; against an explicit AllocateMemory'd block (the idiomatic use case, and
; the one this project's PeekS/PokeS UTF-16LE conversion is verified
; against - see docs/architecture/roadmap.md's M4c notes on why @stringVar
; itself is a deliberately out-of-scope case), plus reading/writing through
; a plain untyped pointer (*p = @a), closing the M3d deferral, plus a
; deliberately misaligned Poke/Peek (a real regression: an early
; implementation dereferenced a raw reinterpret_cast'd pointer directly,
; undefined behavior UBSan caught on exactly this pattern even though it
; happened to produce the right numeric answer on x86 - real PB itself
; tolerates this the same way, oracle-verified).
a.l = 12345
*p = @a
Debug PeekL(*p)
PokeL(*p, 99999)
Debug a

*blk = AllocateMemory(64)
PokeB(*blk, -5)
PokeA(*blk + 1, 200)
PokeC(*blk + 2, 65)
PokeW(*blk + 4, -1000)
PokeU(*blk + 6, 5000)
PokeL(*blk + 8, -100000)
PokeQ(*blk + 16, 123456789012)
PokeF(*blk + 24, 3.5)
PokeD(*blk + 32, 2.71828)

Debug PeekB(*blk)
Debug PeekA(*blk + 1)
Debug PeekC(*blk + 2)
Debug PeekW(*blk + 4)
Debug PeekU(*blk + 6)
Debug PeekL(*blk + 8)
Debug PeekQ(*blk + 16)
Debug PeekF(*blk + 24)
Debug PeekD(*blk + 32)

s.s = "Hello World"
PokeS(*blk + 40, s)
Debug PeekS(*blk + 40)
Debug PeekS(*blk + 40, 5)

FreeMemory(*blk)

*u = AllocateMemory(32)
PokeB(*u, 1)
PokeL(*u + 1, 123456)
Debug PeekL(*u + 1)
PokeD(*u + 3, 2.5)
Debug PeekD(*u + 3)
FreeMemory(*u)
