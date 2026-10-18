# 0.0.14.14 gameplay measurement and control corpus

The inherited replay runner proves native determinism, not retail parity. Its
unlabeled output remains usable through the existing `compare(list, list)` API or
the explicit `--legacy-native` CLI option. Neither path can produce a retail
comparison claim. The default comparator now requires a labeled measurement.

## Measurement shape

`mml-gameplay-trace/1` is an object with `metadata` and `frames`. Each frame has a
contiguous integer `frame`, beginning at zero, and a normalized `input` object:

```json
{"held_psx_mask":16,"fire_pressed":false,"lock_held":false,
 "advance_dialogue":false,"lock_pressed":false,"pause_pressed":false,"skip_held":false}
```

`metadata` requires:

- `source_kind`: `retail_reference`, `native_replay`, or `synthetic_test`.
- `game_profile`: exact owner-data profile `id` and lower-case `sha256`.
- `producer`: emulator/build `name`, `version`, `artifact_sha256`, and
  `settings_sha256`. A retail capture should additionally retain BIOS identity,
  game identity, settings, source capture path, and bounded-read address evidence.
- `scenario` and `initial_state_id`: the experiment and a reproducible initial
  player/environment state. A shared label alone does not prove equivalent state.
- `timing`: `clock`, `sample_phase`, positive integer `rate_num` and `rate_den`.
- `coordinate_profile` and `control_profile`: explicit coordinate convention and
  installed control table. `USA-profile-0` is the source-backed table at
  `0x800425FC`; other profiles must not be silently mixed.
- `capture_fields`: unique frame-field names excluding compulsory `frame/input`.
- `field_units`: a unit/semantic identity for each captured field. Heading uses
  `turn_4096` when the known 4096-unit wrap is appropriate.
- `input_sha256`: computed by `mml_trace_io.py` from actual normalized recorded
  inputs. This is independent of the TSV-file fingerprint in the control corpus.

Frame fields can include `position`, `fixed_position`, `velocity_work`, `heading`,
`state`, `grounded`, `contact`, `vertical_velocity`, `animation_request`,
`animation_frame`, `camera`, `lock_target`, `buster_enabled`, `projectiles`, and
`hp`. Capture only fields whose meaning and bounded source read are established.
Use JSON null for unavailable data; zero and false are actual measurements.
For native work velocities, the source stores signed halfwords with sixteenth
world units per update; preserve those raw values rather than converting them to
position deltas. Positions use the PS1 y-down coordinate convention.

Producer identities may differ between retail and native; comparable game,
scenario, initial state, controls, coordinates, timing, input stream, and captured
field semantics must match. A GPU-vsync snapshot is not automatically equivalent
to a player-update epilogue, nor does guest FPS establish the simulation cadence.
No interpolation, global offset correction, rescaling, or frame search occurs.

## Commands

Metadata is an explicitly prepared JSON object containing the required fields;
the tool computes its input hash but does not create missing provenance.

```sh
python3 tools/mml_trace_io.py wrap-native native-rows.json native-metadata.json native.trace.json
python3 tools/mml_trace_io.py ingest-retail observed-retail-rows.json retail-metadata.json retail.trace.json
python3 tools/p2/trace_compare.py retail.trace.json native.trace.json --output comparison.json
python3 tools/p2/trace_compare.py native-first.json native-second.json --legacy-native
python3 tools/mml_control_corpus.py build/control-corpus-01414
python3 -B tests/test_parity_trace.py
```

Ingest refuses to overwrite captures. Exit 0 means observed fields match, exit 1
means gameplay values diverge, and exit 2 means the trace/profile is malformed or
incomparable. A rejected profile is not a measured gameplay mismatch. Trace
metadata records provenance; tooling alone cannot authenticate an emulator run.

The report contains first divergent frame/field/reference/candidate/delta,
per-field maximum and mean numeric error, mismatch counts, and measured/unknown
coverage. A null retail field is excluded and remains `UNKNOWN`. A measured
reference field missing from the native capture diverges. Numeric states never
compare equal to booleans. Heading error follows the measured field unit, not a
generic angle guess.

`MATCHES_MEASURED_FIELDS` means only observed retail-to-native values matched.
Native/native agreement remains `CANDIDATE`. `PHYSICS_1TO1` remains false even
when a partial capture matches, since unmeasured dispatch/collision/gameplay
behavior cannot be established by the comparator.

## Reproducible experiments, not golden data

The generated corpus contains 21 plain input scripts: idle, acceleration, release,
reverse-input brake, reverse, three requested turn goals, turn reversal, jump tap,
jump hold, landing, air control, ascending/descending slope, ledge, step, wall,
knockback, firing movement, and lock movement. Environment-dependent cases list
their required source surface/target/stimulus and start state. None contains
fabricated expected output. A script run on the default spawn is not evidence of
a slope, ledge, step, wall, enemy hit, or valid lock target.

The inherited turn durations are preserved as experiments, but names TURN_45,
TURN_90, and TURN_180 do not assert that their inputs create those angles. Native
fire/lock bridge flags are not silently translated into retail pad bindings; the
retail capture must identify the corresponding pad schedule and dispatch behavior.

## Previous reference evidence

The local `work/pass-20261003-visual/ps1-reference/README.md` documents PCSX-Redux
`f78f8ef8`, interpreter, OpenBIOS, owner USA disc `SLUS00603`, and genuine
320x240 tutorial captures. Its settled camera/projection measurements are bounded
scalar snapshots. They do not provide frame-aligned movement/physics traces.
`first_encounter_idle` was explicitly excluded there after player defeat/title
attract. Do not reuse it as an encounter or gameplay parity capture.

The old replay camera is `RuntimeSession::camera`, while the 0.0.14.12 frontend
render framing had separate calibrated parameters. Those camera sources require
explicit semantic identity before they can be compared to retail values.

The automated comparator tests use invented *synthetic test fixtures* only to
exercise validation and first-divergence behavior. They are not saved as retail
capture artifacts and provide no game-behavior evidence.
