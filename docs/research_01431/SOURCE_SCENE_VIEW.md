# Scripted view ownership — 0.0.14.31

Status: **SOURCE_BACKED_PARTIAL**. PHYSICS_1TO1 remains false. No hardware or
complete scene/raster parity is claimed.

The script interpreter's vectors are not already a renderer camera. Original
`80038894` selects `8001E550` when retained mode byte `800A395C` is not1;
the supported phase5 constructor explicitly writes3. `8001E550` uses an eight
byte earlier base (`800BC730`) than the188-byte camera view (`800BC738`). This
base difference matters: the nontracked target adds camera vector+8 to the
retained vector+98, not+A8. An initial authored differential caught and corrected
that offset error before native integration. The rejected first private oracle
is retained separately; accepted fixture/proof come from `scene-view-oracle-r2`.

The supported branch requires retained byte `800BC733=0` and camera byte+0=0.
Each target component adds two unsigned32 words with source wrapping and takes
signed arithmetic shift16. Distance, yaw and pitch come from signed halfwords
+42, +3A and+3E. The caller then executes `80038324 → 80037FB4`, which produces
transposed packed-table Y/X rotation and translated camera work at `1F800008`.
The producer supports source shifts0,1 and2, the three original script setters
currently admitted by the interpreter. Unsupported shifts hold explicitly.

The view helper uses **two different trigonometric owners**: the packed matrix
table at `80092EC4` and scalar quarter-wave table at `80092348`. The existing
ReferenceTables owns the former. The new bounded, hash-guarded owner-local
`source_scene_view.l3sv` sidecar retains the latter (1025 signed halfwords).
Using a packed table or host sin/cos for the scalar functions would change the
original arithmetic. No executable instructions are loaded or run by the native
runtime, and this sidecar must remain outside source releases.

Projection controls have their own lifetime. The unchanged fresh executable
prefix `80011F48..80011F58` calls the original offset setter with160/120 and H
setter with384. The probe executes all14 original instructions in this prefix.
The new `initialize_source_scene_projection_80011f48` operation admits these
three controls **only at an explicit fresh executable lifecycle**. It retains
DQA/DQB and must not act as a fallback after a save, warm area load or unknown
camera restoration. The owner-local profile validates exact EXE identity,
reserved fields, dimensions/counts, payload coverage, CRC, scalar range and
endpoints. The cooker additionally checks the five actual caller instructions
before emitting decoded controls. The source release includes only the codec,
authored tests, addresses, hashes and proof counts.

`produce_source_scene_view_8001e550` takes immediate borrowed const snapshots of
profile, packed ReferenceTables, phase5 scene and phase6 script, plus the calling
session's current EBD/resource admission. It returns value-owned semantic R/T,
eye and target fields. It neither retains a resource pointer nor changes camera
state, event counters or story flags. The frontend must call it against the
current active-area witness each frame and consume that value in the same frame;
it must never cache a prior admitted result across area/profile/save/session
replacement. The result is absent for expired resources, missing validity bytes,
unowned mode/control/shift/optics, tracked actor or target-point camera branches.
Renderer receipt does not acknowledge scene completion or original actor update
cadence. Original scratch matrix padding is not a semantic transform input and
is not fabricated by this adapter.

The native transform is an explicit coordinate conversion. Existing vertices
are `[rawX,-rawY,rawZ]/512`; native camera forward is-Z. Each original Q12
coefficient is retained without normalization, then conjugated by these input
and output axes. Translation divides by `512 << sourceShift`. This consumes the
actual source transform instead of creating a generic look-at camera. The extra
40 pixels on each side center the original320x240 window inside400x240. H and
vertical framing stay unchanged; the FOV is derived mathematically from owned H.
This is the user's requested extra side scenery. Source center coordinates are
returned explicitly. Fractional offsets and nonpositive H remain unsupported.
The frontend still owns its native near/far planes, clipping, float precision,
GTE saturation, depth/raster and draw ordering; these remain **CANDIDATE** or
**UNKNOWN**, not original PS1 projection fidelity.

The accepted oracle runs384 complete original radial branches, including signed
extremes, wrapped targets, both trig quadrants, noncanonical authored matrix
coefficients and all three source shifts: **288451 unchanged original CPU
instructions**, **1536 restricted interpreted MVMVA commands**, zero patches and
zero helper hooks. Its fixture includes only authored values. The inherited GTE
slice does not independently prove hardware FLAG/saturation or raster behavior.
Native tests compare all nine coefficients, three translations and narrowed eye
components, test coordinate adaptation/vertical optics, reject19 malformed
profiles, exercise absent/invalid owners atomically, and run4096 allocation-free
frames. Clang warnings-as-errors and ASan/UBSan pass. The actual ARMv6K/GCC16
compile passes warnings-as-errors; compiler stack usage for the hot producer is
456 bytes (not a measured peak across its call chain).

Exact reproduction from source:

```sh
python3 -B docs/research_01431/source_scene_view_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE --out /path/to/new/private/oracle
python3 -B tools/cook_source_scene_view.py \
  --exe /path/to/owner/ROCK_NEO.EXE --out /path/to/new/private/sidecar
python3 -B tests/test_source_scene_view_cooker.py
```

Integration: register `src/world/source_scene_view.cpp` and the fixture-backed
`test_source_scene_view` plus Python cooker regression. Load the2126-byte sidecar
once, invoke the explicit fresh projection initializer before the supported
source constructor, and call the semantic producer when the current source
script owns the camera. Fill each C3D matrix row with `FVec4_New(x,y,z,w)`; direct
copying into `C3D_Mtx::m` would reverse the vector component layout. Keep the0143
fallback independent. At an explicit hold report the exact missing owner and do
not claim that the generic frontend view is the source scene camera.
