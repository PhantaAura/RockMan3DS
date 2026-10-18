#include "legends3ds/render/negative_terrain.h"
#include "legends3ds/render/area_placement.h"
#include "legends3ds/render/positive_terrain.h"
#include "legends3ds/tile_transform.h"
#include <stdexcept>

namespace l3 {
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int16_t wrap16(int32_t value) {
    const uint16_t bits = static_cast<uint16_t>(value);
    return static_cast<int16_t>(bits < 0x8000u ? int32_t(bits) : int32_t(bits) - 65536);
}
}

AreaNegativePlacement collect_area_negative_placement(const RuntimeMap& map,
    const St00RenderSource& render, size_t area_index, size_t max_instances) {
    require(area_index < map.areas.size(), "negative placement area outside map");
    const auto& area = map.areas[area_index];
    require(uint64_t(area.width) * area.height == area.cell_count,
            "negative placement dimensions mismatch");
    require(uint64_t(area.cell_first) + area.cell_count <= map.cells.size(),
            "negative placement cells outside map");
    require(render.default_negative_descriptor == 0xFFFFu ||
            render.default_negative_descriptor < render.descriptors.size(),
            "negative default descriptor outside render");
    for (uint16_t descriptor : render.special_negative_descriptors)
        require(descriptor == 0xFFFFu || descriptor < render.descriptors.size(),
                "negative special descriptor outside render");
    AreaNegativePlacement out;
    for (uint32_t local = 0; local < area.cell_count; ++local) {
        const int16_t cell = map.cells[size_t(area.cell_first) + local];
        if (cell >= 0) continue;
        const uint16_t packed = static_cast<uint16_t>(cell);
        const bool special = (packed & 0x80u) != 0;
        const uint16_t descriptor = special
            ? render.special_negative_descriptors[(packed >> 2) & 31u]
            : render.default_negative_descriptor;
        if (descriptor == 0xFFFFu) {
            if (special) ++out.special_cells_unbound;
            else ++out.default_cells_unbound;
            continue;
        }
        require(out.instances.size() < max_instances, "negative instance budget exceeded");
        const int32_t gx = int32_t(area.start_x) + int32_t(local % area.width);
        const int32_t gz = int32_t(area.start_z) + int32_t(local / area.width);
        require(gx >= 0 && gx <= 255 && gz >= 0 && gz <= 255,
                "negative grid outside retail byte domain");
        out.instances.push_back({local, packed, descriptor,
            {wrap16(gx * 512 - 0x7F00), -int32_t((packed & 0x1F00u) >> 1),
             wrap16(gz * 512 - 0x7F00)}});
    }
    return out;
}

uint64_t count_area_negative_mid_primitives(const St00RenderSource& render,
    const AreaNegativePlacement& placement, uint64_t max_primitives) {
    uint64_t count = 0;
    const auto slot = st00_descriptor_slot(St00RenderBank::Primary, St00Lod::Mid);
    for (const auto& instance : placement.instances) {
        require(instance.descriptor_index < render.descriptors.size(),
                "negative count descriptor outside render");
        const auto record_index = render.descriptors[instance.descriptor_index].record_indices[slot];
        if (record_index == 0xFFFFu) continue;
        require(record_index < render.records.size(), "negative count record outside render");
        const auto& record = render.records[record_index];
        for (uint32_t stream_index : record.stream_indices) {
            require(stream_index < render.streams.size(), "negative count stream outside render");
            const auto& stream = render.streams[stream_index];
            require(stream.material_slot < record.materials.size(), "negative count material slot invalid");
            require(stream.base_shift <= 15, "negative count shift invalid");
            require(stream.primitives.size() <= max_primitives - count,
                    "negative primitive budget exceeded");
            count += stream.primitives.size();
        }
    }
    return count;
}

uint64_t count_area_combined_mid_primitives(const RuntimeMap& map,
    const St00RenderSource& render, size_t area_index, uint64_t max_primitives) {
    const uint64_t positive = count_area_placed_render_primitives(render,
        collect_area_positive_placement(map, area_index), St00Lod::Mid, max_primitives);
    const uint64_t negative = count_area_negative_mid_primitives(render,
        collect_area_negative_placement(map, render, area_index), max_primitives - positive);
    return positive + negative;
}

std::optional<size_t> next_area_with_mid_terrain(const RuntimeMap& map,
    const St00RenderSource& render, size_t start_area, uint64_t max_primitives) {
    if (map.areas.empty()) return std::nullopt;
    require(start_area < map.areas.size(), "terrain search start outside map");
    size_t area_index = start_area;
    for (size_t visited = 0; visited < map.areas.size(); ++visited) {
        if (count_area_combined_mid_primitives(map, render, area_index, max_primitives))
            return area_index;
        area_index = area_index + 1 == map.areas.size() ? 0 : area_index + 1;
    }
    return std::nullopt;
}

std::vector<ExpandedRenderPrimitive> expand_negative_terrain_mid(
    const St00RenderSource& render, const AreaNegativeRenderInstance& instance,
    const NegativeMaterialBinding* binding) {
    require(instance.packed_cell & 0x8000u, "negative expansion requires a negative cell");
    require(instance.descriptor_index < render.descriptors.size(),
            "negative expansion descriptor outside render");
    const auto ri = render.descriptors[instance.descriptor_index]
        .record_indices[st00_descriptor_slot(St00RenderBank::Primary, St00Lod::Mid)];
    if (ri == 0xFFFFu) return {};
    require(ri < render.records.size(), "negative expansion record outside render");
    const auto& record = render.records[ri];
    std::vector<ExpandedRenderPrimitive> out;
    for (const uint32_t si : record.stream_indices) {
        require(si < render.streams.size(), "negative expansion stream outside render");
        const auto& stream = render.streams[si];
        require(stream.base_shift <= 15, "negative expansion shift invalid");
        require(stream.material_slot < record.materials.size(), "negative material slot invalid");
        for (const auto& source : stream.primitives) {
            const auto local = positive_terrain_consecutive_xyz(source);
            ExpandedRenderPrimitive primitive;
            primitive.prefix = source.prefix;
            // HD2 establishes geometry identity only. The default atlas table
            // must not lend its palette/UV state to the special branch.
            primitive.texture_binding_known = !(instance.packed_cell & 0x80u) && binding && binding->known;
            primitive.material_word = primitive.texture_binding_known ? binding->material_word : 0;
            primitive.uv = primitive.texture_binding_known ? negative_material_quad_uv(*binding) : source.uv;
            for (size_t v = 0; v < 4; ++v) {
                const auto xz = legends_tile_transform_xz(wrap16(instance.origin.x),
                    wrap16(instance.origin.z), local.x[v], local.z[v], stream.base_shift,
                    instance.packed_cell);
                primitive.world_x[v] = xz.x;
                primitive.world_y[v] = instance.origin.y;
                primitive.world_z[v] = xz.z;
            }
            out.push_back(primitive);
        }
    }
    return out;
}
} // namespace l3
