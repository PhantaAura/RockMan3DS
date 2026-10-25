# Resume 0.0.15.16 (WCA complete in Azahar: tutorial to 0x622)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.15.

## Completed
- **World Comes Alive (Gate A), Azahar.** One continuous normal-boot run
  (no spawn file, scripted pad, no forced flags) reaches every tutorial flag
  through **0x622** (frame 10925; again at 13690 on the faster build). Flag
  table: `docs/validation_01516/RESULTS.md` ("Gate A"). Fixes it needed:
  - ledge grab `0x80040818` + action 13 hang/pull-up `0x8003EB08`/`0x80040178`,
    wall finder `0x800497CC` (`docs/research_01516/LEDGE_GRAB.md`);
  - action-3 phase-5 pool release `0x80031824` (Area 2 Reaverbots, param 0xA0,
    charge once and release themselves);
  - `+75 = +74`, `+77 = +76` mirrors at `0x8003C920` (walk-off fall after a climb);
  - slope codes 8..11 (`0x8004BDC8`, earlier this checkpoint);
  - route script and autoplay door steps (Area 2 loop through walkable cells).
  Not exercised: box 0x620/0x621, the 0x60E action check. Boot still starts at
  the hand-off with `opening_scene=SKIPPED`.
- **Gate C.** Mega Man's draw poses and projects each shared vertex once:
  9.3 -> 2.7 ms per frame; route average 20.1 -> 24.5 FPS in Azahar (CPU-tick
  proportions, not hardware). Next largest costs: terrain 6-15 ms, OT flush
  3-6 ms, actors up to 5 ms.
- **Gate D.** `docs/validation_01516/HARDWARE_CHECKLIST.md` (PENDING).
- **Gate E.** `docs/validation_01516/NEXT_1_0_STEP.md`: next is the tutorial
  exit past 0x622, observed first in the psxrecomp oracle.
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

- E0 targets, doors and the box are textured: their mesh pages are cooked into
  the per-area actor cache (recipe `01516-e0-textures-1`). They draw through
  the scene E0 material plan, with semi-transparent door pulse and box fade.
  The box fade's ScaleMatrix growth is ported. The GPU command buffer is
  now 512 KiB (Area 2 overflowed the 256 KiB default).

- Tutorial sound effects and Roll's voice (`docs/research_01516/AUDIO.md`).
  Effects follow the original's queue consumer, VAB tones, pitch, volume/pan,
  distance gate and SPU ADSR. Message AUDIO plays the XA clip, and unvoiced
  messages play the typing blip. The audio is cooked by `cook_sfx_bank.py`
  and `cook_xa_voice.py` (the latter needs the CUE/BIN; set `CUE=`). Azahar
  needs the user's own `sdmc:/3ds/dspfirm.cdc`.

- Break and burst effects (`docs/research_01516/EFFECTS.md`). Subtype 0x80
  (0x80043F34: init, grow/fade 0x8004441C, RotMatrixY, the 0x8002F9C4 EBD
  row) and the 0x8004920C point floor are ported. They are fed by:
  - the E0/05 box break (0x42);
  - the E0/08 debris (three 0x83 puffs) and burst (0x82);
  - the class 20/03 and 20/19 defeats (0x41, now at the actor's +0x14).

  Effects are drawn with their original meshes and sprite page. The E0/07
  gate's camera shake (0x800398E0 → 0x800A395F) is ported. Azahar: targets
  and box effects spawn, animate, draw textured and release.

- Area music (`docs/research_01516/MUSIC.md`). The tutorial BGM is the
  m_op01b SEP, not XA:
  - 0x80019D18 queues request 0 for areas 1/2;
  - handler 1 (0x8001AC58) plays sequence 0;
  - the song loops itself with NRPN 20/30.
  
  `source_music` ports the game side and the libsnd sequencer and voice
  manager, and mixes the 15 music voices in software, with stage 0's SPU
  reverb (Studio Small, out 0x4080). `NativeMusic` streams the mix on NDSP
  channel 4. The bank is cooked by `cook_music_bank.py` (L3MU, private pack).

- Buster lock-on homing (0x80043B54, `0x80033F04` with the shot's
  1060/143/71 turns): leased shots now steer toward their target for 9
  frames. Host-tested; not watched in Azahar yet.

- Renderer R1 (`docs/research_01516/RENDER_FIDELITY_R1.md`):
  - terrain faces now get the original depth cue (DPCS to the area FarColor
    with SetFogNear DQA/DQB, and the bit-28 rule);
  - the back-face threshold `0x1F8001C0` is the player's view depth each
    frame (writer `0x800238BC`);
  - per-cell visibility, the far table and see-through walls are traced but
    not ported.
- Renderer R2:
  - the CLUT depth ramp is ported as fitted per-word/per-step RGB scales
    (L3CR sidecar, cooked from the real ramp palettes; CANDIDATE);
  - the positive-family near cull (max SZ < 0xC1) is ported.
- Renderer R3 (merged into main with the see-through branch; Azahar screenshots
  at the Area 1 start and mid-turn show no holes): per-cell terrain
  visibility (`0x80020BB4`, wedge `0x80021258`/`0x80021548`, test
  `0x80021834`) is ported. It is wired into `draw_terrain`, with the side cut
  widened to 200/384 for the 400-pixel view (CANDIDATE). At the Area 1 start,
  black faces fall from 108 to 34 (original 17) and the frame interval from
  40.8 to 33.1 ms in Azahar.
- Renderer R10: Reaverbots (0x8002831C/0x800289A0) and E0 meshes (0x8002A6E8)
  are packets. The black Area 2 screen was a camera bug (target y ignored the
  player's height, since 0.0.15.9), now fixed; E0 targets verified on screen.
- Renderer R9: Mega Man is drawn through the OT (one colour word, CLUT ramp,
  NCLIP, AVSZ bucket). Next: E0 meshes (`0x8002A6E8`, decoded) and class-20
  actors.
- Renderer R8: near replay `0x8002235C` and negative LODs (L3NM v2 near
  tiles). All default terrain at the Area 1 start is packets. Next: actors into
  the OT; special-cell subdivider `0x80022A94`.
- Renderer R7: negative cells (`0x80021B14`) are packets too (FT4/FT3, cell
  ramp from `0x800C0DC0`). Near-replay cells (<0x401, `0x8002235C`) stay 3D.
  Next: port `0x8002235C`, then actors into the OT.
- Renderer R6: subdivider `0x8002466C` ported; every positive terrain face is
  a packet with GTE SXY (no seams, affine). Next: negative-cell packets
  (`0x80021B14`), then actors into the OT.
- Renderer R5: GP0 packet translator (`psx_gpu.h` + `psx_draw_ot`). Positive
  terrain faces are 0x80026158 POLY_FT4 packets in a PsyQ OT, drawn affine at
  w = 1. Next: port the subdivider 0x8002466C so every face is a packet, then
  switch xy to the GTE SXY; then the negative family and actors. Widescreen
  400 px is a deliberate feature (user).
- Renderer R4: per-IDX Near/Mid/Far LOD (`0x800267AC`) and the mode-2
  off-screen test (`0x80025BE8`). The host replay now gives exactly the PS1's
  17 black faces at the Area 1 start; Azahar shows 25 (8 extra in the 400-px
  side strips, CANDIDATE). Next: the negative near replay `0x8002235C`, OT
  order and subdivision `0x8002466C`.
- Renderer R3 (merged): see-through walls (`0x80024F30`/`0x80025900`). Still
  never seen firing in Azahar (0 in every merged-build trace). A front face
  nearer than Mega Man over the screen centre is drawn after the actors,
  semi-transparent with its TPAGE ABR (STP texels only). Host-tested. The
  Azahar trace shows 0 such faces at the Area 1 start, which is expected;
  not yet seen on screen. Mode 0's near list is not modelled.

- Reference oracle: a psxrecomp static recompile of our US disc now runs on
  the Mac with a TCP debug server (`fntrace` per call with ra/a0–a3/frame,
  `read_ram`, write traces). It is verified as US, and ROCK_NEO is native. ST00 is not
  native yet. Local only, never committed. See
  `docs/research_01516/PSXRECOMP_ORACLE.md` and `tools/psxrecomp_dbg.py`.

## Validation
`docs/validation_01516/RESULTS.md`: host 166/166 after merging effects and music, earlier 162/162.
Area music in Azahar: request 0 plays in Area 1 in real time with reverb (trace;
the user heard it on the merged build), native PASS, Azahar trace (pop-out
and hand-off). The byte-0x0B fix was not run in Azahar (screen locked).

## Next
0. Tutorial exit past 0x622 (`NEXT_1_0_STEP.md`): watch it in the psxrecomp
   oracle, trace the 0x622 owner in the ST00 overlay, port only the hand-off.
   Then the opening scene, title screen and New Game.
   Still open from WCA: box 0x620/0x621 and 0x60E; terrain cost (largest
   remaining, 6-15 ms); effects (0x800341BC explosion proxy, bit-0x20 children);
   music gaps (Gaussian interpolation, reverb FIR, pause during loads).
1. Re-run `docs/validation_01516/autoplay/popout_corridor.txt` with the
   screen unlocked. Confirm action 8 leaves phase 0, and check the landing
   against a short PCSX-Redux probe.
2. The hit proxy for the hidden pop-out (`0x800341BC`, bit 8).
3. psxrecomp oracle trial: reach gameplay over TCP pad input, record a ledge
   grab (`0x80040818`) and diff it against the port in a host test. Then
   capture and compile ST00 so tutorial functions are traceable.
4. Then, from 0.0.15.15:
   - the `+0x44D` camera writers;
   - tutorial type 16;
   - the top-screen message window and voice.

## Blockers
- Effect textures: the upload order is CANDIDATE. The common page comes from
  EXIT_SUP, and Area 2 inherits ST00_01.
- The 0x1F8001C0 back-face threshold (522) is measured in Area 1 only. Its
  writer is untraced.
- Cell visibility not yet checked by eye for edge holes. `0x1F800078` bit 2
  (far table choice) is UNKNOWN.
- No physical Old 3DS test (checklist ready, PENDING).
- Azahar runs about 24 FPS on the route; 30 FPS not reached yet.
- The work80 writer for 0x1F is CANDIDATE.
- The box's E0 pool slot (15 = static E0 count + conditional order) is
  CANDIDATE. It only feeds the RNG work check and the proxy's byte.
