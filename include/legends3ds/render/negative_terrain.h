#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/render/negative_materials.h"
#include "legends3ds/st00_render.h"
#include <cstddef>
#include <optional>
#include <vector>

namespace l3 {
struct AreaNegativeRenderInstance {
    uint32_t local_cell = 0;
    uint16_t packed_cell = 0;
    uint16_t descriptor_index = 0xFFFFu;
    St00CellOrigin origin{};
};
struct AreaNegativePlacement {
    std::vector<AreaNegativeRenderInstance> instances;
    uint32_t default_cells_unbound = 0;
    uint32_t special_cells_unbound = 0;
};
// Default and HD2-selected descriptors are explicit neutral identities, never
// inferred from a room number. Missing identities retain separate cell counts.
AreaNegativePlacement collect_area_negative_placement(const RuntimeMap& map,
    const St00RenderSource& render, size_t area_index, size_t max_instances = 65536);
// Counts every cell/stream occurrence without allocating geometry or another
// placement vector; malformed references and amplification above the cap fail.
uint64_t count_area_negative_mid_primitives(const St00RenderSource& render,
    const AreaNegativePlacement& placement, uint64_t max_primitives);
// Shared diagnostic/native admission for unique positive IDX placements and
// every bound negative cell. Missing identities contribute no geometry; a valid
// empty area returns zero. Malformed references and total cap excess throw.
uint64_t count_area_combined_mid_primitives(const RuntimeMap& map,
    const St00RenderSource& render, size_t area_index, uint64_t max_primitives);
// Search inclusively from start_area, wrapping at most once. Empty maps or maps
// without bound Mid primitives return nullopt. Invalid starts, malformed areas
// and cap excess throw. This selects inspection geometry, not retail progression.
std::optional<size_t> next_area_with_mid_terrain(const RuntimeMap& map,
    const St00RenderSource& render, size_t start_area, uint64_t max_primitives);
// Negative records use consecutive X/Z bytes, encoded flat height and the
// packed quarter turn (ROCK_NEO 0x80021C3C / 0x8002246C). These are distinct
// from positive unsigned-height XYZ records. Only the established Mid path is
// bound here; original near subdivision/UV behavior is not invented. HD2 proves
// special geometry identity, while its material state remains explicitly unknown.
std::vector<ExpandedRenderPrimitive> expand_negative_terrain_mid(
    const St00RenderSource& render, const AreaNegativeRenderInstance& instance,
    const NegativeMaterialBinding* binding);
} // namespace l3
