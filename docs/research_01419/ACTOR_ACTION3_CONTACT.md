# Original action3 contact preparation and pointed sound producers

`prepare_actor_action3_contact` supplies the independently owned movement and
contact values required by the action3 reducer. It is `SOURCE_BACKED_PARTIAL`,
read-only and allocation-free. It preserves exact source constants and holds
unsupported geometry instead of inventing an unobstructed move or floor.

## Admitted motion and transaction ownership

Preparation admits only action3 phase3/4 when the original signed16 wrapped
timer decrement is nonnegative and packet33C mask02000000 is clear. Other
branches return `NotMovingBranch` without a prepared input. Phase3's original
pre-decrement timer below4 produces turnlimit256; phase4 preserves source
work64. An unknown limit or target heading remains unknown.

The caller stages current raw/fixed into the previous callback snapshots as
8010226C originally does. The supplied contactwork must match both mirrors;
all four lanes, velocity and state74 are retained in the preparation snapshot.
The source turn helper computes heading with that exact limit. Resident
original reference data supplies the **forward768** fixed delta, then signed32
wrap and original80030150 rounding update the first three raw lanes. Padding
lanes retain their original bits.

Actual bounded8004B6AC/8004B414 wall queries/mutation and80048EA4 floor queries
must succeed. The preparation includes post-wall raw/fixed/velocity/state74,
original contact word, floor word and turned heading. The action3 reducer owns
the subsequent rawY/fixedY floor stores, source33-unit threshold and full
previous-snapshot rollback. Preparation never publishes an actor/pose move.

Until the inherited contact gather's edge behavior is repaired independently,
queries with any neighbor outside the active area are explicitly held. That
prevents the known continue-versus-original-early-return mismatch from being
admitted as faithful contact. Unsupported slope and response families also hold.

The runtime transaction must validate initial action, phase, timer, work64,
packet, heading, target and all contactwork fields against the active actor
before accepting prepared values. Consumers, pose reset/advance and common
state1 tail remain part of the larger atomic transaction.

## Exact sound176/180 producer arguments

Action2 calls sound176 at80102B3C with **a1=actor_work+14 in the delay slot**,
before its motion call. Action3 phase2 calls sound180 at80102D8C, with the same
pointer set at80102D84. Neither caller supplies invented coordinates.

Original8001A050 copies both32-bit words at a1+0 and a1+4 to queueentry+4/+8,
sets typebyte0=1 and the sound ID at halfword2, then advances the producer by20
bytes. Thus the payload comprises all four original raw shorts, including the
fourth lane. `actor_source_sound_position_words` packs those words portably.
It must consume the **pre-action** raw14..1B snapshot. Using committed action2
coordinates would shift the sound source after the original producer call.

When the original producer pointer equals800BE6D8, the enqueue is suppressed
and the action body still continues. This is distinct from an unknown or
uninitialized consumer. Proving queue production does not prove original audio
assets, decoding, mixing, spatialization or audible output.

## Validation

The read-only source probe executes the original action3 body through the
point after the floor query and before its stores. Target heading and GTE delta
are declared premises; original turn, accumulator, rounding and complete
wall/floor instructions execute. The native comparison admits **832 exact
matches out of1,152 authored preparations**, including208 wall corrections;
320 unsupported cases are held. **All24 materialized owner-stage preparations
match**, using resident owner reference.l3ref.

**12 original sound producer plus queue-writer cases** prove176/180 arguments,
pre-motion words, fourth-lane retention and full-queue suppression. Malformed
or missing preparation inputs are guarded, and640 repeated preparations make
no allocations. Warnings-as-errors compilation passes.

The public authored fixtures contain no owner map/texture/model/audio payload.
The private stage fixture retains owner bounds/positions/trig context outside
source delivery. These are authored/materialized comparisons, not live retail
parity, complete AI/combat or physical Old3DS validation.

```sh
python3 -B docs/research_01419/actor_action3_contact_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT --out /new/local/proof
l3test_actor_action3_contact tests/fixtures/actor_action3_contact_01419.tsv \
  tests/fixtures/actor_sound_pointed_01419.tsv \
  /local/proof/actor-action3-contact-owner-stage.tsv /local/owner-pack
```
