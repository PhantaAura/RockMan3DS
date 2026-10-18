# 01422 source hit consumer and paired proxy bridge

Status: **SOURCE_BACKED_PARTIAL**. Original-instruction differentials are authored
state comparisons. They are not a live retail combat trace or Old 3DS validation.

## Original owner profile

Read-only `ROCK_NEO.EXE` SHA-256:
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
Tutorial overlay SHA-256:
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
No original instructions, assets, binary, or cooked owner payload is embedded in
these source files. Fixtures use authored fields and record values.

## Hit consumer801041B0

`class20_03_hit.h/.cpp` implements the original scalar/control function through
80104470: packet masks, low24 field, U16 HP subtraction and signed result, previous
HP, byte09-selected wrapped counter, flags, actions6/7/8, phase/aux reset, heading,
work36C/work15E, parameter03 and positional audio request. No guessed damage,
invulnerability, knockback motion, reaction motion or final defeat rules supply
these fields. Source packet33C remains present; only the later outer callback
clears it after its consumers.

An exact matching selected record, packet, health, hit context, helper angle and
audio readiness are required only on branches that read them. Missing branch
inputs hold the entire state. Optional unsigned byte09=0 is known zero, not absent.
Actor variant32E must match the supplied record identity. Owner metadata currently
retains only base actor variant0; the legacy stats-selection identity field
`source_variant` is a table tag, not actor.variant32E.

`class20_03_hit_source_probe.py` executes original hit instructions and the actual
8001A050 audio queue writer. The80031198 angle return is an explicit authored hook.
787 cases execute77,539 original instructions. Native fixture comparison and4096
allocation-free consumer ticks pass. Fixture SHA-256:
`0d7bf4be4aecec31d95e526b7c86f00fdb7bb75d207d7744f1968c1b19651843`.

## L3CBv3

`source_combat_profile.h/.cpp` and `cook_actor_combat.py` retain unsigned byte09
from the same subtype-selected16B stats record, with optional byteC preserved.
Parsed v1/v2 retain their format and roundtrip exactly. V1 has neither optional
field; v2 has optional byteC; v3 uses row10 bits0/1 for byteC/byte09 presence,
row9 for byteC, row11 for byte09. Unknown presence bits and nonzero absent payloads
are rejected. Older encoders cannot discard known new evidence silently.

Two real owner selectors were checked against exact source table selection and
retained byte09 without publishing their values. Eight Python cooker tests and
C++ v1/v2/v3 canonical/malformed/unsigned/roundtrip tests pass.

## Source paired proxy registration and hit packet production

`source_hit_proxy.h/.cpp` implements original800341BC registration with a fixed
64-row bank and source800345A8..80034668 traversal plus800346AC kind0 pair logic.
It preserves quantized extents, packed identity, parallel producer word, mode rule
lookup, source pair order, unsigned packet wrapping, and hit point/word writes.
Class IDs must differ in high8 before pair admission. The selected mode rule is
an owner-local8-byte table at80097D00, loaded through a validated32B L3PR sidecar.
`cook_hit_proxy.py` reads that table from the exact owner profile; no source
constant substitutes for it. Decode is bounded/canonical/CRC checked and atomic.

Registration masks target+4 using current global80098798. If masked bit80 is set,
low8 are cleared before ORing the supplied extra word. Masked high24 owner bits
copy the pointed8B to leased owners+BC. Missing required owner pointers hold the
whole registration. Original global initializer80033EBC sets registration count
800988E0=0 and mask80098798=255 at80033ECC. Source8003545C/800354E0 allocate/release
high owner mask leases; their native ownership layer is not implemented here.

For overlapping kind0 records the target packet ORs other producer high8. It adds
other producer low16 only if the selected bit from its own producer middle8 is
clear: source BLTZ at80034BF4/80034D70 skips addition when that bit is set. Packed
identity bit00800000 stores the other point at target+8/A/C and word1 at+E. For
class03 target33C those are work344/346/348 and work34A. Aliased packet targets
retain source sequential writes.

872 authored native fixture cases match439,900 original instructions; additional
original alias/full-capacity/three-target pair checks raise the total to442,048.
4096 registration/pair ticks allocate nothing. Fixture SHA-256:
`6af78488d0cc7baba15abb6e12147bfa67c7c80cd727cdd2a42e5a16dfaa8305`.
Two L3PR Python tests and native malformed/unknown/atomic/alias/capacity cases pass.

## Exact callback call sites for integration

Class03 tail80102398..801023F4 requires work00bit1 and work328bit4. It copies
work180 pointed8B to work334, then calls800341BC with pointwork334, extent80,
a2=0, mode8000, classwork01, targetwork33C,
producerword=(U16work330)|04880000, identityflags00800000, extra0. It clears
work33C **after registration** at801023F4. Pair traversal produces next-frame
packets. Preserve proxy_position_180; do not substitute presentation floats.

Subtype6 outer8010F354 calls800341BC using work90/92/94, extent32, a2=0,
mode4000 if byte0A!=0 otherwise8000, classwork01, targetwork9C,
producerword08000000, identityflags0, extra128. It separately calls80051BAC for
physical contact.80051BE4 writes physical work+7C, not these hit packets.

## Pending boundaries

- Kind1..3 composite proxies explicitly hold; the kind0 subset does not invent
  segment/capsule semantics.
- The earlier original80034EB8/800358F0 player/GTE consumers and timing80076A84
  are declared return boundaries in the proxy proof.
- Child producer08000000 has no low16 damage. Its registration alone is not
  evidence of Buster strength or a complete player/enemy HP encounter.
- Original Buster/player shot array producers, high owner mask leases, complete
  native outer finalization, actions7/8 motion and final defeat remain pending.
- A combined native proxy→hit-consumer chain test remains pending; this pass
  independently proves both functions and their matching packet/context layout.
- Inherited historical01421 `combat_byte_c_source_probe.py` explicitly expects
  its old v2 cooker output. Manual rerun against current v3 cooker needs a
  compatibility update; its saved source evidence remains historical.
- No live retail parity, playable combat completion, or physical Old 3DS result
  is claimed by this slice.
