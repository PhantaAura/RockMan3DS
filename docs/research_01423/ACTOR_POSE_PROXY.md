# 01423 actor root collision point and class03 outer proxy tail

Status: **SOURCE_BACKED_PARTIAL**. The proof uses authored work, pose and trig
states with original instructions. It is not a live retail combat trace or an
Old 3DS measurement. Owner EXE SHA256 is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`;
ST00 overlay SHA256 is
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.

## Root producer recovered

Class03 animation update `80032488` calls packed pose decode `80033A28`, then
`8003346C`, which calls `80033598`. The root collision point is not the float
rendered center and does not use the root pose rotation. It uses:

1. Original `8007B5A8` rotation of `[0, heading56, 0]`, copied to work160.
2. Original `8007ADE8` column scaling of that work160 matrix.
3. Signed16 narrowing of each decoded root translation channel multiplied by 8.
4. Original `8007AD38` MVMVA MAC outputs for this matrix/vector.
5. Addition of signed raw14 XYZ shifted by scratch `1F800074`; final stores
   narrow to signed16 at work180/182/184.

Work186 is untouched. The source class03 tail copies all four halfwords, and
masked owner leases can copy both pointed words. The producer therefore requires
an explicit retained work186 value; it cannot fill that lane from a render
matrix or assume zero on slot reuse.

Ordinary scaling shifts each signed work158/15A/15C value and narrows it through
a stack halfword before `8007ADE8`. If work06&40 is set, root scaling instead
uses unsigned class20 bytes388/389/38A:
`arithmetic_shift32(low32(scale * byte) << (shift & 31), 5)`.
ScaleMatrix retains low32 product wrapping before arithmetic shift12 and final
signed16 narrowing. The root MAC sums signed16 products in the GTE accumulator;
MAC output is used, not saturated IR. Three products cannot overflow44 bits.

The new `actor_pose_runtime` module implements this bounded root producer with
resident owner-local `ReferenceTables`. It retains the original CPU rotation
product order even for authored noncanonical zero-angle table entries.
Child rotation matrices, child position work188 onward, rendering and general
GTE flags/projection remain outside the native module.

## Original ownership checks

Class03 initializer `8010244C..80102460` explicitly writes512 to work158,
15A and15C; this matches `ImportedActorInstance::raw_scale`. Its `801024A8`
store clears packet33C and `801024AC` clears flags340. It does not clear
point344/346/348 or word34A independently.

Fresh class20 pool setup `8002E844..8002E850` calls the original clear writer
`800177CC` over all16×3C0 bytes at8009F108. The proof dirties work186 in every
slot, executes that original clear and checks all16 values are zero. The
allocator `80031418` does not clear work186: a separate dirty-sentinel check
proves preservation. Fresh point344/346/348 and word34A are zero through the
same pool clear; on reused slots their previous values remain until an owner
writes them.

Game initialization `80013344` writes scratch shift0. Stage setup calls
`8003697C` at80013638; that helper writes shift0. Alternatives `800369C0` and
`80036A1C` write shifts1 and2. Script paths at8001EF74/84/94 call those helpers.
The probe executes all three setters with an authored projection-helper return
boundary and records the resulting shift. A caller must retain this owner state
or provide an explicit source receipt; a presentation viewport does not select
the scratch shift.

## Exact outer tail

The new `class20_03_proxy` module implements `80102370..801023F4` with the
existing exact registration function. The source gate is work00&1 and
work328&4. If admitted, it copies full pointed8B work180→334 and calls
`800341BC` with extent80, a2=0, mode8000, source work01, target33C,
producer `(U16work330)|04880000`, identity flags00800000 and extra0.
Packet33C clears after the call, including a source full-bank return. It also
clears on a skipped gate. Other outer consumers must have committed before a
caller invokes this tail; missing prerequisites hold all tail mutations.

The persistent target carries flags340, point344/346/348 and word34A. Pair
execution subsequently writes next-frame packets and context. The hit consumer
keeps its packet until the later outer tail, preserving the original chronology.

## Differential validation

`actor_pose_proxy_source_probe.py` executes576 full original producer cases and
864 outer tail cases. Producer cases cover signed heading lookup, authored
noncanonical trig, scale wrap, both scale branches, retained186, source shifts
including MIPS shift masking, and child counts1/3/13 with changed root rotations.
The source child transform loops execute, while only the root outputs are
compared. Tail cases cover both gates, mask/owner copies, producer U16,
packet clear and capacity63/64.

The restricted GTE adapter supports only MVMVA(sf1,mx0,v0,cv3), including MAC and
saturated IR outputs. It excludes RTPS and general FLAG behavior. The proof
executes582,428 original CPU instructions, with2,304 restricted GTE commands
in the producer fixture cases. The standalone native test matches every row
and4096 hot producer/tail ticks allocate nothing.

Pose fixture SHA256:
`7930303b4af8b648d02c5579464a453a704dcd519f7d7011f817e6b4cdd89fd9`.
Tail fixture SHA256:
`f38c594875227e814af9fc5c8976ad616edb1d9d76746c9382423b4531f106c1`.

An additional original chain executes pose→outer tail→paired registration and
pair writes→hit801041B0→audio queue8001A050→outer tail. The same native chain
matches HP97, counter3, action6, cue160 and the final packet clear. Its second
proxy mode1 and producer08000003 are authored premises, **not recovered Buster
semantics**. Earlier player/GTE consumers remain explicit return boundaries.

Reproduce from this source directory, choosing a new evidence directory:

```sh
python3 -B docs/research_01423/actor_pose_proxy_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261008-worldalive-01423/actor-pose-proxy-new
```

No owner instructions, packed poses, mesh data, table payload or executable bytes
are included in the neutral fixtures or this module.
