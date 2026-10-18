# 0.0.14.46 — outgoing scene lifetime

The source now retires outgoing CPU scene resources after a committed area
replacement, even when recorded replay scheduling stops the old actor-update
lambda. The affected candidate intro still completes its Area0-to1 handoff in
Old-model Azahar. One resource retirement is observed, with no old-pool cleanup
or scripted scene-view calls after replacement. Full World Comes Alive remains
unfinished; this is a technical prototype checkpoint.

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`. No Git repository, branch or commit. Parent45 ZIP is preserved;
all2161 parent manifest entries were verified before edits. Changed files are
listed in CHANGED_FILES.txt. Only native frontend lifecycle/guards and checkpoint
identity changed; existing runtime, original scalar methods, owner-local assets,
candidate RNG, renderer feature gates and 0143 fallback were reused.

The 01445 replay gate invalidated on replacement and therefore never invoked
the actor lambda containing retirement. CPU actor resources could stay resident.
Terrain rebuilding already released GPU textures, so this is not evidence of an
additional old GPU-texture leak. The fix retires CPU models outside that gate,
after successful commit. Failed staging does not run that retirement block;
existing GPU scene-restoration limitations remain unchanged. See IMPLEMENTATION.md.

Actual toolchain: devkitARM r68-1 / GCC16.1.0 / libctru2.7.0-1 / Citro3D1.7.1-2,
ARMv6K/mpcore hardfloat. Host139/139 PASS in5.20 seconds, actual native build
PASS, no warnings in retained native log. Binary 905104 bytes, SHA256
`d2196f150846d904349c85f269366dca6d220900520fb0b2e40a98fa20ad3f44`. All 280 staged native hashes match current source.
Exact commands and results: TEST_COMMANDS.md and build logs. Desktop checks are
host tests/probes; no desktop game window was exercised this pass.

Azahar2126.0, Old model, CPU100%, Vulkan: candidate shared RNG PLUS recorded
constructor clock, not fresh boot/live retail chronology. Actual head UV reset,
254-call native mix prefix and NDSP pause still precede area replacement.
Retirement memory observation: `# area_event=outgoing_scene_released,area=1,elapsed_us=0,package_bytes=701943,decoded_estimate=810616,load_peak_estimate=1898932,malloc_live=1886568,linear_free=27937280,vram_free=5523456`.
After replacement: `# area_event=source_transition_after,area=1,elapsed_us=1781403,package_bytes=701943,decoded_estimate=810616,load_peak_estimate=1898932,malloc_live=1886632,linear_free=27937280,vram_free=5523456`.
Post-transition malloc_live is 1886632 bytes versus 2289712 in the prior matched
candidate run (403080 fewer bytes, approximately394 KiB). This is a separate-run
allocator observation, not an exact model allocation size or hardware peak.
All counters are emulator observations, not physical peak or FPS measurements.
Both areas are inside the existing multi-area package, not different package
files. Normal exit released package bytes to zero. Genuine screenshot paths,
hashes and exact build/scope are indexed in emulator-observation.json. Previous
01445 normal enhanced/safe captures remain historical evidence; those modes
were not repeated for this frontend-only lifecycle change.

Camera investigation confirmed 01445 already used the recovered retail baseline
optics. No guessed distance/FOV/spawn rule was added. The obstructed destination
view is still unresolved. The destination event poll also correctly holds for
its unknown original transition-status owner; loading success was not used to
invent that source value. Fresh scratch counter origin, live RNG chronology,
map-edge fall/recovery, full attack/Buster/lock-on/HP/reaction/defeat and natural
tutorial progression remain incomplete. PHYSICS_1TO1=false. Physical01446,
hardware frame pacing and peak/call-chain memory remain pending. Both shaders
and vertex layout are unchanged from01443/01445.

Source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.46_Source.zip`. App-only SD ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01446_WorldComesAlive_SD.zip`. Actual binary: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01446_WorldComesAlive.3dsx`.
Source includes cumulative code/project/scripts/docs/authored fixtures. Excluded:
ROM/CHD/BIN/CUE/executables, retail models/textures/audio/video, cooked payloads,
DSP, saves, emulator profiles/captures, caches and generated build output.
No extraction/recooking or usage reset occurred. Adjacent SHA256 files and the
external release receipt verify CRC, every manifest hash and native inputs.

Next: recover destination camera/spawn and transition-status ownership from the
original reference, then continue the real encounter. Keep missing semantics
unknown and retain the existing safe path. This checkpoint does not finish WCA.
