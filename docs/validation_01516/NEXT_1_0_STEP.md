# After World Comes Alive: first small 1.0 step (plan)

Status: PLAN. 1.0 scope is New Game → the first Tron Bonne fight, nothing past it.

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
- Area 3 has **no actors** in the port (`instances=0`): the boss is not a
  placed EBD object. Finding and porting its spawn and AI is the next step.
- Arrival walk: port stops at z -1857, original continues to -1632
  (CANDIDATE: the shortened second walk in commit_source_door_transition).

## Order (user, 2026-10-25)
1. The tutorial 1:1: the Area 3 boss (spawn, AI, defeat) and what follows it.
2. Then the title screen, New Game and the opening scene before Area 1
   (all three observed in the oracle).
3. Then stage by stage toward the first Tron Bonne fight.

## Open items from WCA to carry
- Optional box flags 0x620/0x621 and the 0x60E action check were not exercised.
- Performance: about 20 FPS in Azahar; Mega Man's draw fixed first (Gate C).
- Physical Old 3DS test: PENDING (HARDWARE_CHECKLIST.md).
