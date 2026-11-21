# Resume 0.0.15.6 (M2 — first Area 1 door)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.5.

## Completed
- Ported the player door event: query `0x800514FC`, gate `0x80051970`,
  `0x80050C2C` case 3 with `0x800519E4`, and the `0x800C3558..71` request lanes
  (`src/flow/source_door_event.cpp`, `runtime_player_event_primitive_800514fc`).
- `RuntimeSession` runs it after player contact while flag 0x200 is clear. The
  commit reuses `apply_source_handoff_spawn_80015ee8`, then applies the event-0
  substate-8 arrival `0x80048898` and the net event-0 flags.
- 3DS: A now sends psx Circle (0x2000). The main loop commits a door request
  through the same resource-staging path as the ST00 handoff.
- `tools/l3door_replay` phase 2 (Up, then Circle at the door) reaches Area 2 at
  (−256,−2,6208), heading 2048, as the source computes. New test
  `source_door_event`. Host suite 148/148.
- Evidence: `docs/research_01506/DOOR_EVENT_80050C2C.md`,
  `docs/validation_01506/RESULTS.md`.

## Next
- Port event 0 (`0x800485E4`) so the Area 2 door opens and Mega Man walks in:
  the overlay door object (`0x80112568..`, flags 0x7B1/0x7B2, IDX descriptor
  swap), the forced walk `0x80048A9C` (event +0x10 counter), and the fades.
  Without it he is left in the Area 2 vestibule in front of the closed door.
- Owner: run `docs/validation_01506/AZAHAR_TEST.md`; send `boot_01506.log` and
  `hardware_01506_*.csv`.
- Still open from 0.0.15.5: the 32 s / 44 s camera spots; pack section 35 into
  `ST00.mmlpkg`.
