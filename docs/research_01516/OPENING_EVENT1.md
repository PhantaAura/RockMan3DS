# New Game opening: ST00 event 1 (research + wiring, 2026-10-30)

Status: SOURCE_BACKED_PARTIAL. The original event-1 logic was already ported
(phases 2, 4, 5, 6 and the phase-7 hand-off, `src/flow/source_event_*.cpp`)
but only reachable through the diagnostic `SOURCE_EVENT_BOOT=1` SD files, and
the build no longer cooked its assets. This pass cooks them and makes NEW GAME
boot that path. **Not yet run end to end on the port** (see Open).

## Original handler (ST00 overlay, read from the instructions)
`0x801010C4` switches on event+04 through table `0x80100018`:

| Phase | Code | Effect |
|---|---|---|
| 0 | 0x80101104 | wait until busy `0x800A8A72` == 0 |
| 1 | 0x80101120 | `0x80042238(player)`; player byte0 &= 0xFC (hidden) |
| 2 | 0x8010114C | `0x8001B33C` (stop sequences), `0x8001D58C(0x20000003)` (CD/XA command, id 245) |
| 3 | 0x80101168 | wait `0x8001D878`; event+0C = 55 |
| 4 | 0x80101184 | count down +0C; at 0: phase 5, fade `0x80017C30(2)` |
| 5 | 0x801011B4 | constructor `0x80100EFC` (camera 0x8001E6C0, 1 class-40 + 15 E0 objects) |
| 6 | 0x801011D0 | scene callback `0x8010102C`, camera `0x8001EC0C`, timeline `0x8001F070`; when busy 0 and (event+09 == 19 and +10 == 44, or `0x8001DE84`): fade 10, phase 7 |
| 7 | 0x80101248 | same three calls; when busy 0: request stage 0 / area 1 at (2822,-1,-4833) yaw 0 status 2, clear flag 1540, player byte0 \|= 1, `0x80042314`, `0x8001D494(0,1,0)` |

`0x8001DE84` (EXE) is the Start skip: it only returns Start (`0x800C0C2A & 8`)
when `0x800C1B6A` != 0 or flag `896 + event id` (897) is set. On a fresh New
Game 897 is clear, so the scene cannot be skipped and ends by the timeline.

Timeline `0x8011B2E8`, 8-byte records (duration, index, callback):
515/0 `0x80100738`, 351/1 `0x801007F0`, 245/2, 190/3, 90/4, 50/5, 145/6,
430/7, 125/8, 145/9, 93/10, 37/11, 97/12, 26/16, 50/17, 370/18, 60/19
`0x80100ECC`, terminator 0xFF. Indices 13..15 do not exist. The callbacks set
class-40 actor (event+24) actions 1..12 at fixed local frames, E0 actions,
the scene tint bytes `0x80122428..38`, and 0x83 dust effects (RNG `0x8007FE50`).

## Oracle (psxrecomp, US disc, 2026-10-30, turbo, title Start/Cross only)
Frames are oracle frames; scratch trace not committed.

| Frame | Main phase/sub | Stage | Event act/id/phase/+09 | Player |
|---|---|---|---|---|
| 2073 | 3/2 | 0 | 1/1/4/0 | — |
| 2171 | 3/2 | 0 | 1/1/6/0 | (80,0,-544) scene start |
| 3208..8095 | 3/2 | 0 | 1/1/6/1..19 | records advance in table order (0..12, 16..19) |
| 8183 | 3/2 | 0 | 1/1/7/19 (+10 = 47) | |
| 8224 | 2/0 | 0, area 1 | 0/1/7 | (2822,-1,-4833) heading 0 |
| 8370 | 3/2 | 0, area 1 | 0/1/7 | control |

Phase 6 lasts 6012 oracle frames (2171 → 8183); the timeline sum is 3019
ticks, so the scene ticks every second frame (CANDIDATE: 30 Hz game logic).
Story flags set at control: 510, 897, 906, 967, 1216, 1319, 1418..1423,
1443, 1444, 1472 (1540 clear). These are the reference for the port's flag
state after its hand-off.

## Port changes (this pass)
- `tools/build_tutorial_pack.py`: cooks `source_event.l3ep` (poll table),
  `source_event_phase2.l3eq`, `source_event_command.l3ec`,
  `source_prop_lift.l3pl`, `source_effect_dispatch.l3f2`, and
  `materials/scene_actor40_initial.l3sm` (recipe `01516-opening-1`).
- `tools/m1_mac_build.sh`: cooks the phase-2 XA stream
  `audio/stream_245.l3xa` from the CUE (cached).
- `platform/3ds/source/main.cpp`: when the title will run and all event-1
  assets are present, boot arms `g_new_game_opening`: the source event path
  (Area 0, fresh ST00 event subset) plus the scene E0/actor40/texture/camera
  owners and XA, instead of the direct hand-off spawn. `opening_skip.txt` in
  the SD folder restores the direct hand-off for tutorial-route tests.
- `docs/validation_01516/autoplay/opening_newgame.txt`: title → NEW GAME →
  7000 idle frames (use with `diagnostic_title.txt`).

## Open
- **No Azahar run yet.** `azahar -w` stalls at the game list while the Mac's
  current Space is a full-screen app (also seen by the Reaverbot session).
  Needed: the opening run (phases 0..7 → Area 1 control, compare flags above),
  then the tutorial route to `0x622` from that start.
- The 01444 diagnostic run stopped at frame 689 inside timeline record 1 (it
  was not a deadlock: record 0 is 515 ticks). Lower-80 subtype 2 callback
  `0x80116E68` was held then and may still hold.
- Gameplay after the hand-off now runs with `g_source_event_boot` set; the
  0x622 route was only validated with it clear.
- Phase-6 tick rate vs the port's 60 Hz session tick (see oracle) is unchecked.
- Out of scope: ST01 intro event 71 and the attract demo (ST07 event 10).
