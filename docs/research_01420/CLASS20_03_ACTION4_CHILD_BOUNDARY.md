# Next action4 and child boundary

Status: **SOURCE_BACKED_PARTIAL**, research only. No action4 or child runtime
was added by this investigation. The evidence identifies explicit source
prerequisites for continuing after action9 selects action4.

The action4 entry is `80102FC0`, ending before `8010324C`. Phase0 sets request21,
phase1 and timer13, clears work330 and emits positional cue177 before motion.
It falls through phase1's decrement, giving timer12 in that initial update.
While phase1's signed timer remains nonnegative, source8002FEA4 receives
forward−112, vertical0 and strafe0. When the timer becomes negative, phase2
starts with16 for variant32E zero or0 for nonzero; phase2 decrements in that
same update. Phase2 at timer<=0 sets phase3, increments the current animation
request, sets timer11 and calls child helper `80104078`.

Phase3 decrements the signed timer. Nonnegative values feed forward
`−timer*32` into the source motion/wall/floor family. Negative expiry sets
phase4/timer6 and immediately enters phase4's decrement to5 and forward224
motion. Phase4's negative expiry sets common-transition flag01 and installs
work35C from unsigned combat-record byteC through the selected source statistics
table and variant32E. That byteC has not yet been added to the runtime metadata
contract. Floor changes of33 or more copy all four prior raw/fixed lanes back;
they do not independently change action4 to another action.

The child helper calls original allocator `80031518`. Its pool has32 slots at
`8009CF88`, stride10C and bitmap `800987CC`; allocation picks the first clear
bit from80000000 through00000001. It initializes defined scalar slots and
sets byte6=3. Full allocation returns zero and the helper writes zero into
parent work374, emits no child cue and performs no child writes. Runtime
capacity, slot cleanup and lifecycle must implement this contract together.
A resource pointer or allocation readiness flag alone is insufficient.

After a successful allocation, helper80104078 sets child byte0=1. Source
variant32E zero selects child subtype6/parameter03=0, fixedY equal to parentY
minus60*65536, work9C=32 and workA0=16, and emits cue178 using the current parent
raw14..1B. Nonzero variant selects subtype9/parameter03=3 and fixedY equal to
parentY minus80*65536. Both routes copy parent fixedX/Z and unsigned heading56
to child work64, convert fixed coordinates to raw mirrors and invoke original
`8002FFC4` with forward1760, vertical0 and strafe0.

The source pool updater `80017210` tests byte0 & byte6 &1 and selects resident
callbacks from table `80089820` for subtypes below128. Slot6 points to
`8001FBC4`; slot9 points to `8001F820`. These are useful next trace anchors.
This report does not label the child subtype as a fully implemented projectile
or infer damage, hit delivery, resource ownership, lifetime or rendering.
Those consumers remain UNKNOWN.

The read-only proof executes392 authored action4 body cases and144 authored
child initializer/allocator cases, for57,907 original instructions. Action4
body motion/wall/floor and child invocation are declared authored boundaries;
the child cases execute the actual allocator, scalar stores, original cue178
writer, fixed conversion and8002FFC4 accumulator/rounding. The GTE delta and
pool/parent state are authored premises. Original owner files remain unchanged.
The source-safe output contains authored scalar states, code addresses and
hashes. No instruction listings or retail payloads are copied into source.

```sh
python3 -B docs/research_01420/class20_03_action4_child_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/owner-local/action4-child-proof
```

Evidence: `docs/validation_01420/action4-child-source-evidence.json`.
The next admitted implementation requires the byteC statistics owner, complete
32-slot pool allocation/release, distinct negative/positive motion preparations,
source audio intents and separately proved subtype6/9 lifecycle/dispatch.
The callback/common-tail shared RNG seed and consumer cadence remain independent
requirements; this investigation does not choose a seed or supply random samples.
