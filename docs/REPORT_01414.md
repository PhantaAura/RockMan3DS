# 0.0.14.14 — package actors, player work and companion integration

This cumulative source checkpoint continues **0.0.14.13_ObjectResources** in the existing project. It is a native technical prototype with controllable movement and provisional firing, not a faithful playable tutorial or completed remaster. PHYSICS_1TO1=false. No usage reset was redeemed.

## Source and toolchain

Exact editable source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.14_GameplayParity`.

No Git repository, active branch, commit, or Git dirty state exists in the supplied cumulative checkout. Parent source is preserved under `work/Legends3DS_0.0.14.13_ObjectResources`; synced `sources/` and original archives were not edited. The earlier 0.0.14.6c baseline and 0143 renderer survive through the cumulative source/history documents. The audit and initial porting map are `docs/PORTING_MAP_01414.md`; inherited 25/25 host suites passed before editing.

Actual installed toolchain: devkitARM r68-1, GCC16.1.0, libctru2.7.0-1, Citro3D1.7.1-2. Native flags remain `-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft`. Stack reservation remains256KiB. No New3DS feature was introduced. The native GCC renderer-warning guard remains fatal and includes its negative-control check.

## What changed and what is connected

| System | Implemented behavior | Practical limit |
|---|---|---|
| Package/runtime binding | Optional section0x21 L3RBv1 carries exact L3SFv1 flow and L3EBv2 EBD slices. Typed `St00FlowV1` backend separates local map index from source flow selector. Native main consumes package.runtime_areas and instantiates supported actors. | Backend selection is explicit; wider flow/event/audio/message/material semantics are not guessed. Exact geometry identity still gates tutorial sidecars. |
| Actor resource consumer | Class20 mesh descriptors/packets become neutral PbdModel geometry. Original initializer profile supplies Q12 scale512, shade0x4210 and heading. Static source pose/hierarchy enters the enhanced native draw path. | Two area2 records share a mesh:52vertices,4triangles,46quads. Class/subtype/initializer evidence is not enemy identity or AI. Texture0x3c80002b is absent from matching caches. Actor floor and progression-selected pose remain UNKNOWN. |
| Actor lifetime | Bounded original16-slot admission, shared resource ownership, per-area explicit unload and load diagnostics. | Geometry accounting is estimated element storage, not allocator overhead. Supported actor geometry is1600B on64-bit host,1520B on32-bit ARM. |
| EBD ownership | Immutable retained EBD bytes are shared across active/registered area copies; explicit mutation detaches. | All five source payloads remain resident:567232B uniquely retained. Full disc/early-game streaming is not complete. |
| Movement | Ordinary running action2 replaces inherited slow action1. Source-backed startup, direction offsets, animation gates, reverse/release subset and idle action4 turning feed the native RuntimeSession. | Running jump and broader firing, scripted, knockback, slopes and recovery owners remain incomplete. |
| Animation | Actual S-record pose/countdown/control bytes feed work+A4..A7. Source-selected nonzero records and forced same-request resets are retained. Animation updates at gameplay cadence. | Actor pose selection is still diagnostic. Initial retail animation pointer/record identity is not fully captured for arbitrary replay seeds. |
| Collision | Normal native ticks call the bounded source cell/contact subset: rectangle floor/wall/ceiling and negative packed cells. At most4 nearby cells, bounded response passes, no per-step allocation. | Diagonal/slope/deep-overlap unsupported cases remain UNKNOWN; visual floors are not collision proof. |
| Timing | Nominal30Hz gameplay accumulation is separated from synchronized rendering. Bounded catch-up keeps fixed physics steps and reports debt/discarded suspend time. Loading/UI focus do not accrue movement catch-up. | Measured PS1 trace has2VBlanks per player update. Exact real-device clock/pacing remains HARDWARE_PENDING. |
| Bottom companion | Optional visibility, button tab/zoom/pan/center commands, player heading/follow map, touch gestures and two-buffer raster reuse. Area changes invalidate caches. | No invented inventory, currency, portrait or EN. Native touch/button-combination acceptance still requires dedicated device testing. |
| Diagnostic entry | Optional owner-local `diagnostic_spawn_01414.txt`: `area x y z yaw`; bounded map/coordinate/render admission; ignored by safe renderer. | Explicit inspection setting only. Retail area/spawn identity is not established by this file. |

The 0143 shader and original fallback player/terrain draw path remain available; imported actor drawing is gated off on that renderer. No substitute actor assets were generated or borrowed. Source-derived neutral flat RGB128 is used where the actor texture has no verified binding; final GTE shading remains unresolved. The inherited stateful texture/CLUT/load-order implementation remains intact.

## Retail measurement evidence

The owner USA executable has SHA256 `10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`. Source-only evidence retains hashes, addresses, bounded scalar results and raw measurement fields; original game binaries remain outside the source release.

Source probes passed97 locomotion scalar cases. Existing contact probes include2444 rectangle response cases,1008 floor/ceiling cases,2304 negative-cell cases and6 jump-launch cases. Deep overlap/slope and omitted wider calls are excluded explicitly. Actor unpacking was corrected from66 to75 source channels/24rotation vectors and checked against the original bounded consumer arithmetic. The descriptor consumer copies96bytes, correcting a prior24-byte research description.

Actual reference execution used PCSX-Redux f78f8ef8 dev290, interpreter, bundled OpenBIOS, owner disc data and isolated unused memory-card paths. This is development reference evidence, not a PS1 hardware measurement. Existing ARM64 dynarec SIGILL is avoided by using the interpreter. Owner saves were not modified.

The **first25 forward player updates** match portable native replay across position, fixed position, velocity work, heading, movement state, animation request/frame/controls and two raw work fields. The reference observes speed512 and displacement32world units per player update. Labeled comparison/provenance is in `docs/validation_01414/flat-forward-profile-comparison.json`; the retained source-evidence summary is `flat-forward-bounded-comparison.json`. Both map areas1/4 contain the coordinates; native area1 provides floor-1, while original active area ID remains unproved. Thus this is MATCHES_MEASURED_FIELDS for the stated scalar window only. It is not complete retail physics or hardware parity.

The remaining95 samples are excluded because tutorial ownership removes input. Grounding was not directly captured in retail and is reported unknown. Original camera vector0x800a39c0 was actually an actor-position mirror; it is excluded rather than relabeled as a valid eye measurement. Camera/combat/events are not matching claims. Arbitrary IDLE animation phase may require recording the retail S-record pointer/index; a pose number alone is not that identity.

## Builds and checks

Final one-command local build (no retail extraction or recooking):

```sh
python3 -B tools/build_hardware_test.py --pack ../pass-20261003-objects/pack-v2 --out ../pass-20261003-gameplay/install-01414-r5
```

It verifies the existing101-output pack manifest/hashes, packages already-cooked slices, runs host build and31/31 CTest suites, validates package/runtime activation/unload, then actually compiles the ARMv6K target and builds a local SD layout. Tests include inherited core/importer/graph/material/CLUT/save/provenance/incremental checks and new shared payload, actor, contact, trace, companion and clock checks. Core and new C++ tests retain assertions; portable core uses warnings-as-errors. Focused actor AddressSanitizer/UBSan tests also pass.

Additional exact commands:

```sh
cmake -S . -B ../pass-20261003-gameplay/install-01414-r5/host -DCMAKE_BUILD_TYPE=Release
cmake --build ../pass-20261003-gameplay/install-01414-r5/host --parallel 4
ctest --test-dir ../pass-20261003-gameplay/install-01414-r5/host --output-on-failure
../pass-20261003-gameplay/install-01414-r5/host/l3replay ../pass-20261003-objects/pack-v2 docs/validation_01414/flat-forward-25-input.tsv /NEW/output.json --area 1 --seed docs/validation_01414/flat-forward-diagnostic-seed.tsv
python3 -B tools/p2/trace_compare.py docs/validation_01414/flat-forward-retail-labeled.json docs/validation_01414/flat-forward-native-labeled.json --output /NEW/comparison.json
```

Native output: `work/pass-20261003-gameplay/install-01414-r5/build/Legends3DS_01414_GameplayParity.3dsx`,505408B, SHA256 `9af8cefdbcf647a008fb4afa0f10c78acf9db0780fea752a9ea79c51e1c04b02`.

Local SD tree: `work/pass-20261003-gameplay/install-01414-r5/sd_out/3ds/Legends3DS/`. It contains owner-local cooked data and must stay local; it is excluded from the source ZIP. There is no standalone desktop game frontend claim: the portable host build/tests/replay are working.

Failures were retained rather than suppressed: two initial fixtures assumed slow movement or omitted animation-table flags; corrected source-running speed also placed a combat smoke target within an immediate hit, so its target is now placed relative to the player. The first native compile caught a released-key/camera-up variable collision, fixed by distinct names. Review fixed discarded animation record selections and loading catch-up. Sandbox GUI launch failed on macOS services; the explicitly authorized isolated emulator launch succeeded. An initial keyboard-only attempt did not advance; generated HID replay did. Original failed logs stay under install-01414/r2/r3; final logs are copied into docs/validation_01414.

## Native execution, images and remaining blockers

Azahar2126.0 settings explicitly use Old model (`is_new_3ds=false`), CPU100%, resolution1; isolated run/config/SD trees preserve prior runs. Final r5 native boot, gameplay rendering, package bindings, movement and actor draw submission were exercised. Diagnostic area transitions were exercised in the earlier 01414 build recorded in areas-emulator-summary.json. Generated controller inputs are labeled synthetic emulator tests. Final movement/turn replay recorded source position Z12520→11560 and heading0→480 in native CSV; viewpoint occlusion persists and one retained capture has a black top screen. This does not establish collision/camera parity. Images are genuine original window captures, kept separately from the code-only release. Paths/build/device hashes are listed in `docs/validation_01414/SCREENSHOTS.json`.

Original reference-position and default area2 views remain occluded by terrain. A source-origin-based actor inspection position shows the player and source-derived flat-shaded mesh; it is **diagnostic placement**, not proof of retail spawn or completed actor behavior. The original top-screen PS1 HUD is still unbound: GAUGE.BIN is absent from the current cooked pack, and HUD placement/state semantics need evidence. Top framing remains widescreen with extra scenery; no invented replacement top HUD was added.

Physical Old3DS/XL was not tested in this session. Emulator counters do not prove real GPU/CPU timing, load spikes, texture bandwidth or frame pacing. Stack high-water usage is not instrumented. Runtime CSV tracks malloc live/free, linear/VRAM free, owned geometry/texture bytes, phase times, triangles and draws; resource estimates are not whole-device RAM measurements. Per-frame player contact/pose helpers have allocation checks, but the inherited projectile container can still allocate on firing.

Other blockers: natural tutorial progression, generalized event execution/unknown opcodes, transition ownership, combat/enemy AI, firing cadence/damage/recovery, audio playback, full original cutscenes/expressions, dynamic camera/lock-on parity, complete save semantics and broader gameplay beyond the tested bindings remain incomplete. Source-backed candidate placement can clip/occlude; actual actor floor and texture resolution must be recovered before claiming faithful actor presentation. No post-boundary story details are included.


Retained actor-inspection HLE sample ranges (not physical measurements):

| Counter | Observed min–max |
|---|---|
| frame_before_log_us | 9733–90293 |
| update_us | 3–60 |
| render_us | 7925–16624 |
| bottom_ui_us | 18–6828 |
| draw_calls | 17–17 |
| triangles | 2084–2084 |
| owned_linear | 82560–82560 |
| owned_texture | 3670016–3670016 |
| malloc_live | 1885776–1885776 |
| linear_free | 27926272–27926272 |
| vram_free | 5523456–5523456 |

Timing columns are microseconds; memory columns are bytes. Owned linear counts geometry buffers; texture residency is counted separately and can also consume linear space. Counts are sparse samples and exclude stack high-water, executable size and unsampled peaks. Host/ARM package estimates differ with pointer/container size.

## Source handoff

`docs/CHANGED_FILES_01414.json` lists cumulative changes relative to01413. CURRENT_CHECKPOINT, readiness, evidence ledger and source manifest describe this pass. The new source-only archive is `deliverables/Legends3DS_0.0.14.14_Source_r1.zip`; its adjacent `.sha256` records the actual archive digest. ZIP inventory/CRC and source hashes are checked. It includes cumulative source, build scripts, tests and text evidence; excludes all original/cooked Capcom payloads, ROM/CHD/BIN/CUE/executables, saves, models/textures/audio/video, screenshots, reproducible caches and generated builds. Earlier recovery ZIPs and original archives remain untouched. The preserved initial Source.zip is superseded by Source_r1.zip solely for these report wording corrections and the updated handoff path; compiled code is unchanged.

Next concrete work: capture retail active area and actor floor/pose/material ownership; solve the known occluded camera/terrain views using matched retail visibility data, then validate the existing native build on physical Old3DS. Do not interpret this checkpoint as a completed tutorial, full game or1:1 remaster.
