# 0.0.15.5 — Oct 19 playtest regression

## Reproduction
- **Recording vs trace:** the recording started at 11:27:18. Trace
  `hardware_01504_4001398034682.csv` started at 11:27:14.68 and samples at
  30 Hz, so recording time ≈ frame/30 − 3.3 s.
- **Fall (trace frames 2141–2216):**
  - Frame 2141, (−141,−1,−5582): the floor query returns nothing on flat ground,
    so the fall branch starts (action 7/4).
  - Frame 2147, (−33,44,−5389): Mega Man is below the floor surface, inside the
    first door slab.
  - Frame 2154, (−33,−1,−5153): he is past the door.
  - Frame 2197, (−317,−1,−4575): floor lost again.
  - Frame 2216, z=−3825: no Area 1 cells remain, contact is NoData, and he
    falls without limit to y=5858.
  - The camera follows him under the terrain, which is the recording's
    "world disappears" view at 74–80 s.
- **Host replay:** replaying the recorded input from frame 2132 (idle at
  (−334,−1,−5750), heading 2620, then Up+turn-left psx 0x0410) through
  `RuntimeSession` reproduces frames 2141, 2147 and 2154 exactly, but only when
  the map initialization is not applied.

## Root cause
- **3DS boot path:** the 3DS build loads the generalized `boot.mmlpkg`, which has
  no section 35. The boot log says `map initialization profile=0 writes=0`.
- **Skipped sidecar:** the loose `st00.l3init` holds the original ST00 startup
  initialization: 994 IDX rewrites, cooked as `stage0_startup`,
  SOURCE_BACKED_PARTIAL. It was applied only when no package loaded.
- **Host tests:** they always applied it, which is why they never showed the
  problem.
- **Recorded path:** all 40 floor differences along it (frames 807–810,
  2001–2040, 2141–2202) fall in IDX 62, 78, 123 and 138, which the
  initialization rewrites.

## Fix
- **Helper:** `apply_sidecar_map_initialization` applies the sidecar L3MI to a
  package without section 35, checked against the package's own map/render
  section CRCs.
- **3DS build:** it calls the helper only after proving those sections
  byte-identical to the sidecars.

## Results
| Check | Result |
|---|---|
| `l3door_replay ST00.mmlpkg PACK --no-init` (0.0.15.4 behaviour) | FAIL: floor lost at replay frame 9 = trace 2141; 31/40 floorless; ends past the door at (−151,−1,−5153) |
| `l3door_replay ST00.mmlpkg PACK` (0.0.15.5) | PASS: 994 writes; 0 floorless or airborne frames; 24 door-slab contacts; stops at (−325,−1,−5409) |
| Host suite | 147/147 PASS (map_initialization_package extended) |
| Area 1 audit 200×3000 | 0 escapes, 0 falls, 0 unsupported |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 927,256 B, SHA-256 `4f88021476ef64805792fe256dbdc333dbe46d6f75d68e389fa055144feec639` |
| Interactive Azahar replay | **NOT RUN**: the Mac screen was locked, so macOS refused key input and window capture. The build is installed on Azahar's SD. |
| 32 s / 44 s camera events | UNVERIFIED. The trace near frames ~1060 and ~1420 shows normal grounded movement with no floor difference. The initialization also rewrites render bank selection for every IDX, so missing or wrong walls are a candidate, not a proven cause. |
| Physical Old 3DS | Not run |

## Not the cause
- No deep-overlap recovery fired.
- Contact status stayed SourceBackedPartial until the grid ran out.
- The door (event code 0x301 at (−256,0,−5376), aux record 3 → Area 2) opens
  only on a button press in the original (0x80051970). Its walk-through
  behaviour is unported, but it did not cause this failure.
