# Resume 0.0.15.13 (WCA — enemy attacks no longer freeze)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.12 (`06a9d35`).

## Completed
- Owner Azahar test of 0.0.15.12: Reaverbots froze when close and could not be
  hit. Cause: action 10 (lunge, chosen by the pursuit when Mega Man is near)
  was unported, so the body held every frame and its hit proxy never updated.
- Ported ST00 actions 10 (`0x80103DFC`), 6 (`0x80103554`, hit knockback) and
  5 (`0x8010324C`, damaged bounce then self-destruct) in
  `src/world/class20_03_late_actions.cpp`, wired after the hit consumer.
- Evidence: `docs/research_01513/LATE_ACTIONS_5_6_10.md`.

## Validation
`docs/validation_01513/RESULTS.md`: host 153/153, native PASS. Installed on
Azahar's SD; owner test pending (`docs/validation_01513/AZAHAR_TEST.md`).

## World Comes Alive status
| System | Status |
|---|---|
| Environment rendering | Partially implemented (doors look closed) |
| Movement and collision | Partially implemented |
| Original follow camera | Validated in Azahar (ordinary follow only) |
| Buster combat | Partially implemented; shots fire in Azahar, hit/defeat checked on host |
| Enemy behaviour | Partially implemented: lunge, knockback and damaged bounce ported (0.0.15.13); Azahar pending |
| First door progression | Validated in Azahar (from the diagnostic door spawn) |
| Original lock-on | Experimental: runs, target selection not yet observed |
| Enemy rewards and pickups | Partially implemented: gameplay lifecycle ported; diagnostic visuals; Azahar pending |
| Opening cutscene | Not implemented |
| Dialogue and facial expressions | Not implemented in gameplay |
| Natural tutorial progression | Not implemented past the first door |
| Physical Old 3DS testing | Not run |

## Next
1. Azahar: confirm enemies lunge and resume, take Buster hits, and die; then
   check the reward pickups (0.0.15.12) from a real defeat.
2. If shots still never hit (`impacts=0`), trace the Buster segment vs. actor
   proxy pair test (`consume_proxy_pairs`) with the owner log.
3. Pickup model `0x8002F9C4`; tutorial progression (Roll's radio events).

## Blockers
- **Azahar:** 0.0.15.13 not yet played.
- **REA:** cannot open PS1 code; use Ghidra headless / the overlay decoder.
- **Physical Old 3DS:** not tested.
