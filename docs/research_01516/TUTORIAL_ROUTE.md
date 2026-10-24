# The complete tutorial chain (ST00 Areas 1–2)

Status: **SOURCE_BACKED_PARTIAL**. Read from the ST00 overlay (`9490c200…`)
with Ghidra 12.1.4 and checked against the instructions. The trigger list is
the cooked L3TR; objects come from the cooked stage and conditional-spawn lists.

## Trigger records (E0/06, 0x8011CB24)
| Area | Type | Msg | Region (world x / z) | Gate → sets |
|---|---|---|---|---|
| 1 | 0 | – | 3072..3583 / −5120..−4609 | voice 0 until 0x610 |
| 1 | 1 | 0 | 2560..3071 / −6144..−5633 | radio + instructions → 0x610 |
| 1 | 2 | 2 | 2560..3071 / −9728..−9217 | "Turn right there." → 0x612 |
| 1 | 3 | 3 | −512..−1 / −10240..−9729 | → 0x613 |
| 1 | 4 | 4/5 | −3584..−3073 / −10240..−9217 | by heading → 0x614 |
| 1 | 5 | – | −3584..−3073 / −8192..−7681 | voice 10 until 0x615 |
| 1 | 6 | 6/7 | −4096..−2561 / −5120..−4609 | needs **0x607**; → 0x615; msg 7 and **0x61F** once no class-20 is left in the region |
| 1 | 7 | 9 | −512..−1 / −9728..−9217 | → 0x616 |
| 1 | 8 | 11 | −512..−1 / −8192..−7681 | → 0x617 |
| 2 | 9 | 0 | −1536..1023 / 7680..8703 | y −0xC1 for 90 frames → 0x618 |
| 2 | 10 | 2 | −512..−1 / 10240..10751 | → 0x619, sets **0x60B** |
| 2 | 11 | 3 | −3072..−2049 / 11264..14335 | on landing → 0x61A |
| 2 | 12 | 5 | −512..−1 / 14336..14847 | skipped once 0x608 and 0x609 → 0x61B |
| 2 | 13 | 6/7 | −6656..−6145 / 12288..12799 | unless 0x608 and 0x609 → 0x61C |
| 2 | 14 | 8 | anywhere | on **0x60E** (action-button check on a target) → 0x61D |
| 2 | 15 | 10 | −512..−1 / 14848..15359 | → 0x61E, **0x622** (tutorial done) |
| 2 | 16 | 11 | 2048..5631 / 11776..13311 | **0x620** pulse in the region → 60 frames → msg → 0x621 (ignores 0x622) |

## Objects that drive the flags
- **E0/00 (conditional spawns):** when a spawned object's pool slot clears,
  `0x80111AD4`/`0x80111C2C` set its record's +20 flag (1585..1609), and
  spawning skips records whose flag is set.
- **E0/07 (`0x80115A4C`), Area 1:**
  - Switch at (−3328, −1, −4864), parameter 1: sets **0x607**.
  - Gate at (−3328, −384, −5168), parameter 0, IDX cell 119: on 0x607 it writes
    descriptor 19 (word0 0x40500) over 21 (0x41500). On **0x61F** it sets
    **0x60A**, restores 21 and releases.
  - The room holds six dormant **subtype-0x19** Reaverbots (spawn records
    1593–1598, callback `0x80104864`).
- **E0/08 (`0x80115E70`), Area 2:**
  - Targets (parameter < 0x80, 100 HP) set **0x608** (low-5-bit parameter 0)
    or **0x609** when shot.
  - Checking one with the action button sets **0x60E**: player +0x184
    (`0x800B5334`) equals the target. This is not the Buster lock-on (see below).
  - Parameter ≥ 0x80 is the doors: they release once 0x608 and 0x609 are set.
- **E0/05 (`0x80113EE0`), Area 2 box** at (5120, −385, 12544), parameter 2,
  from the E0/00 entry-gate group (record flag 1609):
  - init `0x80113F2C`: flags 0x0E, 160 HP (0x22 when parameter & 0xF0);
  - every frame it draws rand() into +0x2E (RA `0x80114020`);
  - hit `0x80114370`: 0xA3 without a damage bit, else HP −= damage (0xA0), and
    at HP ≤ 0 sound 0xA2 and the break action;
  - proxy `0x801140BC`: radius 0xA0 at y − 0x80, mode `0x8011CB00[param>>4]`,
    word `0x8011CAF8[param>>4]`, stack1c 0, stack20 0x80;
  - break `0x80114170` over 4 frames, then release. Parameter low nibble 1 →
    reward record 2 plus the flag; 2 → the flag (**0x620**) plus reward record
    (+0x2E & 1); otherwise the reward only. The reward is `0x80047228` with
    the L3CB E0/05 row's word00.
  - The way in is the corridor at z 12288..12799. The cells at z 11776 and
    12800 next to it are solid.
- **Area 2 Reaverbots** (subtype 3, parameter 0xA0, tail 0x60B) wake on 0x60B.
  Also in Area 2: class 40/04 (tail 0x60F) and two class-20/35s.

## Port status (0.0.15.16)
| Piece | Status |
|---|---|
| Triggers 0–16 | ported (16 added in 0.0.15.16) |
| E0/00 defeated flags | ported (flags raised the update after release) |
| E0/07 switch and gate | ported. Sounds, the 0x800398E0 shake, 0x8001B2F0 and the model animation are not |
| Subtype 0x19 Reaverbots | ported (`class20_19`); host-checked on the real Area 1 map, Azahar fight not yet run |
| E0/08 targets/doors, action check 0x60E (`0x800411DC`) | ported; Azahar: check → msg 8/9, both targets destroyed, doors open |
| E0/05 box (0x620) | ported (`source_box_e005`, textured, fade scale); Azahar: box breaks → 0x620 → msg 11 (Right+A) → 0x621, reward expands 14 pickups |
| class 40/04, subtype-3 parameter 0xA0 wake | **not checked** |
| Sound effects, Roll's voice, typing blip | ported (`AUDIO.md`); BGM not ported |

Azahar, normal boot and scripted route: Area 1 messages 0 → 1 → 2 → 3 → 4 → 6
→ 7 in the handler order, and the switch raises 0x607. Message 7 follows
message 6 at once because no subtype-0x19 is spawned. The original's room
fight is missing.

## Subtype 0x19 (0x80104864), ported in 0.0.15.16
- **States:** init `0x80104A08`, active `0x80104BE4`, release.
- **Actions (`0x8011BF38`):** idle, wander, chase (`0x80031BEC` turn 0x30, forward
  = animation cue +0xA6), alert/back-off (detection on with flags 0x20),
  hop/drop-in, lunge (sound 0xBA, parabolic hop; on 0x2000000 it bounces back
  into the hop).
- **Selector `0x80105D10`:** weights 4/6/3/3 over rand&15 → alert, wander +0x10,
  wander +0x10, wander −0x10. Pick 1's zero store is overwritten through the
  delay slot.
- **Hit `0x80105DEC`:**
  - a packet without 0x38000000 → sound 0xA3;
  - otherwise HP drops and stagger builds against record byte 9 (0xA0 below the
    limit, 0xA2 at it); a strong hit (0x20000000) or a full stagger turns it
    from the hit point and starts the knockback hop;
  - at HP ≤ 0, flags 0x88; four frames later the defeat (effect 5/0x80/0x41,
    reward from record word 0, sound 0xA1, release).
- **Trap-room variant:** parameter 1 with +0x1A = 0x607 starts hidden in action 4
  phase 0xFF; on 0x607 it becomes visible, enables its proxy (flags |= 6) and
  drops in with rand-driven velocity.
- **Records (L3CB rows 0x20/0x19):**

  | Selector | HP | Stagger | Attack | Detection | Reward |
  |---|---|---|---|---|---|
  | 1 | 32 | 16 | 16 | 6 | 0x103 |
  | 2 | 64 | 32 | 32 | 6 | 0x103 |

  HP is byte 8 directly. L3DP gains profile 6.
- **Host check on the real Area 1 map** (`reaverbot19_area1_sim.cpp`, owner
  data): both records drop from y −200 to the floor (y −1) in 34 frames and
  commit 1200/1200 frames. Undetected they wander and alert; detected they chase
  and lunge. With the gate closed (IDX 119 → 19) they stay in the room.

## Correction: 0x60E is the action-button check, not lock-on
- **`0x800411DC`** runs from the player idle (`0x8003F5D8`) and walk
  (`0x8003F7F0`) states.
- **Gates:** `0x8003A13C`, +0x174 bits 0xC0, +0x177, and pressed (+0x11C) &
  +0x136 (the action button, Circle).
- **Scan:**
  - pools: class 20 (16 × 0x3C0 at 0x8009F108), class 40 (4 × 0x5A4),
    class 60 (32 × 0x10C) and class E0 (32 × 0x50 at 0x800BF118);
  - an object is a candidate when work[0] & 3 == 3 and its +0x10 holds check
    data;
  - it keeps the one with the lowest `0x800414F0` score under 208, stored in
    player +0x184 (score in +0x182), and returns 1. The state handler then skips
    its own input for that frame.
- **E0/08:** its init sets its check data (+0x10 = 0x8011CD14 / 0x8011CD20). When
  +0x184 equals it, it sets 0x60E and clears both.
- **Buster targeting:** separate. `0x80034EB8` picks a proxy per shot inside
  screen windows (x 0x83–0xBD for lane 0). It works in host tests but needs an
  enemy roughly centred ahead.

## E0/08 and the action check, ported in 0.0.15.16
- **`source_target_e008`:** the target and door states
  (`0x80115EAC`/`0x801161D4`/`0x801164A8`/`0x80116658`/`0x8011668C`/`0x80116750`/`0x80116A08`)
  and the shared hit records 0x80122620 + 16 × (parameter & 1). Real Area 2:
  - targets 0x61 at (−5888, 12032) → 0x609 and 0x60 at (−5888, 13056) → 0x608;
  - doors 0x81/0xC1 own IDX cell 258 (0x110900 → 0xF0900); 0xA0/0xE0 own cell
    335 (0x72400 → 0x71400).
- **`source_action_check_800411dc`:**
  - E0 candidates only, with zero rotation; probes 0x50/0x80/0xB0 with
    tolerances 12/24/36; nearest below 208;
  - CANDIDATE: the integer sqrt (0x8007A578 uses a table), and the 0x8003A13C
    gate (byte 0x800A3962) is taken as open during gameplay.
- **Drawing:** the E0 single-part decoder now admits subtypes 5 and 8 (the
  0x80051B68 draw is CANDIDATE). The meshes are untextured, tinted by +0x40
  (FRONTEND_DIAGNOSTIC).
- **Two Buster bugs found along the way:**
  - The shot-0x81 state 0 republished its slot's last hit packet: the local
    target was copied from +0xC4 before the clear. So every shot after a hit
    "impacted" on its second frame. Now rebuilt after the clear (regression
    test in `source_buster_shot81`).
  - External proxy registrations now receive the 0x800BE478 owner table
    (`ChildRuntime::proxy_owner_pointers`), so a target leased by a homing pick
    can re-register.
