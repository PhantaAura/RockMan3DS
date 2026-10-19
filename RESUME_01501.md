# Resume 0.0.15.1 (M1 + M2 partial)

Repo: GitHub `PhantaAura/RockMan3DS` (`main`) — see `AGENTS.md`. Parent: 0.0.15.0 (M1).

## State
- M1 (0.0.15.0): source Buster/hit/defeat is the default combat on the enhanced
  renderer; `gameplay_profile.ini SOURCE_COMBAT_DEFAULT=0` restores old defaults.
  **Not yet validated on the Mac** (devkitARM build + Azahar pending).
- M2 (0.0.15.1): Area 0/1 boundary escape fixed (host-verified with real data):
  - Default boot starts at the original handoff (overlay 0x80101274: Area 1,
    (2822,-1,-4833), heading 0) instead of a diagnostic spawn inside a solid
    Area 0 block. Area 0 is the opening-scene area; it is skipped in this mode.
  - Contact solver: wall/ceiling pushes restart the candidate scan
    (0x8004BDC8 case 1 -> LAB_8004C144 -> 0x8004B6AC LAB_8004BB24); fixes
    sinking through the floor when landing against a wall.
  - Untranslated responses apply the 0x8004B414 failure contract (restore the
    +2C pre-move pose) instead of accepting free motion (CANDIDATE hold).
  - Floor rule 0x8003C4EC as decompiled: leaving the floor falls (no-floor 32767).
  - Null selected contact list = no geometry (not unsupported).
- Audit: `l3edge_audit PACK 1 2822 -1 -4833 200 3000` -> 0 escapes / 0 falls
  (0.0.15.0: 191 / 182). Synthetic regressions: `tests/test_map_edge.cpp`.

## Build / test
- Cloud: `cmake -S . -B ../host -G Ninja -DCMAKE_CXX_COMPILER=clang++ && cmake --build ../host && ctest --test-dir ../host` (146 tests).
- Mac: `tools/m1_mac_build.sh` (also prints the Azahar SD folder). Test card:
  `docs/validation_01500/AZAHAR_TEST.md`.

## Next (M2b)
Port the diagonal wall families so they slide instead of hold: 0x8004BDC8 cases
0x21..0x52 -> 0x8004F8BC / 0x8004FBE8 / 0x8004FF14, family classifiers
0x8004E490 / 0x8004E750 / 0x8004EA10 / 0x8004ECD0 (absent from the Ghidra C
export; disassemble). Verify against the original with the MIPS probe
(`docs/research_01415/tutorial_actor_identity_probe.OverlayProbe`) on Area 1
diagonal corners. Then 0x80050400 deep overlap.

Blockers: see `state/CURRENT_CHECKPOINT.json`. `PHYSICS_1TO1=false`.
