# ST00 Area 3 boss: event ownership, callback and defeat

Status: **SOURCE_BACKED_PARTIAL**. The class 40/02 callback (AI, Buster
damage, stagger, gauge, defeat flag 0x606, release) is translated and wired
(section "Callback translation", 2026-11-01). Player damage and game over do
not exist in the port yet, and the post-win event 3 is mapped but not ported.
The earlier sections below describe the replayed intro slice that the
callback has replaced; their observations remain the reference data.

## Original evidence

The Area 3 source object list has 11 records. Its first record is class 00,
event ID 3; there is no placed class-40 boss. The Area 3 EBD row 0 is class
`40`, subtype `02`. A PSXRecomp run of the owner-supplied US SLUS-00603 game
reached Area 3 after flag `0x622` and showed that event 3 allocates class-40
slot 0 at `0x800C3580`.

The live class-40 wrapper is `0x801076E8`. Its state table is at `0x8011C054`;
state 0 selects `0x801078B8`. A clean pre-fight save (slot 2) was replayed for
420 original frames. Event 3 replaces a stale class-40 slot at frame 49 and
initializes the boss at frame 52 with:

- class/subtype/parameter `40/02/01`;
- spawn position `(-256,-385,1001)`, heading `0`;
- Q12 scale `(512,512,512)`, shade `0x4210`;
- HP `1024` at work `+0x6E`;
- action/phase/substate `1/0/1`;
- pose request/record `0/0`;
- a 12-part articulated model from Area 3 EBD row 0.

Initializer `0x801078B8` allocates two owned class-37/subtype-10 children at
work `+0x514` and `+0x518`, obtains HP from EBD stats byte 8 shifted left 5,
and selects action 1 for parameter 1. The wrapper dispatches action 1 through
`0x80107B48`; the intro uses substates `0x80107F40`, `0x80108C18`,
`0x80107FAC`, and `0x801084D4`.

The observed entry sequence is:

- Mega Man arrives at z `-1920`, is briefly placed at `-1984`, then is forced
  forward in 16-unit steps to z `-1632`;
- phase 0 waits for that endpoint, then phase 3 installs request 3 and a
  12-tick countdown;
- phase 1/request 1 moves the boss from z `1001` to `-327` in 83 steps of 16;
- phase 2/request 2 moves it from `-327` to `-1223` in 28 steps of 32;
- one handoff update returns to action 1/phase 0, then action 6 phase 0 sets
  work `+0x4E4` to 80;
- the first action-6 presentation cycle advances through phases 1..5 with
  request/countdown pairs `11/12`, `11/9`, `12/19`, `12/9`, and `13/14`,
  then returns to action 1/phase 0. The next attack selection is not yet
  translated.

Ghidra headless analysis of the preserved owner-local ST00 overlay confirms
that `0x80107FAC` initializes phase 0, compares work `+0x80` against
`0xC8000`, decrements the 80-tick decision timer while distant, turns and
moves through shared helpers, and selects actions 2, 3, or 6. The clean replay
above covers the near-player branch and its shared animation phases; movement,
collision and the other distance branch remain outside the native slice.

The next boundary is now statically resolved as well. `0x80107B48` is the
outer action-1 wrapper, not another attack body. It refreshes work `+0x4E0`
bit `0x20` through `0x8003648C`, optionally calls `0x8010A448` for bit 1,
then dispatches work byte `+9` through the table at `0x8011C068`. Substate 1
selects `0x80107FAC`. When the dispatched body returns with work `+0x4E0`
bit 0 set and HP remains positive, the wrapper resets work `+0x0A` and chooses
substate 1 when the proximity bit is set or the prior substate was 2; otherwise
it routes 0/4 through 3, restores 3 from work `+0x0D`, or returns the remaining
states to 0. HP below 1 selects substate 0 and begins the separate death path.
This chooser is source evidence for the next implementation slice; it is not
yet native encounter behavior.

`0x8003648C` is not a stateless radius comparison that can be replaced with a
guessed distance. It passes actor work `+0x504`, the actor pointer and heading
`+0x56` to `0x800364B0`. That body first compares work `+0x80` with a
profile-table byte shifted by 20, then advances a state byte at detection
work `+7`, samples the global player position through `0x80030628`, stores
relative halfwords at detection work `+8..+0x0C`, and uses collision/status
helpers before returning the boolean consumed as work `+0x4E0` bit `0x20`.
Therefore the pure native chooser accepts the already-refreshed bit. Wiring
it into live combat requires the original `+0x504` detection state and work
`+0x80` writer; this boundary is intentionally not replaced by a Euclidean
shortcut.

A separate sampled active encounter, retained only as a later-state witness,
had:

- class/subtype/parameter `40/02/01`;
- position `(-256,-385,-1223)`, heading `4095`;
- Q12 scale `(512,512,512)`, shade `0x4210`;
- HP `1024` at work `+0x6E`;
- action/phase `6/5`;
- pose request `13`, record `9`, countdown `1`;
- the same 12-part articulated model.

The callback uses the shared animation (`0x80032488`), packed-pose
(`0x80033A28`), hierarchy (`0x8003346C`), contact/damage, and class-40 draw
paths. Those later observations define the next translation boundaries; they
are not acknowledged as implemented combat behavior.

## Native slice

### Event-owned player entry

The 2026-10-27 PSXRecomp slot-2 replay resolves an important ownership detail.
With injected input cleared, frame history reports pad `0xFFFF` on every frame
7354 through 7382. Player work `+0x18` nevertheless advances from z `-1952`
through `-1920`, `-1888`, ... to exactly `-1632`. Each coordinate is visible
for two 60 Hz presents, proving a 16-unit step per 30 Hz fixed update. The
event therefore owns the authored player position; it does not synthesize Up
input or route the step through ordinary terrain collision. This is
`VERIFIED_1TO1` for the neutral pad, fixed increment and endpoint only.

`source_area3_boss_spawn_spec_801076e8` admits exactly one class-00 event ID 3.
Event IDs are area-local, so `append_source_area3_boss_801076e8` additionally
requires ST00 source area index 3; an early Azahar run exposed that Areas 1 and
2 also use event ID 3, and the area guard prevents false boss ownership there.
The appender then decodes the first class `40/02` EBD row, binds fresh request
0 and a bounded immutable bank for original requests 0, 1, 2, 3, 11, 12 and
13, and appends one dynamic owner to the existing actor area.
`tick_source_area3_boss_intro_80107b48` runs independently of diagnostic actor
gates and is owned by the Area 3 event rather than the gameplay camera. It
commits only the bounded entry position until z `-1632`, retains that endpoint
through the recovered presentation cycle, uses a neutral pad and bypasses
ordinary map collision, so neither the doorway threshold nor the still-live
door-arrival state can restore the room spawn. Dialogue, cutscene and
source-player gates remain intact. It follows
the observed phase
path and movement increments, advances only resident pose data, executes the
first bounded action-6 presentation cycle, and stops at the next action-1
selection boundary. It does not synthesize a placed object or change the
established class-20 runtime.

Owner-local probe result on the current cooked Area 3 package:

```text
status=SourceBackedPresentation actors=1 row=0 parts=12
pose_request=0 pose_record=0 hp=1024 action=1 phase=0 substate=1
spawn_z=1001 decoded_bytes=46398
```

Host tests cover absent, unique, and duplicate event-3 records, fresh source
constants, non-Area-3 rejection, the complete admitted intro/first action-6
cycle, exact endpoints, zero tick-time allocations and the pure action-1
post-dispatch chooser (including HP-zero/death-bit behavior). The real-package probe is
`l3probe_area3_boss`; retail bytes remain outside Git.

The complete host suite passes 170/170 and the Old 3DS target builds. Continuous
Old-3DS-mode Azahar trace `hardware_01516_4002111785105.csv` reaches flag
`0x622` at frame 9772, enters the original owner, reaches CombatReady at frame
10090 and keeps Mega Man at `(-256,-385,-1632)` through frame 10800. It exits
cleanly at frame 10854. Earlier trace `hardware_01516_4002111246033.csv` is
retained as negative evidence: releasing ownership at the endpoint let the
door-arrival player state restore z `-1984` while the boss continued. The
validated build retains event ownership only until intro leg 6.

## Next exact step (superseded 2026-11-01, see "Callback translation")

Port event 3 (poll on 0x606) with its two class 40/03 scene actors, smoke
emitters, camera program, XA stream 1 and the Area 4 request; then Area 4
event 2. Player damage (boss body contact 8, shockwave proxies) and game over
are a separate system the port does not have yet.

## Callback translation (2026-11-01)

Read from the ST00 overlay with Ghidra headless on a copy of the project
(the shared MCP projects were locked), cross-checked with capstone and the
PSXRecomp oracle. Native: `src/world/class40_02_boss.cpp`, wired in
`tick_source_area3_boss_801076e8` (`actor_runtime.cpp`) and the 3DS loop.

Wrapper `0x801076E8`: copy +0x14..+0x28 to +0x2C, shade 0x4210, `rand()` at
RA 0x80107774 into +0x4E2, state table `0x8011C054` (0 init, 1 `0x80107B48`,
2 release `0x80107F00`, 3 null, 4 `0x80107D04` for parameter 0), then the
animation (flags bit 0x10: advance or restart), shadow, and the hit proxy
`0x800341BC(+0x4EC = pose point +0x180, radius 0xD0, 0, 0x8000, work+1,
target +0x4F4, +0x4E8 | 0x04880008, 0x800000, 0)`.

Init `0x801078B8` (parameter 1): two class-60 subtype-0x0A arms (pool
`0x80031518`, byte0 0x25, limb +0x0D = 2 / 4), flags +0x4E0 = 0x1E,
detection `0x80036478(+0x504, record byte0F, -0x60)`, bounds +0x78 =
`0x8011C0A4` {-192,192,-192,192,-300,0}, HP = record byte08 << 5 into
+0x6E/+0xB0/+0xB2, floor `0x80048E6C`, state 1 / substate 0, gauge
`0x8005D620(work, 0, 0, 0x51, 0x203050)`.

Stats record `0x8009884C[2] + 16` (selector 1: `0x8008EF64`, selector 2:
`0x8008F200`; oracle RAM): `06 05 04 03 00 00 00 00 20 50 10 00 00 00 00 05`
-> HP 1024, stagger limit 0x50 << 2 = 320, detection profile 5. Now cooked
into L3CB as class 40/02 record 1 (recipe `01516-boss40-1`).

State 1 `0x80107B48`: `0x8003648C` every update -> flags bit 0x20; hit
`0x8010A448` while bit 2; substate table `0x8011C068`; chooser when bit 0
(HP < 1 -> 0, and on stage 0 once: bit 0x40, `0x8001DA8C(0x606)`,
`0x8005D6E8`; 0/4 -> 3 with +0x0D = 1; 3 -> +0x0D; 2 or detected -> 1;
else 0); +0x4EA decays by 1.

| Substate | Code | Behaviour |
|---|---|---|
| 0 | 0x80107F40 | wait for detection or a hit packet |
| 1 | 0x80107FAC | timer 80; near (+0x80 < 0xC8000): slam if facing (0x80031DCC ±0x80) else turn; far: turn ≤ 0x20, step 0x100, footstep sound 200; timer out -> charge (+0x4E6 = 60) |
| 2 | 0x801084D4 | step 0x200 until +0x80 ≤ 0x47FFF, timer, wall or a hit bit 0x2000000; on cue +0xA6 == 1 three effect128/0x42 shockwaves at 12000/9200/6400 ahead with floor y, shake `0x800398F8(30, 2000, 5000)`, sound 0xC2 |
| 3 | 0x80108C18 | 12-tick turn by |delta|/12, then +0x0D |
| 4 | 0x80109038 | 28-tick stagger (stagger mode bit 4 off meanwhile) |
| 6 | 0x80109638 | slam: requests 11/12/13, timers 13/10/20/10/15, arms -> substate 3 (Y-scale pulse 0x80110D24), shockwaves at timer 3 of phase 3, sounds 0xBF/0xC0/0xC1 |

Hit `0x8010A448`: damage = packet & 0xFFFFFF; no type bits 0x18000000 ->
sound 0xA3; else HP -= damage; at HP ≤ 0 on stage 0 `0x80042208` (player
HP ≥ 0 sets 0x800B55F9 = 1) gives HP 0 and clears bit 2, else HP 1; sound
0xA0/0xA2; in stagger mode +0x4EA += max(damage, 10) and reaching 320 enters
substate 4; shade 0x7FFF and arm flash (+0x0E |= 0x20).

Oracle (oracle-only HP writes, savestate slot 9 is HP 24 mid-slam):
Buster hits take 48 or 24; after HP 0 the slam finishes, the chooser sets
0x606 at frame +107 and substate 0; event 3 starts the same update.

Substates 5 and 7..14 and state 4 are the parameter-0 variant (region tables
0x8011C0B4..0x8011C148) and are held as UnsupportedSubstate/State.

## Post-win event 3 (mapped, not ported)

ST00 poll table (overlay 0x8011ADF8): flag 0x604 -> event 1 (opening),
0x605 -> event 2, 0x606 -> event 3, 0x7B0 -> event 0. Event 3 handler
`0x80102058` (`tools/cook_cutscene.py --event 3` finds it):

| Phase | Effect |
|---|---|
| 0 | `0x8005D7C0`; wait `0x80042238(player)` |
| 1 | stop sequences `0x8001B33C`, XA `0x8001D58C(0x20000001)` |
| 2 | wait `0x8001D878`; +0x0C = 30 |
| 3 | count down |
| 4 | constructor `0x80101F7C`: hide player, camera `0x8001E6C0`, program `0x8011BB3C` (194 words), boss slot +8 = 2 (release), spawns `0x8011BB14` / `0x8011BB28` (class 40/03 scene actors, callback `0x8010A68C`, states `0x8011C178`), timeline `0x8011BE44` |
| 5/6 | timeline callbacks, camera, `0x8001F070`; on busy 0 and timeline end (or the Start skip `0x8001DE84`) fade 10; then request area 4 (status 2), clear 0x606, player visible, `0x8001D494(0,1,0)` |

Timeline records (ticks, callback): 45 `0x80101B6C`, 340 `0x80101B9C`
(five smoke emitters `0x80116D60`), 75, 45 `0x80101D88`, 67 `0x80101DAC`,
39, 50, 90 `0x80101DE4` (smoke off, class-1 object at (-256,-728,-880)),
60 `0x80101EF0` (`0x8002EE90(0x40,0x80)`), 115 `0x80101F44`.
Oracle: event 3 runs 1982 frames (12329 -> 14311), then Area 4 loads and
event 2 starts (flag 0x605); Mega Man at (500,-7169,512).

### Event 3 scene actors and hand-off (2026-11-07)

- Spawn records `0x8011BB14` / `0x8011BB28` (20 bytes: [1] subtype, [2] 3,
  [3] family 0x40, [12..17] position, [18..19] heading) create class-40
  **subtype 5** at (-256,-384,-640) heading 0 in slot 1 (event+0x24) and
  **subtype 8** at (-256,-384,-1024) heading 0x800 in slot 2 (event+0x28)
  (PSXRecomp: slot bytes `03 41 05 00` / `03 42 08 00`, work stride 0x5A4).
- ST00 class-40 table (base `0x8011A4A0`, fitted on 40/02 = `0x801076E8`):
  40/03 `0x8010A68C` (an enemy-style init with stats/detection),
  40/05 `0x8010D824` (scene Mega Man, init `0x8010C930`: the opening's actor,
  already partly ported), 40/06 `0x8010E500`, 40/07 `0x8010EC54` (scene Mega
  Man variants; all copy their position into 0x800B51C4), **40/08 empty**:
  the subtype-8 scene boss is dispatched from elsewhere (CANDIDATE: EXE).
  Its timeline commands: +9/+0A from callbacks, +0x128 = 0x0C / 0x0D.
- The hand-off writes 0x800C356C = 2, 356F = 4 (area), 356E = 0 (stage),
  3560..3564 = 0, 356A = 0, 356D = 0, 3570/3571 = 0xFF, clears 0x606, makes
  the player visible, `0x8001D494(0,1,0)`, `0x80042314`. Area 4 then starts
  event 2 (poll flag 0x605), which places Mega Man (oracle (500,-7169,512)).
  Porting event 3 alone would leave the player at the Area 4 origin: events
  3 and 2 must land together.
- The PSXRecomp debug build does not trace overlay functions (fntrace on
  0x8010A68C / 0x80116D60 returned no entries); RAM reads work.

## Post-win events 3 and 2 ported (2026-11-07)

Decompiled from the ST00 overlay (Ghidra headless, read-only copy).

- **Event 3 `0x80102058`** (native: `src/flow/source_st00_post_win.cpp`):
  0 `0x8005D7C0` (not ported), wait `0x80042238`; 1 `0x8001B33C`, CD
  `0x20000001`; 2 wait `0x8001D878`, +0x0C = 30; 3 count down; 4 constructor
  `0x80101F7C` then falls into 5; 5 fade 10 when 0x80098A72 == 0 and (+8 ==
  0xFF or `0x8001DE84`), falls into 6; 6 callback `0x80102018`, `0x8001EC0C`,
  `0x8001F070`, then at busy 0: `0x8005D9EC` (not ported), the hand-off,
  `0x8001DCD0(0x606)`, +0 = 0, `0x8001D494(0,1,0)`, 0x800B51B0 |= 1,
  `0x80042314`. A fade started at phase 5 is busy for the phase-6 check
  of the same tick.
- Constructor `0x80101F7C`: player +0 &= 0xFC, `0x8001E6C0(0,0,0)`, camera
  program 0x8011BB3C, **0x800C3588 = 2** (not the boss's +8; no absolute
  reader in the EXE or the overlay: UNKNOWN), spawns 0x8011BB14/BB28,
  +0xA4 = 0x8011BE44, +0x0B = +0x0A = 0, +0x0C = 0.
- Spawn record byte 0 is the event slot (+0x24 + 4n): event 3 slot 0 =
  40/05 at (-256,-384,-640), slot 1 = 40/08 at (-256,-384,-1024) heading 0x800.
- Event 3 callbacks (t = event +0x10): B6C t15 slot1 +9 = 1; B9C t0 slot1
  +9 = 3, smoke `0x80116D60(pos,1,1,0,-2,0)` at t 0xE6 (two), 0xF0, 0xF5,
  0x104 (offsets from slot 1 +0x14); D88 t0x23 slot1 +0x128 = 0x0C; DAC t0
  slot0 +9 = 1; DE4 t0 both slots +9 = 0 and `0x80031988` on the five smoke
  handles, t0x3C object `0x80031618` (+0 1, +2 5, +0xC 5, +0xD 0, +0xE 2,
  pos (-256,-728,-880)); EF0 t0 slot1 +9 = 2, +0x128 = 0x0D,
  `0x8002EE90(0x40,0x80)`; F44 t0 slot0 +9 = 0. D80, DDC, F74 are empty.
  Every +9 write also zeroes +0x0A.
- **Event 2 `0x8010194C`**: 0 wait busy 0; 1 wait `0x80042238`, player
  +0 &= 0xFC; 2 `0x8001B33C`, CD `0x20000002`; 3 wait `0x8001D878`, +0x0C =
  30; 4 count down, at 0 fade 2; 5 constructor `0x80101888`
  (`0x8001E6C0(0,0,0)`, camera 0x8011B4B4, 14 records from 0x8011B388,
  timeline 0x8011BA4C); 6 fade 10 on busy 0 and (Start or +8 == 0xFF),
  falls into 7; 7 callbacks/camera/timeline, at busy 0: **356C = 2, 356E = 2
  (stage 2), 356F = 0**, 3560..3565 = 0, 356A = 0, 356D = 0, 3570/3571 =
  0xFF, `0x8001DCD0(0x605)`, +0 = 0, `0x8001D494(0,1,0)`, `0x80042314`.
  Event 2 ends the tutorial by loading **stage 2**, which the port has no
  data for: the request is retained and reported.
- Event 2 slots: 0 = 40/05 at (500,-7169,512) (the oracle's "Mega Man"
  position is this scene actor; the player stays hidden), 1 = 40/08,
  2/3 = family 0xE0 class 2, 4 = 0x20/0x0A at (-112,-6232,-1408), 5/6 =
  0x20/0x1D; records 7..13 reuse slot 5 (0x20/0x1D, +4 = 2..8).
- Event 2 callbacks write slots 0..4: +9/+0A states 1..11 on slot 0,
  0..3 on slot 1, +0x0B/+0x0C, +0x15E (0x2108, 0x4210), +0x56 = 0x800,
  +0x1C..+0x24 16.16 positions, slot 1/4 +0 |= 2; 0x80101528 spawns an
  object at (500,-7504,236); 0x801016EC calls `0x8011877C` twice. Empty:
  618, 660, 668, 670, 78C, 83C, 880.
- Area 4's init (`0x8010038C` case 4) shares area 0's tail: player +0 &=
  0xFC, +8 word = 0, `0x8001DA8C(0x605)` (area 3: +0x44E = 1).
- Camera: event 3's program interprets to its terminator in the native
  interpreter; event 2's has op 0x80 at word 359 (not interpretable yet).
- Local run of the real cooked packages: event 3 requests area 4 927 ticks
  after its constructor (oracle: 1982 frames for the whole event); event 2
  requests stage 2 2739 ticks after its constructor.

Not ported: the scene actors (40/05, 40/08 and event 2's cast) — their
commands are recorded per slot; the smoke emitters, `0x80031618` objects,
`0x8011877C`, sound `0x8002EE90`; the XA streams 0x20000001/2 (no measured
span in `cook_xa_stream.py`); applying the event camera to the 3DS view.
