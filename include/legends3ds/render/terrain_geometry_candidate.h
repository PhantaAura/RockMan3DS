#pragma once
#include "legends3ds/st00_render.h"
#include <array>

namespace l3 {

// CANDIDATE: the cooked format retains all twelve compressed bytes, but the
// inherited gather cross-mixes consecutive triples and discards four bytes.
// This reconstruction is supported by measured cooked geometry, not a proven
// executable trace. The baseline expansion stays available independently.
struct TerrainCandidateLocalXYZ {
    std::array<int8_t, 4> x{};
    std::array<int8_t, 4> y{};
    std::array<int8_t, 4> z{};
};

constexpr std::array<uint8_t, 6> kTerrainCandidateStripIndices{{0, 1, 2, 1, 3, 2}};

TerrainCandidateLocalXYZ terrain_candidate_consecutive_xyz(const SourceRenderPrimitive& primitive);

ExpandedRenderPrimitive expand_terrain_candidate_primitive(const SourceRenderPrimitive& primitive,
    uint32_t material_word, const St00CellOrigin& origin, uint16_t packed_cell,
    uint8_t effective_shift);

std::vector<ExpandedRenderPrimitive> expand_terrain_candidate_descriptor_lod(
    const St00RenderSource& source, size_t descriptor_index, St00RenderBank bank,
    St00Lod lod, const St00CellOrigin& origin, uint16_t packed_cell,
    uint8_t global_scale_shift = 0);

} // namespace l3
