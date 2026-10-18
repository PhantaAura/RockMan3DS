# Original class40 later-wrapper coordinate tail

The later `8010D824` wrapper copies actor position halfwords to player previous
position, invokes the state/pose prefix, appends a shadow through `8002C8CC`, runs
timed material cues through `8010C820`, then copies actor position halfwords to
player primary position. The shadow helper is a queue producer. It does not
perform actor contact response.

`admit_source_scene_actor40_final_mirrors_8010d824` admits a
`SourceBackedPartial` final position receipt after the source state/pose prefix.
Its receipt preserves the earlier previous halfwords and matches the current
actor primary halfwords and source tag. The function does not mutate work,
resources, queues, clocks, or player bytes and allocates no heap memory.
Shadow, timed cues, and material presentation remain explicitly pending.

## Why the position survives

The unchanged original cue handlers `8010C5D0` and `8010C644` set clock globals
and invoke `8002D404` and `8002D320`. `8010C6B8` advances those clocks and invokes
the same helpers at duration boundaries. `8002D404` computes a packed offset and
contains no stores. `8002D320` selects material part pairs 16/17 or 18/19 through
the descriptor word at +112. It writes two words per primitive at a 12-byte
stride. Those writes could corrupt actor coordinates if a destination aliased
the actor pool; an authored original-MIPS case demonstrates that corruption.

The native guard checks the descriptor, table, selected records, and all
nonempty source/destination primitive spans against the same resident immutable
EBD payload. It rejects unaligned pointers, incomplete spans, arithmetic
overflow, expired or changed resource ownership, and any EBD virtual interval
overlapping the complete original four-slot class40 bank
`[0x800C3580,0x800C4C10)`. Source tokens remain integer offsets into separately
allocated immutable payload storage; they never become native pointers.
Zero primitive counts skip their pointed spans, as the original skips those
loads/stores. The guard is deliberately conservative about complete 12-byte
records and the table header.

`8002C8CC` stores its output in the fixed shadow queue and increments its count.
Its optional floor query copies position to fixed scratch and passes caller
stack bounds to its classifiers. The complete store-opcode audit covers the
shadow, floor wrapper, candidate collector, bounds loader, cell lookup, seven
XZ classifier blocks, Y classifier, and material helpers. The floor/classifier
write destinations are caller stack or fixed scratch; actor position is read.
The floor maximum branch had one observed outcome in the bounded dynamic run;
its other outcome and unexecuted geometry branches are covered by the store
destination audit, not claimed as dynamically exercised geometry.

## Verification

The probe executes unchanged owner EXE and overlay instructions with no helper
hooks or instruction patches. Mutable overlay clock data are read through the
CPU memory dictionary; original code fetch remains unchanged. Its interpreter
instrumentation records addresses, branch outcomes, and stores.

- 48 shadow cases cover signed position limits, work76 zero/nonzero, queue
  counts 0/63/64/UINT32_MAX, and the original floor helper on its empty-candidate
  branch.
- 220 cue cases cover both original fourteen-entry cue tables, clock equality,
  positive/nonpositive duration, subsequent control records, and authored
  event phase/time matches, misses, and sentinel branches.
- 14 authored material cases cover both part pairs and primitive counts
  0/1/2/3/63/254/255. All 1,444 actor bytes remain unchanged.
- Seven original final-halfword-copy cases match native player CRCs and prove
  exact byte and knowledge-mask isolation.
- 16 selected tail branch sites execute both outcomes. The complete run
  executes 50,913 original instructions.
- 180 native guard cases cover missing work knowledge, actor identity, stale
  receipt, resource lifetime/directory changes, all primitive pointer lanes,
  alignment, truncation, aliasing and virtual-region overflow. Admission remains
  allocation-free. The actual owner EBD also passes the guard.

The public fixture contains authored scalar positions and CRCs only. Original
owner work snapshots, raw EBD/overlay bytes, and write/branch traces stay under
the private pass directory. The public proof contains hashes and derived facts.
REA's cached PS-X EXE adapter was unsupported in the earlier attempt; this work
does not claim a successful REA analysis.

This establishes the narrow final player-halfword receipt under the resident
resource guard. It does not establish retail scheduling, native UV animation,
shadow presentation, or hardware parity.

## MODLOG

2026-10-13: Confirmed `8002C8CC` shadow semantics; proved bounded original tail
actor invariance plus material non-alias guard. Added allocation-free final
mirror admission, unchanged-MIPS probe, authored fixture, and native rejection
tests. Shadow/material presentation remains pending.
