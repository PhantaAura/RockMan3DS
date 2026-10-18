# 0.0.14.6c — local Old 3DS hardware test

This continues the supplied project. It is a technical gameplay prototype with existing movement, turning, animated player rendering, projectiles and manual area selection. It is not a faithful or complete Mega Man Legends port. Compilation and host tests do not establish that it is playable on hardware. Runtime results are recorded separately in the milestone report.

## Source and audit

The input was the cumulative 0.0.14.6b checkpoint (`ca9a2f72cc7bd31e50ca47b678fb9abadbc2c3e18fd500fa4435c6959b083d0b`), derived from the supplied 0.0.14.6a integration source. No Git repository, branch or commit was present. See `BASELINE.json`, the pre-implementation `PORTING_MAP_0146c.md`, and `pack_audit.md` / `.json`.

The existing cooked tutorial pack passed 100 manifest hash/length checks and 22 reader/probe commands. It still has 1,184 terrain primitives with unresolved materials, ambiguous/incomplete object mappings and unverified animation, UV, palette and transparency semantics. The user authorized this existing pack for local use only. No raw disc/executable analysis, extraction or cooking was performed in this milestone. This source archive contains no cooked commercial assets. A faithful port needs authorized missing source/data and independent reference captures/traces.

## Reproduce without extraction

Run from the source root:

```sh
python3 tools/build_hardware_test.py --pack /absolute/path/to/existing/legends3ds/tutorial --out /absolute/path/to/new/output
```

The output must be new. This validates the existing pack, compiles host code and runs CTest, compiles the actual devkitARM target, and copies the same cooked files into the SD layout. Do not run the inherited `build_mml3ds.sh` or `tools/build_mml3ds.py` for this restricted pass: those workflows can recook data. Historical research and build instructions are retained as reference, not new authorization.

Toolchain used: devkitARM r68-1 / GCC 16.1.0, binutils 2.46.0-1, libctru 2.7.0-1, Citro3D 1.7.1-2, Picasso 2.7.2-3, 3dstools 1.3.1-3. Native flags use ARMv6K/mpcore and standard `3dsx.specs`. No New 3DS speedup, extra cores, C-stick, ZL/ZR or MVD dependency was introduced. The top display is mono 400×240; the bottom is diagnostic text/map.

## Original Old 3DS test

1. Copy the contents of the new output's `sd_out` onto your homebrew SD card. The executable belongs in `3ds/Legends3DS/` and the existing data in `3ds/Legends3DS/data/legends3ds/tutorial/`. Keep other builds separately. If copying over an existing Legends3DS folder, back up that folder first.
2. Launch `Legends3DS_0146c_HardwareTest.3dsx` in Homebrew Launcher. The bottom launch screen identifies version 0.0.14.6c. Press A for the selected/full renderer; X starts the existing safe 0143 fallback. START exits.
3. In gameplay: Circle Pad/D-pad forward and turn, L/R turn, Y fires a projectile, SELECT cycles the five test areas, START exits. Backward movement, jump, lock-on targets, dialogue triggers and full collision are not established in this frontend. X/A are inherited bindings, not proof of those features.
4. Test movement/turning/projectiles in area 0, cycle each area once, and return to area 0. Record visual faults, responsiveness and any crash or hang; do not call projectile firing enemy combat. Area 4 is much denser than the other areas.
5. Photograph both screens at launch and in the scene, including the visible player and diagnostic lines. Report original model, homebrew environment, whether the player/terrain/textures appear, and whether movement/Y/SELECT/START respond.
6. Keep `3ds/Legends3DS/boot_0146c.log` and the timestamped `hardware_0146c_*.csv`. Preserve each run before relaunching because the boot log is reset. The CSV includes model-query result, memory layout, reserved heaps, live allocator use, free linear/VRAM, input bits, position/yaw, projectile count, timing, submitted draw calls and triangles.

The original-device run is the acceptance gate. An emulator is useful for debugging but cannot establish physical frame pacing, GPU limits or SD loading performance.

## Measurements and limitations

Frame/phase times are CPU-observed; render time includes synchronization. `frame_before_log_us` excludes the current CSV write/flush. The frame interval includes the previous frame’s logging overhead, but sparse samples can omit that following frame; these samples cannot establish worst-case frame pacing. The HUD interval is one frame behind. `Log FAIL` indicates an open/write/flush failure. Samples are written on input/area changes and every 120 frames; they are not a full high-frequency profiling trace or exact peak-memory proof. Draw/triangle counts report submissions, not visible pixels or overdraw. `owned_linear`/`owned_texture` track renderer allocations; `malloc_live` is allocator-reported live memory, and reserved heap/region values are capacity, not live allocation. Never sum overlapping categories as if they were independent.

No audio playback or streaming is integrated; queued host audio requests are not audible output. The frontend has a hard-coded follow camera, incomplete collision, no registered enemy targets/event progression, no natural transitions/cutscenes, no touchscreen input or SD save/load UI. Full-area submission and per-frame player allocations remain performance risks. Stereo is disabled. The host build is a library, probes and tests, not a desktop graphical game.

Changes are deliberately bounded: immutable shared map ownership, synthetic ownership/lifetime regression checks, diagnostic launch screen and runtime counters/logs, explicit build identity and a no-cook build/SD workflow. Existing renderer, shaders, data readers, player motion/animation, projectiles, area loading and fallback path are retained.
