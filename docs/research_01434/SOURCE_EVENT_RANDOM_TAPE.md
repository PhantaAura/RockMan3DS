# Optional recorded scene RNG tape, 0.0.14.34

`source_event_random_tape` loads a bounded L3SR recorded OpenBIOS scene prefix
and supplies its completed original call results to the `80100D6C` consumer.
The service contains no seed, recurrence or generated results. Selection is
explicit. Missing or mismatched context holds the cursor and the callback.

The actual private capture decodes as50 callback records and186 scene returns.
Its paired constructor directly observed halfword6796 and scratch byte0=1.
The first random callback directly observed halfword9336, position raw16
`[0,0,-574]`, actor state08=1 and pool-low bitmap `E0000000`. Loop-listener count
6802 differs from the constructor halfword by6; no `loops-1` relation is used.
The old01432 checkpoint must not be combined with this new capture. Root binds
the paired constructor only after the matching successful phase5 constructor
and resident-resource witness. Other recorded fields retain replay scope.

## Canonical L3SR

The header is256 bytes: magic/version/header-size, scope1, record count1..64,
record size144, exact total size, CRC32 over bytes32..end and a zero reserved
word. Its six digests at32..223 identify EXE, tutorial overlay, area-zero EBD,
selected BIOS, semantic trace and audited global RNG trace. The first four are
the exact supported owner identities; trace digests must be nonzero.

Bytes224..231 retain the paired constructor: scratch halfword, scratch byte0,
cadence2, main3, subphase2, event1, phase5. Scratch byte0 may be0 or1. Bytes232..255
are zero. No listener loop count or implied halfword equation exists.

Each144-byte record owns frame, source actor pointer, state08, count0/3/6,
scratch halfword, three raw16 position words, both pre-allocation pool80 bitmap
words, event1/phase6/active1 labels, global ordinal before/after, and six16-byte
RA/V0/ordinal receipts. Reserved and unused receipt fields are zero. The current
supported prefix begins at frame0 and has consecutive frames and state08=1.
Its callback count must match the original signed-frame/bitmap allocation rule.
Global ordinals satisfy `0<before<=after<=32767`; each return is consecutive
within the callback and has the exact original RA. Gaps between callbacks are
retained rather than filled with generated competing consumers. Frame31 and
later original zero-call callbacks remain explicit records.

## Preparation and publication

Preparation checks the current selected callback, complete event fields,
recorded scratch value, class40 pointer/header/occupancy/state/position, both
pool bitmap words, the callback's actual bounded allocation plan and all four
current owner identities. It requires an explicit matching resident P5 witness
receipt and nonzero current resource owner/generation supplied by RootSession.
It never rewrites positions, pool bits or scratch values to make a record fit.
Consequently unported actor movement or effect reclaim can produce an exact
context mismatch hold. That hold is an unresolved owner, not a reason to relax
the recorded context.

The prepared typed batch borrows the service's current generation. A held
callback returns AwaitingCommit and changes neither cursor nor generation.
Only a completed original D6C prefix with the exact committed call count advances
one record, including count0. Commit rechecks the packet/cursor/generation;
duplicate, altered, stale or partial receipts do not advance it. Exhaustion
holds. Accepted area/save/profile changes invalidate the service in RootSession.

Phase6 appends optional receipt/owner arguments with null defaults and forwards
them from `SourceEventInputs`. Its common commit publishes the callback's pool,
camera, control and continuation stores together. Missing/stale receipts leave
that journal unchanged. The step result retains `random_callback_consumed_count`
and typed status before a later camera/timeline/caller result can replace the
prefix result, allowing root to commit the tape once after a completed callback.

## Validation

Authored tests cover canonical decode, truncation, CRC/reserved/header/identity
guards, unsupported actor state/ordinal bounds, strict context mismatches,
completed/held/partial/altered/stale cursor commits, and32 zero-call callbacks
including frame31. The actual7456-byte private cooked capture was independently
accepted by the native decoder with50/186 coverage and the directly recorded
constructor/first-callback fields. No actual tape bytes are embedded in source.

Four focused host tests pass: phase6 integration, random-effect consumer,
recorded tape and RootSession recorded-scene wiring. The standalone ARMv6K
warnings-as-errors compile passes after two misleading-indentation brace fixes.
Reported stack: load-time decoder9488 bytes, prepare128 bytes and commit32 bytes;
the large fixed profile is copied at installation, not during each callback.
No original instruction oracle was repeated for tape plumbing. Root validation
owns complete builds, emulator results and the actual trace/cooker audit.

The scope remains one recorded selected-OpenBIOS scene prefix. Complete native
chronology, actor/effect lifecycle, resources/draw, scene completion, retail parity
and physical Old3DS behavior remain unresolved.
