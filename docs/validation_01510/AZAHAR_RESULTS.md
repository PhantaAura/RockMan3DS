# 0.0.15.10 Azahar results (Oct 19 2026, driven by Claude)

Azahar 2126.0, Old 3DS model, keyboard input. Build SHA-256 matches the
`pass-20261019-165744-m2-01510` output (967,580 B).

| Check | Result |
|---|---|
| Boot | PASS. Both display tests pass, the Area 1 handoff spawn works, 59–60 FPS. |
| Original follow camera | PASS (visual + trace). Mega Man is framed from behind down the first corridor. A quick turn trails then swings behind him. `source_follow_camera` eye/target match 0x80038894 (distance 512, eye 511 behind, Y −187/−150). The first frame at spawn starts with the eye inside the start-room wall, so Mega Man is hidden until he walks forward. |
| First door → Area 2 | PASS (trace + visual). Diagnostic spawn (1,−334,−1,−5750,2048): `door_transition_before` in area 1, then `door_transition_after` in area 2 ~1.9 s later. Mega Man ends at (−256,−1,7104), matching `l3door_replay`. The trace shows him moving from z=−6191 to the slab without Up held; the exact press frame is not in the sparse trace. |
| Enemies | PASS (visual). The region-3 conditional spawn puts two class-20 Reaverbots beside Mega Man in the door room. |
| Lock-on 0x80034EB8 | PARTIAL. It runs every frame: status 0, the proxy bank projects 1–3 enemies. With the Reaverbots beside him, no target was selected (`broad=-1`, `window=-1`). |
| Buster shot | NOT OBSERVED. Three Y presses arrived (held 0x800 at frames 495/522/542), but `projectiles` stayed 0 and the action stayed 0. Needs investigation. |
| Start corridor enemies | None spawned in the normal-boot run (no actor records). |

Not covered: lock-on with an enemy straight ahead; physical Old 3DS. Logs are
kept owner-local; they are not committed.

**Follow-up (0.0.15.11):** the shot was spawned. The CSV `projectiles` column
counts the retired candidate projectile, and the diagnostic ribbon had zero
length. See `docs/research_01511/BUSTER_SHOT_PRESENTATION.md`.
