# Optional recorded constructor-loop snapshot, 0.0.14.33

`source_loop_snapshot` supplies a strict codec and pure admission predicate for
one owner-local recorded OpenBIOS scene-entry checkpoint. It creates no default
counter. The frozen01432 read-only observation measured original constructor
`80100EFC` at loop4134 with scratch halfword4133, cadence2, event1/phase5,
main3/subphase2 and scratch byte0=0. Its status is MATCHES_MEASURED_FIELDS for that
reference case. The reference run later failed; no full completion or universal
retail/native initialization is inferred.

The decoder accepts exactly176 bytes of L3LS version1, selected-reference flag1,
CRC32 over bytes16..175, the supported EXE/overlay/EBD/BIOS digests, and zero
reserved bytes. It requires the recorded constructor tuple and `loops>0` with
`uint16_t(loops-1)==scratch_half`. Upper loop bits are retained; the low16 relation
does not reject another protocol-valid wrap count. Actual capture provenance is
separately pinned by the cooker, not established by CRC32. Decode and encode
publish no partial output on failure.

| Offset | Field |
| --- | --- |
| 0 | L3LS magic |
| 4 | uint16 version1 |
| 6 | uint16 total size176 |
| 8 | uint32 selected recorded OpenBIOS scope1 |
| 12 | uint32 CRC32 of bytes16..175 |
| 16,20 | uint32 constructor PC, recorded loop ordinal |
| 24 | uint16 scratch halfword |
| 26..31 | cadence, event ID, event phase, main phase, subphase, scratch byte0 |
| 32,64,96,128 | 32-byte EXE, overlay, EBD, BIOS SHA-256 identities |
| 160..175 | zero reserved bytes |

The pure predicate requires explicit recorded-replay selection, a successful
original phase5 constructor receipt, the pre-constructor active/event/phase
fields, matching boundary PC, matching recorded main/subphase/cadence/scratch0
owners, exact resident identities, and a nonzero current owner/generation. It
rejects missing fields, replacement owners, stale generations, repeated admission
and a different boundary. It performs no counter or lifecycle stores. Root
RuntimeSession wiring owns one-shot publication and invalidation on accepted
area/save/profile changes. Main/subphase/cadence replay owners must retain their
recorded scope and must not be described as recovered live native globals.

`cook_source_loop_snapshot.py` pins the actual frozen01432 observation and
reference-profile file hashes, validates their read-only/scope flags and bounded
scalar domains, and hashes the supplied EXE and selected installed BIOS. Program
overlay `ST00.BIN` and area-zero EBD `ST00_00.BIN` are independent required inputs;
the tool never borrows a program-file EBD for the area owner. It runs no original
instructions. The output directory must be new and outside source. The176-byte
checkpoint and provenance stay private and are excluded from source releases.

The actual repaired cook passed under private `recorded-loop-r2`; a native C++
decoder independently accepted those bytes and retained the measured scalars.
The initial failed area-owner cook log remains private. Public tests contain
authored protocol states only. Host C++17 warnings-as-errors tests cover nine
wrap/domain round trips, all truncated lengths, re-CRC malformed scalar/identity
and reserved cases, object validation, output atomicity, missing/wrong/stale
owner admission, duplicate rejection and4096 allocation-free predicate calls.
Eight Python test methods cover strict JSON, scalar/scope guards, canonical bytes,
the frozen-capture identity requirement and independent program/area owners.
An independent GCC16.1 ARMv6K warnings-as-errors compile passed: decoder stack
200 bytes, encoder216 bytes, admission24 bytes. No original instruction oracle
was rerun. Root validation reports connected integration separately.

This optional checkpoint does not prove the BIOS power-on producer, complete
native chronology, RNG/effect consumers, scene completion or physical Old3DS
behavior. No native frame, input mask or fabricated zero initializes halfword2.
