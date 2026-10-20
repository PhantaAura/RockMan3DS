# Why the 0.0.15.10 Buster shot was not seen

Azahar 0.0.15.10 (`docs/validation_01510/AZAHAR_RESULTS.md`): three Y presses
arrived, but no shot was seen, CSV `projectiles` stayed 0 and action `+9`
stayed 0.

## Finding

The shot was spawned. Only its on-screen ribbon was empty, and that has been
true since 0.0.15.0.

Host replay of the main.cpp chain with the owner pack and the Azahar spawn
(`-334,-1,-5750`, heading 0), with no enemy proxies, as when `broad`/`window` were −1:

- Fire admission passes: shot enable `+0x175` bit 0 is set and `0x8004324C` is
  not busy.
- `0x80034EB8` returns status 0, then `0x80035724` allocates slot 0
  (`0x800BFB20`).
- `0x80017154` → `0x800433C0` registers the segment and keeps the shot for 11
  frames, then releases it.

Each frame the diagnostic ribbon drew from +2C/+30 to +14/+18 of the shot
record. The original subtype-0x80 code makes these two points equal before
anything can read them (R3000, `docs/research_01503/psx_mips_dis.py` on
`ROCK_NEO.EXE`; the script does not decode `lwl`/`lwr`/`swl`/`swr`, which are
decoded by hand below):

| Address | Instruction | Effect |
|---|---|---|
| `0x8004351C..38` | `lwl/lwr` player `+0x14/+0x18`, `swl/swr` shot `+0x2C/+0x30` | state 1: owner point into +2C |
| `0x8004353C..50` | `+0x0B` −1, `bne` | 11-frame timer; 0 → release `0x80031930` |
| `0x80043568` | `jal 0x80037694` | contact → impact `0x80043740`, state 2, player `+0x198` = 1 |
| `0x80043628..44` / `0x80043674..90` | player `+0x400/+0x404` (if `+0x0E`) else `+0x418/+0x41C` → shot `+0x14/+0x18` | endpoint |
| `0x80043700` | `jal 0x8003430C` (a0 = +0x14, a2 = +0x2C, `0x28000008`) | segment registration |
| `0x80043708..24` | `lwl/lwr` shot `+0x14/+0x18`, `swl/swr` shot `+0x2C/+0x30` | +2C := +14 |

`ChildRuntime::tick_source_busters_80017154` follows this exactly, so after
every registration +2C equals +14. The ribbon therefore had zero length on
every frame, giving degenerate triangles and nothing on screen.

The other two signals in the report do not count shots:

- CSV `projectiles` counts `RuntimeSession::projectiles()`, the candidate
  projectile retired in 0.0.15.0. Source Buster shots live in
  `ChildRuntime::buster_pool()`.
- Action `+9`: no source writer of a fire action on Y is ported. The animation
  callback that asserts `+0x174` bits 0x10/0x20 is still untraced.

## Bisect

The ribbon draw (`draw_source_busters_uv`) and the +14 → +2C copy are identical
at 0.0.15.5 (`39895b3`), 0.0.15.9 (`553c95a`) and 0.0.15.10 (`3fe4703`). The
0.0.15.10 lock-on wiring is not a regression: the shot is spawned through
`0x80034EB8` exactly as the 0.0.15.9 direct `0x80035724` call spawned it.

## Fix (0.0.15.11)

- `ChildRuntime` records, per slot, the two endpoints it hands to `0x8003430C`
  (`buster_registered_segments()`). This is native bookkeeping only; gameplay
  never reads it.
- `draw_source_busters_uv` draws that recorded segment. In state 1 it runs from
  Mega Man's `+0x14` to the endpoint. On the state-0 frame it is still endpoint
  to endpoint, so a shot is visible on 10 of its 11 frames.
- Traces: `# source_buster_runtime` now has `live=` (allocated shots), and
  `# source_buster_draw` is logged on every frame that draws a shot.

## Still CANDIDATE / UNKNOWN

- The endpoint is the native forward ray (0x800 units from the muzzle). The
  writers of player `+0x400/+0x418` are untraced, so the ribbon length is not
  original.
- The ribbon is FRONTEND_DIAGNOSTIC. The original shot sprite/effect renderer
  is not ported.
- The Y edge still stands in for `+0x174` bits 0x10/0x20; fire cadence and the
  action `+9` change on firing are UNKNOWN.
