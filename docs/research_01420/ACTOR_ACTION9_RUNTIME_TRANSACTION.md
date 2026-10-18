# Imported action9 runtime transaction checks

The imported actor bridge now consumes the source action9 contact preparation,
its actor-to-player turn target, the separately reversed player-to-actor angle,
and raw player heading. It validates source contact snapshots and animation A6
before running the source body. The newly proved detector predicates admit the
source detection preparation; this result supplies the effective flag10 used
by both the contact producer and body.

The transaction publishes source work, contact velocity/state74, imported pose,
current animation cursor, source sound producer records, and detection global
notification together. Missing or stale evidence holds the whole transaction.
Original source sound176 uses the point after wall response and before floor
stores or full-lane rollback. It differs from action2's pre-motion point.

`test_actor_action9_runtime` uses authored scalar data and tiny authored pose
banks, without retail geometry, textures, statistics, tables or animation data.
The original body/contact proofs are separate. The integration regression
checks:

- Actual source contact preparation followed by phase0 fallthrough/request1 pose
  restart, preserved snapshots and committed contact fields.
- Negative-cell wall Y correction before176 copies its two source position words.
- Floor-gap rollback of all four raw/fixed lanes, while176 retains the post-wall
  point copied before floor/rollback.
- Twenty-four stale producer signatures, including raw/fixed fourth lanes, previous
  snapshots, velocity, state74, class, action, phase, flags, heading, distance,
  target angle, A6, source turn result, producer status and raw/fixed/floor/contact payload consistency.
- Twelve stale detection, missing angle/player-heading/queue/bank/request, malformed
  pose and nonzero-packet holds that preserve work, pose, contact and queue bytes.
- A late missing bank after detector156/body176 are staged; both remain unpublished.
- Original full-queue drop without blocking the source body or changing retained bytes.
- Signed animation A6 values0,80,FF without a source176 request.
- Phase2 without movement or sound dependencies, preserving the unsigned raw
  player-heading difference in work36E.
-384 admitted prepare/detector/body/audio/pose ticks without heap allocation or
  replacement of the pose translation storage.

Build/run through the inherited CMake test target:

```sh
cmake --build /local/host --target test_actor_action9_runtime
/local/host/test_actor_action9_runtime
```

This verifies native integration under explicit authored detector/reference
premises. It is SOURCE_BACKED_PARTIAL and does not establish live retail parity,
complete shared BIOS random ownership, enemy attacks/defeat, audible playback,
or physical Old3DS behavior.
