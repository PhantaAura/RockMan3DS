# After World Comes Alive: first small 1.0 step (plan)

Status: ACTIVE ROADMAP. Version 1.0 is the complete main campaign; the next
contiguous milestone remains the tutorial boss and its post-victory event.

## Where the boot path stands (2026-10-25)
- Power-on reaches a hardware-test start menu, not the original title screen.
- New Game starts at the source hand-off 0x80101274 in tutorial Area 1 with
  `opening_scene=SKIPPED` (boot log): the opening cutscene is not ported.
- The tutorial (stage ST00) is playable through 0x622 in Azahar, normal boot.
- ST00 has five sub-areas on the disc (ST00_00..ST00_04); the port plays
  Areas 1 and 2 and now enters Area 3 (the boss room). Areas 0 and 4: UNKNOWN
  (Area 0 is probably the opening scene; not checked).

## What the original does after 0x622 (psxrecomp oracle, 2026-10-25)
Observed in the MMLUS recomp (US disc) over its TCP debug server; Area 1 and
Area 2 were reached with two oracle-only RAM teleports (noted, not a route).
- New Game plays an opening of about 6,000 frames before control (flag 0x604
  during it): Mega Man in a dark ruin, a large refractor, a scripted walk and
  fall. He then lands at the Area 1 start (2822, -1, -4833), where the port's
  boot hand-off already starts.
- Trigger 15 (Roll's message) sets 0x61E and 0x622 together, as in the port.
- The door at the end of Area 2 (z about 15583) opens with Circle and loads
  **ST00 Area 3**: Mega Man arrives at (-256, -385, -1920) and walks to -1632.
- Area 3 is a **boss fight**: a large Reaverbot with its own health gauge.
  Losing goes to game over and back to the title screen. What follows a win
  is not observed yet.

## Port status of that exit
- The port's door event at the far door already requests area 3 with the same
  spawn (-256, -385, -1920); the 3DS build commits it (Azahar, frame 1440:
  Area 3 loaded, music request 1, Mega Man grounded at z -1857). The route
  script now presses A at that door after 0x622.
- Area 3 has no placed actor instances. Event 3 is now proved to allocate
  class `40/02`. The port dynamically decodes its real 12-part model at the
  fresh original spawn `(-256,-385,1001)`, forces Mega Man to z `-1632`, and
  follows the observed request/movement phases to z `-1223`, then executes the
  first source-observed action-6 request 11/12/13 presentation cycle. The
  owner is explicitly restricted to source Area 3 because event ID 3 is reused
  in earlier tutorial rooms. The native path admits the event-authored position
  with a neutral pad and retains z `-1632` through the presentation while
  preserving the source dialogue/cutscene/player gates. Host and Old 3DS builds
  pass. Continuous Azahar trace `hardware_01516_4002111785105.csv` reaches
  `0x622` at frame 9772, CombatReady at 10090, retains z `-1632` through frame
  10800, and unloads cleanly at 10854.
  Attack AI, damage, HUD, defeat and the next event remain missing. See
  `docs/research_01516/AREA3_BOSS_SOURCE_SLICE.md`.

## Order (user, 2026-10-25)
1. The tutorial 1:1: the Area 3 boss (spawn, AI, defeat) and what follows it.
2. Then the title screen, New Game and the opening scene before Area 1
   (all three observed in the oracle).
3. Then stage by stage toward the first Tron Bonne fight.

## Contiguous campaign milestones

- **M1 — Tutorial boss:** start after `0x622`; Area 3 event 3, scripted entry,
  class `40/02` AI/attacks/collision, 1024-HP gauge, Buster reactions, death,
  game over, and original post-win transition. Accept with focused host state
  tests plus an uninterrupted Old-3DS-mode Azahar run. Current deviation:
  spawn and intro are source-backed; live combat behavior remains unimplemented.
- **M2 — Opening/title flow:** power-on title, New Game, ST00 Area 0 opening,
  and the existing Area 1 handoff with original flags/camera/audio. Accept from
  cold boot through control in Area 1 and through M1 without a shortcut.
- **M3 — First Tron Bonne encounter:** add each required area, NPC, mission,
  transition and boss in story order. Accept from M2's end through that fight,
  with save/restart progression checked.
- **M4+ — Remaining main route:** one contiguous story segment per checkpoint;
  each names its start/end flags, required areas, interactions, boss, host
  tests, Azahar result and known deviations before the next segment begins.
- **Final — Campaign/release:** final boss, ending/credits, Continue/save/load,
  game-over/retry, remaining mandatory systems, optional-content ledger, broad
  regression, bounded memory, and physical Old 3DS performance validation.

## Open items from WCA to carry
- Optional box flags 0x620/0x621 and the 0x60E action check were not exercised.
- Performance: about 20 FPS in Azahar; Mega Man's draw fixed first (Gate C).
- Physical Old 3DS test: PENDING (HARDWARE_CHECKLIST.md).
