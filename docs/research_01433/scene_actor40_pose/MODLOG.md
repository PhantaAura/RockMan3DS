# Class40 original integer first-pose banks — 0.0.14.33

Universal Modder reverse-engineering workflow. Status: SOURCE_BACKED_PARTIAL.
Owner code, actual table/pose bytes and retail work/bank fixtures remain private.
The frozen0.0.14.32 tree and archives were not edited.

The admitted scene path remains phase5 work04=1 ->ST00 class40 dispatcher
8010D824 ->fresh initializer8010C930. The preceding scalar prefix establishes
request0/record0, original heading2213 and scale512. The new consumer completes
the work-owned first-pose stores from800322A8 ->80033A28 ->8003346C ->80033598.
It does not acknowledge source08 or perform the remaining initializer globals,
player/wrapper stores, later animation requests or full scene progression.

The original root heading uses8007B5A8 CPU rotation, preserving signed-angle
lookup, low32 products, intermediate arithmetic shifts and signed16 stores.
Part composition uses8007A8A8 MVMVA(sf1,mx0,v0,cv3) saturated IR results.
8007ADE8 scales columns after the complete unscaled hierarchy has been built;
its low32 product/ASR12 and full32-bit final matrix-word store are preserved.
Positions use8007AD38 MAC outputs, signed16 narrowing and parent positions.
Children with translation/rotation index0 copy the root transform. Unsupported
retained/detached transforms and per-part scaling hold rather than creating
unproved transforms. Draw-control signs remain separate from transform writes.
All admitted three-term signed16 products fit the original44-bit MAC domain;
GTE timing and flags not read by these source instructions are outside the model.

The source scratch shift1F800074 is an explicit optional input. The known
projection setup branches8003697C/800369C0/80036A1C write0/1/2 respectively.
The producer admits these values only; it does not choose a branch from native
frame count, apparent model size, controller state or a camera approximation.
The existing4096-entry owner ReferenceTables supplies the original packed
sine/cosine values. No replacement sine implementation or table is added.

Exact first-pose root points are [80,-90,-544], [160,-180,-1088] and
[320,-360,-2176] for shifts0,1,2. Source matrix rounding differs between those
branches. The integer presentation adapter divides matrix elements by
4096*2^shift and positions by2^shift to express the already produced bank in
raw terrain units. Vertex projection, material shading/depth/fog, draw order,
visibility and physical hardware output remain separate verification targets.
A submitted18-part count still does not prove that the actor is visible.

The initial pose commit publishes original sequence148/152 tokens, the exact
first source record word164 and request172, followed by matrix-bank first20
bytes at+240+part*20hex and the three position halfwords at+180+part*8.
Bank trailing bytes and fourth position halfwords are preserved. Actual
8002E88C/800177CC cold pool clearing proves those initial retained bytes;
no fresh-zero assumption is applied to an arbitrary recycled slot.
The temporary yaw-matrix copy at work+160 also transfers12 stack-tail bytes
at174..17F. Their source stack owner is unavailable. The native commit marks
those copied lanes unknown while publishing only the proved first20 bytes.
It never substitutes zero or a prior pool value as a claimed original result.

All load-time mesh/pose pointers remain bounded by the existing decoder and
immutable EBD witness. Commit preflights weak payload identity, all14 active
source directory rows/load base, source resource tokens, subtype/dispatch,
first request/record, required known work lanes, original descriptor copy,
ReferenceTables and scratch shift before mutation. Missing premises preserve
both work bytes and knowledge. Producer/presentation/commit allocate nothing.

Stable APIs in source_scene_actor40_pose.h:

- produce_source_scene_actor40_integer_pose_8003346c(work,resources,refs,shift)
- commit_source_scene_actor40_initial_pose_800322a8(work,resources,ebd,refs,shift)
- present_source_scene_actor40_integer_pose(result)

Validation:3 actual first-pose cases plus6 signed-coordinate wrap cases compare
all18 original part banks and every known work byte. They execute65304 unchanged
CPU instructions and648 restricted MVMVA commands, with zero instruction patches
or helper hooks. The cold reset and exact phase5 constructor are executed before
each case. The162 actual bank comparisons and2322 owner/unknown holds pass.
36 independent authored cases cover signed/noncanonical lookup values, CPU
product overflow, IR saturation, negative/mixed scales and all admitted shifts;
48684 original instructions and432 MVMVA commands produce the public authored
fixture.252 dependency/hierarchy guards pass. Focused C++ compilation uses
Wall/Wextra/Werror; focused new-code ASan/UBSan passes. Independent probes link
the frozen01432 core read-only plus the new translation unit. The module independently compiles for devkitARM16.1.0 ARMv6k/VFP hard float
with O2/Wall/Wextra/Werror. Object text is5444 bytes with no data/BSS. Static
stack is1352 bytes for the producer,760 for commit and72 for presentation.
Root's fresh complete host/ARM/emulator results are reported separately.
