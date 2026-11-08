# Mega Buster firing pose and shot sprite

Status: **SOURCE_BACKED_PARTIAL**. Owner `ROCK_NEO.EXE` (SHA-256 `10d2008f…`),
read from the R3000 instructions (`docs/research_01503/psx_mips_dis.py`) and
the owner decompiler export, plus the psxrecomp oracle for the live player
bytes. Branch `fix/buster-arm-model`.

## Why
Mega Man fired without moving his arm, and the shot was drawn as a
diagnostic line. The port spawned the shot directly from the Y edge and never
ran the original firing chain or the shot's sprite renderer.

## Firing chain (player work `0x800B51B0`)
- **Fire input `0x80040B68`** (from the action handlers of table
  `0x80089FF8[+9]`, context 0 for actions 0/1/4/6/14, 1 for 2, 2 for 7/8).
  Buster branch: `(+174 & 0xC0) == 0`, button `+13A & +11C` (or `+176 & 3 ==
  2`), counter `+4FA[+171]` > 0, `+175 & 5 == 5` → `0x80040E00(4, context,
  row[+171].byte4)`. A queued re-fire (`+177 == 1`) uses base 0 and sets
  `+178 = 1`.
- **`0x80040E00`**: context 0 → action base+1; context 1 with flag 0 resets
  `+9/+0A`; otherwise action base.
- **`0x80040E9C`** (already ported): actions 4/5 set `+174 = 0x86/0x82`,
  `+104 = 0x800423DC()`, clear `+178/+105/+100`, and ask the main track for
  `+104` when `+174` bit 4 is clear.
- **`0x800423DC`**: left-arm request `0x60`, or for actions 2/8 by stance
  `+112`: 1 → `0x63`, 0 → `0x61`, 3 → `0x62`, else `0x60`.
- **`+0x175 = 7`** at player init (`0x8003C7E0` → `0x8003C840`); measured 7 in
  the oracle. The port stored 1, so this gate could never pass.

## Buster lane `0x80042F78`
Run from `0x8003CA00` when `(+174 & 0xC0) == 0x80`, through
`0x80042734` → `0x8008A0C4[+171]`. States `+178` (jump table `0x80010D00`),
with row = `0x80089A84[+171]` (bytes 0..4, now cooked in L3BP/3):

| State | Action |
|---|---|
| 0 | when the overlay key frame `+100` = row[0]: `+178 += 2`, `+174 \|= 0x20` (the shot) |
| 1 | restart main (unless bit 4) and overlay at row[0]; `+178 = 2`; `+174 \|= 0x20` |
| 2 | `+174 &= 0xCF`; with row[3] and ammo: at frame row[1] + `0x80037664` → state 3, else `+178 += 2` |
| 3 | frame row[2] → state 4; a held button queues a re-fire (`+174 = 0`, `+177 = 1`) |
| 4 | overlay control `+103 == -1` → `+174 = 0`, `+177 = 0` |

`0x80037664(w)` = halfword +6 of the weapon's level row at `+4F7 + w*8`.

## Counter refill `0x8005C8A4`
Weapon 1 (the Buster) has counter 3 at level 0, and `0x80035724` decrements
it per shot. Object callback `0x8005C8A4` (table `0x8008D7C0`) refills it:
while `+171 == 1`, a timer at object `+0x0F` counts to 17 (0 at level 7) and
then stores the full count, the weapon 1 level row (`+0x4FD`) halfword +2
(`0x80037604`). Its condition is counter != 0 with full > 0, or counter == 0
with full >= 2. The object that owns it is untraced; the port runs it once per
gameplay frame (CANDIDATE). Without it, Azahar showed firing stop after three
shots.

## Overlay track and pose
- `0x8003C418`: when not busy, the overlay mirrors the main track (`+104 =
  +AC`, `+100 = +A4`). It starts with `0x800321C8` when `+104 != +105`,
  otherwise ticks with `0x800323FC(+174 & 3)`, which writes the pose selector
  `+106`.
- Request groups (`0x800320B8`/`0x800321C8`/`0x800329D4`): 0x00–0x3F main
  `PL00M00S/M` (pose action + 1); 0x40–0x5F right arm; 0x60–0x7F left arm,
  `PL00L04S/M` when `+171 != 0`, else `PL00L00S/M`.
- `PL00L04S/M` actions 0..3 = requests 0x60..0x63: 20 keys of one tick each,
  event bytes at keys 4/9/13, last key control −1 (hold).
- `0x800325D4`: selector 1 takes parts 0..4 from the overlay bank, 2 takes
  parts 0, 1 and 5..7 (torso and left arm), 3 takes 0..7. The overlay frame
  unpacks with the main channel map (channels 0..20).

## Shot sprite
`0x80029A38` draws the shot pool through `0x800898D0[+0 & 0x1C]` with the block
at `+0x88`. The 0x81 shot sets flags 0x2A, so it is kind 0x08, the sprite
`0x80029C18`:
- `+88` material `0x80097DAC[+4FC]` (level 0: `0x3EB4002E`);
- `+8C` `0x2E808080`, centre `+90` = position, UV 192/96, half size
  `+98 = 0x80097DCC[+4FC]`, angle `+9A` += 5 per frame;
- flag 0x20 rotates the corners by the 64-step tables `0x800BAEB0`
  (rcos) / `0x800BBCF8` (rsin) built at `0x80012028`; flag 0x08 uses the
  colour word without a depth cue.

## Port
- `source_player_fire.{h,cpp}`: fire input (Buster branches), `0x80040E00`,
  `0x800423DC`, lane `0x80042F78`, overlay track, selector merge.
- `main.cpp`: lane before the session tick, fire input and overlay after it
  (CANDIDATE placement within the frame); the spawn `0x80034EB8` reads
  `+174` bit 0x20; the renderer and the integer pose merge the overlay bank.
- The pack cooks `left.l3anim` from `PL00L04S/M`; L3BP/3 adds the fire rows
  and the shot materials.
- `child_runtime`: the 0x81 shot writes its sprite block;
  `draw_source_buster_sprites_uv` draws it.
- Test: `tests/test_source_player_fire.cpp` (authored data).

## Results
- Host: `test_source_player_fire` (fire, lane timing, re-fire, refill,
  selector merge) and `test_source_buster_profile` (L3BP/3); suite 179/179.
- `tools/m1_mac_build.sh` PASS, 0 warnings.
- Azahar 2126.0, Old 3DS, unattended autoplay, Area 1 start with no enemies:
  8 single presses → 8 shots, one per press; holding Y for 60 frames → a shot
  every 8 frames (the lane's state-3 re-fire). That is 23 shots in total, all
  spawned through `0x80034EB8` (`spawn_status=0`), with
  `# source_buster_sprite,drawn=1` while shots fly. Traces show
  `+174 = 0x82/0xA2`, `+104 = 0x60` and `+106 = 2` during firing. In the
  door room, a capture during a shot shows the left arm raised forward.

## Not ported / CANDIDATE
- A standing shot's main-track request (`0x60`, action 5) is HELD: the main
  track keeps its animation and only the overlay parts change.
- The `+170` special-weapon lane, `0x80037440`, the `+176` buffer writers and
  the right-arm banks.
- The native Y button stands in for Square (`+13A`), held or pressed.
- Shot levels other than 0 (only `0x3EB4002E` is in the material cache).
