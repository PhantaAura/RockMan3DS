# Source stats-selector lifecycle admission

Status: **SOURCE_BACKED_PARTIAL**. The fresh source startup initializer sets
raw selector800C1B6B to1. This is a recovered lifecycle operation, not an
assumption that all saves, areas, versions or reference states use1.

The established USA owner EXE/overlay hashes are verified by the reproducer.
Only selected call edges, scalar outcomes and code-range hashes are emitted.
No combat record, instruction listing, original function table, save payload
or asset is packaged.

## Fresh source boot chain

1. EXE entry80068000 calls main80011C90 at800680A0.
2. Original main prefix constructs callback800131FC and passes it as argument
   A1 to80012E10 at80011CD0. Two authored initial-helper cases execute this
   construction; preceding helper side effects are declared return hooks.
3. Helper80012E10 passes that same callback as A0 to8007FF10. Its original
   trampoline targets B0 function0E, the OpenThread service in the official
   [OpenBIOS handler table](https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/kernel/handlers.c).
   Actual BIOS scheduling, thread stack and GP ownership are excluded.
4. Original callback800131FC clears dispatch state bytes80098B1C/1D and selects
   index0 from table80080894. JALR at80013240 enters8001326C.
5. Original initializer8001326C performs its setup sequence, then the
   four-instruction block8001332C..8001333C writes byte1 at800C1B6B. The
   initializer advances dispatch state80098B1C to1. The probe stops before
   the following BIOS/pad update, leaving later menu/gameplay behavior outside
   this proof.

The narrow writer executes without hooks for every previous selector byte
0..255 and varying incoming V0. Every result is1. Five whole loop→initializer
cases execute authentic dispatch and writer instructions with declared
setup/graphics/subsystem hooks; these are **partial source control-flow
proofs**, not a booted-game trace or implementation of those helpers.

## Snapshot copy and selected table

Original block80065B18 writes800C1B6B at80065DE4 using a byte read from
800B5F4F. All256 authored source values are copied unchanged without helper
hooks. The caller/lifetime of that block and the persistent field meaning are
not proven here; call it `byte800B5F4F`, not an invented save/difficulty name.
It demonstrates why restoring unknown snapshot state must invalidate a
fresh-only selector claim.

Original consumer80064718 selects one stats-table address for raw selector2
and another for every other byte value. All256 original consumer cases match
that condition. This evidence does not embed either table's bytes or prove
all object/context/profile lookups. A native loader must use the chosen
owner-derived combat record and retain its actual field0A, rather than
substituting the first record of the selected table.

## Native contract

A source-owned initialization profile may explicitly record
`raw_stats_selector=1` when the native session performs the equivalent
**fresh source boot initialization**. Such a profile should carry the
initialization operation/provenance, and be distinct from unknown or loaded
persistent state. Area loading alone does not set this global.

Restore/import of unknown game state invalidates that known value until the
actual selector is recovered or supplied through a source-backed state
adapter. Do not set1 each frame, respawn, area transition or reload. Native
combat-profile admission then requires the supported raw object index,
context high byte and a correctly selected owner-derived record. The selector
proof closes one prerequisite; it does not automatically validate all combat
tables, hit ownership or enemy behavior.

The current reference process was unavailable during this bounded audit.
Historical tutorial snapshots with selector1 are consistent with the source
initializer, but they do not prove later writers, loaded saves or a BIOS RNG
seed. No original user save was read or changed by this task.

## Reproduction

```sh
python3 -B work/Legends3DS_0.0.14.19_WorldComesAlive/docs/research_01419/stats_selector_lifecycle_source_probe.py \
  --exe owner-data/extracted/ROCK_NEO.EXE \
  --dat owner-data/extracted/CDDATA/DAT \
  --out work/pass-20261006-worldalive-01419/rng/stats-selector-source-evidence.json
```

The output is exclusive and the input hashes are checked. Results include
256 narrow fresh writes,256 snapshot copies,256 table-consumer cases,5
dispatch cases,2 main-prefix cases and the original registration trampoline.
The successful run executed **54,103** probe instructions. Evidence SHA-256:
`52226986230aa19c6ae72400184268a18897920ab0e20574eb92ca55ef683143`.
This remains **SOURCE_BACKED_PARTIAL**; no retail/native parity, complete
startup port or hardware validation follows from the scalar differential.
