# Original phase5 class40 actor resource and first pose

Universal Modder reverse-engineering workflow. Status: SOURCE_BACKED_PREFIX.
Original instructions, EXE/overlay/EBD payloads, decoded retail geometry and
owner-derived initializer fixtures remain private. Hardware and complete scene
parity are not claimed.

The original phase5 record creates subtype5 in the four-slot class40 pool and
sets work04=1. ST00 class40 table8011A5B0 selects subtype5 table8011A4B0;
dispatch1 selects8010D824. Fresh work08 selects8010C930. The global class40
script route800153EC/80014AC0 is a different dispatch owner and must not be
substituted for this scene initializer. An initial investigation followed that
global route; the actual work04 store and overlay dispatch join corrected it
before acceptance and native integration.

8010C930 sets work00|2, calls8002F9C4 with a null variant, stores scale
344/346/348=512, request172=0, pointer120=0, and heading86=2213. It calls
800322A8 with request0/record0. The accepted native prefix stops at that call;
work08 remains0 and constructor position is preserved. It does not fabricate
completion of the following pose producer, wrapper/player stores, overlay
animation-table bindings, behavior updates or callback acknowledgements.

The original null-variant first match is EBD row0, class40/subtype5. The exact
immutable area0 payload is bound by the existing phase5 resource witness.
The decoded resource has18 source draw slots,307 vertices,146 triangles,
170 quads and2 source material words. Decoded owned mesh elements use10212
bytes, excluding allocator overhead. Source vertex/index/UV/material bytes
are retained exactly; no native playable-character mesh is substituted.
The shared parser now exposes explicit class40 mesh/pose entry points.
Class40's24 descriptor/matrix slots are separate from class20's13-part bound.
Source pointers are bounded individually; adjacent resource pointers do not
supply invented lengths. The18-part initial pose is request0/record0 from
800322A8 ->80033A28's actual frame table and128-byte packed consumer window.

The original common matrix producer80033598 selects class40's work+240 matrix
bank. Its ordinary scale branch reads signed halfwords344/346/348, shifts by
the opaque source scratch byte1F800074, narrows via stack halfwords and calls
8007ADE8. That helper multiplies each matrix column by the original scale,
keeps low32 bits and shifts arithmetically by12. Thus4096 is an observed
source divisor. Position uses the same scratch shift. The float presentation
bridge expresses that hierarchy in raw terrain units with scale512/4096;
its unrounded sine/matrix products remain CANDIDATE for integer/GTE parity.
Root position/translation, parent order, repeated root transforms, detached
parts and signed draw controls follow the original branches. Missing fields
or per-part scaling flag40 hold instead of introducing a fallback.

Work350 shading is retained by8010C930. The global route's4210 shading store
must not be borrowed. Original DPCS/fog/depth, material mutation, face ordering
and final hardware output remain separate consumers. Native textured drawing
using this float bridge must retain its presentation-candidate label.

Native actual-scene API:

- decode_source_scene_actor40_phase5_resources(profile,witness,active_ebd,cap)
- initialize_source_scene_actor40_prefix_8010c930(pool40_slot,resources,active_ebd)
- build_source_scene_actor40_pose_matrices(pool40_slot,resources)

Resources retain a weak immutable payload identity plus all14 active directory
rows/load base. Any expired/replaced payload or changed active metadata holds
the prefix atomically. No owner payload duplication or hot allocation occurs.
The outer adapter must independently prove the source pool token, occupancy
and original dispatcher lifecycle. A native prefix receipt must not increment
source work08 or act as a complete source callback acknowledgement.

Accepted evidence:24 actual initializer-prefix cases compare all1444 bytes
against5976 unchanged original instructions, zero instruction patches/hooks.
6144 expired-owner holds and explicit changed-directory/missing-field/wrong
route holds preserve work bytes/knowledge.28 authored global-script cases
compare1791 original instructions as independent prerequisite coverage;
those fixtures contain authored script words and retained state only.
18/24-part decoder/hierarchy, invalid counts, invalid parents/indices, memory
budgets and atomic script guards pass. Focused new implementation and decoder
compile with Wall/Wextra/Werror; focused ASan/UBSan checks are recorded in the
native validation evidence. Existing frozen core was linked read-only; no
frozen tree or synced sources were edited.

The optional global L3A4 cooker/profile contains the exact initial script prefix
from owner EXE800810C8 and is unrelated to the actual phase5 bootstrap. It is
owner-local research only and must not be required or distributed for the
actual phase5 scene actor path.
