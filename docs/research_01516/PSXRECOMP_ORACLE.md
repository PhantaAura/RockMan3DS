# PSXRecomp as a reference oracle (US disc)

Status: VERIFIED (boots, traces) on 2026-10-25, Mac M-series, 8 GB RAM.
Nothing here is retail data. The recomp project, its generated C and all
traces live outside the repo in `~/Desktop/3DSPORTS/psxrecomp-lab/` and must
never be committed.

## What it is

[psxrecomp](https://github.com/Alexbeav/psxrecomp) statically recompiles a PS1
disc to C and builds a native program. With it, our US disc runs as the
original game on the Mac with a TCP debug server, so per-function
traces no longer need PCSX-Redux captures.

## US, not EU: checks done

- Project `game.toml`: `id = "SLUS-00603"`, boot EXE `SLUS_006.03`, digests of
  our own US Track 1 (sha1 `904489e2…f2b037`).
- All 90,594 generated boot-EXE instruction words equal our
  `SLUS_006.03` (sha1 `888039c5…92739`), with 0 mismatches.
- The runtime reports `disc region NTSC-U (serial SLUS-00603)`, and the intro text is English.
- Live RAM after boot holds `ROCK_NEO.EXE` (sha1 `ffc08fd2…1eb1d46`, same file
  as our Ghidra project). `0x80040818`, `0x80046B34` and `0x8004BDC8` match it
  byte for byte, and 92% of its words match overall. The rest starts near
  `0x8008055C` and is presumably data written at runtime (CANDIDATE).

## Layout facts

- The disc boots `SLUS_006.03` (text `0x80010000`, size `0x4A000`), which loads
  `ROCK_NEO.EXE` over the same base (size `0xC9000`). Every function we
  port lives in `ROCK_NEO.EXE`, so psxrecomp sees it as an overlay, not as the
  boot EXE.
- The disc scan (`aot_overlay_pipeline.py extract`) found `ROCK_NEO.EXE` (split
  at `0x8005A000`) plus `ST02`, `ST15`, `ST16`, `ST18` at `0x800FF000`. It did
  **not** find ST00 or the other ~26 areas, which run on the interpreter until
  captured (`overlay_cache = true`, then `compile_overlays.py`).
- 2,234 functions are static native code: ROCK_NEO 991 + 838, stages 195/69/81/60.

## Rebuild recipe (owner machine)

1. `setup_project.sh --yes --name MMLUSRecomp --disc "<US cue>" --dir
   ~/Desktop/3DSPORTS/psxrecomp-lab --players 1 --region USA --no-recomp-ui
   --no-netplay --no-wizard --no-ci --no-fetch-boxart --no-github`
2. `psxrecomp_cli.py generate … --no-toolchain-download` (on macOS the
   portable-toolchain step always fails because activation is Windows-only).
3. Disc scan with Python ≥3.11 (`uv run --python 3.12 --no-project python …`),
   then
   `PSX_STATIC_NO_ISOLATED=1 compile_overlays.py --static --jobs 1 --captures
   aot/generic.json …`. Without the env var, the 740 "isolated interior
   fragment" compiles exhaust 8 GB RAM and swap fills the disk (exit 137).
4. In `CMakeLists.txt` put `GAME_OVERLAY_STATIC_C` **before** `GEN_FULL_GLOB`
   (a multi-value keyword that would otherwise swallow it), then
   `cmake -DPSX_DEBUG_TOOLS=ON` and build with `-j 2`.

## Tracing

`tools/psxrecomp_dbg.py <cmd> key=value…` talks to port 4370.

- `fntrace_arm target=0x80040818`, or launch with `PSX_FNTRACE_ALL=1`, then
  `fntrace_dump target_lo=… target_hi=… count=N`. Each call records `ra`,
  `a0`–`a3`, `s3`, `sp` and `frame` (4 M-entry ring; 16 M calls were logged in 30 s
  of intro).
- `read_ram addr=0x… len=N` returns hex. `wtrace_range lo hi` / `wtrace_dump`
  give RAM writes with the writer's RA, and `watch addr` gives a byte watchpoint.
- `present_shot` writes a PNG of the presented frame.

## Not done yet

- Reach gameplay over TCP pad input and record a ledge grab (`0x80040818`) to
  diff against `test_*` host tests.
- Capture and compile ST00 so tutorial-area functions are native and traceable
  per function.
