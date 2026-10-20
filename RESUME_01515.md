# Resume 0.0.15.15 (WCA — tutorial radio and message box)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.14.

## Completed
- ST00 E0/06 tutorial message-trigger controller (`0x80114424`): walker,
  region test and handler types 0–15 (`src/flow/source_tutorial_triggers.cpp`),
  with lists cooked owner-locally (`tools/cook_source_tutorial_triggers.py`,
  L3TR/1). See `docs/research_01514/TUTORIAL_MESSAGE_TRIGGERS.md`.
- A tutorial dialogue mode in `RuntimeSession`:
  - presentation controls run inline, CLEAR 0x87 clears the page,
  - CALL 0x99 calls a block and returns,
  - SEL/MULTI 0x96/0x97 give a Left/Right Yes/No choice (0xFF continues),
  - CLOSE 0x84 holds the window for its delay before closing,
  - gameplay is gated only by the handlers' player `+0xBD` freeze.
- Glyph decoding (`mml_glyph_ascii`/`mml_glyph_icon`) and a bottom-screen
  message box.
- The Area-1 radio call matches the original in Azahar.

## Validation
`docs/validation_01515/RESULTS.md`: host 155/155, native PASS, Azahar trace.

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
| Dialogue and facial expressions | Partially implemented: tutorial message box (bottom screen, text + Yes/No); no portraits or voice |
| Natural tutorial progression | Partially implemented: E0/06 triggers types 0–15 ported; the first radio call and "Turn right there" validated in Azahar |
| Physical Old 3DS testing | Not run |

## Next
1. The subtype-0x1F pop-out Reaverbot (`0x801062EC`, turns into subtype 3 with
   parameter 0x40). It is the "small one" Roll points out.
2. The player-movement `+0x44D` writers (`0x8003CF58`/`0x8003D584`/`0x8003DCA4`),
   so the follow camera's eye-in-wall pull-in turns on as in the original.
3. Decompile type 16 (`0x801156C8`) and walk the rest of Area 1 and Area 2.
4. The top-screen message window, portraits and voice lines.

## Blockers
- **Voice:** voice lines are not played (`0x8001D648` is only logged).
- **Bank mapping:** area N → bank `st00_0N` is CANDIDATE.
- **Physical Old 3DS:** not tested.
