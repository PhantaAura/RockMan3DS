# Mega Man Legends 3DS

A native Old 3DS port of Mega Man Legends (PS1), rebuilt function by function
from the original executable and checked against a psxrecomp build of the US
disc. It needs your own game files; nothing from the retail game is in this
repository.

**1.0** = New Game → the first Tron Bonne fight. Milestones:

| Milestone | Scope | State |
|---|---|---|
| M1 | Title → tutorial → boss defeat → post-win → stage 2, death/continue working | Ported; boss won from a diagnostic start only. Death → continue bug and the uninterrupted normal-start run are open. |
| M2 | Title and opening | Done |
| M3 | Tutorial end → first Tron fight (story steps S1–S11) | S1–S4 verified in Azahar; S5 (first ruin) in progress; S6–S11 not ported |

Shared systems on main: doors and door locks, area and cross-stage loading,
NPC talk, Data's menu with the original memory-card save screen, collision
with the original contact-list rule and sloped diagonals (verification pending
merge), the shop message commands (not yet checked in Azahar).

Known gaps: invisible enemies/attacks and some grey (untextured) characters,
landscape textures, tutorial camera framing, Buster→normal arm swap after the
tutorial, tutorial boss arms. Performance: about 24–30 FPS in Azahar; no real
Old 3DS measurement yet.

## Start here
- [RESUME_01516.md](RESUME_01516.md) — the one-page current handoff.
- [AGENTS.md](AGENTS.md) — working rules.
- [docs/validation_01516/PROCESS.md](docs/validation_01516/PROCESS.md) — how milestones are run.
- [docs/validation_01516/M3_ROADMAP.md](docs/validation_01516/M3_ROADMAP.md) — story steps to the Tron fight.
- [docs/validation_01516/RESULTS.md](docs/validation_01516/RESULTS.md) — run evidence.

## Build and test
```bash
cmake -S . -B build/host && cmake --build build/host && ctest --test-dir build/host
python3 tests/test_source_tools.py
tools/m1_mac_build.sh   # native Old 3DS build (Mac with devkitARM)
```

Older release notes are in [docs/archive/README_HISTORY.md](docs/archive/README_HISTORY.md);
older resumes, mod logs and porting maps are in [docs/archive/](docs/archive/).
