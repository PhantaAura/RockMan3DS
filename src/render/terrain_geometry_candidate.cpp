#include "legends3ds/render/terrain_geometry_candidate.h"
#include "legends3ds/tile_transform.h"
#include <limits>
#include <stdexcept>

namespace l3 {

TerrainCandidateLocalXYZ terrain_candidate_consecutive_xyz(const SourceRenderPrimitive& p) {
    // Invert the existing L3RG storage gather to recover b0..b11, then consume
    // (b0,b1,b2), (b3,b4,b5), (b6,b7,b8), (b9,b10,b11). UV slots remain paired
    // with their original vertex indices; no hull sort or vertex reassignment.
    return {{p.local_x[0], p.local_x[1], p.local_z[1], p.ignored_raw[3]},
            {p.local_z[0], p.ignored_raw[1], p.ignored_raw[2], p.local_x[3]},
            {p.ignored_raw[0], p.local_x[2], p.local_z[2], p.local_z[3]}};
}

ExpandedRenderPrimitive expand_terrain_candidate_primitive(const SourceRenderPrimitive& p,
    uint32_t material_word, const St00CellOrigin& origin, uint16_t packed_cell,
    uint8_t effective_shift) {
    if (effective_shift > 15u) throw std::runtime_error("candidate terrain coordinate shift invalid");
    const auto local = terrain_candidate_consecutive_xyz(p);
    ExpandedRenderPrimitive out;
    out.prefix = p.prefix;
    out.material_word = material_word;
    out.uv = p.uv;
    const int64_t scale = int64_t{1} << effective_shift;
    for (size_t v = 0; v < 4; ++v) {
        // Keep the existing proved tile quarter-turn convention and signed
        // 16-bit X/Z origin convention. Y is an explicitly candidate recovery.
        const auto xz = legends_tile_transform_xz(static_cast<int16_t>(origin.x),
            static_cast<int16_t>(origin.z), local.x[v], local.z[v], effective_shift,
            packed_cell);
        const int64_t y = int64_t(origin.y) + int64_t(local.y[v]) * scale;
        if (y < std::numeric_limits<int32_t>::min() || y > std::numeric_limits<int32_t>::max())
            throw std::runtime_error("candidate terrain Y coordinate overflows");
        out.world_x[v] = xz.x;
        out.world_y[v] = static_cast<int32_t>(y);
        out.world_z[v] = xz.z;
    }
    return out;
}

std::vector<ExpandedRenderPrimitive> expand_terrain_candidate_descriptor_lod(
    const St00RenderSource& source, size_t descriptor_index, St00RenderBank bank,
    St00Lod lod, const St00CellOrigin& origin, uint16_t packed_cell,
    uint8_t global_scale_shift) {
    if (descriptor_index >= source.descriptors.size())
        throw std::runtime_error("candidate terrain descriptor out of range");
    if (static_cast<uint8_t>(bank) > 1u || static_cast<uint8_t>(lod) > 2u)
        throw std::runtime_error("candidate terrain bank or LOD invalid");
    const size_t slot = st00_descriptor_slot(bank, lod);
    if (slot >= 6u) throw std::runtime_error("candidate terrain descriptor slot out of range");
    const uint16_t ri = source.descriptors[descriptor_index].record_indices[slot];
    if (ri == 0xFFFFu) return {};
    if (ri >= source.records.size()) throw std::runtime_error("candidate terrain record out of range");
    const auto& record = source.records[ri];
    std::vector<ExpandedRenderPrimitive> out;
    for (uint32_t si : record.stream_indices) {
        if (si >= source.streams.size()) throw std::runtime_error("candidate terrain stream out of range");
        const auto& stream = source.streams[si];
        if (stream.material_slot >= record.materials.size())
            throw std::runtime_error("candidate terrain material slot out of range");
        const uint16_t shift = uint16_t(stream.base_shift) + global_scale_shift;
        if (shift > 15u) throw std::runtime_error("candidate terrain coordinate shift invalid");
        for (const auto& primitive : stream.primitives)
            out.push_back(expand_terrain_candidate_primitive(primitive,
                record.materials[stream.material_slot], origin, packed_cell,
                static_cast<uint8_t>(shift)));
    }
    return out;
}

} // namespace l3
