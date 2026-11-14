# M3 roadmap: end of the tutorial → first Tron Bonne fight (1.0 end)

Status: **PARTIAL.** Segments S0–S1 are VERIFIED_1TO1 in the PSXRecomp
oracle (US disc, 2026-11-13). Everything after the Stage 3 area 0 exit
has not been observed yet: those segments are UNKNOWN, and the story
order given for them is CANDIDATE. They are kept here only as planning
placeholders, and no constant in them comes from the original.

Scope (user, 2026-10-30): 1.0 ends with the first Tron Bonne fight. M3
starts where M1 hands back control: ST00 event 2 requests **stage 2, area 0**
(event 2 writes `356E = 2`, `356F = 0`, see
`AREA3_BOSS_SOURCE_SLICE.md` on `claude/event3-postwin-90c613`).

## How the oracle was driven

- Binary `psxrecomp-lab/MMLUSRecomp/build-debugtools/MMLUS_Recompiled`, TCP 4370,
  with turbo on. The driver script lives in the session scratchpad and is not
  committed: it wraps `read_ram`, `set_input`, `write_ram`, `savestate` and
  `screenshot_file`.
- **Use `screenshot_file`, not `present_shot`.** `present_shot` only completes
  when the window presents, and its `seq` stays 0 while the window is hidden.
  `screenshot_file path=…` reads VRAM and always writes a 320×240 PNG.
- `savestate` slots are **0..11 only**, and 1–9 belong to other sessions. M3
  uses slot 10 as a scratch slot and archives each milestone as a named file
  (next section).
- Player position is **three int16s** at `0x800B51B0 + 0x14/0x16/0x18`
  (x, y, z), not int32s. Stage is `0x800C1B60`, area is `0x800C1B61`, and
  main phase is `0x800C1B10/11` (`03 02` = in game).
- **Door and exit records:** the per-area list sits behind scratchpad pointer
  `0x1F800050`, in 24-byte records ending at `h[0] == -1` (the port's
  `St00AuxRecord`).
  - `h3` = destination (low byte = stage, high byte = area); for example,
    `0x0003` leads to ST03 area 0.
  - `h4..h6` = the door sample's x/y/z, and `h7` its heading.
  - `h8..h10` = the arrival spawn, and `h2` = the locked-message id.
  - The lock check `0x800519E4` blocks a door when flag `0x7E0` or
    `0x7E0 | (h0 & 0x1F)` is set.
  Reading this list live in each area is the fastest way to find the exits.
- The shared oracle died once with the starvation watchdog while another
  session ran `m1_mac_build.sh`. Don't run the oracle during native builds.
- Oracle-only shortcuts used: position writes to reach a door quickly (noted
  per segment). Flags were never forced.

### Savestates (archive, outside the repo)

`psxrecomp-lab/MMLUSRecomp/saves/m3/<name>.pst`. To use one, copy it to
`saves/openbios/state_8001262C_slot10.pst`, then send `savestate op=load slot=10`.

| Name | Point |
|---|---|
| `m3_00_stage2_event4` | Stage 2 (sky), event 4 running, frame ≈ 18.6k after the slot-9 boss load |
| `m3_01_st03_crash_control` | Stage 3 area 1, crash site, first control (-4566,-1,1402) |
| `m3_02_st03_a0` | Stage 3 area 0, just through the area-1 exit (3028,-1,-10946) |

Existing slot 9 (other session): tutorial boss at HP 24 mid-slam. Thirteen
Buster shots (R1 lock + Square) win it.

## Ordered story segments

### S0 — Tutorial hand-off → Flutter flight (VERIFIED_1TO1)
- **Start:** `0x606` (boss defeated). ST00 event 3 runs about 1,980 frames; flag
  `0x383` is set and `0x606` is cleared, then Area 4 loads.
- ST00 event 2 (poll `0x605`) runs about 8,200 frames (flag `0xC17` is
  cleared during it), then requests stage 2.
- **ST02 (sky):** the Flutter flies over the sea, then smokes and goes down.
  - Overlay `R3_ST02.BIN` with poll function `0x8010024C` and table
    `0x80106C28`: `0x600 → event 4` (the only entry).
  - Event 4 runs about 6,300 frames.
  - Area byte 356F toggles 0/1/3 during the scene: camera or area cuts inside
    one event.
  - Event 4 ends by clearing `0x600`, setting `0x384` and loading **stage 3
    area 1**.
- **Files:** `ST02.BIN` (overlay, map, STG/HED, `ST02_01.EBD`) and the
  `ST02_00/01/02.BIN` area sets (EBD only, `DUMMY.MSG`: no dialogue).
- **Interactions:** none. **Boss:** none. **Save:** `m3_00_stage2_event4`.
- **Port has:** nothing beyond the ST00 events. Events 3 and 2 are ported on
  the unmerged `claude/event3-postwin-90c613`; ST02 has no runtime.
- **Size: medium.** It needs the first non-ST00 stage load, the ST02 sky,
  the Flutter scene actors from the EBD, event 4's timeline, and the
  `0x600` poll.

### S1 — Crash site, ST03 area 1 (VERIFIED_1TO1 up to first control)
- **ST03 event 8:** frames +650 … +2,870 after the stage load. It sets
  `0x0A`, then `0x03` and `0x388`, and ends with control at
  (-4584,-1,1431). Roll, Barrell and Data stand next to the crashed Flutter.
- **Flags at control (< 0xC00):** 0x3, 0xA, 0x1FE/0x1FF, 0x380–0x384, 0x388,
  0x38A, 0x3C7, 0x4C0, 0x527, 0x58A–0x58F, 0x5A3/0x5A4, 0x5C0, and the door
  locks 0x7E1, 0x7E2, 0x7E4, 0x7E8.
- **Area 1 exits** (live `0x1F800050`, list at `0x8011499C`):
  - `h0=0` → ST03 area 0. Sample (3328,-1008,-4864), spawn (3328,-1,-11520).
    **Walked through (VERIFIED)**, from a position write about 560 units away.
  - `h0=0` → ST03 area 2 (`0x0203`), sample (1792,-1008,7936).
  - `h0=8` → ST03 area 7 (`0x0703`), sample (-4856,0,608), heading 0xC00.
- **Area 0 exits:**
  - back to area 1 (`0x0103`);
  - `h0=3` → **stage 0x0C area 0** at the end of a canyon. Sample
    (-2816,0,-21536), spawn (-6912,-1,8576), door id 3, unlocked (0x7E3
    clear). Walking or pressing Circle at the edge (z ≈ -21407 to -21471,
    collision) **did not trigger it**: UNKNOWN gate, possibly a story flag
    or the action check facing the exact sample.
  - `h0=4` → **stage 9 area 1** (`0x0109`), sample (2048,0,-14752), message
    140, **locked by 0x7E4**.
- Talking to Roll or Barrell with Circle/X/Triangle near them started no
  event (`0x800C4C48` unchanged). Their dialogue path is UNKNOWN.
- **Files:** `ST03.BIN`/`ST03B.BIN` (same overlay `R3_ST03.BIN`, two map/STG
  variants). Area sets:
  - `ST03_00` (+B/C/D variants, with Mega Man PBDs `PL00D03M/S`);
  - `ST03_03`, `_05`, `_06`, `_07` and `_08`, each with its own `.msg`.
  `R3_ST03.BIN` has no static flag→event poll table (it uses
  trigger/door-driven events).
- **Save:** `m3_01_st03_crash_control`, `m3_02_st03_a0`.
- **Size: large** (first open field). It needs:
  - the shared door/exit system for every stage;
  - NPCs with dialogue (Roll, Barrell, Data);
  - ST03 events (8 and later);
  - area variants selected by story state (`ST03_00` vs `00B/C/D`).

### S2… — Stage 0x0C / stage 9 and on to Tron Bonne (UNKNOWN)
Not observed. The next oracle session should start from `m3_02_st03_a0` (or
from `m3_01` to find the Roll/Barrell talk). Then:
1. Find what opens door 3 (watch flags while talking to each NPC, and try
   the action check facing heading 0 at x = -2816).
2. Read the `0x1F800050` door list in every new area, and archive a
   savestate at each stage entry.
3. Record events (`0x800C4C48`), flags, shops and fights up to the first Tron
   Bonne boss gauge, then save `m3_xx_tron_fight` before and after it.

Files likely involved (CANDIDATE, from file names only):
- `ST0C.BIN`/`ST0CB`/`ST0CC` (one overlay, three story variants, `ST0C_00.MSG`,
  `PL00D18M/S` in B);
- `ST09.BIN` + `ST09_00/02/03/05`;
- the city/field stages `ST04`–`ST06` with many lettered variants.
Which of these hold the first Tron fight is UNKNOWN.

## Shared systems (build once)

| System | First needed | Size |
|---|---|---|
| Runtime stage loading beyond ST00 (packs are ST00-only; terrain already cooks for all 37 stage sets, per the repository session) | S0 | large |
| Per-stage poll/event owner (ST02 table, ST03 trigger events) on the existing event VM | S0 | medium |
| Door/exit records at `0x1F800050` + lock flags `0x7E0..` + arrival `0x80048898` (ST00 door path exists: `source_door_event.cpp`) | S1 | small–medium (generalize) |
| NPC actors + talk/action check + message blocks per area `.msg` | S1 | large |
| Story-state area variant selection (`_00` vs `_00B/C/D`, `ST0CB/C`) | S1 | medium |
| Save/Continue (memory card) if the route needs a save point before Tron | UNKNOWN | medium |
| Shops/zenny/items | UNKNOWN (not seen yet) | medium |
| Boss framework (gauge, defeat, post-win) reused from M1 class 40/02 | Tron fight | medium–large |

## Recommended order of port sessions

1. **Merge M1** (`claude/tutorial-boss-4a9127`, `claude/event3-postwin-90c613`)
   so that event 2's stage-2 request exists on main.
2. **Stage loading v1:** load ST02 as the first non-ST00 stage from the
   cooked catalog (terrain, STG/HED, EBD). Accept with Azahar reaching the
   sky scene after event 2.
3. **ST02 event 4** (poll `0x600`): the Flutter flight and crash timeline,
   then the stage 3 area 1 request. Compare with `m3_00_stage2_event4`.
4. **ST03 area 1 + event 8:** the crash scene and first control at
   (-4584,-1,1431). Compare with `m3_01_st03_crash_control`.
5. **Generalize doors/exits** to read each area's list (ST03 areas 1↔0 first).
6. **Oracle mapping session S2+** (this document, continued) before porting
   anything past ST03 area 0.
7. NPC/dialogue system, then stage by stage to the Tron Bonne fight.

## Port coverage today
- **ST00:** tutorial Areas 0–3, events 1 (opening), 3 and 2 (post-win, on a
  branch), and the boss intro plus combat pieces (M1, in progress).
- **ST01:** intro crawl, event 71 (`INTRO_EVENT71.md`).
- **ST07:** attract scene, event 10 (`ATTRACT_EVENT10.md`). ST07 is also a
  campaign stage set (`ST07_00`, `_05`, `_05B`); its campaign use is UNKNOWN.
- **Nothing else.** ST02, ST03 and everything past them are missing at
  runtime: stage loading, actors, NPCs, events and doors.
