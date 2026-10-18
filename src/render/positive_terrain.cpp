#include "legends3ds/render/positive_terrain.h"
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
int32_t checked_coordinate(int64_t value) {
    if (value < std::numeric_limits<int32_t>::min() ||
        value > std::numeric_limits<int32_t>::max())
        throw std::runtime_error("positive terrain coordinate overflows");
    return static_cast<int32_t>(value);
}
} // namespace

TerrainPositiveLocalXYZ positive_terrain_consecutive_xyz(const SourceRenderPrimitive& p) {
    // Invert the legacy cooked gather, retaining vertex order b0..b11:
    // XYZ0=(b0,b1,b2), XYZ1=(b3,b4,b5), XYZ2=(b6,b7,b8), XYZ3=(b9,b10,b11).
    // Conversion to uint8_t recovers the byte value, including Y >= 128.
    return {{p.local_x[0], p.local_x[1], p.local_z[1], p.ignored_raw[3]},
            {static_cast<uint8_t>(p.local_z[0]), static_cast<uint8_t>(p.ignored_raw[1]),
             static_cast<uint8_t>(p.ignored_raw[2]), static_cast<uint8_t>(p.local_x[3])},
            {p.ignored_raw[0], p.local_x[2], p.local_z[2], p.local_z[3]}};
}

ExpandedRenderPrimitive expand_positive_terrain_primitive(const SourceRenderPrimitive& p,
    uint32_t material_word, const St00CellOrigin& origin, uint8_t effective_shift) {
    if (effective_shift > 15u)
        throw std::runtime_error("positive terrain coordinate shift invalid");
    const auto local = positive_terrain_consecutive_xyz(p);
    const int64_t scale = int64_t{1} << effective_shift;
    ExpandedRenderPrimitive out;
    out.prefix = p.prefix;
    out.material_word = material_word;
    out.uv = p.uv;
    for (size_t v = 0; v < 4u; ++v) {
        // Multiplication avoids undefined left shifts of negative signed values.
        out.world_x[v] = checked_coordinate(int64_t(origin.x) + int64_t(local.x[v]) * scale);
        out.world_y[v] = checked_coordinate(int64_t(origin.y) - int64_t(local.y[v]) * scale);
        out.world_z[v] = checked_coordinate(int64_t(origin.z) + int64_t(local.z[v]) * scale);
    }
    return out;
}

std::vector<ExpandedRenderPrimitive> expand_positive_terrain_descriptor_lod(
    const St00RenderSource& source, size_t descriptor_index, St00RenderBank bank,
    St00Lod lod, const St00CellOrigin& origin, uint8_t global_scale_shift) {
    if (descriptor_index >= source.descriptors.size())
        throw std::runtime_error("positive terrain descriptor out of range");
    if (static_cast<uint8_t>(bank) > 1u || static_cast<uint8_t>(lod) > 2u)
        throw std::runtime_error("positive terrain bank or LOD invalid");
    if (global_scale_shift > 15u)
        throw std::runtime_error("positive terrain global coordinate shift invalid");
    const size_t slot = st00_descriptor_slot(bank, lod);
    const uint16_t ri = source.descriptors[descriptor_index].record_indices[slot];
    if (ri == 0xFFFFu) return {};
    if (ri >= source.records.size())
        throw std::runtime_error("positive terrain record out of range");
    const int64_t origin_scale = int64_t{1} << global_scale_shift;
    const St00CellOrigin scaled_origin{
        checked_coordinate(int64_t(origin.x) * origin_scale),
        checked_coordinate(int64_t(origin.y) * origin_scale),
        checked_coordinate(int64_t(origin.z) * origin_scale)};
    const auto& record = source.records[ri];
    std::vector<ExpandedRenderPrimitive> out;
    for (uint32_t si : record.stream_indices) {
        if (si >= source.streams.size())
            throw std::runtime_error("positive terrain stream out of range");
        const auto& stream = source.streams[si];
        if (stream.material_slot >= record.materials.size())
            throw std::runtime_error("positive terrain material slot out of range");
        const uint16_t shift = uint16_t(stream.base_shift) + global_scale_shift;
        if (shift > 15u)
            throw std::runtime_error("positive terrain coordinate shift invalid");
        for (const auto& primitive : stream.primitives)
            out.push_back(expand_positive_terrain_primitive(primitive,
                record.materials[stream.material_slot], scaled_origin,
                static_cast<uint8_t>(shift)));
    }
    return out;
}

} // namespace l3
