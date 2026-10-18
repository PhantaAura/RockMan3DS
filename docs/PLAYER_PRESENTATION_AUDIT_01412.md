# Player presentation audit, 0.0.14.12

The native player is the supplied retail five-component model, not a generated
or primitive stand-in. The owner-local cooked pack contains body, head, right
arm, left arm and feet models. The shared runtime reader joins 18 attachments
into 316 primitives: 116 triangles, 200 quads and 1,148 vertex references. Its
two material words have resolved cache entries in all five baseline areas.
This verifies the current binding structure; it does not establish retail GPU
appearance, palette timing or semi-transparency equivalence.

Fresh read-only checks against the supported ROCK_NEO.EXE confirm that bytes at
`0x80089928` begin with component counts `6,4,3,3,2` and the 18-byte table at
`0x80089938` is `0,8,9,10,12,13,1,1,1,1,2,3,4,5,6,7,11,14`.
The loaded L3PC attachments match this mapping. In particular, head part IDs
15, 16 and 17 share transform 1; they are not extra skeleton transforms.
See inherited `research/visual_evidence_004.md` for the player draw loop at
`0x8002786C` and its pointer table at `0x800898EC`.

The 15-part hierarchy and root/child translation construction are supported
by inherited executable analysis at `0x800331A0` in
`research/tutorial_runtime_006.md`. The present code uses decoded signed root
translation for part 0 and authored base translations for children. No
hierarchy or attachment mapping contradiction was found. The inherited
grounding audit measured the unscaled hierarchy, whose first regular pose
bounds are approximately `[-342.995,-1189.24,-224.857]` through
`[342.995,-0.476532,192.674]`.

Fresh source execution and a paused original gameplay measurement establish a
previously omitted final render transform. The source reduces posed offsets
around the actor by three bits, and scales its Q12 rotation matrices by the
512 scalar at GP+`0x498`. At the measured global shift 0, local geometry therefore
has scale 1/8 while the actor world position is preserved. The resulting float
pose height is about 148.60 source units, or 0.2902 native units at 1/512.
The UV renderer also used the opposite Y rotation sign. Original `RotMatrix`
and the shared pose rotation have `m02=+sinY`, `m20=-sinY`; the final root angle
is raw actor work+`0x56` minus raw work+`0x116`. The new shared
`build_player_actor_render_matrix` applies this sign and scale. Its raw extra
yaw term has no assigned gameplay role. Source coefficient truncation,
translation rounding and state-dependent scale are not fully reproduced;
the renderer remains SOURCE_BACKED_PARTIAL. No constant Y offset was added.
See `CAMERA_PLAYER_TRANSFORM_AUDIT_01412.md` for the source and live evidence.

`pose_player_visuals_into` now writes to reusable caller-owned storage, while
the value-returning function remains available. Material identities are
collected and sorted once from immutable local visuals with
`collect_player_visual_material_words`. Posing changes vertex coordinates and
cannot change these identities. The active native draw path can reuse both
without changing submission order, transforms, UVs, primitive colors, or the
animation mapping. The independent 0143 draw function stays intact. The scale
and facing corrections above are separate from this exact storage equivalence.

`tests/test_player_visual_storage.cpp` contains a frozen copy of the preceding
pose algorithm. With assertions enabled, it verifies exact float-coordinate
and metadata equivalence on 128 synthetic frames spanning every transform,
triangles and quads, and invalid-input/empty-input behavior. An allocation
counter around only the reused pose call verifies zero allocations and stable
storage after initial sizing. Its optional owner-pack argument checks all 761
regular decoded frames in 55 frame-table actions, the attachment mapping and
all ten required area/material bindings. The test uses existing owner data in
place and emits only aggregate evidence. It does not supply a PS1 golden pose.
Another 98 host cases verify the final actor matrix at zero, positive and
negative quarter turns, wraparound and raw extra-yaw subtraction. They compare
against the established shared Y rotation, scale local points by 1/8, preserve
actor world translation and leave pose capacity/metadata unchanged.
The host posed record size is 68 bytes; the old 316-record vector plus its
316-word material scratch requested about 22,752 bytes each frame. The new
path retains bounded startup storage instead. This is not a whole-engine
zero-allocation or physical Old 3DS timing claim.

Remaining blockers are explicit in `validation_01412/player-presentation-evidence.json`:

- Source scale and Y sign are established. Exact fixed-point rendering,
  dynamic scale, every extra-yaw state and complete pose/world composition
  still need a matched original/native pose comparison.
- Main-family S(request) to M(request+1) sequencing is structurally supported;
  the native loop advances only while movement byte +0x09 equals 1. Original
  idle cadence, arm-bank blending and complete state-dependent animation
  behavior remain UNKNOWN. This optimization leaves that selection intact.
- The native projectile drawing functions submit one untextured triangle per
  active projectile. The current projectile state has no model, material,
  effect frame or retail render-entry identity. `DKOUKA01.TIM` is retained as an
  indexed owner resource, but its name or presence does not prove it is the
  Buster effect. A source-backed effect loader/initializer/render packet chain
  is required before replacement. The current triangle remains a labeled
  CANDIDATE diagnostic proxy.
- Contact response is still absent from normal native movement; a correct idle
  model height cannot establish grounding, slopes or wall behavior. Native
  screenshot parity and physical Old 3DS acceptance remain pending.
- The original top-screen HP/Buster HUD is absent from the native top draw
  path. The lower screen uses the separate mockup-inspired runtime HUD. It
  does not establish or replace original top HUD artwork/render bindings.

No owner model, texture, animation or executable payload was copied into source
or distribution. The transform correction follows original source and live
scalar measurements; no model, UV, palette or animation-role guess was
introduced. Owner data and all previous source checkpoints were preserved.
