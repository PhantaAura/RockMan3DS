# Source-backed class20/03 wake-up action3 body

`class20_03_step_action3` translates the bounded original ST00 function at
`80102C9C..80102FC0`. It reuses the existing offset-named actor callback state.
This is `SOURCE_BACKED_PARTIAL`, with explicit helper and consumer ownership;
it does not complete the encounter, combat, detection, or the outer callback.

## Source fields and original branch sequence

The body first clears work330 when packet33C has mask02000000. The original
phase dispatch admits0..5; other byte values return after that packet store.

- Phase0 selects request2, stores work64=0, timer32C=5 and selected combat-record
  byteA in work330. It falls through phase1 and decrements the timer that tick.
- Phase1 decrements with signed16 wrap. A negative result sets phase2/timer5
  and immediately enters phase2's decrement.
- Phase2 decrements; a result at or below0 starts phase3 and emits sound intent
  180 through8001A050. Flags328 mask20 selects the **signed** source byte0D as
  the next timer, then clears that mask. Otherwise the timer becomes45. Sound
  and byte0D ownership are preflighted before any scalar change commits.
- Phase3 alone checks the pre-decrement timer below4, then sets phase4/work64=256.
  Phase4 enters after that block and preserves its existing work64. Both then
  decrement. A negative result starts phase5, increments the request byte
  (normally2→3), sets timer10/work330=0 and falls through phase5 that tick.
- A nonnegative phase3/4 result with packet mask02000000 selects action6 and
  clears the complete phase/aux halfword. Otherwise80031BEC computes a turn
  with source work64; the body submits original forward768 to8002FEA4 and
  calls8004B414/80048E6C. Prepared motion must retain that exact heading.
- The floor store updates rawY/fixedY. The33-unit rollback threshold compares
  with the **previous callback snapshot Y**, not the current entry Y. A rollback
  selects action8, clears the phase/aux halfword, sets parameter mask40,
  restores all four raw and fixed lanes, and sets timer-1. Wall contact maskF00
  also sets timer-1. The turned heading survives this position rollback.
- Phase5 parameter mask20 calls80031824. That original helper updates the
  class20 allocation bitmap and clears work00; native resource/allocation
  cleanup remains an external consumer. The reducer emits the explicit release
  intent and scalar clear only after readiness is acknowledged. The timer then
  decrements; a negative result writes work35C=30 and flags328 mask1.

No animationA6 cue is read by this action body. No sound semantics, damage,
attack identity, HP, or post-action behavior has been inferred from these IDs.
Request3 must be decoded from its own original pointer and pose table before
native animation can accept the phase3/4 completion.

## Transaction and runtime integration

Every unknown, stale prepared heading, inconsistent packet, or unavailable
consumer leaves the entire callback state unchanged and returns no intents.
Missing work64 only blocks a branch that actually needs it; phase0 and the
phase3 timer threshold produce their own original value. Missing byte0D only
blocks the flags20 phase2 transition that reads it.

The larger state1 transaction must retain packet33C, bind the selected original
combat-record byteA, stage this action body and the common state1 tail, and
prepare the correct request-specific pose before publication. Action3 motion
preparation must use forward768 and its original turn, resident original trig,
supported wall/floor queries, and stage the wall wrapper's velocity/state74.
It must not reuse action2's forward52 or force an authored successful wall query.

`release_consumer_ready` acknowledges ownership; it does not implement native
pool bitmaps, resource unload, or actor cleanup. Passing it without a real
consumer would invalidate the runtime claim. Likewise sound readiness never
proves that original audio has played.

## Reproducible evidence and limits

Run the read-only probe against the established owner executable and DAT
folder, with a new output directory:

```sh
python3 -B docs/research_01419/class20_03_action3_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT --out /new/local/proof
```

The authored fixture exports67 before/after scalar fields plus prepared motion
and sound/release intents. It includes signed timer wrap, phase fallthrough,
packet paths, delay extremes, the±32/33 floor threshold, complete snapshot
rollback, contact masks and release. It exports no original instructions,
statistics tables, animation or other owner payload bytes. The native test
loads these source-safe scalars and compares every field/intent. Guard checks
also verify absent/stale inputs and480 allocation-free entries.

The original action/turn/fixed accumulator instructions execute in the probe,
but statistics selection, target angle, motion delta, contact, floor, sound and
allocation bitmap have declared authored external premises. Consequently these
fixtures are not live retail traces and are not retail parity. Original consumer
behavior and exact native preparation require their independent runtime proof.
