# Auxiliary prop release with source-owned bits

Status: `SOURCE_BACKED_PARTIAL`. E0 slot14's original80112D14 release calls80031988 with an E0 pointer. Its address arithmetic selects pending8009898C, used800988FC and bit08000000. The helper must preserve that side effect; freeing the E0 allocation bit instead would change the source.

## The foreign word is a count

800988FC is the original deferred primitive queue count, not an allocation bitmap. Both988FC and9898C are zero in the hash-guarded owner's loaded EXE payload. Original8002CFA4 resets the count at8002CFE8. Original enqueue8002D464/8002D4B8 increments only below256; their drains8002D50C/8002D9A0 clear it. Consequently a freshly admitted count stays in0..256 under these writers without requiring its exact value.

The tutorial has one additional unchecked increment at80112648, in E0/subtype1 callback8011216C. An unknown occupied E0 roster entry or subtype1 permanently revokes the bounded-count premise. The current retained opening constructor supplies subtype3 records; the native helper still checks the actual current occupied roster on every service rather than trusting that historical fact forever. Other overlays, untracked writers, unsupported restores and observation gaps must invalidate ownership.

The exact count remains **unknown**. Its proved known-zero mask isFFFFFE00; bits0..8 have no supplied numeric value. With pending0 and the original08000000 bit, every count in0..256 yields `(pending|bit)&count =0`. The leaf therefore writes the exact foreign pending result and clears the prop's work00, while retaining the E0 allocation mask. Slot31's original100 bit intersects the unknown count bits and remains held. Other foreign addresses remain unknown too.

## Runtime API

`SourcePropForeignGlobals` lives with the session's original global owners, not an area or renderer. `initialize_source_prop_foreign_globals_from_executable_load(state,generation,admittedFreshLoad)` applies the actual supported EXE-load lifecycle once. False/zero admission leaves words unknown. Do not invoke it as a per-frame/area reset or use it to recover a warm state whose earlier writers were not modeled.

`prepare_source_prop_foreign_release(state,propID,currentGeneration,roster)` validates the encountered roster and returns a one-use receipt. Its used_word is intentionally empty; used_bits_known_mask carries only the proved zeros. Call it before every E0 service so a previously encountered unchecked producer cannot disappear unnoticed. An empty known roster is allowed, but preparing a particular detached/unallocated prop remains held.

For a release callback, pass the prepared owner's address to the existing `SourcePropAttachInputs`. The leaf combines known bits through the originalAND and requires every output bit to be known before publishing any prop/owner state. It retains exact fully known external-word receipts for the inherited differential tests. After a complete released callback, `commit_source_prop_foreign_release` publishes the pending word under its matching source generation/revision; stale, repeated, corrupted or held commits reject. The wrapper should prepare each release ticket immediately before use and restore its small prop record if a subsequent global commit unexpectedly fails.

This is neither a guessed zero count nor an E0free. Missing ownership still holds atomically. The release branch continues its original matrix/attachment work after80031988, as in the inherited module.

## Verification

The bounded original probe checks the two initial loaded words,16 capped enqueue cases and three reset slices. It executes238 unchanged instructions with zero hooks/patches and exports no owner instruction bytes or assets. The cap preserves its valid range; an already invalid greater-than256 value is not silently normalized.

The new native test checks every257 possible bounded count values against the unchanged attachment reducer, plus fresh-load/stale/roster/revocation/alias/atomic/one-use guards. It preserves unknown exact count and E0 allocation. The inherited180 original attachment cases and6156 atomic guards still pass with zero callback allocations. Clang and actual devkitARM ARMv6K GCC compile with warnings as errors. GCC reports304-byte attachment stack,24-byte preparation stack and8-byte commit stack; whole-frame stack/hardware performance remain pending.

```sh
python3 -B docs/research_01440/source_prop_foreign_globals_probe.py --exe /ABS/ROCK_NEO.EXE --overlay /ABS/owner-overlay.bin --out /NEW/ABS/source-proof.json
test_source_prop_foreign_globals
test_source_prop_attach tests/fixtures/source_prop_attach_01439/attachment-authored.tsv
```

## Remaining shared RNG chronology

The existing selected-OpenBIOS trace has2575 global returns between constructor ordinal133 and first scene-callback boundary2708. Subtype2 accounts for1281. Of the other1294, already ported subtype3/4 source consumers account for909:204 at801173C4/80117408 and705 spread across801177B4/801177D0/801177E8/80117804/80117814/801178A0/801178B4. Their producers and actual ordered receipt provider still must be wired.

The385 remaining E0 calls are96 at8011367C;90 at80113238;54 at801133E4;24 at80113E54;23 each at80113CC8/80113CF4/80113D0C/80113D2C;21 at80113AD8;4 each at80113D94/80113DC0. The bounded lift consumer covers the114 lift-site calls, leaving271 other E0 calls. These counts are original passive evidence, not a native schedule.

The cheapest complete opening-provider route is to connect actual subtype3/4 producers plus the shared lower80 transactional caller, then close these E0 producer sites and the preceding133 original introductory calls. No sparse replay may skip competing returns or start at a convenient inferred seed. Missing parent/context/cadence ownership must remain held or explicitly `CANDIDATE`; selected-OpenBIOS recurrence is not Sony retail parity. `PHYSICS_1TO1=false` and physical Old3DS validation remain unchanged.
