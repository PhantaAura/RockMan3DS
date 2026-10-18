# Actor body, hit packet, original shadow and proxy commit

Status: **SOURCE_BACKED_PARTIAL**. This connects existing source-backed consumers in the native actor path; it does not establish live firing, complete enemy combat or retail chronology.

The inherited actor bridge held every nonzero proxy packet before body/animation. Its later pose/proxy API also held pending packets, so even a source no-damage `08000000` packet had no safe continuation. Clearing it early would lose source behavior or permit repeated HP/audio on a failed later consumer.

`tick_imported_actor_state1()` now accepts an optional explicit `ImportedActorProxyTailContext`. It stages the existing hit/body, queue writes and resident animation choice; computes the original fixed-point collision root for that selected pose; plans the actual shadow producer and preflights the original proxy finalizer. Only after all consumers succeed does it publish body, pose, audio, child allocation, shadow record, proxy record/leased BC-C0 and packet clear. The original separate/default API remains guarded. Native caller passes the existing shared child shadow/proxy queues, current collision map/reference tables, area index and explicitly admitted fresh source mask. It avoids repeating the independent tail when `proxy_finalized` is true.

Read-only shared preflights preserve the source registration, shadow and animation math. No new HP, projectile strength, invulnerability, reaction, defeat, physics, event or RNG constants were supplied. Missing source data continues to hold.

## Original shadow owner

Narrow original tail `8010234C..80102370` proves JAL `80102368` calls `8002C8CC(work,48,512,0)`. The 128 authored cases execute that actual caller and shadow queue writer. Only the floor helper `80048E6C` return is an explicit authored boundary. Eight cases consume that return; full queues and nonzero work76 skip it exactly as the original. All 16 dirty work76 sentinels are cleared by original cold pool setup `8002E840..8002E854`. Total: 15,639 unchanged caller/shadow/clear instructions. No executable/overlay bytes or instruction listings are distributed.

The native integration queries the current map with actual actor bounds, current/previous position mirrors and source class before appending the source shadow record. Unsupported floor geometry holds. `source_shadow_work76` is UNKNOWN by default; the existing explicitly fresh pool/pose subset can admit its proved zero. Warm/recycled/untracked state cannot infer zero. A full 64-record shadow queue bypasses work76/floor reads as source does. Shadow rasterization remains absent.

The initial inference that cooked spawn raw0=1 meant shadow was disabled was wrong: the original initializer ORs bit2. It was withdrawn. The owner witness confirms initialized work00=3, and the implemented shadow transaction handles that actual branch.

## Verification and precise limits

The final always-on actor regression covers actual existing parent allocation and child callback through the shared pair producer, an imported actor body/pose/tail continuation, 128 original shadow rows, missing mask/leased-point/work76/HP guards, source full-bank/full-shadow behavior, future animation-root selection and no hot-loop allocation. The child chain uses authored rules. A separately authored damage packet exercises the existing exact hit reducer: HP64→63/audio/clear commit together; retrying a held later tail applies no partial damage, and the next callback does not apply it twice. These are deliberately authored scalar premises, not genuine player Buster firing.

Warnings-as-errors standalone regression passed. Six targeted inherited CTest targets passed before the final positive shadow/fixture additions; the parent pass must rebuild and rerun `actor_source_transaction` for final integrated CTest acceptance. Results are in `actor-transaction-r5.log` and `actor-tail-evidence.json`.

The owner-local existing package witness decoded real objects, original five-part meshes/poses, map, references, combat-relevant detection and child/rule profiles. Five supported initialized records across two tutorial areas committed body plus shadow, and the area1 record registered its original proxy. Two actual child callbacks committed, but actual owner rules admitted **zero pairs**; other children remained held. It therefore does not claim a real enemy encounter or a source projectile hit. A separate explicitly authored `08000000` packet on the real area1 actor continued and cleared with shadow and proxy; four area2 cases correctly held at the missing original predicate. The far player and copied attack-phase premises are explicit diagnostics. See the two separate original witness logs.

## Commands

From the cumulative source directory; choose new private output directories for the probe:

```sh
python3 -B docs/research_01442/actor_shadow_tail_probe.py --exe /OWNER/ROCK_NEO.EXE --dat /OWNER/CDDATA/DAT --out /NEW/PRIVATE/shadow-proof
cmake --build /ABS/HOST --target test_actor_source_transaction test_source_actor_consumers test_actor_pose_proxy test_source_hit_proxy l3test_actor_animation_bank test_child_runtime -j4
ctest --test-dir /ABS/HOST -R '^(actor_source_transaction|source_actor_consumers|actor_pose_proxy|actor_pose_proxy_runtime|source_hit_proxy|actor_animation_bank|child_runtime)$' --output-on-failure
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I include docs/research_01442/owner_area_actor_witness.cpp /ABS/HOST/liblegends3ds_core.a -o /NEW/PRIVATE/owner-witness
/NEW/PRIVATE/owner-witness /OWNER/EXISTING_COOKED_PACK
/NEW/PRIVATE/owner-witness /OWNER/EXISTING_COOKED_PACK --authored-packet
```

No owner payload, assets, executable, DSP component or save is included. PHYSICS_1TO1 remains false. Actual native build/emulator/device results belong to the parent pass; this document makes no new physical hardware claim.
