# Source Buster scalar producer and native lease owner

Status: **SOURCE_BACKED_PARTIAL**. The original producer `80035724`, allocator
`80031598`, cold reset `8002E7F4..8002E83C`, and lease `8003545C/800354E0`
execute in the private original-instruction probe with no helper hooks or
instruction replacements. This is not a live retail firing trace or complete
Buster implementation.

`ChildRuntime::spawn_source_buster_80035724` now owns the actual separate
16-record source pool (`800BFB20`, stride `10C`, bitmap `800989F0`). Cold reset
clears each record, sets byte01 to `16|index`, and clears allocation/deferred
release masks. Reuse resets only the allocator's proven fields. Existing actor
children retain their independent family60 pool.

The producer consumes an explicit admitted player lane: signed player170 or
171, matching owner table byte at `80089A8B + 12*selection`, matching signed
counter at player`4FA + 8*selection`, pose muzzle at player`3C8` or `3E0`,
player01 and heading56. It writes the original subtype, active bit, lease index,
raw/fixed XYZ, stack10 low16, class and heading fields. Counter32767 and selected
rows4/7/12/13 suppress decrement; other counters wrap as original signed16.

For nonnegative selected proxy indices, the target comes from the real native
registered proxy bank. Its original identity is checked against its owned
target. Lease allocation updates the caller-owned source mask and target
flags, and binds the Buster's source identity to its actual owned BC/C0 words.
The existing original GTE lease consumer updates/clears those words in the
Buster pool. Mask exhaustion stores lease indexFF and leaves flag03 zero.

Missing player/table/target/mask premises hold all mutations atomically. Pool
exhaustion returns zero before reading these premises, as the original does.
The producer allocates no heap memory; its retained pool adds4288 bytes to the
native owner. Genuine ARMv6K compilation reports392 bytes static stack for the
spawn function,40 for the lease adapter,16 for cold reset and32 for BC/C0
publication. These are compile-time observations, not measured device memory.

Validation:

- `source_hit_proxy`:758 original allocation/release differentials, including
  full/low-byte masks, each24-slot hole and source low5 shift release behavior.
- `child_runtime`:258 original producer differentials compare all268 bytes of
  the final allocated record, allocator/lease masks, target flags and selected
  counter. They include both muzzle lanes, stack10 high bits, signed row
  selections, signed counter wrap, full lease masks,16-slot capacity and
  exhaustion. The original probe executes927450 instructions.
- The owner test exercises actual Buster allocation → target lease → owned
  BC/C0 → original GTE consumer, and asserts no hot-path heap allocations.
- Targeted host tests passed2/2 with warnings as errors. Both modified modules
  also compile with genuine devkitARM ARMv6K warnings as errors.

Commands (from the project workspace):

```sh
cmake --build work/pass-20261013-worldalive-01436/host-encounter --target test_child_runtime test_source_hit_proxy -j2
ctest --test-dir work/pass-20261013-worldalive-01436/host-encounter -R '^(source_hit_proxy|child_runtime)$' --output-on-failure
python3 work/Legends3DS_0.0.14.36_WorldComesAlive/docs/research_01436/source_buster_spawn_probe.py --exe owner-data/extracted/ROCK_NEO.EXE --out NEW_PRIVATE_OUTPUT
```

The native gameplay firing caller still needs admitted original player pose
muzzle outputs, table rows, counter ownership and source firing/selection
chronology. The imported subtype's callback/resource/movement/contact and
retirement are not executed by this producer. This change does not borrow the
desktop projectile, invent damage, or establish Buster→enemy defeat. Retail
tables, muzzle states and original instruction listings remain private; source
fixtures contain only authored scalar premises and resulting scalar records.

Machine-readable evidence: `source-hit-lease-proof.json` and
`source-buster-spawn-proof.json`.
