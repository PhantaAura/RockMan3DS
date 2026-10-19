# Resume 0.0.15.7 (M2 — event 0 door sequence)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.6.

## Completed
- Ported event 0 (`0x800485E4`): phase 0 hand-over `0x80042238`, phase 1's 14
  substates (table 0x80010D6C), phase 2 release `0x80042314`; counter +0x10 is
  `0x8001F070`'s per-frame increment. Forced walk `0x80048A9C` with the
  0x8008AA04 limit (stage 0: 108). Player state 3 `0x80041B8C`, state 0
  `0x8003C7B0`, the `0x8003C07C` input selection and the state-1 +0xBD bit-8
  clear (`src/flow/source_door_event.cpp`).
- ST00 door object (states 0–5, table 0x8011C928) with the IDX descriptor swap
  (`0x8003AC24`, word0 & ~0x1000); native copy-on-write map in `RuntimeSession`.
- The load is offered only at substate 6; the commit runs substates 6–8, then
  the session opens the Area 2 door, walks him in and closes it.
- `tools/l3door_replay`: Mega Man ends in Area 2 at (−256,−1,7104), past the
  door cell; flags clean; floor present. Return trip checked by a scratch probe.
  `source_door_event` extended. Host 148/148; native build PASS, 0 warnings.
- Evidence: `docs/research_01507/EVENT0_DOOR_SEQUENCE.md`,
  `docs/validation_01507/RESULTS.md`.

## Next
- Owner: run `docs/validation_01507/AZAHAR_TEST.md`; send `boot_01507.log` and
  `hardware_01507_*.csv`.
- Rebuild the 3DS terrain (or draw the door leaf) when a door's descriptor is
  swapped; today the closed slab stays visible while he walks through it.
- Cook the ST00 area object list so door objects come from their real spawns
  (now CANDIDATE lazy spawn) and read the lock flag 0x7E0 + id.
- Camera owners `0x8001E6C0`/`0x8001EC0C`, door sounds, BGM check, fades.
- Optional: replace the full-map copy for the swap with a small descriptor
  override table if Old 3DS memory gets tight.
- Still open: 32 s / 44 s camera spots; pack section 35 into `ST00.mmlpkg`.
