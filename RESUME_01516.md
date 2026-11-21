# Resume — 0.0.15.16 (current handoff, 2026-11-21)

One page. The full log of this checkpoint is in
[docs/archive/resume/RESUME_01516_log.md](docs/archive/resume/RESUME_01516_log.md).

## Where it stands
- **main** has M2 (title/opening), the M1 tutorial and boss port, and M3 story
  steps S1–S4 verified in Azahar: ST02 event 4 → ST03 crash site, event 8
  scenes 0–2 (Inspector talk, police car), ST03 areas 0–2, the stage 0x0C ruin
  guard, the north gate into stage 4 (Apple Market) with its townspeople, and
  the junk-shop lady whose "Yes" sets flag 0xE (unlocks the first ruin).
- Shared systems on main: door events/locks (every stage, every area load),
  cross-stage loading, generic NPC behaviours 1/4/6/8, Data's menu with the
  original memory-card save screen and 8 KiB card block, the shared BIOS rand
  seed, shop message commands (host only), map-only bottom screen.

## In flight (two sessions)
- **stage-loading** (`claude/stage-loading-9d47ea`): S5 — stage 9 ruin entry,
  trapped man, class-0x19 Reaverbots on a reusable enemy framework, event 0x17
  (rescue). Host-tested; full S5 route running in Azahar. Next: S6+.
- **shared systems** (`claude/tutorial-boss-4a9127`): collision — the original
  contact-list rule (0x8003A3B4), the ST0C gate swap and sloped diagonals
  0x24/0x34/0x44/0x54; regression runs finishing. Then M1 as a playable
  release: death → game over → continue (1:1), first-divergence comparison of
  the tutorial corridor deaths against the oracle, one uninterrupted
  normal-start run. Then render fixes.

## Open, in order
1. M1 release: death/continue bug (player keeps playing at 0 HP, invisible);
   uninterrupted normal-start tutorial → boss → post-win → ST02 run.
2. S5–S11 to the first Tron fight.
3. Render/feel (user reports): invisible enemies and attacks, grey characters,
   white ST03 path / black sky, tutorial camera framing, arm swap after the
   tutorial, tutorial boss arms.
4. Junk shop Azahar check; seed profile 0x0325C4C7 for the shared rand.
5. Real Old 3DS run per milestone (`boot_01516.log`, `hardware_01516_*.csv`).
6. Final 1.0 recomp-vs-3DS comparison (`tools/compare_recomp_3ds.py` now
   checks clears and fails on extra flags; flags only, not movement/camera).

## Rules that bit us
- One Azahar run at a time (shared SD folder); check `pgrep "[a]zahar -w"`.
- A locked Mac screen stops Azahar and the oracle; keep it unlocked.
- Killing `m1_mac_build.sh` leaves a stale lock at
  `$PROJECT/work/mml3ds-cache-01500/.installer.lock` (holds the PID).
- Merge each "slice ready" with host ctest, `tests/test_source_tools.py` and
  `tools/m1_mac_build.sh` before pushing main.
