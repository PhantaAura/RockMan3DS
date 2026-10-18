# Class40 fresh initializer and player mirror ownership, 0.0.14.34

`source_scene_actor40_init` completes the actual class40/subtype5 dispatch1
fresh branch at `8010D824 -> 8010C930`. It uses the retained phase5 resource
witness, existing exact initializer prefix, and integer first-pose producer.
After `800322A8` returns, the remaining initializer contains only two global
word stores and the increment of work byte08. The source writes identities
`0x8011C338` and `0x8011C3A4` to `80122470` and `80122474`, then byte08 becomes1.
Those values are source identity tokens, never native pointers. No further
helper or conditional remains in this initializer suffix.

The native transaction publishes work and both globals only when every resource,
reference-table, explicit scratch-shift and pose guard succeeds. Its12 unowned
temporary-stack lanes remain invalid, as in the existing integer pose consumer.
Unknown owners preserve the prior state. Later byte08 values hold; this module
does not implement subsequent movement or animation callbacks.

The fresh wrapper captures actor halfwords14/16/18 before initialization for
source player halfwords2C/2E/30, then captures the same actor offsets afterward
for source player14/16/18. `apply_source_scene_actor40_player_mirrors_8010d824`
writes exactly these12 player bytes and their12 known bits. Other player bytes
and known bits are preserved. This receipt does not assign fixed16.16 positions,
heading, action state or a native physics accumulator.

`source_player_update_gate_8003c07c(player, known)` exposes the exact original
first-player dispatch admission: `player[0] & player[6] & 1` must be nonzero.
It returns no result unless both bytes have explicit known ownership. Original
signed load of byte00 has the same low-bit result for every byte value. Flags at
player+BD select/clear/copy controller input; they do not suppress the following
`8003C418` player dispatch. The original update pass calls `8003C07C` at
`80015DD4`, before the class40 sweep `80017094` at `80015DEC`. Thus the wrapper's
mirror stores follow normal player update. This establishes that local ordering;
it does not establish complete native scheduler chronology or retail parity.

The unchanged owner oracle executes12 complete wrapper cases with the actual18
part resource banks, shifts0/1/2 and position boundaries. It compares216 part
banks, all known actor bytes, both global identities and the entire1524-byte
player work. It executes90,660 CPU instructions and864 restricted MVMVA commands
with zero helper hooks or instruction patches. Another32 authored mirror cases
execute576 source copy instructions. The exact seven-instruction dispatch prefix
is exhaustively checked for all65,536 byte00/06 combinations, executing458,752
unchanged instructions. Raw owner resources and full original outputs stay in
private evidence directories outside the source tree.

Host C++17 warnings-as-errors and ASan/UBSan pass. Native checks include32 mirror
cases,65,536 gate cases, both unknown-gate guards,96 complete-initializer atomic
guards and zero hot allocations. GCC16.1 ARMv6K/VFP hard-float compilation passes
with957 bytes of text and zero data/BSS. Reported static frames are2944 bytes for
the full initializer,64 for the wrapper,8 for the gate and16 for receipt
application. These are per-function frames; the caller, nested prefix and pose
frames require whole-build stack assessment. Hardware and complete scene parity
remain untested.

Registered targets are `test_source_scene_actor40_init` and
`l3probe_source_scene_actor40_init`. The authored test takes
`tests/fixtures/source-scene-actor40-mirrors-01434.tsv`. The owner comparison probe
takes, in order, the owner phase5 package, raw EBD,16384-byte original trig bank,
and private `owner-wrapper.tsv`. Its Release target retains assertions with
`-UNDEBUG`. `scene_actor40_init/source_scene_actor40_init_probe.py` regenerates
private oracle evidence using `--exe`, `--overlay`, `--ebd`, and a new `--out`
directory outside the source tree. Published JSON stores scalars and hashes only.
