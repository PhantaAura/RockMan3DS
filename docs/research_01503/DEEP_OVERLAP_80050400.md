# 0x80050400 deep-overlap recovery and 0x8004B6AC correction history (0.0.15.3)

Evidence: USA `ROCK_NEO.EXE`, decoded with `psx_mips_dis.py` (prints only
instructions), cross-checked against the owner-local Ghidra export and against Ghidra 12.1.4
headless disassembly of the same bytes (375/375 instructions identical over the
three verified ranges; REA 6.7.0 could not open PS-X EXE or MIPS ELF targets). Every claim
below is from the R3000 code unless marked otherwise.

## Call path — VERIFIED (instructions)
`0x8004B6AC` scan → `0x8004BDC8` dispatcher → `0x8004F44C(pos=0x1F8000C0,
pre=0x1F8000C8, mode)` → `0x80050400(pos, pre, mode)` when the X, Z and Y
eligibility helpers (`0x80050808/0x800508BC/0x80050970`) all reject: no single
face correction fits that axis's per-pass movement delta, or its direction is
blocked. This is the only call site. Modes reaching it in the native subset:
1 (primitive code 1) and 3 (code 3 while airborne and moving up). Mode 0
(code 2) and `|0x43` (codes 0x0B/0x0C) stay unported.

## 0x80050400 — VERIFIED (instructions 0x80050400..0x80050804)
Rectangle: `0x1F8000F0/F2/F4` = x0/y0/z0, `F8/FA/FC` = x1/y1/z1 (bounds-expanded).
- `cx`: `|x0-px| < |x1-px|` → `x0-px`, else `x1-px` (tie → high face). Same for `cz`.
- `cy`: mode bit 2 → `y0-py` (top face); else nearest face, tie → `y1`.
- Corrections are stored as halfwords; comparisons use their sign-extended magnitude.
- Mode bit 1 and `|cy| < min(|cx|,|cz|)` and `(blocked & 0x30) == 0` → Y:
  `pos.y += cy`, `pre.y += cy`, `h.y = -cy`, `0x80050BF4` (blocked = `cy>0 ? 0x10 : 0x20`,
  returns `cy <= 0`).
- Otherwise X if `|cx| < |cz|`, Z if `|cz| < |cx|`; a tie picks X unless
  `blocked & 0x03`, then Z. (The `0x0C` test at 0x80050740 cannot change the result.)
  X: `pos.x += cx`, `pre.x += cx`, `0x80050B74`; Z likewise with `0x80050BB4`. Returns 0.

`pre` moves with `pos`, so the movement delta seen by later candidates is unchanged.

## Correction history — VERIFIED (instructions 0x8004BB24..0x8004BC1C, 0x8004F5F0..0x8004F8B4)
- h = `0x1F800100/102/104`, the current response's negated correction. u =
  `0x1F800106/108/10A`, the pending correction.
- On every restart: `pos += u`, then `u = h`, then `h = 0`. Up to 8 restarts are
  allowed; the 9th returns `0x80000000` (rollback).
- Writes to h:
  - `0x8004F44C`: X/Z only in the branch where both horizontal axes are eligible.
    Y always, except that branch in mode 3.
  - `0x8004F8BC/FBE8`: eligible branch only. `0x8004FF14`: Y only.
  - `0x80050400`: Y only.
- Clears of u:
  - `0x80050B74` clears u.x if the previous blocked byte was X (0x03).
  - `0x80050BB4` clears u.z if the previous blocked byte was Z (0x0C).
  - All floor/ceiling snaps (negative cells, code 1 step, code 3, code 15) clear u.
  - Any response returning true clears u at `LAB_8004C144`, case 3, `LAB_8004C0C8`
    and `LAB_8004E17C`.
  - `LAB_8004C0C8` also clears u.x/u.z on false.
- Grounded `+74 = 0` is published only for a true result with `dy >= 0`, and not by case 3.
- Effect: a correction is undone at the next restart when the following response is
  on another axis and does not clear it.
- `0x1F80010C..116` copies only feed the x5 diagonal fallback, which is not ported
  (UNKNOWN for that path).

## Other fixes made while porting (instructions)
- `0x8004F8BC/FBE8` both-ineligible Y branch also moves `pre.y`.
- `0x8004FF14` Y branch stores blocked `0x20` on either side and returns 1.

## Not established
- No paired PS1 movement trace has been captured for corners or for deep overlap. All
  vectors are hand-derived from the instructions plus synthetic geometry.
  `PHYSICS_1TO1=false`.
