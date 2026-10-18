# Native diagnostic area transition ownership

The SELECT area switch in `platform/3ds/source/main.cpp` now admits the next
area and prepares its actors/render resources before committing the session's
area identity. Missing or malformed material sidecars, geometry admission
failure, decoded material admission failure, and allocation errors reject the
switch. The outgoing CPU session, collision map, actors, player state, and queued
events remain active when preparation fails. Inspection counters are restored.

The native path uses `transition_area_resources` from `area_package.h`. Host
tests exercise that same prepare/commit/restore ordering with synthetic packages
and a simulated partial render-resource allocation failure. These tests do not
execute Citro3D or establish physical Old3DS behavior.

Old and new GPU buffers/textures and decoded material caches are not retained
together. Once sidecars pass preflight, the old GPU/cache set is released and
the target set is built. If that build or the session's fallible stage-work copy
fails, the outgoing GPU/cache set is rebuilt from its owner-local sidecars.
Partial target ownership is released during rebuild. The outgoing actor set
remains alive until the target renderer and session commit both succeed.
Actor mesh admission is 512 KiB per area, so two such bounded CPU mesh sets may
temporarily coexist during preparation; poses, diagnostics, and allocator
overhead are additional allocations.

This is conditional restoration, not preservation of the same GPU allocation.
If the outgoing SD files become unreadable or allocations also fail during
restoration, all partial render/cache resources are released and the existing
fatal error screen reports restoration failure. Execution does not resume with
a mixture of areas.

Accepted session transitions copy stage/work before changing identity, then
move that copy without allocation. They clear outgoing targets, projectiles,
lock state, auxiliary triggers, dialogue glyphs and pending audio/script
requests, cutscene identity, and script camera transition state. The read-only
registered-area lookup used to stage actors does not change active collision
ownership or consume events. Invalid registration indices are rejected before
adding a tutorial registration.

The material preflight validates the existing cooker's canonical L3MC layout,
CRC, sorted records, TPAGE depth, page/palette dimensions, and palette indices.
Shared/overlapping index or palette payloads are rejected before decoding,
preventing a small file from expanding repeated slices into many CPU/GPU pages.
Each sidecar input is bounded at 4 MiB. Terrain geometry buffers are admitted at
4 MiB. Combined decoded terrain/player material caches are admitted at 8 MiB;
combined terrain/player uploaded texture pages are admitted at 8 MiB. Uploads
also enforce the texture cap if an SD file changes after admission. These are
candidate policy limits, not measured physical Old3DS available-memory budgets.
Package/map/render buffers, actor data, temporary decode/input buffers,
framebuffers, shader state, and allocator overhead remain additional residency.

The supplied owner-local tutorial caches contain at most 12 terrain pages and
2 player pages in one area: 14 RGBA pages total, or 3,670,016 texture bytes.
Their host admission is checked separately; no cooked sidecars or source game
assets are included in this source change.

Native package loading still occurs only at boot. The map/render data, negative
material bindings, and registered runtime/EBD areas remain resident through the
diagnostic SELECT switches. Malformed optional boot packages can fall back to
the existing trusted loose-data boot path. This change does not implement
runtime package replacement, lazy stage/EBD streaming, door traversal, new
event handlers, or wider retail actor behavior. The separate 0143 renderer
fallback remains available.

## Optional ST00 conditional spawn binding

Optional MML2 section 34 contains the bounded `L3SP` v1 source catalog. Packages
without that section retain their existing behavior. Catalogs require exactly
five local map areas. If runtime bindings are supplied, their local/source area
selectors must match: remapping source area 3 into local area 1 is rejected
instead of attaching area 1's conditional records to it. The catalog and each
per-runtime-area copy are admitted against decoded/peak policy before loading.
Malformed domains, counts, ranges, CRCs, duplicate sections, and budget failures
reject the complete load and leave the caller's outgoing package intact.

`package_cooked_area.py --spawns existing.l3spawn` validates and copies an
explicitly supplied cooked catalog. `build_hardware_test.py` accepts the optional
`st00.l3spawn` file through the verified cooked-pack manifest and passes
`--spawns` only when that manifest declares it. An undeclared file cannot opt
itself into the package. The builder rechecks pack hashes after compilation
before wrapping any payload. `.l3spawn` and renamed binary data remain excluded
from source releases. The source importer/cooker, controller semantics, and
native session consumption are separate from this package ownership layer.
