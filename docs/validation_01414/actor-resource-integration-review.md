# First imported actor resource review

The native decoder supports the source initializer at `0x80106974` for class
`0x20`, subtype `0x35`, parameter byte 3 equal to zero and dispatch byte 4 equal
to zero. This selector is independent of room number. Its null-variant EBD
lookup uses the first matching class/subtype row; the local area 2 case is row 3
and source records 18 and 19.

The near model has two draw parts, 52 logical vertices, four triangles and 46
quads. The two instances share one decoded geometry resource whose owned
element estimate is 1,600 bytes. Their raw headings are 0 and 2048 in the
4096-unit turn domain. The initializer's scale is 512 on every Q12 axis,
equivalent to 0.125. Apply this scale once; source world placement is unscaled.
The existing neutral PBD draw bridge expands the mesh to 288 submitted vertices.

The source copies 24 **words**, or 96 bytes, from descriptor slot 0 plus 0x10.
It uses 20-byte mesh descriptors, 12-byte triangle/quad packets, and stride-eight
vertices. Every logical packet/vertex range and the renderer's rounded groups of
three vertices is checked against the retained EBD payload. No directory pointer
is treated as a resource end. The decoder preserves source part byte 3 without
assigning gameplay meaning to it.

The packed-pose consumer copies 32 words, or 128 bytes. Its unpack block emits
75 consecutive 12-bit channels into 25 padded XYZ vectors, comprising signed
root translation and 24 angle vectors. The original arithmetic matches all 75
authored synthetic channels. The earlier 66-channel claim covered only a prefix
and is corrected. Pose requests 0/record 0 and 2/record 16 are source-valid static
diagnostic cases; their initial state choice and sequence advancement remain
unresolved. `pose_selected_by_retail_state` stays false.

The matrix bridge follows the source yaw, root rotation, parent rotation and
translated-parent order. Index-zero child transforms copy the root. Negative
parent slots and descendants requiring those transforms are disabled. A parent
hidden by draw control still supplies its initialized transform. Tests cover
multiple parents, quarter turns and the scale being applied once. Float matrices
do not claim PS1 integer/GTE rounding parity.

Both parts reference `0x3c80002b`, using CLUT `0x3c80` and TPAGE `0x2b`.
The source ORs TPAGE with `0x20`; this material is already set. Neither the
active area's 12 terrain materials nor its two player materials contain an exact
match. Imported texture residency remains UNKNOWN. Shade `0x4210` expands to
RGB 128/128/128 before GTE shading; flat neutral presentation uses that value,
while final GTE lighting remains unresolved.

The initializer calls the floor wrapper `0x80048E6C` before installing its body
bounds pointer at work+0x78. Fresh work therefore queries with six zero bounds
and zero Y delta, as the source null branch at `0x8004B3C0` dictates. The actor
floor API samples the same rectangle/negative-cell subset as the existing
contact runtime without running player movement. Actual actor positions each
gather three candidates and encounter one unsupported family/slope, so neither
receives a proved floor height. Their source Y can be shown as diagnostic
placement, with the floor status UNKNOWN.

`test_actor_resources` passes bounds, packet indices, 75-channel unpack,
hierarchy, memory-cap rejection, shared lifetime, strong pose failure and floor
subset checks. A 240-call matrix/floor loop allocates no memory. Decode and pose
binding are area-load operations; no sequence-tick parity is claimed. The
owner-local probe reports zero actors and zero decoded resource bytes after
unload. Evidence JSON contains only hashes, addresses, counts and scalar bounds;
retail mesh, pose, texture and executable bytes are not distributed.
