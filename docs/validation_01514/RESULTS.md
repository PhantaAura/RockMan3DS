# 0.0.15.14 validation: World Comes Alive gameplay matrix

Build: `pass-20261020-122714-m2-01514`. `Legends3DS.3dsx` is 994,696 B,
SHA-256 `c4164659a6b82a945f91f6ce339bcc88097f1d1b3bda96707c9ad536846f450a`
(not committed). Host: 154/154 PASS. Owner `l3door_replay`: PASS (fall,
door, terrain patch, Buster). Native: PASS, 0 warnings.

**Azahar method.** Azahar 2126.0 ran in Old 3DS mode at 100% CPU with the
owner's existing graphics settings. The SD card held only the Legends3DS
folder from this build. Full-screen control was not approved, so runs used the
opt-in `diagnostic_autoplay.txt` (scripted input, FRONTEND_DIAGNOSTIC) with the
diagnostic spawn. The scripts are in `autoplay/`. Results come from the
hardware CSV traces: no screenshots were taken, so on-screen appearance was not
checked this pass. The owner's separate Azahar session was left untouched.

| System | Result | Evidence |
|---|---|---|
| Movement and collision | **Validated in Azahar (scripted)** for walking, tank turning and wall stops. The door slab stopped him at z −5409, other walls at z −6191 and x −1503, and the Area-2 door at z 6433. Y stayed −1 in every run (−2 only on door arrival frames). No falls or escapes. Diagonal walls and deep overlap were not driven this pass (host `l3edge_audit` / tests only). | door and combat CSVs |
| Follow camera | **Validated in Azahar (trace only)**: `source_follow_camera` 0x80038894, distance 512, `unsupported_eye_tests=0` in all samples. | door CSV |
| Buster firing | **Validated in Azahar (trace)**: 30/30 Y presses spawned a subtype-0x81 shot (`spawn_status=0`). It flew 100 units/frame for up to 18 frames: 23 shots hit walls, 7 hit enemies. On-screen drawing was not checked (FRONTEND_DIAGNOSTIC). | combat CSV |
| Lock-on | **Partially functional**: `0x80034EB8` runs and projects 1–3 enemies, but never selected a target in these runs (`window=-1`). Homing `0x80033F04` is not ported. | combat CSV |
| Enemy behaviour | **Validated in Azahar (trace)**: waiting, chasing (9), lunge (10), attack (4) and damaged bounce (5) all run with body status 0. The 0.0.15.12 freeze is gone. Mega Man's HP stayed 100: whether enemy attacks damage him was not tested. | combat CSV |
| Enemy damage and defeat | **Validated in Azahar (trace)**: hits deliver 24 (0x800375D4 row 1, level 0). Enemy 18 went 64 → 40, then did the action-5 bounce and self-destruct, defeated at frame 563 (effect 128, reward 0x80047228, sound 161). Enemy 17 went 64 → 16. The action-6 knockback was not reached. | combat CSV |
| Reward spawning and collection | **Validated in Azahar (trace)**: `0x80046B34` spawned 2 pickups (1 discarded). They flew, rested on the floor (state 2), blinked (3) and despawned. One was collected (state 5) for money 0 → 5, exactly once. The pickup model is still a FRONTEND_DIAGNOSTIC marker. | combat CSV |
| First door transition | **Validated in Azahar (trace)**: Up into the slab, then A. `door_transition_before` area 1, then `after` area 2 in 1.93 s, arriving at (−256,−2,6208) and walked to (−256,−1,7104). Matches `l3door_replay`. | door CSV |
| Area 2 entry and return | **Validated in Azahar (trace)**: control returned in Area 2 (turn and walk). Steering to (−256, 6464) and A sent him back to Area 1 in 2.01 s at (−256,−2,−5184). He then walked about 1,000 units on Area-1 floor. Door visuals were not checked. | door CSV |
| Physical Old 3DS | **Not run.** | — |

## Performance (Azahar, sparse CSV samples, n=547)
Emulated, with a second Azahar instance running at the same time. This does
not establish hardware pacing.

- Interval: median 16,655 µs, p95 18,584 µs, max 107,341 µs (one spike).
- Render (including sync): median 15,774 µs. Update: median 3 µs, p95 652 µs.
- Draw calls 14–17, triangles 1,680–2,154.
- `malloc_live` ≤ 1,939,848 B; linear free 27.4 MB.

## Original reference
PCSX-Redux `f78f8ef8` with OpenBIOS (pad-driven probe,
`docs/research_01514/redux_buster_probe.lua`). Details in
`docs/research_01514/BUSTER_SHOT_81.md`.
