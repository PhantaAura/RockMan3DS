# Imported actor request bank

Status: **SOURCE_BACKED_PARTIAL** for imported table selection, bounded cache
construction, scalar reset and sequence/pose playback. Complete request-reset
side effects, original callback scheduling and the full AI callback remain
independent gates.

The earlier initializer-selected request0 cache now has a general load-time
counterpart in `actor_animation_bank.h/.cpp`. Its caller provides explicit
request IDs whose call sites have been established. Each ID selects its own
original sequence and frame-pointer tables through the existing bounded actor
decoder. The bank does not infer table lengths, invent animation names, or
substitute request0 when another ID is absent. The byte request domain has a
maximum of256 explicit IDs; each request still has the existing256 reachable
record bound.

The class20/subtype03 source sites currently establish requests0,1,2,4,6,21
and22. The initializer and action0 choose0; actions1,2 and9 choose1; action3
chooses2; actions5 and10 choose4; action6 chooses6; action4 chooses21 and
increments it to22 in phase2. These statements describe table indices selected
by examined code, and do not admit the actions' other unproved inputs. See the
[callback boundary](../research_01417/CLASS20_03_CALLBACK_BOUNDARY.md),
[action0/1 evidence](../ACTOR_CLASS20_03_01415.md), and
[scalar sequence proof](../ACTOR_SEQUENCE_01415.md).

`decode_ebd_actor_animation_bank()` constructs a candidate before the caller
binds it. Duplicate requests, missing pointers, malformed pose windows,
unclosed record chains and a decoded-element budget failure reject the whole
candidate. The caller shares one immutable bank per resource row among actor
instances. Accounting includes the bank, allocated request slots, sequence
records, decoded poses and their translation elements. Embedded sequence
metadata is counted once. Allocator overhead, active pose copies, retained
source data, other CPU storage and GPU resources are excluded; this is not a
total-memory cap.

`actor_animation_request_ready()` preflights a selected request's initial pose.
`actor_animation_reset()` validates the target before copying record0 and its
decoded pose into the existing live storage. `actor_animation_step()` computes
the next cursor locally and validates the destination before committing either
cursor or pose. These functions preserve both on failure and return an explicit
status for missing requests, invalid caches or incompatible part storage. They
never decode, allocate or resize a vector during playback. The checked scalar
`actor_sequence_reset()` also preserves its cursor on invalid record bounds.

The authored `test_actor_animation_bank` fixture uses independently constructed
sparse request tables, including diagnostic ID255 to check the generic byte
domain. It tests exact-cap acceptance, cap-minus-one rejection, late candidate
decode failure, shared lifetime, changed-request reset, unknown-request hold,
invalid next edges, incompatible target storage, countdown wrap and the-1
terminal branch. Global allocation tracking remains unchanged through480
request resets and1,920 sequence steps. These authored cases are not a retail
replay.

The owner-local probe reads the existing cooked area1 EBD, SHA-256
`28bdb4d9e9a7f32e5c5ec7f8cb8e346cfcc46dc69d0a0c4b54deaacb4e3ccd87`,
without recooking or exporting source payloads. Its source-safe result is
[actor-animation-bank-owner-probe.json](../validation_01418/actor-animation-bank-owner-probe.json).
All seven selected IDs form supported contiguous closed sequences:

| Request | Reachable records | Distinct pose windows | Distinct decoded poses |
| --- | ---: | ---: | ---: |
|0|8|8|8|
|1|44|44|44|
|2|6|6|6|
|4|19|19|19|
|6|10|10|10|
|21|19|19|19|
|22|17|17|17|

On the64-bit host, the combined bank owns45,650 estimated element bytes;
mesh and bank together own48,902, within the512KiB area cap. `sizeof`-based
accounting is target-specific. The minimal0/1 bank owns17,280 host element
bytes. It passes960 imported sequence steps with eight request resets, zero
frame allocations and an absent-ID255 hold. Callback cadence remains
**CANDIDATE** and complete reset behavior remains **UNKNOWN**.

The reproducible scalar-only owner check is:

```sh
l3actor_animation_bank_probe EXISTING_PACK/ebd/area_01.l3ebd
```

The [probe source](../../tools/l3actor_animation_bank_probe.cpp) contains no
retail bytes. Its JSON reports counts, memory estimates and validation outcomes,
and does not output original pose, sequence, pointer-table or executable bytes.
