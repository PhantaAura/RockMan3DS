# Resume 0.0.15.14 (WCA — Buster shot, connected combat loop)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.13 (`830a9ab`).

## Completed
- 0.0.15.13: enemy actions 10/6/5 ported, so enemies no longer freeze.
- `d4c91e2`: opt-in `diagnostic_autoplay.txt` (scripted input, steering to a
  pickup or a point), so Azahar can be tested without screen control.
- `0745bf2`: original Mega Buster shot subtype 0x81, impact effect 0x82, and
  the fire lane (+174 bit 0x20 / weapon +171). It was found by running the
  original game in PCSX-Redux. See `docs/research_01514/BUSTER_SHOT_81.md`.

## Validation
`docs/validation_01514/RESULTS.md`: host 154/154, `l3door_replay` PASS,
native PASS. Azahar (scripted, trace evidence) covers: shoot → enemy damage
→ damaged bounce → defeat → rewards → collect +5 money, and door Area 1 → 2
→ 1.

## World Comes Alive status
| System | Status |
|---|---|
| Environment rendering | Partially implemented (doors look closed; not rechecked) |
| Movement and collision | Validated in Azahar (walk/turn/walls); diagonal/deep overlap host-only |
| Original follow camera | Validated in Azahar (trace) |
| Buster combat | Validated in Azahar (trace): subtype 0x81 shots hit; visuals diagnostic |
| Enemy behaviour | Validated in Azahar (trace): actions 0/3/4/5/9/10; player damage untested |
| Enemy damage and defeat | Validated in Azahar (trace) |
| Original lock-on | Partially functional: never selected a target; homing unported |
| Enemy rewards and pickups | Validated in Azahar (trace); model is a diagnostic marker |
| First door / Area 2 and return | Validated in Azahar (trace) |
| Opening cutscene | Not implemented |
| Dialogue and facial expressions | Not implemented in gameplay |
| Natural tutorial progression | Not implemented past the first door |
| Physical Old 3DS testing | Not run |

## Next
1. Highest priority: Roll's radio events and tutorial progression after the
   first door, using PCSX-Redux probes (pad-driven, as in
   `redux_buster_probe.lua`) to fix the event order.
2. Lock-on: find why no target is selected (the lane-0 window is
   x 0x83..0xBD, depth < 0x500), then port homing `0x80033F04`.
3. Whether enemy attacks damage Mega Man (action-4 child, contact packets).
4. Pickup model `0x8002F9C4`; Buster shot sprite presentation.

## Blockers
- **Azahar screen control:** not approved this pass, so there are no screenshots;
  evidence is CSV traces from scripted runs.
- **Persistent loadout:** the writer of `0x800B5F88/89` is untraced; the
  tutorial values (0, 1) are MEASURED.
- **REA:** cannot open PS1 code; use Ghidra headless.
- **Physical Old 3DS:** not tested.
