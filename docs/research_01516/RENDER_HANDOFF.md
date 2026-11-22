# Render and feel work: handoff (2026-11-21)

One page to start the render/feel items in a fresh session. Branch base:
`claude/tutorial-boss-4a9127` (collision through 9941e12, game over 6619d01).
Order agreed with the main session: M1 release first (title -> tutorial -> boss ->
post-win -> ST02, death -> game over -> title working), then these items, except
where the tutorial first-divergence comparison shows one of them changes gameplay.

## Items, in order
1. **Invisible enemies and attacks (tutorial).** Some Reaverbot attacks and some
   enemies do not draw. Compare live object/effect slots on the oracle (class 0x20
   pool `0x8009F108`, stride 0x3C0, byte 0 = active, +2 = subtype; effect pool) with
   what the 3DS draws at the same tutorial spots. Trace lines to use on the 3DS:
   `reaverbot19`, `effect_tick`, `effect_draw`, `source_effects`, `e0_object_material`.
2. **Buster / normal arm per area.** Post-tutorial Mega Man keeps the Buster left
   arm. The port cooks a fixed set 2 (`BUSTER_ARM_MESH_SET.md`, "Still open").
   Port the area-entry rule `0x8001F3C8` (+0x171 = +0x173, +0x170 from 0x800B55FF or
   +0x172, gated on 0x800B55FE) and the `0x8002786C` set choice at runtime: cook every
   resident arm set, pick per area. Check 0x800B55FE/+0x171 on the oracle at ST03
   crash site (`m3_01`), ST04 town (`m3_06`), ST0C (`m3_03`).
3. **Tutorial camera.** Keep the 400-px widescreen (deliberate). Compare camera eye,
   target, distance and projection H (vertical framing) with the oracle at a few
   tutorial points; 3DS trace: `source_follow_camera` (eye/target/distance/yaw).
   See `REAVERBOT_CAMERA_FIX.md` for the camera port's current state.
4. **Grey characters.** Some characters draw plain grey: find which classes miss
   materials/CLUTs (cook or material path).
5. **Tutorial boss arms.** `TUTORIAL_ROUTE.md` notes untextured tinted meshes;
   confirm on the oracle (savestate 3 = boss fight) whether and how the arms draw.
6. **Landscape textures.** User evidence, ST03 area 1 forest near the crash site
   (lower left of the area map): the dirt path is pure white (terrain material with
   no texture/CLUT) and everything above the tree line is black (check whether the
   original draws a sky/background layer there). Start from `RENDER_FIDELITY_R1.md`.

## Tools that work
- Oracle: `~/Desktop/3DSPORTS/psxrecomp-lab/MMLUSRecomp/build-debugtools/MMLUS_Recompiled`,
  TCP 4370, client `tools/psxrecomp_dbg.py`. `screenshot_file` with `path=` writes a
  PNG; `read_ram`/`write_ram`, `savestate op=load slot=N`, `set_input`, `wtrace_range`.
  Savestate slots (openbios): 1 ST00 area 2, 3 boss fight (area 3, life 60),
  7/8 area 1 start, 9 boss area. M3 states in `saves/m3/` (copy into slot 11).
  Serialize: check `pgrep -x MMLUS_Recompiled`, back up `saves/card1.mcd`, log
  start/stop in the side-session notes, close your own PID.
- ST0C overlay disassembly: dump RAM 0x80100000.. from the oracle, then
  `uv run --python 3.12 --no-project --with capstone python ...` (capstone is not in
  the system Python).
- Azahar: one run at a time (`pgrep -x azahar`); move the shared SD folder aside,
  install the build + `diagnostic_autoplay.txt`, cap the run, restore the folder.

## Facts settled this session (do not re-derive)
- IDX raw6 is set at stage load by `0x8003A3B4` (descriptor+0x18 -> +2), cooked by
  `parse_st00`; matches oracle RAM on every row of ST00/ST03/ST0C.
- Game over returns to the title (main 1); NEW GAME restarts at 80/80.
