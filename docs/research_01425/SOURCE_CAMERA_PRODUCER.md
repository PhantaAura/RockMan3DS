# 01425 original scratch camera and projection owners

Status: **SOURCE_BACKED_PARTIAL**. The native module recovers persistent writes
from direct-eye helper `800384F0` and four projection setters plus fog setup.
Current native camera binding remains unavailable. Ordinary follow updates,
interpolation, obstruction, script transitions and camera chronology are not
established by these authored tests.

Owner EXE SHA256 is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
ST00 program overlay SHA256 is
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
The probe guards both inputs. Original owner bytes, disassembly, packed tables
and live private traces are excluded from the source release.

## Concrete owner boundary

Two identified producer paths write the eight words at `1F800008`:

| Original path | Persistent source writes | Required update input owner |
| --- | --- | --- |
| `80037FB4` | Transposed rotation and translated matrix copied at `80038164..80038198`; eye mirror and additional scratch at the later tail | Target XYZ, distance, yaw, pitch, scratch shift, packed matrix trig, separate scalar camera trig and prior eye |
| `800384F0` | Eight matrix words copied at `800385CC..80038600`; narrowed eye XYZ at `1F800028` and `800A39F0`; masked rotation at `800A3A10` | Original direct eye XYZ, yaw, pitch, owner packed trig and retained matrix padding |

This identifies bounded original writers; it is not an exhaustive claim about
every alias store, later overlay or dynamic caller. No direct EXE call to
`800384F0` was found by the bounded JAL scan. A native caller must establish that
the direct-eye owner actually applies before using its result. The module does
not adopt it as the current ordinary camera.

`produce_source_camera_800384f0()` uses complete original `800384F0` as its
differential boundary. It ports its persistent output slice:

1. Store rotation `[(-pitch)&FFF, yaw&FFF, 0]`.
2. Original `RotMatrixYX8007B838` using owner-local packed Q12 trig, retaining
   original low32 products and arithmetic shift order.
3. Original `TransposeMatrix8007B568`.
4. Original `ApplyMatrixLV8007A9B8` applied to low32-negated eye XYZ. The helper
   splits each signed vector into high and low magnitude parts, runs
   MVMVA(sf0) and MVMVA(sf1), then adds the low32 high MAC shifted by3.
5. Copy all eight matrix words and narrow eye XYZ to signed16 for the original
   scratch and signed32 mirrors.

The split-vector algorithm preserves the original INT_MIN negation and high
half narrowing. Replacing it with a single generic matrix product would lose
those semantics. Three signed16 products fit the GTE44-bit accumulator; sf0
MAC results retain low32 wrapping before the original CPU shift.

Transpose writes18 coefficient bytes. Its destination padding halfword at
`1F8000F2` remains retained before the32-byte camera copy, so the API requires
that halfword explicitly. Missing reference table, eye, angle or padding owner
returns no camera/eye output. Neither scratch reuse nor a captured matrix
establishes padding or update ownership. The helper's transient GTE control,
IR/MAC and FLAG side effects are outside this native persistent-output module.

## Projection control ownership

The EXE scan finds ten direct CTC2 writes to controls24..28, all in the initial
GTE setup or these four leaf setters. ST00 program overlay has no direct CTC2
write to those controls. Indirect calls to the setters still require the
actual caller state; this scan does not establish every runtime value.

| Control | Original owner | Exact write |
| --- | --- | --- |
| OFX/OFY24/25 | `8007B118` | low32 of each input shifted left16 |
| H26 | `8007B138` | signed16-narrowed raw CTC2 storage; RTPS interprets H as unsigned16 |
| DQA27 | `8007B0C0` | signed16-narrowed input |
| DQB28 | `8007B0CC` | full raw32 input |
| DQA/DQB27/28 | `8007A1CC` | signed division of low32 `-320*a0` by signed `a1`; DQA narrows through its setter, DQB receives source constant01400000 |

Original initial GTE setup `8007A520..8007A560` writes H1000, DQA-4194,
DQB01400000 and OFX/OFY0. The caller `80011F40..80011F58` then changes OFX/OFY
to160/120 through `8007B118` and H to384 through `8007B138`. Those are
source-owned initialization writes, not proof that the controls remain current
at a later player lease.

Stage shift setters `8003697C`, `800369C0`, `80036A1C` call fog helper
`8007A1CC` with unsigned halfword `800BC81C` (scaled by2 or4 only below the
original13107 limit) and denominator384. `80036A88` also calls it with its
fourth input; `80036B40` reloads stage halfword `800BC81C`; `80037358` receives
the selected record halfword at caller record+0x24. The current stage/record and
script chronology remain explicit owners. The module does not guess DQA from
the renderer's viewport or choose one initialization value permanently.

`SourceProjectionControls` retains each of the five fields independently.
The leaf setters update only their owned fields. `bind_source_projection_controls()`
copies controls24..28 only after all five owners are available. Missing a field
preserves every GTE field. Projection ownership does not bind controls0..7 or
the current scratch camera. Fog setup rejects both original division-break
domains, zero divisor and INT_MIN/-1, without modifying either fog field.

## Authored differential and limits

`source_camera_source_probe.py` executes768 complete original direct-eye
calls and634 original projection helper calls, totaling227,883 original CPU
instructions and1,536 restricted MVMVA commands. Source instructions are not
changed or hooked. The matrix trig inputs are authored, including
noncanonical entries and actual alias indexes; no owner table scalar is
exported. Inputs include full signed32 extremes, low32 product wrapping,
signed16 narrowing and nonzero retained matrix padding.

The CPU instruction differential uses a restricted GTE arithmetic adapter
following the already documented MVMVA model. It compares persistent outputs
only and is not an independent hardware GTE differential or retail camera
parity. The existing original-emulator lease observer reads camera transfer at
`80035508`; that separate boundary does not prove camera producer inputs or
updates.

The standalone native test matches every authored row, verifies ownership
holds and division-break atomicity, and runs4096 camera/fog/control ticks
without heap allocation. The same test also passes the undefined-behavior
sanitizer. Fixture hashes:

- Camera: `a631f5963c9b2c6fd2d7b53ad5b09b08713655d19c514f17a20dd93bf5e81ac4`.
- Projection: `05c597ea15f9232b3346e71873977183df97b1cfb41c16230adad2aa5fcc8df0`.

Reproduce with new output outside the source directory:

```sh
python3 -B docs/research_01425/source_camera_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --st00 ../../owner-data/extracted/CDDATA/DAT/ST00.BIN \
  --out ../pass-20261010-worldalive-01425/player-camera-new
```

The reusable boundary stops here. `80037FB4` live update inputs, independent
camera/GTE observations, player target/window selection `80034EB8`, Buster
lease/muzzle allocation `80035724` and projectile/damage chronology remain
unavailable. This work does not promote native physics or Buster readiness to
1:1.
