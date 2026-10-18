# Grounding and cell alignment audit, 0.0.14.8

The supplied cooked player model already places the idle feet at the flat map
height. The clear positional error found in this audit is a half-cell X/Z
offset in generic map lookup and the native diagnostic spawn. No artificial
player Y offset, floor response, physics constant, or animation adjustment is
justified by these observations. PHYSICS_1TO1 remains false.

## Evidence and correction

`src/world/l3map.cpp`, `runtime_map_collect_contact_candidates_8004b1e0`,
preserves the inherited translation of retail containing-cell selection:
`(signed_query + 0x8000) >> 9`. Its returned renderer/contact origin is
`grid * 0x200 - 0x7F00`. That is the cell center, 0x100 above the lower bound.
Before this pass, `runtime_map_find_cell` used a 0x7F00 lookup bias, incorrectly
assigning the first half of each actual containing cell to the preceding cell.
The first native positive-cell spawn then added another 0x100 to the center,
placing the actor on a seam rather than at the diagnostic tile center.

The corrected generic query follows the already translated retail containing
sample. The sum is widened before adding to safely reject extreme malformed
queries; the returned center uses multiplication rather than signed shifting.
The existing retail candidate gather now uses the same widened floor division
and center multiplication, preserving arithmetic-shift results while keeping
previous-cell samples below grid zero defined in portable C++.
The diagnostic native spawn must use that same center, with its existing IDX Y.
This is a diagnostic spawn correction, not reconstruction of retail New Game
placement or progression.

Existing cooked evidence corroborates the centered convention. The first cell
in area 0 has IDX origin (-768, 0, -2816); its selected contact group spans
-257 through +257 relative to the tile origin in both X and Z. The old native
spawn was (-512, 0, -2560), 0x100 beyond that center on each axis. Area 1 has the
same centered contact bounds. The bounds are evidence for X/Z alignment;
the associated y0/y1 contact fields are not assumed to mean floor height.

## Model and vertical position

The existing `build_player_pose_matrices` uses decoded signed root translation
and authored skeleton translations. Both native renderer paths add the posed
player coordinates to the RuntimeSession actor position, then use the same
`world_vertex` Y inversion used by terrain. There is no separate player Y
conversion or presentation offset in those paths.

An external host diagnostic read only the existing local cooked pack, reused
the unchanged 0.0.14.7 pose and legacy render helpers, and reported aggregate
bounds before the terrain candidate was integrated:

| Decoded pose action | Frames | Maximum feet Y range, relative to actor |
| --- | ---: | ---: |
| 1 | 31 | -0.476532 to 19.6633 |
| 13 | 32 | -35.3567 to 34.7815 |

Action 1 frame 0 has feet maximum Y -0.476532. Areas 0 and 1 spawn at actor Y 0
and all legacy-expanded primary/mid terrain vertices are at Y 0. At the
native scale 1/512, that idle foot gap is less than 0.001 renderer units. Action
numbers here describe decoded records and do not assign unproved gameplay
semantics. The listed animated ranges do not justify a corrective constant.

The vertical renderer conversion and near-zero idle gap make broken terrain
coverage the leading explanation for the floating appearance in these flat
areas. This inference still needs the current native screenshots to confirm.

## Remaining vertical collision gap

`RuntimeSession::tick` invokes motion, targeting/projectiles, dialogue and
camera updates. It does not invoke a floor or contact solver. The map probe
exposes contacts, and `apply_retail_contact_resolution` translates a supplied
solver result, but the native frontend does not supply one. Area 2 has multiple
decoded tile heights (-576, -384, -192, 0), so traversing height changes remains
unsupported. Contact primitive dispatch/response must be recovered and bound
before claiming ground adhesion, slope, ledge, or retail collision behavior.

The native frontend also constructs its view directly from the actor rather
than using RuntimeSession's script/gameplay camera state. This audit does not
establish retail camera constants and makes no camera behavior change.

## Regression checks

`tests/test_map_cell_alignment.cpp` checks both axes around tile edges and
centers, including the negative-to-positive world boundary at grid 64,
signed-halfword low/high edges, outside coordinates, malformed cell storage,
and INT32 extreme queries. Sixteen synthetic cells cover all packed
quarter-turn values and both default/alternate contact list selectors. Generic
cell/contact identity must match the first containing sample of the existing
retail candidate gather. This does not invent contact rotation semantics.
An additional synthetic negative-grid area verifies all four ordered preceding
neighbor samples at the low signed-halfword boundary, including centers below
grid zero. The combined alignment check passes address/undefined-behavior
sanitizers when compiled against the modified lookup/gather source.

The map-sharing fixture's one-cell synthetic locations now start at grid 64
because their existing player positions are small nonnegative coordinates.
All allocation identity, lifetime, save, and contact assertions remain intact.
The package-loader fixtures already use center (-256, -256), and the integrated
core fixtures cover two cells per axis; neither requires coordinate changes.

The new alignment and existing sharing checks passed when compiled with
warnings-as-errors against the modified lookup source and the unchanged
0.0.14.7 core components. The cumulative CMake test suite and native build must
be run after integration. No retail executable, raw disc file, or original
save data was opened or changed by this audit.
