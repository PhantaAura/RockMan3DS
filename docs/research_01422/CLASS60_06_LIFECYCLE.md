# ST00 class60 subtype06 lifecycle — 01422

Status: SOURCE_BACKED_PARTIAL. The owner EXE and ST00 overlay are read-only and SHA-256 checked by both probe scripts. No executable words, texture pixels, model payload or source UV-table bytes are included in the source release. Authored scalar fixture inputs deliberately differ from retail data.

## Original dispatch and native implementation

Stage initialization installs the ST00 class60 dispatch table in scratch1F800060. Subtype06 resolves to8010F354, with five scalar states8010F47C/8010F674/8010F8E8/8010F9F4/8010FC88. The native module ports their full268-byte work mutations, prior position snapshots, motion80110070, velocity wrap before signed24576 clamp, state-dependent UV selection, packet branches, flicker timing, and child pending release intent. Missing trig, profile, contact, proxy, collision registry, shadow floor/count, audio or effect allocation receipts hold the entire slot. Unsupported bounds and geometry remain held. Unknown states are never invented.

Pool boot8002E8D8 clears32x268 bytes and restores class identity bytes60..7F; ordinary zero initialization does not reproduce these identities. The previously proved allocator/pending release routines are reused. Pending-mask reset and consumer ownership must be explicit in the caller.

Contact8010FDD4 executes the admitted original wall query and source response, including low-velocity floor stop, high-velocity bounce, axis/diagonal reflection, cue179 pointed raw words, and52-byte effect131 primitive copy intent. The stage11/Y=-1 special branch is held. Body release80110164 yields effect128 with parameter66 when byte0A is zero, parameter2 otherwise, copying8 raw position bytes to effect+38. These are allocation/consumer intents; they do not prove the later effect callbacks.

## Corrected consumer identities

80051BAC appends active work pointers to the physical-contact registry and always clears work7C. It is not a renderer. The new bounded registry preserves that append/clear behavior;128 entries is a native safety budget, not a claimed retail constant. The later pair dispatcher80051BE4 is not ported by this module.

8002C8CC emits at most64 shadow/floor records, drops a full queue before querying floor, and packs the supplied radius/scale/type. It is not the child's projectile renderer. The shadow rasterizer is unported.

Actual primitive selection80029AAC admits the normal child flags to80029C18. The primitive contains center90/92/94, half-extent98, UV96/97, CLUT high88 and tpage low88, and color8C. Native helpers reproduce preprojection camera points and packed UV arithmetic, including carries from additions31/7936/7967. Stateful VRAM/material resolution must be reused. Source GTE projection and fog are not proved by these integer fixtures; native presentation remains partial. Rotated/alternate primitive layouts are excluded.

## Verification and limits

Original-instruction probe callback-r3:2160 callbacks;192 motion edge cases;9 primitive cases;723289 instructions including6701 pool-boot instructions. Native tests compare all268 output work bytes. Coverage includes816 release branches,408 original effect128 initializer stores and1008 contact boundaries.

Contact-consumer probe contact-r1:768 authored geometry contexts;730006 original instructions;264 nonzero original wall results. Native admitted660 cases, held108 unsupported cases, matched240 corrected contacts,120 effect131 primitive copies and240 cue179 writes. Registry36 and shadow288 cases matched native output.

No live retail RNG chronology, observed enemy encounter, damaging attack, hardware performance, physical Old3DS validation or 1:1 gameplay claim follows from these authored contexts. Root integration must stage real queues/proxy/effect/audio receipts; a bare readiness boolean is not evidence of a live consumer.

## Commands and fixture arguments

Run owner-side probes outside the source tree:

```sh
python3 -B docs/research_01422/class60_06_source_probe.py --exe /OWNER/ROCK_NEO.EXE --dat /OWNER/CDDATA/DAT --out /NEW/private/callback
python3 -B docs/research_01422/class60_06_contact_source_probe.py --exe /OWNER/ROCK_NEO.EXE --dat /OWNER/CDDATA/DAT --out /NEW/private/contact
```

All new modules and tests were compiled with `clang++ -std=c++17 -Wall -Wextra -Werror`. Link class60 tests against the inherited legends3ds_core library and class60_06.cpp; registry/shadow test needs source_actor_consumers.cpp. Root registers these CTest arguments:

```sh
test_class60_06 tests/fixtures/class60_06_callback_01422.tsv tests/fixtures/class60_06_motion_01422.tsv tests/fixtures/class60_06_primitive_01422.tsv
test_class60_06_contact tests/fixtures/class60_06_contact_01422.tsv
test_source_actor_consumers tests/fixtures/source_collision_registry_01422.tsv tests/fixtures/source_shadow_01422.tsv
```
