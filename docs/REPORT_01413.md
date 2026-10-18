# 0.0.14.13 — EBD resources and initializer evidence

This pass continues the cumulative 0.0.14.12 source at `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.13_ObjectResources`. There is no Git repository, branch or commit. Original archives and synced project sources remain intact. This checkpoint addresses the GCC warning, EBD byte retention and a bounded initializer/resource trace. It does not add imported actor rendering or rewrite the engine.

The recorded GCC `-Wtype-limits` warning came from comparing an unsigned enum against zero. The source correction was already in the baseline. This pass adds native `-Werror` for that compilation unit and a genuine GCC regression: an isolated copy restoring the historical expression must fail, while current code must compile cleanly. No diagnostic is suppressed. See [the warning report](GCC_WARNING_FIX_01413.md).

L3EB version 2 retains the complete original EBD bytes alongside normalized source-relative rows. Version 1 remains compatible. Resource windows require a caller-known length and preserve null pointers, aliases and overlaps. The loader rejects invalid ranges, overflows, truncated files, CRC failures and disagreement between cooked rows and the retained source directory. Resource types and full lengths are not inferred from pointer order. Existing `TutorialRuntimeArea` ownership carries the retained bytes through the native loader. See [the format contract](L3EB_FORMAT_01413.md).

Only five EBD sidecars were recooked from profile-verified owner containers. They retain 113,808 / 117,360 / 117,360 / 101,808 / 116,896 bytes exactly: 567,232 bytes total. The runtime probe loads all five areas and exposes 198 non-null pointer windows. Other verified cooked files were reused. Original inputs and owner saves were not modified.

The [initializer trace](OBJECT_INITIALIZER_CHAIN_01413.md) now identifies the actual first object: its C0 callback reaches a script initializer, with no EBD lookup. A separate actor record follows the full subtype, dispatch-byte, parameter and state chain to initializer `0x80106974`. A null variant selector chooses EBD row 3. Source code stores all three pointers and consumes a specific 24-byte window from slot 0 plus a u32 from slot 1. Names such as model or animation are not assigned without format evidence. This is **SOURCE_BACKED_PARTIAL static evidence**, not proof of native actor execution or rendering.

## Validation

All **25/25 CTest suites pass**, including inherited checks, new EBD format/bounds/CRC/legacy tests and the GCC warning regression. The modified EBD source and dedicated test pass ASan and UBSan; inherited linked library code is not fully instrumented. Actual host and native build logs contain no compiler warnings.

Toolchain: devkitARM r68-1 / GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2, ARMv6K/mpcore. The compiled native binary is 455,892 bytes, SHA-256 `06f5c8fc62721d192421f2a341510098c2ce58bf5b98c85b4f280220e7cff658`:

`/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-objects/install/build/Legends3DS_01413_ObjectResources.3dsx`

Owner-local SD layout: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-objects/install/sd_out`. All native source snapshot hashes match the final source. The trusted 0143 shader/draw path is preserved; active renderer behavior changes only in version labels and retention diagnostics.

Azahar 2126.0, Old-model configuration, CPU 100%, resolution 1, runs the exact binary. Its boot log confirms `EBD retained source bytes=567232 payload areas=5`, first frame and ready. Existing synthetic movement, turn, firing and diagnostic area transitions run. The original window capture is kept with its binary hash and emulator labels in `validation_01413/capture-manifest.json`; the completion-dialog capture is explicitly excluded from gameplay evidence. Gallery: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261003-objects/SCREENSHOTS.md`.

No physical Old 3DS testing occurred. Existing camera/player occlusion and missing original top HUD remain. No new imported actor or faithful tutorial is claimed. This remains a native technical prototype; `PHYSICS_1TO1=false`.

## Memory and limits

567,232 bytes is the total source data loaded before session registration. The inherited area registry, location registry and active-area copy duplicate that data. At area 1, EBD copies total 1,251,824 bytes. Native counters under emulation report 2,565,112 bytes of live heap, 71,640 bytes of owned linear storage and 3,670,016 bytes of textures. Borrowed resource lookups allocate nothing; loading and registry copies do allocate. Shared immutable payload storage and area streaming remain future work. These emulator observations do not establish physical device headroom or FPS.

## Reproduction

```sh
cmake -S . -B /absolute/host -DCMAKE_BUILD_TYPE=Release
cmake --build /absolute/host --parallel 4
ctest --test-dir /absolute/host --output-on-failure
python3 -B tools/check_renderer_gate_gcc.py --compiler /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc
```

Exact native command and binary identity are in `validation_01413/build-result.json`; the five targeted cooker commands, input profile hashes and byte comparisons are in `ebd-owner-roundtrip.json`. Exact GCC and sanitizer commands are recorded in their validation evidence. The initializer probe reproduction command is in its report and emits addresses, hashes and scalar selectors only.

The existing one-command cooked-pack builder remains:

```sh
python3 -B tools/build_hardware_test.py --pack /absolute/existing/tutorial --out /absolute/fresh-output
```

It accepts version 1 or 2 EBD sidecars and compiles with the actual toolchain. The owner-data cooker automatically emits version 2 for newly cooked EBD inputs. Legacy sidecars cannot expose bytes they never retained; recook just those sidecars when upgrading.

## Handoff

Next bounded step: decode the proven actor resource layout and reproduce its initialized work fields before adding a native draw binding. The first C0 record separately needs its command consumer traced. Full resource extents, actor presentation, original script execution, physics/contact/spawn/camera/HUD, natural progression, audio/cutscenes and hardware acceptance remain incomplete. Do not treat retained bytes as decoded gameplay.

The cumulative source ZIP includes code, build scripts, project files, tests, text documentation, evidence and `SOURCE_MANIFEST.sha256`. It excludes original ROM/CHD/BIN/CUE/EXE, retail saves, Capcom textures/models/audio/video, every cooked payload, generated binaries and reproducible caches. Local SD output contains owner game data and must not be redistributed. No usage reset was redeemed.
