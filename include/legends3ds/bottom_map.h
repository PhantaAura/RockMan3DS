#pragma once
#include "legends3ds/l3map.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

enum class BottomMapCellKind : uint8_t {
    Empty = 0,
    Geometry = 1,
    Special = 2,
};

struct BottomMapModel {
    size_t area_index = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    std::vector<BottomMapCellKind> cells;
    bool player_in_area = false;
    uint16_t player_x = 0;
    uint16_t player_z = 0;
};

// Builds a compact player-facing minimap directly from the cooked L3MP grid.
// Positive cells are normal geometry, negative cells remain visibly distinct,
// and zero cells remain empty. This intentionally does not invent traversal,
// door, room-name, or secret semantics that have not been recovered yet.
BottomMapModel build_bottom_map_model(const RuntimeMap& map,
                                      size_t area_index,
                                      int32_t player_world_x,
                                      int32_t player_world_z);

// Samples the map into a fixed character/raster grid without changing the
// underlying area data. Useful for the first 3DS bottom-screen backend and
// host validation; later Citro2D rendering can consume the same model.
BottomMapCellKind sample_bottom_map_cell(const BottomMapModel& model,
                                         uint16_t sample_x,
                                         uint16_t sample_z,
                                         uint16_t sample_width,
                                         uint16_t sample_height);

} // namespace l3
