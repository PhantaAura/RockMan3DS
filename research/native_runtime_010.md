# Tutorial Core 0.0.10 — Native Runtime Bootstrap / Dialogue Bridge

## Evidence policy

0.0.10 was validated against the user's exact USA extraction and the project CHD. The runtime implementation uses disc-derived cooked formats and exact executable reconstruction where proved. Public reverse-engineering is a cross-check; remembered behavior is not used as a substitute for source evidence.

No copyrighted dialogue, raw executable bytes, XA sectors, textures, models, or other Capcom asset bytes are stored in this source tree.

## 1. Pointer-free tutorial EBD runtime data (`L3EB`)

The previous core could parse an original EBD directory on the host but did not carry that proved lookup data into the cooked runtime pack. `L3EB` v1 fixes that boundary.

Each record preserves only the proved directory keys and payload-relative references:

- object class;
- subtype;
- optional variant;
- key/flags byte;
- three source-payload-relative resource offsets (or explicit null).

Absolute PS1 pointers are removed. CRC validation is applied when loading the cooked file.

The five tutorial areas cook to EBD directory row counts:

| Area | Rows |
|---:|---:|
| 0 | 14 |
| 1 | 49 |
| 2 | 49 |
| 3 | 40 |
| 4 | 6 |

Area 2 intentionally reuses the same EBD source relationship already evidenced in the original stage data rather than inventing a new directory.

## 2. Native cooked-area bootstrap

`load_l3st00_flow()` now reconstructs the pointer-free `L3SF` package into native `St00FlowSource` structures. `load_tutorial_runtime_area()` then combines:

```text
st00.l3flow
+ ebd/area_XX.l3ebd
+ original-style pool allocation / work seeding
= TutorialRuntimeArea
```

This is the runtime boundary intended for the 3DS: after the computer-side cooker runs, the native program does not need the original PS1 container pointers or executable merely to instantiate a tutorial area.

Real cooked-pack probe results:

```text
area 0: objects=1  aux=0  EBD=14  work_slots=1
area 1: objects=16 aux=3  EBD=49  work_slots=15
area 2: objects=20 aux=2  EBD=49  work_slots=19
area 3: objects=11 aux=0  EBD=40  work_slots=10
area 4: objects=1  aux=0  EBD=6   work_slots=1
```

The six still-ambiguous area-2 bindings remain ambiguous. 0.0.10 does not convert uncertainty into fake certainty.

## 3. Exact Classic player compatibility routines

Direct translation from the exact USA executable adds:

- `0x80041F54`: signed directional accumulator/integrator;
- `0x80042154`: signed turning accumulator with the original reversal zero crossing;
- `0x80041EF4`: directional input-pair wrapper;
- `0x80042044`: turning input-pair wrapper;
- `0x8004324C`: player state predicate;
- `0x80042528`: class-83 helper work seed.

The earlier exact Buster enable/disable helpers remain intact. The wrappers take formerly-global state explicitly when needed, keeping the portable core deterministic without pretending an unknown global has a semantic name.

## 4. Cooked message loader and runtime cursor

`L3MS` was already a pointer-free cooker format. 0.0.10 makes it executable runtime input through:

- `load_l3message()`;
- `MmlMessageRuntimeCursor`;
- semantic event kinds for proved controls;
- verbatim surfacing of unknown controls.

Real tutorial message probes:

```text
st00_01: blocks=15 glyph=930  newline=41 audio=21 wait=16 next=13 end=12 sel=4  multi=2 unknown=6
st00_02: blocks=21 glyph=1902 newline=80 audio=28 wait=33 next=41 end=13 sel=10 multi=5 unknown=15
```

These counts prove the cooked banks can be loaded and walked by native code. They do **not** claim that original timing, selection branching, XA synchronization, or cutscene ownership is complete yet.

## 5. Regression protection

A new cook against the exact extraction produces **90 files**: the previous runtime pack plus five `L3EB` directories. Comparing every pre-existing non-manifest 0.0.9 cooked file (84 files) against 0.0.10 yields **0 SHA-256 differences**.

This is important: the new runtime bridge is additive and does not silently re-cook already validated geometry, animation, texture/palette, message, or flow data differently.

## 6. Build / platform status

Host Release build and tests pass with warnings treated as errors. The current environment does not have devkitPro/libctru/Citro3D, so no `.3dsx` or hardware-render claim is made from this checkpoint.

The next keeper work toward 0.1 is to connect the cooked/runtime evidence to the actual per-frame stage behavior loop, collision/player states, Buster/lock-on, camera choreography, XA/audio, encounter behavior and cutscene progression, then validate the resulting slice on real 3DS hardware.
