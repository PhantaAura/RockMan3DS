# 0.0.14.22 World Comes Alive — partial cumulative checkpoint

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.22_WorldComesAlive`. No Git branch/commit is available. Baseline: 0.0.14.21 Source ZIP, SHA256 4d53bb62202a7bcea50a9595dbd389d0c932234e553aae7f79d8be72cc113274. This preserves the cumulative project and0143 fallback; World Comes Alive is not finished.

New connected work: checked parent→shared32slot child delivery; original child motion/contact/proxy/registry/shadow/audio ownership; exact hit/proxy reducers; source poll→event1→player/logical driver receipts; optional shared observed-seed OpenBIOS RNG service. Its native scheduling is CANDIDATE, strict retail tape remains unchanged, and PHYSICS_1TO1 remains false. No models/animations were recreated.

81/81 inherited/new host checks pass. Actual Old3DS ARMv6K build r5 passes with zero compiler warnings using devkitARMr68-1/GCC16.1.0/libctru2.7.0-1/Citro3D1.7.1-2. Binary655404B, SHA256 03e468efc42f55eb1e31248b05072638a541f4224277b4044148df6a13816abe. The desktop portable core and probes build/run; no complete desktop game was exercised. Native compiled files match the saved source hashes. Logs and exact commands are in docs/validation_01422 and docs/TEST_COMMANDS_01422.txt.

Azahar2126.0 Oldmodel100%CPU/1x runs recorded movement/jump and developer area0→1 load. Three real imported actors update; one sampled action2/phase1/request1 instead of NeedsRandomSample. This uses ACTOR_RNG_CANDIDATE=1, not original chronology parity. Current replay allocates no child; child delivery/lifecycle are exercised by authored original-instruction differentials and integration tests. Source event boot gate OFF in this native run; the fresh source subset→phase2 is tested on host. No physical hardware testing occurred.

Latest sampled native frame: {'frame': 600, 'area': 1, 'draw_calls': 18, 'triangles': 2184, 'owned_linear': 71640, 'owned_texture': 4194304, 'malloc_live': 1937440, 'linear_free': 27412992, 'vram_free': 5523456, 'update_us': 3, 'render_us': 15441, 'frame_before_log_us': 15473}. Sparse timing maxima are in native-observation.json and include replay pauses/host synchronization; they are not hardware performance. Child owner occupies23940B static memory; sharedRNG1280B. Exclusive ARM childtick2608B/actorbridge2624B stack estimates exclude nested call chains and hardware stack peaks.

Original screenshot files, outside the asset-free sourceZIP:
- Azahar2126.0 Old3DS model; 0.0.14.22 r5; tutorial diagnostic gameplay: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01422/captures/azahar-old3ds-01422-r5-gameplay.png`
- Azahar2126.0 Old3DS model; 0.0.14.22 r5; area1 actor overlap/camera clipping after replay completion; paused: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01422/captures/azahar-old3ds-01422-r5-actor-overlap-bug.png`

Visible material/alpha artifacts persist. The second capture deliberately records actor overlap and camera clipping after the movie ended; it is bug evidence, not a successful scene acceptance.

The supplied Emerald3DS comparison supports preserving original game logic behind small platform interfaces, owner-local cooked data, bounded memory and diagnostics. No Emerald code/assets were imported into this project; its GBA rendering approach does not replace the PS1 renderer.

Remaining blockers:
- Original RNG input/update/competing-consumer chronology remains UNKNOWN; optional shared native service is CANDIDATE and rolls back held callbacks.
- Player/Buster GTE hit consumers80034EB8/800358F0 and source pose-derived actorpoint180 owner remain unconnected; real HP/reaction/defeat encounter absent.
- Child effects128/131 and shadow/primitive rasterization/fog/physical pairs remain unported; no live child in current replay.
- Eventphase2 original sound/command consumers8001B33C/8001D58C remain held; native boot subset defaultsOFF; no full original cutscene/message/normaltransition sequence.
- Imported player materials/alpha and actor overlap/camera obstruction remain inaccurate; originalPS1FOV/HUD not verified.
- Natural tutorial progression, full early-game boundary and physical Old3DS validation remain incomplete.

Assessment: a native technical gameplay prototype, not a complete playable tutorial port. Full readiness uses only the six requested evidence labels in state/CURRENT_READINESS.json.

Private local SD layout: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261007-worldalive-01422/integrated-native-r5/sd_out`. Code-only complete cumulative source ZIP: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.22_Source.zip`. It excludes owner retail/cooked content, binary outputs and caches. SHA256 is delivered in the separate .sha256 file; original inputs/checkpoints remain untouched.
