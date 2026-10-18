# 01415 source-backed spawn and event correction

`event_source_probe.py` reads only the hash-checked owner-local USA executable
and ST00 overlay. Its JSON records addresses, derived behavior, aggregate
checks and code hashes; it exports no source bytes or original function tables.

The initial area0 C0/03 record dispatches to `0x80119398`. Its four-byte timeline
entries select a palette index, duration and signed advance/loop/end control.
The consumer `0x80119580` extracts RGB555 channels, interpolates sixteen color
halfwords and sets bit15. `0x800AD168` is the color buffer base. The historical
01413 `selected_script`, `command_base` and command-pointer interpretation is
superseded by the JSON correction. No current runtime executes that inferred
command stream, so no runtime behavior was disabled.

Area1 C0/21 seeds the E0/00 room controller through `0x80119BE8` and explicit E0
slot allocator `0x800317AC`, trying slot31 first and descending if unavailable. Its per-area conditional descriptor table is
reached through `0x8011C898` and the selected ST00 table `0x8011A46C`. Empty
areas0/4 have null descriptor pointers; C0/21 is authored only in areas1/2/3.

E0 state1 `0x80111810` walks twelve-byte descriptors. A different qualifier
from signed work+1C permits the signed16 player X/Z, floor-divided by512, to
enter two unsigned half-open intervals. The last passing descriptor supplies
work+18 and state2. State2 `0x80111CD0` executes on the next dispatcher tick;
its twenty-four-byte records contain a twenty-byte object record and a u32
story flag predicate. Set predicates skip allocation. The source flag reader
`0x8001DA58` uses bank `0x800BE378`, byte `id>>3` and mask `0x80>>(id&7)`.
The initial three class20/03 records have predicates1599/1600/1601. Native
code can use semantic flag IDs without adopting the source byte layout.

State2 commits qualifier to work+1C, descriptor id to work+1D and returns to
state1. The source controller tracks its conditional allocations as id pairs
at `0x801224EC`; group changes affect those allocations. Its old-group
allocation masks also drive removed-object flag writes. This does not prove
a complete native death/pickup/persistence lifecycle or enemy AI. Static area
objects have separate ownership.

The optional local `L3SP` v1 catalog preserves five source areas, seven selected
regions and forty-five records (including duplicate lists referenced by
separate regions), in1272bytes. It contains no PS1 pointers. The native reader
validates canonical area/region/record coverage, CRC, finite counts, signed
work selector domains and flags0..2047 before decoded vector allocation.
The companion cooker operates on the owner's extracted overlay; cooked data
belongs outside source releases. Existing flow and package formats remain
compatible when the new binding is absent.

The actual event mechanism is independent of C0/03 and EBD resource lookup:
`0x8001D990` calls stage poll table `0x80088FA8`; ST00 uses `0x80100598` and
its flag-to-event table `0x8011ADF8`. Flag1540 starts event1 via `0x8001DDE4`.
The event handler table `0x80089024` selects `0x801010C4`; its state table is
`0x80100018`. Per-actor command interpreter `0x8001EC0C` switches on a word's
high byte, while `0x8001F070` advances eight-byte phase timing entries. These
are real executable consumers; unknown operations remain unknown.

Event1's final state7 branch waits for busy halfword `0x800A8A72` to become
zero, then requests stage0/area1 at XYZ(2822,-1,-4833), yaw0, status2; it clears
flag1540 and deactivates the event. The bounded probe checks this branch under
an explicit completion premise with preceding update/cleanup calls hooked.
The message request consumer chain `0x80063BC8 -> 0x80063C30 -> 0x80053A08`
selects source payload `0x80153000`; a complete initial cutscene, message or
C0-to-trigger-to-message-to-transition run is not claimed.

Validation:105 original spatial-gate cases,5 palette interpolation cases,
16 flag reads, and1 conditional final transition branch pass. Host
`test_st00_spawn` covers negative-coordinate boundaries, same qualifiers,
last-passing descriptor selection, roundtrip and malformed catalog rejection.
Hardware and full retail parity remain untested.
