# Legends3DS 0.0.14.6b engineering checkpoint

This cumulative patch extends only the supplied 0.0.14.6a source baseline. It is a substantial importer, validation, replay and diagnostics checkpoint. The complete requested A–Z retail reimplementation is **not finished**. Full physics, native event execution, enemy/combat behavior, camera/cutscene acting, expression/audio playback and arbitrary-area frontend loading remain research/integration work.

## Delivered

- All 225 inherited files retained, with hashes and an audit map. The frozen fallback shader is unchanged. Independent renderer masks and CPU timing/draw diagnostics are compiled in the actual 3DS frontend.
- A generalized incremental owner-data pipeline emitting 205 CRC-checked native envelopes, resource provenance, a 4,115-node / 3,916-edge graph and a code-only regression corpus. It preserves 166 gameplay-state nodes and identifies 31 geometry identities among 37 geometry packages.
- Corrected inherited undefined behavior when scaling negative terrain coordinates; a negative-coordinate regression and halt-on-error sanitizers now pass.
- Native envelope validation now checks CRCs, duplicate section kinds, unaligned reads, overflows and overlap. The native validation tool also invokes the inherited map/render/object/EBD/message readers.
- A conservative ROCK_NEO.EXE research index: 319 entry-rooted function candidates and 64 source-address candidates, with file offsets, direct calls, callers, tail edges, memory operands, global-address candidates, indirect-control-flow gaps and save anchors. These are candidates, not newly proven semantic function names.
- A 55-entry structural animation evidence map with source hashes and request-call evidence. No animation semantics inferred from numeric proximity.
- Nine deterministic RuntimeSession replay scenarios, run twice and again with animation disabled. All nine repeat exactly; non-animation state is unchanged by presentation. Eleven missing movement scenarios remain explicitly UNKNOWN. No retail golden trace was compared.
- Safer native persistence: unique temporary writes, validation before exposing data, backup recovery, alias rejection and protection against replacing the valid backup with corrupt primary bytes.
- A one-command host/owner/native build, source safety checks, source release packager, checkpoint metadata, exact changed-file list and readiness matrix.

## Measured owner-data results

The owner extraction matches all inherited reference fingerprints. Fresh broad import produced 37 maps and renders containing 203 local areas, 135,572 cells, 3,891 descriptors, 10,569 contact primitives and 104,789 render primitives. EBD/message validation covered 136 directories, 4,620 rows, 129 banks, 17,594 message blocks and 721,779 tokens. Object scanning retained 34 candidate tables, 207 lists and 649 records. Audio catalogued 38 XA resources and one STR resource; routing is unproved.

All 205 new envelopes pass their native readers. First incremental run: 228 decoded tasks and 111 same-content cache hits. Second unchanged run: 339 hits, zero decodes. Geometry sharing does not merge gameplay states. Areas without a directly contained geometry quartet do not acquire an invented binding from their filenames.

The known tutorial compatibility pack contains 101 files. Terrain materials resolve 14,328 of 15,512 primitives; 1,184 remain unresolved. Two player material words resolve in all five tutorial states. The five known states have different resource layouts and are reported as EARLY_AREA_A–E in the attached corpus. The broader structural sweep does not establish playable reachability or ordering.

## Exact validation

| Check | Result |
|---|---|
| Inherited Debug CMake build before changes | PASS; core + push2, 2/2 |
| Updated Debug and Release C/C++ build | PASS; warnings-as-errors on core, bridge and new tools/tests |
| Updated CTest | PASS; core, push2, integrity, pipeline_python, 4/4 |
| Python pipeline unit cases | PASS; wire format, graph invariants, recursive traces, MIPS CFG, cache invalidation |
| ASan + UBSan CTest, halt-on-error | PASS; 4/4 |
| Inherited flight_qa.sh | PASS; all six stages |
| Broad geometry/script/object/audio probes | PASS; measured counts above |
| Native envelope readers | PASS; 205/205 |
| Repeatability and animation isolation | PASS; nine scenarios, three executions each |
| Fallback/UV shader fingerprint verifier | PASS |
| Cumulative H1–H10 source verifier | PASS after correcting its stale standalone paths |
| devkitARM/libctru/Citro3D build | PASS; real 3DSX, no compiler warnings in final native log |
| Physical Old 3DS test | NOT RUN; HARDWARE_PENDING |

Exact command outputs and binary hash are recorded in `validation.json`. Source archive integrity is recorded by the adjacent SHA-256 file and internal SOURCE_MANIFEST.sha256.

## Limits and next dependencies

The new envelopes are host-produced and native-reader-validated; the 3DS frontend still loads the inherited tutorial pack. General area activation needs verified geometry/state bindings and retail event/transition dispatch. Existing frontend SELECT area cycling remains a developer mechanism, not natural tutorial progression. Current camera and combat defaults remain candidates. The replay does not include frontend out-of-map rollback or a complete retail collision solver.

Unknown EBD opcodes, object roles, save fields, palette changes and animation meanings remain unknown. The executable index resolves direct control flow only; indirect dispatch and caller-specific dataflow need further tracing before replacing gameplay rules. The original validated 8 KiB save was absent from the supplied archives. No original saves were modified.

Device memory peaks, stack use, GPU orientation/transparency, audio output, power-loss save recovery and frame pacing still require Old-3DS testing. Host timing/profiling instrumentation is not a hardware result. Native persistence retains inherited v2 header limitations described in PACKAGE_AND_REPLAY_0146b.md.

No owner executable, disc data, textures, audio, video, saves or cooked assets are in the source ZIP. Local owner output and the optional compiled executable are separate.

## Readiness

| System | Status | Evidence / remaining limit |
|---|---|---|
| BOOT | HARDWARE_PENDING | Real native binary built; no device test this task. |
| RENDERER | SOURCE_BACKED_PARTIAL | Frozen fallback shader verified; independent gates and timings compiled. |
| TERRAIN GEOMETRY | SOURCE_BACKED_PARTIAL | 37 inherited geometry cooks; 31 byte identities. |
| TERRAIN TEXTURES | SOURCE_BACKED_PARTIAL | 14328/15512 tutorial primitives resolved; 1184 dynamic cases unresolved. |
| MEGA MAN MODEL | SOURCE_BACKED_PARTIAL | Five retail components cooked; host pose probe passes. |
| MEGA MAN MATERIALS | SOURCE_BACKED_PARTIAL | Two material words resolved in five tutorial states; GPU appearance pending. |
| ANIMATION | SOURCE_BACKED_PARTIAL | 55 structural mappings; request/frame evidence and presentation isolation tests. |
| MOVEMENT | SOURCE_BACKED_PARTIAL | Inherited ordinary-motion helpers retained; nine integration replays repeat. |
| PHYSICS | UNKNOWN | No PS1 golden trajectories; full state dispatch/solver not reconstructed. |
| COLLISION | SOURCE_BACKED_PARTIAL | Inherited cell-indexed contact data and response helpers; full solver pending. |
| CAMERA | CANDIDATE | Inherited session and frontend camera approximations remain; retail camera ownership incomplete. |
| LOCK-ON | CANDIDATE | Inherited integration selection is not verified retail acquisition. |
| BUSTER | CANDIDATE | Inherited tunable projectile defaults, no newly proved cadence. |
| COMBAT | CANDIDATE | Existing bridge retained; retail damage and reactions unverified. |
| ENEMIES | CANDIDATE | Object lists discovered; dispatch semantics not reconstructed. |
| OBJECTS | SOURCE_BACKED_PARTIAL | 34 candidate pointer tables, 207 lists, 649 records; roles unknown. |
| EBD / EVENTS | SOURCE_BACKED_PARTIAL | 136 directories cooked; raw opcode semantics not inferred. |
| DIALOGUE | SOURCE_BACKED_PARTIAL | 129 message banks parsed; waits retained; event coordination incomplete. |
| TRANSITIONS | CANDIDATE | Location API exists; automatic retail transition bindings unknown. |
| CUTSCENES | CANDIDATE | Inherited isolated cue framework; original scene execution not complete. |
| EXPRESSIONS | UNKNOWN | Retail expression-state mapping not established. |
| AUDIO | SOURCE_BACKED_PARTIAL | 38 XA and one STR catalogued; runtime routing/playback incomplete. |
| SAVE | SOURCE_BACKED_PARTIAL | Executable anchor xrefs and safe native host persistence; original validated save absent. |
| CONTENT GRAPH | SOURCE_BACKED_PARTIAL | 4115 nodes/3916 edges; measured containment and identities, unknown relationships explicit. |
| GENERALIZED IMPORTER | SOURCE_BACKED_PARTIAL | 205 envelopes accepted by native readers; unknown components omitted. |
| EARLY-GAME IMPORT | SOURCE_BACKED_PARTIAL | Five tutorial state structures tested; broad structural sweep does not establish early-game reachability. |
| BOTTOM SCREEN | CANDIDATE | Inherited developer frontend and map retained; retail UI unfinished. |
| QoL | CANDIDATE | Classic defaults retained; no new mandatory gameplay behavior. |
| LOADING | SOURCE_BACKED_PARTIAL | Incremental cook keys/provenance and checked parsers; general frontend area loading pending. |
| MEMORY | HARDWARE_PENDING | Host sanitizers pass; device peaks and allocation coverage unmeasured. |
| FRAME PACING | HARDWARE_PENDING | CPU timing instrumentation compiled; no device measurements. |
| OLD-3DS BUILD | MATCHES_MEASURED_FIELDS | devkitARM/libctru/Citro3D compile/link and 3DSX output succeeded. |
| OLD-3DS HARDWARE | HARDWARE_PENDING | No physical Old 3DS tested. |
