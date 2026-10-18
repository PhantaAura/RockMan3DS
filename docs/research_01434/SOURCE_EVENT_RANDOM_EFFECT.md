# Bounded random-result callback80100D6C, 0.0.14.34

The new `source_event_random_effect` module consumes the original scalar callback
with explicit completed random-result receipts. It owns no random generator,
seed, sample cursor, global control byte or renderer. Missing event/actor/pool/RNG
owners hold before every store. Root owns the enclosing phase6 prefix, shared RNG
transaction, residency admission and native chronology.

## Original operations

The callback reads event-work frame at offset16. Its signed comparison admits
frames below31, including raw words `80000000..FFFFFFFF`; other frames return
without allocation or RNG calls. Frame6 independently clears byte `80122424`.
The module exposes that store as an optional control-byte receipt on successful
commit; it never mutates a hidden phase6/global owner.

The active branch loads the class40 pointer at event offset36 and performs two
calls to allocator `80031618`. This allocator scans family80 at `800A3B40`,64
records of72 bytes, using MSB-first bitmap words `80098838/3C`. Each successful
attempt sets the original bitmap and header stores, then callback fields00=1,
02=131 and03=0. A full pool returns null and the callback skips that attempt's
random calls/stores. Consequently the callback consumes exactly0,3 or6 results.
No capacity error or unconditional six-call batch replaces those source branches.

| Allocation | Axis / source halfword | Random caller return address | Mask / bias | Destination |
| --- | --- | --- | --- | --- |
| First | actor+20 | 80100DD4 | 63 / 31 | record+28 |
| First | actor+22 | 80100DEC | 31 / 15 | record+30 |
| First | actor+24 | 80100E04 | 63 / 31 | record+32 |
| Second | actor+20 | 80100E40 | 63 / 31 | record+28 |
| Second | actor+22 | 80100E58 | 31 / 15 | record+30 |
| Second | actor+24 | 80100E70 | 63 / 31 | record+32 |

Each destination is the low16 of `source_half + (return_v0 & mask) - bias`.
The record's untouched bytes retain their owners. These are scalar fields and
allocation identities; no effect art, resource, update or draw meaning is inferred.

## Native contract

`plan_source_event_random_effect_80100d6c()` preflights event1/phase6, the signed
frame branch, allocator bitmaps/header identities, and the class40 pointer/header
and six source position bytes when a successful allocation dereferences them.
An unread second bitmap is not an invented requirement. The plan reports exact
slots, count and caller return addresses so an external tape/service can prepare
only the required calls. The consumer recomputes that plan before publication.

The prepared batch must be completed and match callback, event-work identity,
frame, source actor, count, caller return addresses and consecutive nonzero global
ordinals. Selected original result values must be within0..32767. Unused trailing
receipt entries are zero. A separate current owner/generation must match the
batch; the batch cannot assert its own current lifecycle token. No native frame,
button, private PRNG or zero substitutes for missing results.

After complete preflight, the consumer copies at most two72-byte records and
publishes only their changes and changed bitmap words. Failure preserves the
whole owner and returns zero committed calls with no control-byte receipt. The
external RNG owner remains untouched. Root commits the RNG transaction only
with the complete callback's pool and control stores. A selected recorded-boundary
candidate service remains a candidate for native chronology, even when these
individual scalar operations match the source.

For phase6 integration, root should finish its existing camera/resource/profile
preflight, call this consumer in the `80100D6C` branch, publish its control receipt
only when committed, and retain the existing camera/timeline/caller continuation.
No later failure may publish only the RNG cursor or only the pool stores. Source
full-pool/null and signed-no-op branches require no RNG owner.

## Evidence and focused validation

One focused oracle completed42 authored owner cases and18,750 unchanged original
instructions, including135 actual selected installed OpenBIOS random calls and
the original allocator/wrapper instructions. It covers empty, fragmented,
bank-crossing, one-free-slot and full pools; frame0/6/30/31, positive signed maximum
and two negative raw frames; retained bytes and16-bit offset wrapping. There are
zero instruction patches and zero helper hooks.

The oracle executes the selected unchanged BIOS rand routine at declared A0
service boundaries after the original wrapper. It does not execute the full BIOS
dispatcher or competing consumers. Initial BIOS states are authored probe inputs;
they are not admitted native seeds or a live retail chronology. The proof hashes
the supplied EXE, tutorial overlay and selected BIOS. `random-effect-source-proof.json`
contains metadata only; the full differential fixture remains private under
`random-effect-oracle-r1`. No original bytes or owner-local fixture are embedded
in source. An initial Python-version `bit_count` incompatibility was corrected
before the full focused run; no broader original proof was repeated.

The public test runs authored atomic/incomplete/partial/wrong/stale receipt and
owner guards, null allocator and signed branches, exact masked/wrapped fields,
header stores, unused bitmap guards, and4096 allocation-free RNG holds. Its optional
private fixture argument verifies all42 complete family80 output owners against
the unchanged-instruction oracle. Host C++17 warnings-as-errors and independent
GCC16.1 ARMv6K warnings-as-errors compile pass. Reported stack use is56 bytes for
planning and472 bytes for the consumer. Root validation reports connected builds
and emulator evidence separately.

Unresolved: full shared RNG/competing-consumer chronology, effect lifecycle and
resources/draw, complete scripted scene behavior, completion/handoff and physical
Old3DS validation. No full World Comes Alive, retail parity or PHYSICS_1TO1 claim.
