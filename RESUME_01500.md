# Resume 0.0.15.0 (M1)

Repo: `work/Legends3DS_0.0.15.0_Claude` — **Git** (`git log`). Parent: 0.0.14.51 (commit 1).

Done: source-backed combat is the default gameplay path.
- `include/legends3ds/gameplay_gates.h`: ACTOR_STATE_FIELDS, SOURCE_ACTOR_PROXY,
  ACTOR_TEXTURES, ACTOR_RNG_CANDIDATE default ON when their SD file is absent;
  explicit files win; malformed fails closed; `gameplay_profile.ini`
  `SOURCE_COMBAT_DEFAULT=0` restores 0.0.14.51 defaults.
- `RuntimeSession::set_candidate_combat_enabled(false)` retires the candidate
  projectile + candidate lock-on whenever the source combat owner is active.
- Source Buster admission respects shot-enable (+175) and busy (0x8004324C).
- Diagnostic ribbon draws live source Buster segments (`draw_source_busters_uv`).
- One-command pack now cooks hit_proxy_rules.l3pr, class60_06.l3c6,
  actor_reactions.l3r7, actor_angle.l3ref (exact EXE/overlay cookers).
  source_rng.l3rg (trace-observed) is copied by `tools/m1_mac_build.sh`.

Pending: Mac native build + Azahar test card (`docs/validation_01500`).

Blockers (unchanged evidence labels):
- Lock-on 0x80034EB8 needs the original gameplay camera matrix (owner UNKNOWN); X does nothing while source combat is active.
- +0x400/+0x418 endpoint writers still CANDIDATE (forward ray). Export note: FUN_80042528 / FUN_800435F8 read them as an effect origin with +0x10 Y.
- 0x80046B34 reward/pickup, 0x80/0x41 defeat visual, map-edge fall, camera — see CURRENT_CHECKPOINT.json.

Next (M2): stay on the map — record the original at Area 0/1 edges in PCSX-Redux and compare.
