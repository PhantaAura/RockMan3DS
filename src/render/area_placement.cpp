#include "legends3ds/render/area_placement.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
} // namespace

AreaPositivePlacement collect_area_positive_placement(const RuntimeMap& map,
                                                     size_t area_index,
                                                     size_t max_instances) {
    require(area_index < map.areas.size(), "placement area outside map");
    const auto& area = map.areas[area_index];
    require(uint64_t(area.width) * area.height == area.cell_count,
            "placement area dimensions mismatch");
    require(uint64_t(area.cell_first) + area.cell_count <= map.cells.size(),
            "placement area cells outside map");
    const size_t addressable_idx = std::min<size_t>(map.idx.size(), 0x8000u);
    std::vector<uint8_t> seen((addressable_idx + 7u) / 8u, 0);
    AreaPositivePlacement out;
    out.bitmap_bytes = seen.size();
    size_t unique = 0;
    for (uint32_t local = 0; local < area.cell_count; ++local) {
        const int16_t packed = map.cells[size_t(area.cell_first) + local];
        if (packed == 0) { ++out.zero_cells; continue; }
        if (packed < 0) { ++out.negative_cells; continue; }
        ++out.positive_occurrences;
        const size_t index = static_cast<uint16_t>(packed);
        require(index < addressable_idx, "placement positive IDX missing");
        const uint8_t bit = static_cast<uint8_t>(1u << (index & 7u));
        auto& word = seen[index / 8u];
        if (word & bit) { ++out.duplicate_occurrences; continue; }
        require(unique < max_instances, "placement positive instance budget exceeded");
        word = static_cast<uint8_t>(word | bit);
        ++unique;
    }
    out.instances.reserve(unique);
    for (size_t index = 1; index < addressable_idx; ++index) {
        if (!(seen[index / 8u] & (1u << (index & 7u)))) continue;
        const auto& idx = map.idx[index];
        require(idx.descriptor_index == 0xFFFFu || idx.descriptor_index < map.descriptors.size(),
                "placement descriptor outside map");
        out.instances.push_back({static_cast<uint16_t>(index), idx.descriptor_index,
                                 {idx.x, idx.y, idx.z},
                                 idx.raw6 == 0 ? St00RenderBank::Secondary : St00RenderBank::Primary});
    }
    return out;
}

uint64_t count_area_placed_render_primitives(const St00RenderSource& render,
                                            const AreaPositivePlacement& placement,
                                            St00Lod lod,
                                            uint64_t max_primitives) {
    require(static_cast<uint8_t>(lod) <= static_cast<uint8_t>(St00Lod::Far),
            "placement render LOD invalid");
    uint64_t count = 0;
    for (const auto& instance : placement.instances) {
        require(static_cast<uint8_t>(instance.bank) <= static_cast<uint8_t>(St00RenderBank::Secondary),
                "placement render bank invalid");
        if (instance.descriptor_index == 0xFFFFu) continue;
        require(instance.descriptor_index < render.descriptors.size(),
                "placement descriptor outside render");
        const auto slot = st00_descriptor_slot(instance.bank, lod);
        const auto record_index = render.descriptors[instance.descriptor_index].record_indices[slot];
        if (record_index == 0xFFFFu) continue;
        require(record_index < render.records.size(), "placement record outside render");
        const auto& record = render.records[record_index];
        for (const auto stream_index : record.stream_indices) {
            require(stream_index < render.streams.size(), "placement stream outside render");
            const auto& stream = render.streams[stream_index];
            require(stream.material_slot < record.materials.size(), "placement stream material missing");
            require(stream.base_shift <= 15, "placement stream shift exceeds expansion range");
            const uint64_t instances = stream.primitives.size();
            require(instances <= max_primitives - count, "placement primitive budget exceeded");
            count += instances;
        }
    }
    return count;
}
} // namespace l3
