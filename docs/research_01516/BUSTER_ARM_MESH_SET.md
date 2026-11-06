# Player mesh set: the tutorial draws the Buster left arm

Status: **SOURCE_BACKED_PARTIAL**. Owner `ROCK_NEO.EXE` (SHA-256 `10d2008f…`),
Ghidra decompilation of `0x8002786C` checked against the instructions, plus a
measurement in the psxrecomp oracle (US disc, saved state at the first tutorial
corridor).

## Report
The user noticed that the port's Mega Man had the "peaceful" arms, not the
Buster arm.

## Original
- `INIT_DAT.BIN` makes three arm meshes resident at boot: `PL00R00P`
  (`0x800FA000`), `PL00L00P` (`0x800FCA00`) and `PL00L04P` (`0x800FDF00`). The
  `L04` set also brings its own skeleton `PL00L04S` (`0x800DD180`) and motion
  `PL00L04M` (`0x800F3600`).
- The player renderer `0x8002786C` draws five banks (body, head, right arm,
  left arm, feet) from one of three 5-word sets at `0x800898EC`:
  `set = +0x171 == 0 ? 0 : (+0x170 == 0 ? 2 : 1)`.

  | Set | Right arm | Left arm |
  |---|---|---|
  | 0 | `PL00R00P` | `PL00L00P` (plain) |
  | 1 | `PL00R04P` (`0x800FB500`) | `PL00L04P` |
  | 2 | `PL00R00P` | `PL00L04P` (Buster) |

  Part counts (6, 4, 3, 3, 2 at `0x80089928`) and the bone lookup at
  `0x80089938` are the same for every set, so the arm swap changes only the
  meshes.
- `0x8001F3C8` (area entry) sets `+0x171 = +0x173` and
  `+0x170 = 0x800B55FF ? +0x172 : 0` when `0x800B55FE` is set.
- `0x8001F5E4` loads the head (`+0x16A` → `0x80089200`), the legs
  (`(int8)+0x16B >> 1` → `0x80089204`) and the special-weapon right arm (only
  when `+0x172 >= 2`, table `0x80089208`).

## Measured (psxrecomp oracle, first tutorial control)
- Player `+0x170 = 0`, `+0x171 = 1`, `+0x172 = 0`, `+0x173 = 1`.
- `0x800B55FE = 1`, `0x800B55FF = 0`.
- Resident head HEAD01 (28) and legs LEG00 (31); no special-weapon arm (`0xFFFF`).

So the tutorial draws **set 2**: the plain right arm and the **`PL00L04P` Buster
left arm**. The port (since 0.0.14.x) always cooked `PL00L00P`, which is set 0.

## Fix
`tools/source_player_arms.py` reproduces the `0x8002786C` choice at cook time.
It reads the set at `0x800898EC` for the loadout (+170 = 0, +171 = 1) and maps
each bank address to the collected PBD with that load address. The pack cooks
`left.l3model` from `PL00L04P` (3 parts, 51 vertices). The composition, player
material cache and runtime are unchanged. A missing bank (for example set 1's
`PL00R04P`, which is not resident in the tutorial) fails the cook instead of
being substituted. Test: `tests/test_source_player_arms.py`.

## Still open
- Arm motion banks `PL00L00M`/`PL00L04M` are resident, but no direct EXE
  reference to their addresses was found, and the port does not play them
  (`right.l3anim`/`left.l3anim` are cooked but unused). Whether the Buster fire
  pose comes from them is UNKNOWN.
- The loadout stays fixed for the session: `+0x170/+0x171` changes at runtime
  (special weapons, set 1) are not followed.
