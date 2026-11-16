# Full-game roadmap (after the first Tron Bonne fight) — research in progress

Status (2026-11-15): **the first Tron Bonne fight has NOT been won in the oracle yet**, so
everything after it is **UNKNOWN**. This file records what was established on the way
(life field, fight behaviour, disc inventory) and the plan for filling in the segments.
Nothing here comes from memory of the original game; the user has not played past 1.0.
Companion: `M3_ROADMAP.md` (branch `claude/m3-roadmap-661ae9`) maps the start up to the fight.

Evidence labels: VERIFIED_1TO1 (seen in the PSXRecomp oracle, US disc) / CANDIDATE / UNKNOWN.

## 1. Player life field (VERIFIED_1TO1)

- **Life is an int16 at `player+0x6E` = `0x800B521E`** (player block `0x800B51B0`).
  Found by tracing the writer of the action byte `player+9` (`wtrace_range` on
  `0x800B51B8..BC`): it is written at `0x80041AA0`, inside the EXE hit handler
  `0x80041A44` (called from the actor update at `0x800419A8`).
- Handler logic (read from the EXE, not an overlay): if `player+0x74 != 0` or the hit mask
  `a1 & 0x41000000` is set → action 12; else if `life(+0x6E) <= 0` → **action 12 (dead)**;
  else if action is already 12 and sub-state `+0xA < 2` → keep; otherwise action 11 (hit stun),
  sub-state `+0xA = 0`. Damage subtraction is `+0x6E = x - damage` (`0x8004193C`).
- At the start of the first Tron fight: `+0x6C = 60`, `+0x6E = 29`. `+0x6C` is CANDIDATE max life.
- `+0x73` (byte) is a stun/knockback counter (decays 4/frame at `0x80041A10`).
- Rejected earlier candidates, with reasons: `player+0xA4`/`+0x100` (animation counter, 284→256 on
  a hit), `0x800A3526` (HUD sprite index, oscillates 0..0xF), `0x800BE4FA` (an effect/pool entry
  that the game re-allocates), the `0x800C0BC0` block (effect actor).
- Oracle-only shortcut used: writing `0x800B521E = 60` every few frames. Position/flags were not forced.

## 2. First Tron Bonne fight (S11) — what was observed

- Start save `m3_32_st05a1_tron_fight_start`: stage 5 area 1, player (1391,-1,6436).
- With life held up, Mega Man survives indefinitely, but is repeatedly knocked down
  (actions 8 / 11 / 12 cycle) and the mech jumps; most Buster rounds do not connect.
- Boss gauge (top right, 62 px at y=31) drained about 1 px per ~3,000 frames with R1 lock-on
  + Square and an occasional Triangle (land-mine). Winning with the Buster this way is
  impractical; the next attempt must work out how the mines/special weapon hit and how to
  stay out of the mech's reach, or find the boss HP field and its damage handler.
- Boss HP field: **not found.** Rejected: `0x800C3594/359E/35AC/35B6` (the mech's x/z position
  words, rewritten every frame; they read 1024 = x of (1024,-1,8448)), `0x800C3720`, `0x800C3A70`
  (rewritten every frame), `0x800C1D18` (writing 1 changes nothing: not HP). A RAM diff between
  gauge 62 and 61 px yielded only those. The next try: trace the enemy damage routine from the
  Buster hit (a write with a small negative delta to a field that is NOT rewritten each frame)
  while a hit is known to connect, or disassemble the hit path as was done for the player (§1).
- Zeroing the boss HP (oracle-only shortcut, no flags forced) would let the game run its own
  victory path; blocked on finding the field.
- Outcome, post-fight scene and the end flags: **UNKNOWN**.

## 3. Disc inventory for later segments (CANDIDATE; names only, contents unread)

`CDDATA/DAT` holds stage sets **ST00 … ST1E** (each `STxx.BIN` overlay-and-map plus
`STxx_NN.BIN` area sets; letter suffixes are story-state variants), e.g. `ST03_00B/C/D`,
`ST05_00B…J`, `ST0F_02B/C/D`, `ST1A_02B/C/D`. Beyond the 1.0 stages (00–09, 0C, 0F, 1D…)
the sets are: 0A, 0B, 0D, 0E, 10–19, 1A–1E. Also: `ARM00L…ARM13R` (arm/weapon models, CANDIDATE
Buster-part/weapon visuals), `HEAD00/01`, `LEG00/01`, `GAUGE`, `GAMEOVER`, `MAP_A1…B3`
(minimaps), `EXIT_MAP/SUB/SUP`, `SUB_KEY`, `SUB_WPN`, `SUPPORT`, `SUPPORT2`, `INIT_DAT`, `DEMO_DAT`,
`BS16VAB1` (boss audio bank). Which stage belongs to which story segment is UNKNOWN until played.

## 4. Segment table (all UNKNOWN until played)

| Segment | Start flags | End flags | Stages/areas | Bosses | Weapons/items/parts | Savestate |
|---|---|---|---|---|---|---|
| S11 Tron fight (end) | 0x386 set, fight live | UNKNOWN | ST05 a1 | Tron's mech | UNKNOWN | `m3_32_…` (start only) |
| F0 post-fight | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | — |
| F1…Fn | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | `full_NN_*.pst` (none yet) |

## 5. Data's save menu / memory card — NOT RESOLVED

- `saves/card1.mcd` and `card2.mcd` exist (128 KiB, header `MC`, no saves) and `game.toml` has
  `memcard_dir = "saves"`; so a card is mounted, but Choosing Save was not yet re-tested with
  `card_data_writes` armed, and the menu handler is UNKNOWN. Relevant commands (docs/TCP_COMMANDS.md):
  `card_data_writes`, `card_data_writes_reset`, `card_txn_dump`, `card_trace_dump`, `fntrace_arm/dump`.
- Plan: load `m3_01_st03_crash_control`, talk to Data (NPC slot 5), choose Save with `fntrace`
  armed on EXE functions and `card_data_writes_reset` first; name the handler from the trace.

## 6. Next steps

1. Win the fight: study the special-weapon hit path (mines) or the boss damage handler; keep the
   life shortcut labelled. Save `m3_33_tron_defeated`, then `full_01_…`.
2. Record each segment's flags (<0xC00 only; ≥0xC00 are temporary) and exits via `0x1F800050`.
3. Do the card test (section 5).
4. Update the table above and label each segment.

Driving notes: oracle runs with `set_input` active-low masks; `0x800B521E` write needs int16;
`wtrace_dump` rings fill in ~100 frames when the range includes the player's animation fields —
narrow the range. Tool-use tip: never start the oracle with output piped to `head` (SIGPIPE kills it).
