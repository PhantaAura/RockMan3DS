# Resume 0.0.15.16 (WCA — pop-out Reaverbot)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.15.

## Completed
- ST00 subtype 0x1F (`0x801062EC`), the "small one" Roll points out:
  - init, the work80 wait, and the 9-frame lunge, with its ScaleMatrix Z growth
    (`0x8007ADE8`);
  - hit `0x80106800`;
  - the rewrite into subtype 3 with parameter 0x40.
  
  Code is in `src/world/class20_1f_popout.cpp`, wired through
  `actor_runtime` and `main.cpp`. See `docs/research_01516/POPOUT_1F.md`.
- RNG caller RA 0x80106378 is admitted.
- L3CB v5 cooks stats-record byte +0x0B, which action 8 with parameter 0x40
  needs. The pack recipe is now `01516-combat-byte0b-1`.
- Work +0x0F context 0x80 (record 1585, the first pop-out) is now admitted. The
  only reader, 0x80102528, masks it with 0x7F, so the HP and record are the same
  as context 0. Host-tested only.
- In Azahar, all three corridor pop-outs spring up and turn into subtype 3.

- The gameplay view was mirrored left-right. The LookAt path now negates
  camera-space x; `test_frontend_handedness` proves the original puts −x
  on screen-right. The HUD and steering are unaffected.

- The original map face test (NCLIP, back-face cull, near-black back-face fill
  in mode 2, threshold 522 measured) is ported. This fixed the doubled top
  screen and the hidden Mega Man at the start. See
  `docs/research_01516/TERRAIN_FACES.md`.

- Tutorial chain mapped end to end (`docs/research_01516/TUTORIAL_ROUTE.md`).
  Ported: trigger type 16, the E0/00 defeated flags (defeated enemies no
  longer respawn), and the E0/07 floor switch and gate in the Area 1 trap room.

- Subtype 0x19 (the trap-room Reaverbots, `0x80104864`) ported. It wakes on
  the floor switch (0x607), drops in, fights, and is defeated with effect,
  reward and its E0/00 flag. Host-checked on the real Area 1 map.

- E0/08 targets and doors plus the 0x800411DC action check are ported. In
  Azahar the check opens Roll's message, both targets break and the doors open.
- Fixed two Buster bugs: a stale hit packet after the first hit (every later
  shot died on frame 2), and leased targets could not re-register.

- E0/05 breakable box (`0x80113EE0`) ported: `source_box_e005`, the
  RuntimeSession box list from the conditional group, its proxy, the E0 RNG
  phase (RA 0x80114020), and L3CB E0/05 reward rows (recipe
  `01516-e005-box-1`). In Azahar the box breaks, 0x620 opens message 11, the
  answer sets 0x621 and the reward drops pickups.

## Validation
`docs/validation_01516/RESULTS.md`: host 162/162, native PASS, Azahar trace (pop-out
and hand-off). The byte-0x0B fix was not run in Azahar (screen locked).

## Next
0. The objects the rest of the tutorial waits on, in route order:
   - run the trap-room fight in Azahar (subtype 0x19 is ported; screen was locked);
   - E0 objects are drawn untextured (diagnostic). The box's break effect
     (0x80031618), sound and 0x80052528 are not ported;
   - checks of the Area 2 subtype-3 parameter-0xA0 wake (0x60B) and class 40/04.
1. Re-run `docs/validation_01516/autoplay/popout_corridor.txt` with the
   screen unlocked. Confirm action 8 leaves phase 0, and check the landing
   against a short PCSX-Redux probe.
2. The hit proxy for the hidden pop-out (`0x800341BC`, bit 8).
3. Then, from 0.0.15.15:
   - the `+0x44D` camera writers;
   - tutorial type 16;
   - the top-screen message window and voice.

## Blockers
- The 0x1F8001C0 back-face threshold (522) is measured in Area 1 only. Its
  writer is untraced.
- The map draw still has no per-cell visibility or Near LOD, so it draws more
  black faces than the original (hidden by depth).
- No physical Old 3DS test.
- The work80 writer for 0x1F is CANDIDATE.
- The box's E0 pool slot (15 = static E0 count + conditional order) is
  CANDIDATE. It only feeds the RNG work check and the proxy's byte.
