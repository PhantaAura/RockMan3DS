#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// Positive MDT values identify complete IDX records. The retail visibility pass
// marks each referenced identity once, then the positive draw pass walks the
// bitmap in ascending IDX order (0x80021914..58, 0x80024BAC).
// The position comes from that IDX row, not from any aliasing grid cell.
struct AreaPositiveRenderInstance {
    uint16_t idx_index = 0;
    uint16_t descriptor_index = 0xFFFFu;
    St00CellOrigin origin{};
    St00RenderBank bank = St00RenderBank::Secondary;
};
struct AreaPositivePlacement {
    std::vector<AreaPositiveRenderInstance> instances;
    uint32_t positive_occurrences = 0;
    uint32_t duplicate_occurrences = 0;
    uint32_t negative_cells = 0;
    uint32_t zero_cells = 0;
    size_t bitmap_bytes = 0;
};
// This area-local adapter admits all referenced positive records; visibility
// culling is separate. It runs during area resource construction, never as a
// per-frame allocation. Signed positive MDT identities are at most 0x7FFF,
// so the temporary bitmap never exceeds 4096 bytes. Unknown negative-cell
// geometry/material behavior is counted but not invented here.
// raw6 == 0 selects the secondary bank (+0x24..2C); nonzero selects primary
// (+0x18..20), matching the positive draw branch at 0x80024CB4..CC8.
// No positive-cell quarter turn is implied by the IDX identity or raw6.
AreaPositivePlacement collect_area_positive_placement(const RuntimeMap& map,
                                                     size_t area_index,
                                                     size_t max_instances = 0x7FFFu);
// Counts the selected bank/LOD streams before any expanded geometry allocation.
// A missing descriptor/LOD pointer contributes zero. Malformed indices and
// material/shift references throw; the caller controls the explicit budget.
uint64_t count_area_placed_render_primitives(const St00RenderSource& render,
                                            const AreaPositivePlacement& placement,
                                            St00Lod lod,
                                            uint64_t max_primitives);
} // namespace l3
