# Cutscene importer (tools/cook_cutscene.py, L3CS)

Every cutscene runs on the same engine services, configured by constants
the event code stores. The importer finds those for any stage event and
cooks them into one package; per-scene behaviour (the record callbacks)
stays hand-ported.

## What it finds
From the EXE event table 0x80089024[id] it follows the handler, its
dispatch tables (bounded by the handler's own `sltiu` when present), the
overlay functions it calls and the timeline callbacks, tracking
straight-line register constants:

| Engine service | Pattern | Cooked |
|---|---|---|
| camera program 0x8001EC0C | word stored to 0x800BC7F0 (camera block +0xB8) | commands parsed by operand count up to op 0xFF |
| timeline 0x8001F070 | word stored to event+0xA4 | records up to the 0xFF terminator |
| spawns 0x8001EAE8 | constant a0 | 20-byte records |
| CD/XA 0x8001D58C | constant a0 | request words |
| area loads | constant a0 of a function storing a0 to 0x800C356F | callback -> area |
| background layers 0x80022FD4 | indexed word the stage init passes | table address |
| event data | `--extra ADDR:BYTES` | raw blocks |

## Validation (2026-11-01)
Run on the three events ported by hand, it finds exactly the tables found
by hand:
- event 1 (ST00): 8 phases, timeline 0x8011B2E8 (17 records + terminator),
  camera 0x8011AF4C (231 words, as `cook_source_event_phase6.py`),
  CD 0x20000003, spawn 0x8011AE0C, background table 0x8011A6F0;
- event 71 (ST01): 11 phases, timeline 0x80100F7C, camera 0x80100F30
  (19 words), CD 0x2000001E, spawn 0x80100F1C, background 0x80100D5C;
- event 10 (ST07): 3 phases, timelines 0x80105E1C / 0x801060D4, cameras
  0x80105DD0 (19) / 0x80105E2C (170), six spawn tables, CD 0x20000005,
  areas 1-4 from callbacks 0x80100944/9B8/B14/B98, background 0x80105658.

## Runtime
`source_scene_runtime`: the L3CS loader (look-up by original address), the
camera block and interpreter 0x8001EC0C, the timeline consumer 0x8001F070.
Event 10 (`source_attract_event10`) binds its tables from the package.
The opening (event 1) still uses its own profile; the narration (event 71)
its own player. Both can move onto the shared runtime later.

## Adding a cutscene
1. `cook_cutscene.py --exe ROCK_NEO.EXE --overlay r3_stXX.bin --event N
   --stage S --report out.json` and read the report.
2. Cook it in the pack (`--out`), plus `--extra` for event-specific tables
   its code reads directly.
3. Port the handler's phases and the record callbacks against the package.
