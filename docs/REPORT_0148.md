# 0.0.14.8 terrain and alignment checkpoint

The cell lookup and debug spawn are corrected to use the same tile centers as the existing contact helper. A separately selectable XYZ/strip decoder reconstructs the retained geometry bytes and is wired into the existing enhanced Citro3D renderer. The original0143 decode/shader/fan path remains intact and was exercised. This is a technical prototype: emulator captures still show separated terrain and a floating appearance. The visual repair is incomplete; this is not the full tutorial or a faithful playable port.

## Source and changes

Project: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.8_TerrainRepair`. No Git repository, branch or commit exists in this supplied tree. Parent is the frozen0.0.14.7 source ZIP, SHA256 `1428a62f96b9b09b1edb21fafa551a6ca167578d866dcd182c52d06f9118e812`, continuing the supplied6c-derived engine. Earlier source trees and archives are unchanged. Complete changed-file inventory is `CHANGED_FILES_0148.json`.

Reused: bounded generalized map/render package reader, nativeRuntimeSession, player model/material/pose/motion, area-local GPU resource cleanup, renderer gates, save/event/replay frameworks. Added candidate expansion and diagnostic gates; fixed map query/spawn/minimap test alignment and signed negative-grid arithmetic. No replacement engine, model offset, physics constant, fabricated palette, event or save semantics were introduced. `PHYSICS_1TO1=false`.

## Build and tests

Actual native binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-terrain/validated-0148-final/build/Legends3DS_0148_TerrainRepair.3dsx` (407224bytes); SHA256 `6bfdec806febd9aa6c6a0421efdd93215716fa8ee5044aeb3bb756bf808c12bc`. Toolchain: devkitARM GCC16.1.0, libctru2.7.0, Citro3D1.7.1, ARMv6K/mpcore. Installed package inventory: `{'devkitARM': ['devkitARM-r68-1', 'devkitARM-gdb-14.1-1'], 'libctru': ['libctru-2.7.0-1'], 'Citro3D': ['citro3d-1.7.1-2']}`. All119 native source snapshot hashes match this checkpoint. No New3DS CPU feature was enabled.

The one-command cooked-only build completed, including all10 inherited/new host tests and the real native compilation:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/tutorial-pack/legends3ds/tutorial --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-terrain/validated-0148-final
```

Use a NEW output directory to repeat. The tool verifies100 manifest payloads and wraps the existing map/render pair; it does not extract or cook game data. Exact command arrays/results are in `validation_0148/build-result.json`; configure/build/test/probe logs are retained alongside it. Desktop/shared portable core builds and all host checks pass; no desktop GUI gameplay capture is claimed.

Four focused tests pass address/undefined sanitizers: map_sharing, area_package, terrain_geometry_candidate and map_cell_alignment. Exact flags/commands/results are in `validation_0148/sanitizer-result.json` and `sanitizer-supported-platform-result.json`. Initial failures are preserved: old minimap fixture used center+half-tile but expected the original cell (fixed with an additional boundary assertion); macOS rejected detect_leaks=1 before running tests (rerun with supported address/undefined checks, no leak-detector claim). No failing checks were disabled.

## Structural evidence and remaining defects

All2493 stored geometry records are planar under consecutiveXYZ recovery. Strip coverage matches2425 only,66 both,2 neither; exceptions remain in original order. This is CANDIDATE evidence, not original-game parity. Current extra shift0 matches66 render/contact footprints within one unit per bound; shifts1to3 match none within8units. No scale increase was applied.

Current frontend emits every positive grid occurrence and rotates by reference-index low bits. Repeated references form adjacent blocks: area0 has40 occurrences/18IDXreferences; area1 has130/126; area4 has2496/624, each repeated in2x2 blocks. StoredIDXXZ lies at reference-block centers. The renderer also skips8 negative cells inarea0 and71 inarea1. These observations identify placement/rotation/special-cell candidates; they do not prove whether to deduplicate, reposition, rotate differently or generate surfaces. Supplied renderer research anchors remain CANDIDATE and lack the needed argument/register trace. Exact aggregates and reproducible cooked-only tools are included.

All69 retained texture/palette resources pass CRC. Three unresolved material words affect1184 expanded primitives; no exactCLUT match exists in the retained pack. Their geometry still submits through opaque fallback. Textures-off captures retain gaps, so missing palettes do not explain the whole visual failure. Dynamic VRAM and narrow-image UV behavior remain unresolved; no unrelated palettes were borrowed.

The idle model's feet already align to flat local ground within0.001 render units; no arbitraryYoffset is supported. RuntimeSession still does not apply the existing floor/contact solver. Movement remains provisional and can leave rendered terrain. Camera placement/ownership, animation timing, lock-on/combat, enemy/event bindings, normal transitions, cutscenes, expressions, audio and retail-save semantics remain incomplete. Broader early-game gameplay and full tutorial are not established.

## Emulator and hardware

Actual407224-byte binary ran in Azahar2126.0 using isolatedXDG profiles, Old-model setting false forNew3DS and CPU100. Five synthetic replays completed with zeroHID type mismatches; current native CSV shows position/headings changing, projectile count1 and diagnostic area0to1. Candidate, textures-off and safe0143 renderer paths booted. Startup is separately captured. Normal user configuration SHA remained unchanged. Synthetic input is not retail parity. No physical hardware testing occurred.

Native header reports new3ds=0, direct3DSX layout2 and96MiB application region. Emulator configuration cannot prove physicalOld3DS memory availability, GPU behavior, load times or frame pacing. Hardware remains HARDWARE_PENDING.

Controls-run sampled maximum live malloc1091352B, owned texture2883584B, owned terrain linear210240B. At diagnostic area transition live malloc823928→1091352B, free linear29634048→28692992B, freeVRAM5523456B unchanged; transition709329us. Package loading408497us. Decoded171988B/loading-peak340636B are admission estimates, not measured peaks. Timing samples are sparse and include logging/synchronization and post-replay idle; no hardwareFPS, worst-case peak or allocation-free frame claim. The same96MiB emulator layout underlies these measurements.

## Screenshots and readiness

Six original400x480 PNGs saved by Azahar Capture Screenshot remain unmodified outside this code-only source release, at `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0148_Azahar_20261001_001`. Exact image paths, device/build/scene labels and SHA256 values are in `validation_0148/emulator-evidence.json`. Current captures show the unresolved visual defects; no desktop renders or fabricated screenshots are presented. `CURRENT_READINESS.json` provides every requested subsystem with only the approved evidence labels.

## Distribution and next step

New complete source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.8_Source_20261001_002.zip`. Source manifest covers the complete cumulative tree. Exclusions: ROM/CHD/BIN/CUE/executables, all retail/cooked models/textures/audio/video/messages/payloads, retail saves, binaries and reproducible build/cache output. Validation logs, source and aggregate evidence are included. Local-onlySD output containing the authorized existing pack: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261001-terrain/validated-0148-final/sd_out`. Do not distribute thatSD tree as source.

Next: establish the original renderer's placement/rotation/special-cell contract from authorized evidence, then test a small generalized correction against these same captures. Bind the proven contact solver separately; keep unknown physics unknown. User hardware testing is still required. Public browser patcher/importer implementation is outside this focused pass and remains pending.
