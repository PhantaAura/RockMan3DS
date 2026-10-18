# Camera and final player transform, 0.0.14.12

The original executable and paused opening-corridor gameplay agree on the
baseline camera and a final 1/8 player render scale. This supports correction
of the current UV renderer's scale and Y rotation sign. It supports a default
camera reconstruction, with full camera behavior and pixel parity remaining
SOURCE_BACKED_PARTIAL.

The supported owner executable SHA-256 is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`.
The original SLUS00603 session has matching bounded composition tables at
`0x80089928` and `0x80089938`, GP `0x80097864`, and scale 512 at GP+`0x498`.
This corroborates the tested address mapping; it does not claim every byte in
the runtime executable is identical. Original executable, model and texture
payloads remain owner-local and are not part of this evidence.

`research_01412/camera_player_transform_probe.py` executes bounded original
integer instruction blocks using synthetic inputs, after checking the exact
owner executable hash. It passed 256 render-translation cases, 27 Q12 matrix
scale cases, nine Y rotation cases and 54 camera-eye cases. Output includes
block hashes, addresses, measured constants and formulas; no original byte
payload or instruction listing is emitted. It does not emulate GTE or execute
the full original pose/state machine.

The source root yaw at `0x80032648` subtracts raw player work+`0x116` from
work+`0x56`. The original rotation routine at `0x8007b5a8` has Y matrix
`m02=+sinY` and `m20=-sinY`, matching `player_pose_rotation_xyz`.
The raw extra field's gameplay meaning remains UNKNOWN. The native UV path's
previous opposite sign was a source contradiction.

The render preparation at `0x800331a0`, including the translation block
`0x800332ac..0x80033318`, reduces each final part translation as follows:

```text
(unscaled_world_translation + 7*(actor_position << global_shift) + 4) >> 3
```

The shift is arithmetic, then the stored translation narrows to signed16.
The source matrix scaling routine `0x8007ade8` scales each signed16 coefficient
by `(512 << global_shift)` and arithmetic-shifts by 12. Global shift is 0 in
the canonical paused gameplay snapshot. Therefore the native float equivalent
is actor position plus 1/8 of the rotated local posed position. Actor position
must not itself be multiplied by 1/8. The shared helper implements that
relationship, retaining native float math; integer coefficient truncation,
the translation +4 rounding and dynamic scale/shift are explicit limitations.

The camera state begins at `0x800a3958`. Read-only observation addresses are:

| Field | Address | Type |
| --- | --- | --- |
| Radial distance | `0x800a3964` | signed32 |
| Camera yaw | `0x800a3968` | signed32 |
| Camera pitch | `0x800a396c` | signed32 |
| Target XYZ | `0x800a39a0` | signed32[3] |
| Computed eye XYZ | `0x800a39f0` | signed32[3] |
| Previous eye XYZ | `0x800a3a00` | signed32[3] |
| Render angles | `0x800a3a10` | signed16[3] |
| Render camera matrix | `0x1f800008` | source matrix |
| Scratch eye XYZ | `0x1f800028` | signed16[3] |
| Global shift | `0x1f800074` | unsigned8 |
| Actor position XYZ | `0x800b51c4` | signed16[3] |
| Raw actor yaw | `0x800b5206` | signed16 |
| Raw actor work+0116 | `0x800b52c6` | signed16 |

The initializer at `0x80038648` sets distance 512, pitch 48 and target Y
offset 150. The ordinary follow update at `0x80038fa4` uses actor XYZ with
source Y reduced by that offset, and actor yaw plus 2048. Optional state yaw
and pitch terms are added at `0x8003925c..0x8003928c`. The core at
`0x80037fb4` computes its unscaled eye with separate arithmetic Q12 shifts:

```text
eye.x = target.x - (((distance*sin(camera_yaw)) >> 12)*cos(pitch) >> 12)
eye.y = target.y - ((distance*sin(pitch)) >> 12)
eye.z = target.z - (((distance*cos(camera_yaw)) >> 12)*cos(pitch) >> 12)
```

The camera scalar sine/cosine routines `0x8007a060`/`0x8007a12c` return
sin(48)=301 and cos(48)=4085. The packed matrix table returns [301,4084] at
48, so it must not silently substitute for the camera's scalar functions.
For camera yaw 2048, cos=-4096: `(-512*4085)>>12` is -511.
Consequently the baseline eye is behind the actor by +511 source Z units;
the equivalent positive intermediate truncates to 510. A float orbit at
other yaws does not preserve this original asymmetric rounding.

The canonical paused original snapshot is
`work/pass-20261003-visual/ps1-reference/camera_measurements/tutorial_reference_idle.json`
relative to the project root. It records:

```text
actor      [2822,  -1, -4833]
target     [2822,-151, -4833]
eye        [2822,-188, -4322]
d/yaw/pitch[512,2048,48]
renderangles[4048,2048,0]
GP scale512; globalshift0; actor yaw0; raw work+0116=0
```

Thus target relative to actor is [0,-150,0] and eye relative to actor is
[0,-187,+511], also equal to scratch eye. After native source-Y inversion
and the existing world conversion 1/512, the baseline horizontal distance is
511/512, eye height 187/512 and target height 150/512. Original GTE H=384,
OFX=160 and OFY=120 corroborate the projection in a 320x240 game image.
The original player segmentation spans approximately 113 pixels in this
settled initial pose; a size inferred from transient running frames is not
accepted as baseline evidence.

The native viewport remains 400x240 with its center at [200,120], while the
original observed projection is 320x240 centered at [160,120]. Keeping the
vertical focal scale expands the native horizontal field of view. This is a
native viewport adaptation, not a matched original full-frame image. The
original top HP/Buster HUD is absent in native; the mockup-inspired runtime
HUD is drawn separately on the lower screen.

Scripted camera ownership, transition timing, interpolation, obstruction,
all other extra-yaw states, projection rounding and dynamic scale are UNKNOWN.
The default reconstruction is not a complete camera implementation. The
legacy 0143 renderer keeps its original constants. Physical Old 3DS screen,
frame pacing and memory acceptance remain HARDWARE_PENDING.

Reproduce bounded owner-local source checks from the project root:

```sh
python3 -B work/Legends3DS_0.0.14.12_VisualParity/docs/research_01412/camera_player_transform_probe.py \
  owner-data/extracted/ROCK_NEO.EXE \
  work/Legends3DS_0.0.14.12_VisualParity/docs/validation_01412/camera-player-transform-source-evidence.json
```

The separate live linkage evidence is
`validation_01412/camera-player-transform-live-evidence.json`. It records
canonical snapshot/capture hashes and bounded measurements so original data
is checked in place rather than copied into source.
