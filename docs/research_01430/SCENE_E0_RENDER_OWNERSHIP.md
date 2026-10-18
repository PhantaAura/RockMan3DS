# Scripted E0 resource to draw ownership

Status: **SOURCE_BACKED_PARTIAL**. This is a native single-resource bridge into
existing PBD render elements, with exact first-callback stores. It is not complete
scene animation, live retail parity, GTE raster parity, or hardware proof.

The frozen0.0.14.29 constructor establishes15 logical E0 records with actual
retained EBD pointer identities. Its native draw loop consumes only the separate
class20 ActorAreaRuntime. Therefore none of those15 new logical records can be
rendered by merely retaining the EBD bytes. Their original update/draw consumer
and native rendering bridge are required.

## Original owners

All addresses refer to the supported USA owner executable and ST00 overlay;
full identities are recorded in `scene_e0_source_proof.json`.

- Stage80100324 stores the E0 subtype dispatch table8011A6CC at scratch1F80006C.
  Subtype3 selects80112B9C.
- E0 update sweep80017528 walks32 slots of80 bytes at800BF118. It invokes the
  subtype callback only when `(work[0] & work[6]) & 1` is nonzero.
- The first subtype3 callback80112BDC ORs flags with6, or14 for logical variant0
  (also10 in the original;10 is outside this admitted current profile). It sets
  the signed12 identity rotation matrix and zero trailing matrix words at+18
  through+34, then increments state byte+08. Variant8 has a separate resource
  lookup; it is deliberately unsupported here. No later variant callback is
  approximated.
- The separate E0 draw sweep80029B94 tests `(work[0] & work[6]) & 2`. It selects
  an EXE draw routine from table800898D0 using `work[0] & 1C`, and passeswork+14.
  Initialized flags7/F both select8002A160.
- Consumer8002A160 reads the retained resource pointer atwork+14, the matrix at
  work+18, integer world position atwork+38, and modulation atwork+40. It selects
  resource+70/+74/+78 by source camera-space depth: Near<=700, Middle701..1800,
  Far>=1801. Each pointer names a20-byte mesh header followed by a20-byte part.
- Consumer8002A6E8 reads that one part's triangle/quad counts, packet pointers,
  material word and vertex pointer. Vertices are8 bytes; triangles/quads are12
  bytes. UV and index lanes match existing neutral PbdTri/PbdQuad storage.
  This establishes packet compatibility only, not the class20 hierarchy,
  animation request maps, or pose banks.

No raw resource extent is inferred from the next directory pointer. All source
pointer windows and packet indices are checked independently before decoded
allocations. Dynamic part selection(flag20) and other draw dispatches hold.

## Native API and lifetime

`load_scene_e0_resources(directory, profile, witness, cap)` requires the actual
current phase5 SHA/directory/first-match ownership witness. It decodes only the
selected15 records and3 tiers, deduplicating descriptor/part identities. The
immutable owner lease is weak: package unload/replacement invalidates admission.
A same-byte new payload must receive a new explicit binding. The caller must
also clear the decoded cache on accepted unload/transition/profile replacement.

`initialize_scene_e0_first_update_80112bdc(work, cache, directory)` is atomic and
allocation-free. It handles only state0 of the admitted variants0..7. State1
returns `NeedsVariantBehavior`; it does not replay initialization each frame.
The caller must independently establish the original actor-sweep order relative
to event construction. Calling it inside a constructor for convenience is not
source chronology evidence.

`scene_e0_draw_view(work, cache, directory, tier)` is allocation-free and requires
owned known flags, resource, matrix, position and modulation fields. Its view
borrows a cached PbdPart and preserves source matrix/position/modulation/flags.
The caller supplies a tier from an owned source camera-space depth, or explicitly
labels a frontend diagnostic override. The source material word remains raw;
PS1 texture page/CLUT state remains the existing stateful resolver's concern.

The cache's budget covers fixed structures, vector capacity and decoded element
capacity; it excludes allocator bookkeeping and GPU/texture allocations. There
are no hot-loop allocations. It does not preempt original blend/depth ordering.

## Evidence and commands

Authored differential fixture: `tests/fixtures/scene_e0_initializer.tsv`.
It contains32 authored80-byte work cases, not original game payloads.
The unchanged original initializer executed1052 instructions with zero hooks or
patches. The native adapter matches every output byte in all32 cases.

Read-only oracle, with owner inputs outside source:

```sh
python3 docs/research_01430/scene_e0_owner_probe.py \
  --exe /path/to/ROCK_NEO.EXE --st00 /path/to/ST00.BIN \
  --out /new/private/scene-e0-evidence
```

Host checks:

```sh
./test_scene_e0
./test_scene_e0 --fixture tests/fixtures/scene_e0_initializer.tsv
./test_scene_e0 /owner/source_event_phase5.l3p5 /owner/ebd/area_00.l3ebd
```

The actual current owner package locally passes15 constructors/first callbacks
and45 tier draw admissions:13 unique decoded meshes,228 unique vertices,
16 triangles and162 quads. Its host decoded cache is11000 bytes; all15 instances
would submit460 triangles before culling. These are measured source/cache counts,
not Old3DS memory or performance measurements.

The owner-local unchanged-instruction first-sweep proof executed26 stage-table
stores,6050 constructor instructions,1432 first E0 sweep instructions, and22 draw
sweep-prefix instructions, zero hooks/patches. The draw prefix stops before the
GTE consumer. This uses declared cold/authored retained controller state, not a
complete NewGame/live-frame chronology.

Authored host tests also cover3LOD storage and exact thresholds, source fields,
15 callbacks,8 atomic holds,5 draw holds,9 malformed resource windows/indices,
cumulative memory cap, profile mutation, same-byte payload replacement, unload,
and4096 allocation-free lifecycle holds.

## Remaining blockers

Later variant callbacks, exact sweep scheduling, dynamic selected parts, depth
shade/CLUT changes, original semitransparency/ordering, animated poses, full
scene completion and physical Old3DS tests remain independently unverified.
No decoded resources, original vertices, textures, code or owner saves are
included in source checkpoints. The safe0143 renderer remains independent.
