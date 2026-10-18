#include "legends3ds/bottom_map.h"
#include <algorithm>

namespace l3 {

BottomMapModel build_bottom_map_model(const RuntimeMap& map,
                                      size_t area_index,
                                      int32_t player_world_x,
                                      int32_t player_world_z) {
    BottomMapModel out;
    out.area_index = area_index;
    if (area_index >= map.areas.size()) return out;

    const auto& area = map.areas[area_index];
    out.width = area.width;
    out.height = area.height;
    const size_t cell_count = size_t(area.width) * size_t(area.height);
    out.cells.assign(cell_count, BottomMapCellKind::Empty);

    for (size_t local = 0; local < cell_count; ++local) {
        const size_t ci = size_t(area.cell_first) + local;
        if (ci >= map.cells.size()) break;
        const int16_t packed = map.cells[ci];
        out.cells[local] = packed > 0 ? BottomMapCellKind::Geometry
                                     : (packed < 0 ? BottomMapCellKind::Special
                                                   : BottomMapCellKind::Empty);
    }

    const auto player = runtime_map_find_cell(map, area_index, player_world_x, player_world_z);
    if (player) {
        const int32_t lx = int32_t(player->grid_x) - int32_t(area.start_x);
        const int32_t lz = int32_t(player->grid_z) - int32_t(area.start_z);
        if (lx >= 0 && lz >= 0 && lx < area.width && lz < area.height) {
            out.player_in_area = true;
            out.player_x = static_cast<uint16_t>(lx);
            out.player_z = static_cast<uint16_t>(lz);
        }
    }
    return out;
}

BottomMapCellKind sample_bottom_map_cell(const BottomMapModel& model,
                                         uint16_t sample_x,
                                         uint16_t sample_z,
                                         uint16_t sample_width,
                                         uint16_t sample_height) {
    if (!model.width || !model.height || !sample_width || !sample_height || model.cells.empty()) {
        return BottomMapCellKind::Empty;
    }
    const uint32_t mx = std::min<uint32_t>(model.width - 1u,
        (uint32_t(sample_x) * uint32_t(model.width)) / uint32_t(sample_width));
    const uint32_t mz = std::min<uint32_t>(model.height - 1u,
        (uint32_t(sample_z) * uint32_t(model.height)) / uint32_t(sample_height));
    const size_t i = size_t(mz) * model.width + mx;
    return i < model.cells.size() ? model.cells[i] : BottomMapCellKind::Empty;
}

} // namespace l3
