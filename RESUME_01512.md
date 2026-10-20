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
