# Class20/03 actions 5, 6 and 10 (ST00 overlay)

Status: **SOURCE_BACKED_PARTIAL**. Ported from the owner ST00 overlay
(`ST00.BIN` PROGBIN at 0x80100000, SHA-256 `9490c200…`), read with
`docs/research_01503/psx_mips_dis.py`'s decoder. No overlay bytes are committed.

## Why
Azahar 0.0.15.12 log (`hardware_01512_4001474722407.csv`): all three area-1
Reaverbots reached `action=10` and then reported body status 2
(`UnknownAction`) every frame. Action 9 (pursuit) selects action 10 when
`work35E==0 && work80<=0x11FFF` — i.e. when Mega Man is close — so enemies
froze next to him. A held body never refreshes its hit proxy, so the Buster
had nothing to hit (`impacts=0` throughout) and no enemy could be defeated.
Actions 6 (from the hit consumer `0x801041B0`) and 5 (state1 tail, HP < 51)
were unported too and would have frozen the enemy at the first hit.

## State1 dispatch
`0x80102698` calls `*(0x8011BEA8 + action*4)`: 0 `80102884`, 1 `801029B4`,
2 `80102AE0`, 3 `80102C9C`, 4 `80102FC0`, 5 `8010324C`, 6 `80103554`,
7 `80103654`, 8 `8010388C`, 9 `80103B58`, 10 `80103DFC`.

## Action 10 (`0x80103DFC`) — lunge
- Phase 0: phase 1, animation +AC = 4, timer +32C = 45,
  +330 = combat record byte 0x0A (`*(0x800A8844)[work+2] + variant*16`).
- Phase 1 (also on the first frame): cue +A6 == 1 → sound 181 at work+14
  (before motion). Timer−1; at ≤ 0: +35E = 45, flags +328 |= 1.
  `0x80031BEC(work+14, 0x800B51C4, heading, 32)` → +36C, heading += it.
  `0x8002FEA4(work, 52, 0, 0)`, `0x8004B414` → +370, `0x80048E6C` → raw Y and
  fixed Y. A raw-Y change ≥ 33 either way restores all four raw/fixed lanes
  from +2C/+34 (lwl/lwr copy).
- Other phases return. No contact-mask exit.

## Action 6 (`0x80103554`) — knockback
- Phase 0: phase 1, animation 6, timer 10, +330 = 0.
- Phase 1: timer−1; < 0 → phase 2, else +46 = 10 − timer.
- Phases 1–2: `0x8002FEA4(work, 0, (int16)+46 << 7, 0)`, `0x8004B414`; contact
  bit 0x1000 with timer < 0 sets flags |= 1. No floor call.

## Action 5 (`0x8010324C`) — damaged bounce, then self-destruct
- Entry: packet +33C bit 0x02000000 clears +330.
- Phase 0: phase 1, animation 4, +44 = (−rsin(heading)) >> 7 (`0x8007A060`),
  +46 = 32, +4E = 16, +48 = (−rcos(heading)) >> 7 (`0x8007A12C`), timer 115,
  flags &= ~8, +330 = combat byte 0x0A.
- Phase 1: cue 1 → sound 181. Timer−1; if < 0 or the packet bit: +330 = 0,
  phase 2, timer 2, into phase 2. Else HP +6E ≤ 0 → flags |= 1. Else when
  timer < 100: fixed X += +44<<16; floor `0x80048E6C`; if old raw Y ≠ floor:
  fixed Y += +46<<16, +46 += +4E, else +46 = 32; fixed Z += +48<<16;
  `0x80030150` raw. Then `0x8004B414` → +370; nonzero: 0xF00 → sound 222;
  0x100/0x200 force +44 negative/positive; 0x400/0x800 force +48.
- Phase 2: timer−1; < 0 → HP 0, +360 = 0, flags |= 0x41, so the same
  callback's tail runs the defeat owner.

## Port
`src/world/class20_03_late_actions.cpp`, dispatched from
`class20_03_step_state1` after the hit consumer. The actor bridge supplies the
map, reference tables, bounds, byte74 and cue (`Class20_03MotionWorld`) and
pre-plans the defeat delivery for the action-5 expiry. Any unsupported contact
holds the whole body unchanged (`NeedsLateActionMotion`).

Unknown / CANDIDATE: the combat byte for variants other than 0 is held;
retail trig tables come from the owner reference pack; animation cue timing
is the imported pose cursor's.
