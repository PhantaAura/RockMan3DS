# Class20/subtype03 parameter20 story predicate

Status: **SOURCE_BACKED_PARTIAL**. The original predicate helper and its
action0 caller are now executed without hooks. The predicate can be bound to
the existing native semantic story flag API for IDs0..2047. This establishes
the flag read and action gate; flag production, event timing, BIOS sampling,
later action3 and the whole retail encounter remain separate boundaries.

The read-only owner inputs are the established USA `ROCK_NEO.EXE`, SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`, and
ST00 PROGBIN SHA-256
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
No original code bytes, instruction listings, flag-bank contents, tables or
assets are exported.

In action0 phase2, parameter03 mask20 selects the predicate path. Original
call site `80102954` reads **unsigned** halfword work362 as the argument to
`8001DA58`. The helper performs one byte read:

| Operation | Original meaning |
| --- | --- |
| Argument | Unsigned semantic flag ID in `a0` |
| Byte address | `0x800BE378 + (id >> 3)` |
| Selected mask | `0x80 >> (id & 7)` |
| Return | Boolean0/1, true exactly when the selected bit is set |

The helper has no call dependencies, random input, geometry, work pointer or
bound check. It does not mutate the flag bank. Flag0 is valid; this caller has
no zero sentinel. The native getter must be evaluated for ID0 as well.

A true result selects action3 and clears the entire halfword at work0A,
including auxiliary byte0B. False keeps action0/phase2. Both cases set
flags328 mask20. The timer32C and work330 remain unchanged on this phase2
invocation. Work80 and packet33C do not affect this parameter20 path; the
probe supplies a far distance and nonzero packet to distinguish it from the
alternate proximity/packet route.

The source initializer copies raw halfword1A to work362. The already proved
common class20 object parser copies eight object bytes beginning at record0C
into work14, so the fourth raw lane comes from the object's tail halfword12.
The native work seed preserves that lane and the scalar initializer preserves
the same copy. This is a separate field from the conditional spawn record's
additional u32 predicate at record20. The initializer copy is covered by the
[scalar transaction differential proof](CLASS20_03_TRANSACTION.md).

For the selected owner area2 parameterA0 class20/03 records, the tail flag is
1547. Their allocation-skip predicates are1602/1603/1604. These scalar IDs
show that activation and allocation use distinct flags; they do not establish
which event creates either flag or when it should change. The inspected
conditional catalog repeats those selected records through two descriptors.

`RuntimeSession::story_flag(id)` and the native save flag API use the same
semantic IDs0..2047. Native physical packing is LSB-first, while the original
bank is MSB-first. Calling the typed getter retains the semantic ID without
copying source bytes into a native save. The probe checks every bit position
under zero, selected-only, other-bits-only and all-bits-set premises and
confirms the corresponding neutral packing result.

The native adapter may therefore supply
`predicate_8001da58 = session.story_flag(id)` when
`id = static_cast<uint16_t>(work362)` is below2048. It must leave that optional
input unknown for larger IDs. The original helper reads beyond the native
256-byte bank for those inputs; the native getter's out-of-range false result
is not an established source result. The probe includes IDs2048 and65535 to
demonstrate this boundary, with explicit synthetic bytes at the computed
addresses rather than relying on original runtime memory contents.

The source-safe reproducer is
[class20_03_story_predicate_source_probe.py](class20_03_story_predicate_source_probe.py).
It passed 8,200 original helper executions and4,096 original action0 caller
executions, interpreting 297,064 instructions. There are **no hooks** in
either path. Every helper case observes exactly the expected one-byte flag
read; every caller case observes that read and only the original helper call.
All flag bytes and work fields are explicitly authored synthetic premises.
This research did not modify the engine implementation.

Owner-local scalar evidence is at
`work/pass-20261006-worldalive-01418/enemy-callback/predicate-source-evidence.json`.
The earlier [event and spawn evidence](../research_01415/EVENT_AND_SPAWN_EVIDENCE.md)
already used the same original flag helper for conditional allocation. This
new proof closes the action0 call-input and truth-direction boundary.

```sh
python3 docs/research_01418/class20_03_story_predicate_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261006-worldalive-01418/enemy-callback/new-predicate-evidence.json
```
