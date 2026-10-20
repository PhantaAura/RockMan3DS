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

## Validation
`docs/validation_01516/RESULTS.md`: host 156/156, native PASS, Azahar trace (pop-out
and hand-off). The byte-0x0B fix was not run in Azahar (screen locked).

## Next
1. Re-run `docs/validation_01516/autoplay/popout_corridor.txt` with the
   screen unlocked. Confirm action 8 leaves phase 0, and check the landing
   against a short PCSX-Redux probe.
2. The hit proxy for the hidden pop-out (`0x800341BC`, bit 8).
3. Then, from 0.0.15.15:
   - the `+0x44D` camera writers;
   - tutorial type 16;
   - the top-screen message window and voice.

## Blockers
- No physical Old 3DS test.
- The work80 writer for 0x1F is CANDIDATE.
