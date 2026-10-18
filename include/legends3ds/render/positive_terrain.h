#pragma once
#include "legends3ds/st00_render.h"
#include <array>

namespace l3 {

// SOURCE_BACKED_PARTIAL: the positive IDX renderer (0x80024BAC) dispatches
// primitive paths including 0x80023EA8/0x80024F30. Those paths consume four
// consecutive XYZ triples: signed X/Z, unsigned Y subtracted from IDX.y.
// L3RG retains all twelve bytes reversibly. Prefix effects remain raw/unknown.
struct TerrainPositiveLocalXYZ {
    std::array<int8_t, 4> x{};
    std::array<uint8_t, 4> y{};
    std::array<int8_t, 4> z{};
};

TerrainPositiveLocalXYZ positive_terrain_consecutive_xyz(const SourceRenderPrimitive& primitive);

// origin is already scaled when the caller supplies a nonzero global shift.
// Positive IDX records supply complete XYZ origins; packed cell low bits are
// not rotation controls on this path. Vertex/UV associations are unchanged.
ExpandedRenderPrimitive expand_positive_terrain_primitive(const SourceRenderPrimitive& primitive,
    uint32_t material_word, const St00CellOrigin& origin, uint8_t effective_shift);

// This entry point takes unscaled IDX XYZ. At 0x80023F50/0x80023F6C..80 the
// local shift is stream.base_shift + global_scale_shift and all three IDX
// origin coordinates are scaled by global_scale_shift. Both are checked here.
std::vector<ExpandedRenderPrimitive> expand_positive_terrain_descriptor_lod(
    const St00RenderSource& source, size_t descriptor_index, St00RenderBank bank,
    St00Lod lod, const St00CellOrigin& origin, uint8_t global_scale_shift = 0);

} // namespace l3
