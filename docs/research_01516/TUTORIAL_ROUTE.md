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
| 2 | 14 | 8 | anywhere | on **0x60E** (lock-on) → 0x61D |
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
  - Selecting one with lock-on (`0x800B5334` == target) sets **0x60E**.
  - Parameter ≥ 0x80 is the doors: they release once 0x608 and 0x609 are set.
- **E0/05 (`0x80113EE0`), Area 2 box** at (5120, −385, 12544): shot → fades
  over 5 frames, drops a reward (`0x80047228`). Parameter low nibble 1/2 sets
  its tail-word flag, here **0x620**.
- **Area 2 Reaverbots** (subtype 3, parameter 0xA0, tail 0x60B) wake on 0x60B.
  Also in Area 2: class 40/04 (tail 0x60F) and two class-20/35s.

## Port status (0.0.15.16)
| Piece | Status |
|---|---|
| Triggers 0–16 | ported (16 added in 0.0.15.16) |
| E0/00 defeated flags | ported (flags raised the update after release) |
| E0/07 switch and gate | ported. Sounds, the 0x800398E0 shake, 0x8001B2F0 and the model animation are not |
| Subtype 0x19 Reaverbots | **not ported**: the trap room clears at once |
| E0/08 targets/doors, lock-on 0x60E | **not ported** (lock-on never selects a target) |
| E0/05 box (0x620) | **not ported** |
| class 40/04, subtype-3 parameter 0xA0 wake | **not checked** |

Azahar, normal boot and scripted route: Area 1 messages 0 → 1 → 2 → 3 → 4 → 6
→ 7 in the handler order, and the switch raises 0x607. Message 7 follows
message 6 at once because no subtype-0x19 is spawned. The original's room
fight is missing.
