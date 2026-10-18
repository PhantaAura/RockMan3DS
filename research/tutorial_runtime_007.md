# Tutorial Runtime Evidence 0.0.7

This checkpoint reconstructs the common EBD key lookup and more of the ST00 auxiliary-event selector from the USA executable. It deliberately keeps unknown gameplay semantics neutral.

## Common EBD directory / lookup

`func_8002F9C4` supplies the currently loaded stage EBD base `0x8016C000` and forwards to common lookup `0x8002FA38`. The lookup reads a 32-bit row count at the EBD base and then walks `count` exact 16-byte directory entries beginning at `base + 4`. Each row contains:

- byte `+0`: normalized object class;
- byte `+1`: subtype, compared with runtime work byte `+2`;
- byte `+2`: optional variant, compared only when the caller supplies a variant pointer;
- byte `+3`: additional key/flags byte, not yet semantically named;
- dword `+4`, `+8`, `+12`: three absolute resource pointers interpreted differently by object family.

The common class normalizer at `0x8002FE74` is reproduced exactly: values below `0x20` become `class & 0x10`; other values become `class & 0xE0`; normalized `0xA0` is remapped to `0x80`.

The stage-object parser `0x8002EA50` copies source bytes `+2/+3` together as a 16-bit value into runtime work `+2/+3`. This matters because original EBD lookup call sites use the optional variant in several distinct ways:

- some pass null, accepting the first class/subtype match;
- `0x8004570C` and `0x800461C0` pass `work + 3`, directly using the copied stage byte `+3`;
- `0x80044184` passes a temporary selector derived from another runtime table;
- `0x8004751C` / `0x80047644` pass temporary selectors selected by runtime state.

Therefore raw stage byte `+3` is a proven *possible* variant source but not a universal rule. The native API keeps variant-aware lookup opt-in.

## Real tutorial EBD validation

The distinct tutorial EBD payloads validate as:

| Payload | Bytes | Directory rows |
|---|---:|---:|
| ST00_00 | 113,808 | 14 |
| ST00_01 | 117,360 | 49 |
| ST00_03 | 101,808 | 40 |
| ST00_04 | 116,896 | 6 |

ST00_02's EBD payload is byte-identical to ST00_01 and areas 1/2 therefore share the same directory for this analysis.

Across all 49 active tutorial stage objects, class+subtype candidate lookup gives **3 unique / 32 ambiguous / 14 common-or-no-local**. Applying raw stage byte `+3` as an opt-in variant gives **28 unique / 21 none / 0 multi** overall and narrows **26 of the 32 class/subtype ambiguities to a singleton candidate**. This is not treated as a final binding because the original initializers do not all source their variant the same way.

The six ambiguous rows that do *not* resolve through raw byte `+3` are all class `E0`, subtype `08`, in area 2. Their source byte `+3` values are `61,81,C1,60,A0,E0`, while the local EBD candidate variants are only `0,1`. This is strong evidence that their runtime variant is derived rather than directly copied; they remain intentionally unresolved pending the relevant initializer trace.

`common/no-local` is not labeled a missing asset. It only means the currently loaded local EBD directory has no class/subtype row; common-engine behavior or a different lookup lane remains possible.

## Original generic object pool geometry and fresh-work seed

The common allocators reached by stage parser `0x8002EA50` use fixed pools. Their exact capacities and work strides are now mirrored by `stage_runtime`:

| Source class | Original allocator | Capacity | Work stride |
|---:|---:|---:|---:|
| 20 | `0x80031418` | 16 | `0x3C0` |
| 40 | `0x80031498` | 4 | `0x5A4` |
| 60 | `0x80031518` | 32 | `0x10C` |
| 80 | `0x80031618` | 64 total (2 × 32) | `0x48` |
| C0 | `0x800316BC` | 16 | `0x24` |
| E0 | `0x80031734` | 32 | `0x50` |

Class 00 takes the original special player-work path and is not allocated from these generic pools. The real ST00 object histogram is `00=3, 20=2, C0=9, E0=35`; peak per-area generic use is **2/16 class-20, 3/16 class-C0, and 15/32 class-E0**.

For a freshly cleared area, the original allocators use first-free slots. `assign_fresh_stage_pool_slots()` reproduces that deterministic order and rejects source lists that would exceed a proven original pool. `seed_fresh_stage_runtime_work()` then creates byte-compatible, pointer-free work mirrors with the exact field copies observed in `0x8002EA50`. The real tutorial seeds **46 generic work mirrors**; the three class-00 records correctly have no generic work mirror. The five source lists total **5,044 bytes of original source-record stride** before allocation.

The fresh seed keeps unknown semantics neutral while preserving the offsets that translated behavior routines will expect. For classes 20/40/60 this includes duplicate raw `+0x0C..+0x13` copies, duplicate 16.16 XYZ positions, the source-byte-7 shifted fields, and observed zeroed fields. Class C0 receives its compact raw tail copy; classes 80/E0 receive the compact raw tail plus signed XYZ halfwords.

## Pointer-free runtime-area bridge

`build_fresh_stage_runtime_area()` now joins every source object with its exact fresh pool allocation, optional work mirror, class/subtype EBD candidates, and source-byte-3 narrowed candidates. The binding state explicitly distinguishes **unique**, **ambiguous**, **no local candidate**, and **ambiguous but narrowed by source byte +3**. This lets the native runtime instantiate all 49 tutorial source objects now while keeping the six proven dynamic-variant cases unresolved until their initializer logic is translated.

## Player firing enable/disable gate

Two tiny original helpers form an important first combat-state gate. `Pl00_shot_enable_off` at `0x800423A0` clears player-work bytes `+0x174` and `+0x177`, clears bit 0 of `+0x175`, copies byte `+0xAC` to `+0x104`, and copies byte `+0xA4` to `+0x100`. `Pl00_shot_enable_on` at `0x800423C8` sets bit 0 of `+0x175`. `player_runtime` translates those byte effects exactly using a 0x5F4-byte compatibility work mirror. The surrounding projectile, targeting, rate-of-fire, and damage logic is still to be translated.

## ST00 auxiliary contact/event selector

The common handler at `0x80050C2C` calls the contact query at `0x800514FC`, which fills signed contact position halfwords `(x,y,z)`, a facing halfword, and the contact/event type used by a jump table. The branches relevant to the current ST00 auxiliary lists are now reconstructed without executing side effects:

- type 1: exact `(x,y,z)` match;
- types 2/4/5/6: exact position plus `((sampleFacing - recordFacing + 0x200) & 0xFFF) < 0x401`, an inclusive `+/-0x200` window;
- type 8: exact position;
- type 3: exact position plus the distinct directional/bounds path below.

### Type-3 exact directional branch

At `0x80050E74..0x80050F14`, type 3 first requires:

`((sampleFacing - recordFacing + 0x200) & 0x400) == 0`.

It then quantizes the sample facing into one of four quarter-turn sectors and compares the sample X/Z coordinate against signed live object-work halfwords at `+0x14/+0x18`:

| Sector | Acceptance test |
|---:|---|
| 0 | `work_18 >= sample.z` |
| 1 | `work_14 >= sample.x` |
| 2 | `sample.z >= work_18` |
| 3 | `sample.x >= work_14` |

The native helper preserves these names as `work_14` / `work_18` because their broader gameplay meaning is not yet proven.

## Current boundary

This closes the structural EBD lookup and ST00 record-selection layer; it does not magically supply enemy AI. The next work is to translate the actual per-frame behavior dispatcher/initializers (including the six dynamic subtype-08 variant cases), finish player collision/state transitions, extend the proven shot gate into Buster projectile + lock-on behavior, and connect stage/camera/audio side effects.
