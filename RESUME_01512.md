# Resume 0.0.15.12 (WCA — enemy reward pickups)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.11 (`319893e`).

## Completed
- Original reward lifecycle after a defeat:
  - `0x80047228` → `0x80046B34` expansion;
  - the subtype-0x87 pickup states;
  - `0x80046A0C` collection paying money and energy.
- Code is in `src/world/source_reward_pickup.cpp`, tables come from
  `tools/cook_source_reward_profile.py` (`reward.l3rw`, cooked into the pack),
  and the 3DS loop runs it after the effect tick.
- Evidence: `docs/research_01512/REWARD_PICKUPS_80046B34.md`.

## Validation
`docs/validation_01512/RESULTS.md`: host 152/152, native PASS. Azahar pending.

2026-10-20 cloud session (Linux, no Azahar, no devkitARM, no retail data):
the Azahar run cannot be done there. Host re-check with Clang 18 passes
151/151; the 152nd test (the renderer guard) only registers when
`arm-none-eabi-gcc` is found. GCC 13 fails the host build on
`-Werror=misleading-indentation` / `range-loop-construct` in about 50 test
files. This is a pre-existing GCC-only style issue, so use Clang on Linux.

## World Comes Alive status
| System | Status |
|---|---|
| Environment rendering | Partially implemented (doors look closed) |
| Movement and collision | Partially implemented |
| Original follow camera | Validated in Azahar (ordinary follow only) |
| Buster combat | Partially implemented; shots fire in Azahar, hit/defeat checked on host |
| Enemy behaviour | Partially implemented |
| First door progression | Validated in Azahar (from the diagnostic door spawn) |
| Original lock-on | Experimental: runs, target selection not yet observed |
| Enemy rewards and pickups | Partially implemented: gameplay lifecycle ported; diagnostic visuals; Azahar pending |
| Opening cutscene | Not implemented |
| Dialogue and facial expressions | Not implemented in gameplay |
| Natural tutorial progression | Not implemented past the first door |
| Physical Old 3DS testing | Not run |

## Next
1. Azahar: defeat a Reaverbot, watch the pickups, collect one, and check lock-on
   with an enemy ahead.
2. Pickup model: port the `0x8002F9C4` stage resource (`0x8016C000` list) so the
   real refractor/energy models draw.
3. Tutorial progression: Roll's radio events after the first room. Use
   PCSX-Redux traces to fix the event order.

## Blockers
- **Azahar:** the 0.0.15.12 Azahar check is pending. The owner's Azahar session
  was open, so the build was not installed on Azahar's SD. Install it from the
  `pass-20261019-211819-m2-01512` output first.
- **REA:** REA (rea-agents 6.7.0) cannot open `ROCK_NEO.EXE`. Its own
  file-type check accepts only ELF, PE and Mach-O, and its raw-binary route has
  no MIPS. The Ghidra PSX loader is installed. Use Ghidra 12.1.4 headless
  directly (`~/Downloads/ghidra_12.1.4_PUBLIC`, JDK Semeru 21) for original-code
  work.
- **Physical Old 3DS:** not tested.
