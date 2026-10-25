# After World Comes Alive: first small 1.0 step (plan)

Status: PLAN. 1.0 scope is New Game → the first Tron Bonne fight, nothing past it.

## Where the boot path stands (2026-10-25)
- Power-on reaches a hardware-test start menu, not the original title screen.
- New Game starts at the source hand-off 0x80101274 in tutorial Area 1 with
  `opening_scene=SKIPPED` (boot log): the opening cutscene is not ported.
- The tutorial (stage ST00) is playable through 0x622 in Azahar, normal boot.
- ST00 has five sub-areas on the disc (ST00_00..ST00_04); the port plays
  Areas 1 and 2. What Areas 0, 3 and 4 are, and what the game does after 0x622,
  is UNKNOWN in the repo.

## Recommended first checkpoint: the tutorial exit (0x622 → next stage)
Small, on the critical path, and it removes the biggest unknown.
1. Observe: in the MMLUS recomp (ST00 runs in its interpreter), play to the far
   door and record what follows 0x622 (messages, scene, stage/area load). Stop at
   the next stage's first controllable moment.
2. Trace the owner in Ghidra (ST00 overlay project): the 0x622 writer, the event
   or hand-off it starts, and the stage-load request.
3. Port only that transition: message/event, unload of ST00, load of the next
   stage package, the spawn. Host test for the hand-off; Azahar run from normal
   boot through it.
Acceptance: the normal-boot route continues past 0x622 into the next stage
without a diagnostic spawn; no content past the hand-off.

## Next after that (in order)
- The opening scene before the tutorial (removes `opening_scene=SKIPPED`).
- The original title screen and New Game menu.
- Then stage by stage toward the first Tron Bonne fight.

## Open items from WCA to carry
- Optional box flags 0x620/0x621 and the 0x60E action check were not exercised.
- Performance: about 20 FPS in Azahar; Mega Man's draw fixed first (Gate C).
- Physical Old 3DS test: PENDING (HARDWARE_CHECKLIST.md).
