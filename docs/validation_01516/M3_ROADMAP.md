# M3 roadmap: end of the tutorial → first Tron Bonne fight (1.0 end)

Status: **PARTIAL.** Segments S0–S7 are VERIFIED_1TO1 in the PSXRecomp
oracle (US disc, 2026-11-13/14): from the tutorial hand-off through the
Citizen's Card and the kids' "secret meeting" at the Apple Market north
gate. The pirate attack and the first Tron Bonne fight (S8+) have not been
reached: those are UNKNOWN, and nothing in them comes from the original.

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
- Oracle-only shortcuts used: position writes to reach a door or NPC quickly
  (noted per segment). Flags were never forced.
- **Position writes must cover every copy:** int16 x/y/z at `+0x14` and
  `+0x2C`, and the 16.16 high halves at `+0x1E/22/26` and `+0x36/3A/3E`.
  Writing only `+0x14` is undone on the next frame.
- **Heading** is `+0x56` (12-bit; 0 faces -z, 0x800 faces +z; facing vector
  `(-sin h, -cos h)`). **Action/talk is Circle** (game mask `0x20` at
  player `+0x136`). In menus X confirms; in Roll's workshop (main phase `05xx`)
  Start leaves.
- **Doors** are type-3 event samples used with Circle while facing within
  ±0x200 of the record heading `h7`, standing on the near side. Walking into
  them does nothing; that's why door 3 "didn't trigger" in the first pass.
- **Talk detection:** player action `+9 == 0x10` while a text box is open;
  NPC talks run as event `0xFF` (`0x800C4C48` = `01 ff`).
- **NPC table:** 16 slots of 0x3C0 bytes at `0x8009F108` (position at
  `+0x14`, class at byte `+2`, handler at `+0x10`). The talk target is the
  nearest entry with a handler within 208 units (`0x800413F0`, written to
  player `+0x184`).
- `fntrace` works only on native EXE functions: the flag test `0x8001DA58`
  (set `0x8001DA8C`, clear `0x8001DCD0`) can be traced, overlay functions can't.
  Tracing `0x8001DA58` for 30 frames lists the flags an area is waiting on.

### Savestates (archive, outside the repo)

`psxrecomp-lab/MMLUSRecomp/saves/m3/<name>.pst`. To use one, copy it to
`saves/openbios/state_8001262C_slot10.pst`, then send `savestate op=load slot=10`.

| Name | Point |
|---|---|
| `m3_00_stage2_event4` | Stage 2 (sky), event 4 running, frame ≈ 18.6k after the slot-9 boss load |
| `m3_01_st03_crash_control` | Stage 3 area 1, crash site, first control (-4566,-1,1402) |
| `m3_02_st03_a0` | Stage 3 area 0, just through the area-1 exit (3028,-1,-10946) |
| `m3_03_st0c_a0` | Stage 0x0C area 0 (forest path), arrival (-6912,-1,8576) |
| `m3_04_st03_police_arrive` | ST03 area 1 after event 8 #2 (police car), flag 0x4 |
| `m3_05_st03_inspector_done` | ST03 area 1 after the Inspector scene, flag 0x0 |
| `m3_06_st04_a0_arrive` | Stage 4 area 0 (Apple Market), first entry, flag 0xD |
| `m3_07_st03_roll_parts` | ST03 area 1 after Roll's "find parts" talk (0x606) |
| `m3_08_st04_junk_quest` | Junk shop (stage 4 area 1) after accepting the quest, flag 0xE |
| `m3_09_st09_a0_ruin` | Stage 9 area 0 (ruin) entrance, flag 0x16 |
| `m3_10_st09_husband_trapped` | Ruin, junk-shop man cornered, 8 enemies (class 0x19) |
| `m3_11_st09_husband_saved` | After event 0x17 (rescue), flags 0xC, 0x397 |
| `m3_12_st04_mine_parts_kit` | Junk shop after the reward (Mine Parts Kit, 0x2F3) |
| `m3_13_st03_citizen_card` | ST03 area 2 after events 0x60/0x61 (Citizen's Card, 0x23/0x46) |
| `m3_14_st04_kids_meeting` | Apple Market at the north gate, kids' secret meeting |

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
    clear). It opens with Circle while facing -z (heading 0 ±0x200) at
    x ≈ -2816 (see S2); the first pass approached at an angle.
  - `h0=4` → **stage 9 area 1** (`0x0109`), sample (2048,0,-14752), message
    140, **locked by 0x7E4**.
- At first control the NPCs only give flavour lines or Data's menu; the
  story continues through the S3 trigger box. (The first pass also faced
  them with the wrong heading field.)
- Area 1's exit to area 2 is reached along the path past the trigger box;
  a straight line from the Flutter hits trees.
- The Flutter door (`h0=8`, message 6) stays locked by 0x7E8 through S7.
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

### S2 — Stage 0x0C forest path and the sealed ruin (VERIFIED_1TO1)
- ST03 area 0 door 3 (`h0=3`, sample (-2816,0,-21536), heading 0, unlocked):
  face -z and press Circle → **stage 0x0C area 0**, spawn (-6912,-1,8576).
  No arrival event.
- Stage 0x0C doors: back to ST03 area 0; `h0=2` → stage 0x0E area 0 (sample
  (3072,0,5264), heading 0x800), **locked by 0x7E2**. A sealed ruin door with
  a guard ("Haven't seen you around before... are you a Digger? ...it's been
  sealed off as long as anyone can remember").
- Optional for the story so far: nothing here sets a flag.
- **Save:** `m3_03_st0c_a0`. **Size: small** (one area, one NPC, a locked
  door) once stage loading and doors exist.

### S3 — Police arrive, Inspector, Barrell leaves (VERIFIED_1TO1)
- Other ST03 doors at this point: area 0 `h0=4` → stage 9 area 1, locked
  0x7E4 (sign: Class B Digger's License required, by order of the Kattelox
  mayor); area 2 `h0=1` → stage 4 (city "N" gate), locked 0x7E1, with an
  officer ("wait until you've spoken with the Inspector... gone to
  investigate the crash site"); area 2 `h0=2` → stage 9 area 0, locked 0x7E2.
- **Trigger box** in the ST03 overlay (`0x80100ADC..`): when 0x3 is set and
  0x4/0x605 are clear, standing inside the 512-unit cell box at
  `0x80114F2C..F` (x -2560..-1536, z 5632..6656, on the path from the
  Flutter toward area 2) sets 0x605 and runs **event 8** (`0x800C4C4C=1`).
  Event 8 here: "...?", the police car drives in; ends setting **0x4**.
- NPCs then: Inspector (class 0x26), officer (0x24), and Barrell in slot 1
  (class 0x25, handler table `0x80117134`). Talking to the Inspector or
  officer gives one line. **Talking to Barrell** runs event 8 again (third
  use, `0x800C4C4C=2`): the Inspector welcomes them to Kattelox, says Mega
  Man needs registration to enter the city, Barrell leaves with the police.
  Ends setting **0x0**.
- Talking to Roll afterwards sets 0x606 (stage-local): "look for parts at
  the shopping area the Inspector mentioned". Data (class 0x11) is the
  save/recharge menu (event 0xFF; Recharge Energy / Save / Tell me / Cancel).
- **Saves:** `m3_04_st03_police_arrive`, `m3_05_st03_inspector_done`,
  `m3_07_st03_roll_parts`.
- **Size: medium.** Needs the ST03 trigger-box/event-8 selector (three
  variants by flags 0x3/0x4/0x604/0x0), NPC talk, and the police scene.

### S4 — Apple Market (stage 4) (VERIFIED_1TO1)
- ST03 area 2 → city gate now unlocked (0x7E1 cleared) → **stage 4 area 0**,
  spawn (-3584,-1,8704); entering sets **0xD** (`0x801005F0`).
- Stage 4 area 0 doors: back to ST03 area 2; `h0=1` → **stage 5 area 0**
  ("N" gate, sample (-3584,0,2880), heading 0): sign "Please insert
  Citizen's card", locked 0x7E1. Shops `0x0104..0x0504` → stage 4 areas
  1–5 (area 1 junk shop, 2 electronics, 4 clothing; 3 and 5 stayed shut).
- Townspeople: flavour lines, plus a hint about "the ruins just outside the
  gate" (no license needed) and that the junk-shop owner goes there.
- **Junk shop lady** (area 1): her husband is out on a dig; Yes → sets
  **0xE**. This clears ST03's 0x7E2 (ruin door in area 2).
- **Saves:** `m3_06_st04_a0_arrive`, `m3_08_st04_junk_quest`.
- **Size: large** (first town: shops, many NPCs, 6 interiors, per-stage
  lock table).

### S5 — First ruin, rescue (stage 9) (VERIFIED_1TO1)
- ST03 area 2 `h0=2` → **stage 9 area 0** (ruin, minimap on), spawn
  (17152,-769,4992); sets **0x16**. Doors: back, `0x8003` → area 1 (id
  bit 0x8000 = real transition), area 2, and two local doors.
- Near (22794,-577,7424) the junk-shop man (class 0x13) shouts "Help me!!"
  (temp 0x609/0x60A); eight class-0x19 enemies on the floor below
  (y -385, around (22.8k, 8.4k)). Buster lock-on from among them clears them;
  each defeat sets one of 0x619–0x620. (Locking on from his ledge targets
  him instead: "Ouch!")
- Talking to him then runs **event 0x17**: thanks, "come by my junk shop",
  he runs home. Sets **0xC**, **0x397**.
- Position shortcut used: teleport to the rescue spot and next to the
  enemies; the ruin's layout between the entrance and there was not walked.
- **Saves:** `m3_09_st09_a0_ruin`, `m3_10_st09_husband_trapped`,
  `m3_11_st09_husband_saved`.
- **Size: large** (first dungeon: minimap, Reaverbot class 0x19 AI and
  defeat flags, NPC-in-danger scene).

### S6 — Reward, Roll's workshop, Citizen's Card (VERIFIED_1TO1)
- Junk shop: the man gives the **Mine Parts Kit** (0x2F3; also 0x22, 0x24)
  and the old support car outside the city walls (Buster-part exchange).
  The shop becomes a shop.
- The crash site is now empty; Roll and Data are at the support car in
  ST03 area 2 ((5870,19954) / (6078,19844)). Talking to Roll runs **event
  0x60**: no parts in town, so they'll dig; she spots the weapon kit. Sets
  **0x1A**, **0x45**, then opens **Roll's workshop** (main phase `0505..07`,
  inside the car: Splash/land-mine explanation, "explain again?" Yes/No).
  Start leaves.
- **Event 0x61** follows: an officer brings the **Citizen's Card** (0x23,
  0x46), warns pirates are heading this way, civilians are being evacuated
  and gates locked; Roll says Gramps is still in the city.
- **Saves:** `m3_12_st04_mine_parts_kit`, `m3_13_st03_citizen_card`.
- **Size: medium–large** (item/key-item system, Roll's development menu,
  two events).

### S7 — Kids' secret meeting at the north gate (VERIFIED up to the block)
- Stage 4 now has no locks. At the stage 5 gate three kids hold a "secret
  meeting" ("Get outta here!"); Mega Man: "If I'm careful, I should be able
  to get close without them noticing"; tutorial: Circle + direction walks
  slowly (player action 1).
- Sneaking straight up the street at slow walk is caught at z ≈ 3580, and
  Mega Man is put back to (-3600,-1,3600). A teleport to z 3050 is caught at
  once. **How to get past them is UNKNOWN** (approach angle or timing, or
  another trigger).
- **Save:** `m3_14_st04_kids_meeting`.

### S8… — Pirate attack and first Tron Bonne fight (UNKNOWN)
Not reached. The next oracle session should start from
`m3_14_st04_kids_meeting`:
1. Solve the sneak (try approaching along the walls or from the side, and
   trace `0x8001DA58` while sneaking to see what flag the kids test).
2. Go through the stage 5 gate with the card; record events and flags up
   to the first Tron Bonne boss gauge, and save `m3_xx_tron_fight` before
   and after it.

Files involved, CANDIDATE where not seen: `ST04*` (Apple Market, seen),
`ST05*` (behind the card gate, not seen), `ST09*` (first ruin, seen),
`ST0C*` (forest path, seen), `ST0E*` (sealed ruin, door seen only).
Which stage holds the first Tron fight is UNKNOWN.

## Shared systems (build once)

| System | First needed | Size |
|---|---|---|
| Runtime stage loading beyond ST00 (packs are ST00-only; terrain already cooks for all 37 stage sets, per the repository session) | S0 | large |
| Per-stage poll/event owner (ST02 table, ST03 trigger events) on the existing event VM | S0 | medium |
| Door/exit records at `0x1F800050` + lock flags `0x7E0..` + arrival `0x80048898` (ST00 door path exists: `source_door_event.cpp`) | S1 | small–medium (generalize) |
| NPC actors + talk/action check + message blocks per area `.msg` | S1 | large |
| Story-state area variant selection (`_00` vs `_00B/C/D`, `ST0CB/C`) | S1 | medium |
| Trigger boxes per area (ST03 `0x80114F2C` cell box) and flag-chain event selectors | S3 | medium |
| Data's menu (recharge, save, tips) as event 0xFF | S3 | medium (save itself: memory card) |
| Shops/zenny/items, key items (Mine Parts Kit, Citizen's Card) | S4/S6 | medium |
| Dungeon minimap + Reaverbot class 0x19 + per-enemy defeat flags `0x6xx` | S5 | large |
| Roll's workshop (main phase 05, weapon development) | S6 | medium–large |
| Sneak / NPC sight check (kids) | S7 | small–medium |
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
6. **NPC/dialogue + talk target + Data's menu**, then S2–S3 (stage 0x0C,
   ST03 trigger box, police and Inspector events).
7. **Town v1 (S4):** stage 4 street, shops' doors, junk shop quest.
8. **First dungeon (S5):** stage 9, minimap, class-0x19 enemies, rescue.
9. **S6:** items, Roll's workshop, events 0x60/0x61.
10. **Oracle mapping session S7+** (this document, continued) before porting
    past the kids' gate.

## Port coverage today
- **ST00:** tutorial Areas 0–3, events 1 (opening), 3 and 2 (post-win, on a
  branch), and the boss intro plus combat pieces (M1, in progress).
- **ST01:** intro crawl, event 71 (`INTRO_EVENT71.md`).
- **ST07:** attract scene, event 10 (`ATTRACT_EVENT10.md`). ST07 is also a
  campaign stage set (`ST07_00`, `_05`, `_05B`); its campaign use is UNKNOWN.
- **Nothing else.** ST02, ST03, ST04, ST09, ST0C and everything past them
  are missing at runtime: stage loading, actors, NPCs, events and doors.
