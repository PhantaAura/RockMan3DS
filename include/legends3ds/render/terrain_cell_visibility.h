#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

// Per-cell terrain visibility of the original map renderer, called from
// 0x800238BC before the terrain families draw:
//  - 0x80020BB4 (scratch 0x1F800075 != 1): a search rectangle ahead of the
//    camera, a wedge walk over it (0x80021258 or 0x80021548 by yaw quadrant,
//    radius 400), then the near window around the camera target (radius 600);
//  - 0x80020A4C (0x1F800075 == 1): every cell of the area, radius 800.
// Each cell with a non-zero packed value goes through 0x80021834: its centre
// (y from IDX +2, or -((packed & 0x1F00) >> 1) for a negative cell) through
// gte_rt, kept when MAC3/4 < the far cut 0x1F800076 and |MAC1| is within the
// side cut. Positive cells set their IDX bit in list BC00/BD00 (one draw per
// IDX, so a positive cell is an alias of its IDX); negative cells are drawn
// per cell (0x8002196C).
struct TerrainCellViewInput {
    std::array<uint32_t,8> camera_1f800008{};       // RT rotation (gte_SetRotMatrix, 0x800238BC)
    std::array<int32_t,3> translation_1f800030{};   // TR (gte_SetTransVector, 0x80020BB4)
    int16_t eye_x_1f800028 = 0, eye_z_1f80002c = 0;  // narrowed camera eye
    int16_t pitch_800a3a10 = 0, yaw_800a3a12 = 0;    // (-pitch) & 0xFFF, yaw & 0xFFF
    int32_t target_x_800a39a0 = 0, target_z_800a39a8 = 0;
    uint8_t lod_1f800074 = 0;
    // Bit 2 selects the far table 0x80097C84 instead of 0x80097C78. Its
    // gameplay value is UNKNOWN; 0 here.
    uint8_t flags_1f800078 = 0;
    uint8_t mode_1f800075 = 0;
    // CANDIDATE (not original): the PS1 side cut is 160/384 of the depth (its
    // 320-pixel view at H 384). The 3DS view is 400 pixels at the same focal
    // length, so true widens the depth term by 5/4 (200/384); false keeps the
    // PS1 cut and leaves holes at the 3DS screen edges.
    bool widen_for_400 = true;
};

struct TerrainCellVisibility {
    bool valid = false;                  // false: draw everything (unsupported input)
    std::vector<uint8_t> idx_visible;      // by IDX index (positive packed cell)
    std::vector<uint8_t> negative_visible; // by area-local cell index
    uint32_t cells_tested = 0, positive_cells = 0, negative_cells = 0;
};

// 0x80037FB4 tail: ApplyMatrixLV of the camera matrix on the negated eye
// (offset - target), stored to 0x1F800030. Equal to the 0x1F800008
// translation when the scratch shift 0x1F800074 is 0.
std::array<int32_t,3> terrain_cell_translation_1f800030(const std::array<uint32_t,8>& camera_1f800008,
    const std::array<int32_t,3>& eye_800a39f0) noexcept;

// Far cut 0x1F800076: 0x80097C78[lod] (or 0x80097C84[lod] with flag bit 2).
// Returns 0 for a LOD outside the tables.
uint16_t terrain_cell_far_1f800076(uint8_t lod_1f800074, uint8_t flags_1f800078) noexcept;

// Fills out (reusing its storage). out.valid is false when the area is out of
// range or the wedge slope divide would trap; the caller then draws every cell.
void compute_terrain_cell_visibility(const RuntimeMap& map, size_t area_index,
    const ReferenceTables& refs, const TerrainCellViewInput& in, TerrainCellVisibility& out);

} // namespace l3
