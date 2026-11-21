# Resume 0.0.15.3 (World Comes Alive M2c)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.2.

## Completed
- Ported `0x80050400` deep-overlap recovery exactly from the R3000
  instructions. It takes the nearest face per axis, uses the original tie and
  blocked-axis rules, keeps the top face in mode 3, shifts the pre-move pose and
  records the source blocked byte. It is wired for code 1 and for
  airborne-upward code 3 (previously Unsupported).
- Ported the `0x8004B6AC` correction history: h/u at `0x1F800100..10A`, applied
  at each restart, with every clear/write site in the supported response paths.
  At corners the previous push is now undone when the next response is on
  another axis, as in the original.
- Fixed two smaller mismatches found while porting:
  - `0x8004F8BC/FBE8` nearest-face Y now also moves the pre-move pose.
  - `0x8004FF14` Y stores blocked `0x20`.
  - Grounded publication now follows `LAB_8004C144`: a true result with dy >= 0,
    never from case 3.
- Evidence: `docs/research_01503/DEEP_OVERLAP_80050400.md`, disassembler
  `docs/research_01503/psx_mips_dis.py`.

## Validation
- Host suite (`/private/tmp/l3-host-01503`): 147/147 PASS. New vectors in
  `tests/test_runtime_contact.cpp` cover deep wall/floor/ceiling overlap,
  dy<0 landing, repeat stability, tie with blocked X, mode-3 code 3, the
  history corner (0.0.15.2 result (96,95) → source (100,95)) and a runtime tick.
- Area 1 audit `l3edge_audit PACK 1 2822 -1 -4833 200 3000`: 0 escapes,
  0 falls, 0 unsupported, 54 cells.
- Embedded-pose sweep, Areas 1–4 real collision data:
  - 0.0.15.2 held 1,016,352 poses as unsupported.
  - 0.0.15.3: 622,988 recover through `0x80050400`, and 389,292 reach the
    original 9-pass rollback.
  - 4,072 (Area 2) still meet unported primitives.
- Native and Azahar: see `state/CURRENT_CHECKPOINT.json`.

## Remaining M2 work
- No paired PS1/native trace for corners or overlap yet.
- Still unported: codes 2/0x0B/0x0C (modes 0/0x43), the x5 diagonal fallback
  (and its `0x1F80010C..116` history), slopes/steps, and the actor-side
  deep-overlap exclusion in `actor_contact`.
- Areas 2–4 transitions are not audited.
- No physical Old 3DS validation. `PHYSICS_1TO1=false`.

Recommended next objective: capture paired PS1/native movement traces through
Area 1 corners to measure the ported history. Then port the remaining code-2 and
x5 dispatcher paths that still hold in Area 2.
