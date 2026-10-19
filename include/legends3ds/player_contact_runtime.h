#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/player_collision.h"
#include <cstdint>
#include <optional>

namespace l3 {

// These six offsets are the PL00 initializer's USA 0x80097D90 bounds,
// installed by 0x8003BED4..0x8003BEDC and consumed by 0x8004B3C0.
// Collision coordinates use retail Y-down world units throughout.
struct RetailPlayerBounds {
    int16_t x0 = -32, x1 = 32, z0 = -32, z1 = 32, y0 = -160, y1 = 0;
};

struct RuntimeContactFloorInterval {
    int16_t y0 = 0;
    int16_t y1 = 0;
};

// Source floor-query interval only, after the family-0/1 rectangle classifier.
// Codes 1/3/15 retain their bounds; 8..11 use the original 0x8004A060 slope
// transform. This does not implement the separate slope movement response.
// Outside rectangles, unsupported codes, and source arithmetic traps return
// no interval. The helper performs no allocations.
std::optional<RuntimeContactFloorInterval> runtime_contact_floor_interval_8004a060(
    const RuntimeMapIdx& idx, const RuntimeContactGroup& group,
    const RuntimeContactPrimitive& primitive, RetailPlayerBounds bounds,
    int16_t query_x, int16_t query_z);

enum class RuntimeContactStatus : uint8_t {
    NoData,
    SourceBackedPartial,
    Unsupported,
    IterationLimit,
};

struct RuntimePlayerContact {
    RuntimeContactStatus status = RuntimeContactStatus::NoData;
    int32_t result = 0;
    L3RetailContactScratch scratch{};
    std::optional<bool> grounded;
    std::optional<int16_t> floor_y;
    std::optional<uint32_t> floor_cell_index;
    std::optional<uint32_t> floor_primitive_index;
    uint32_t candidate_cells = 0;
    uint32_t groups_tested = 0;
    uint32_t primitives_tested = 0;
    uint32_t unsupported_primitives = 0;
    uint32_t deep_overlap_recoveries = 0; // 0x80050400 responses this step
    uint8_t iterations = 0;
    bool wall_contact = false;
    bool ceiling_contact = false;
    bool landed = false;
};

// Read-only source floor query at the requested pose, before movement response.
// The player update calls 0x80048E6C/0x8003C610 before 0x8004B414; callers
// must not use a solver-corrected XZ position for this earlier query.
RuntimePlayerContact runtime_player_floor_query(const RuntimeMap& map,
                                               size_t area_index,
                                               const L3PlayerCompatWork& player,
                                               RetailPlayerBounds bounds = {});

// Bounded, allocation-free translation of the proved rectangle movement
// subset (codes 1, 3, 15), packed negative-cell Y response, and floor query.
// Source: USA 0x8004B6AC/0x8004BDC8/0x8004F44C/0x80048EA4. Code 1 and
// airborne-upward code 3 include the 0x80050400 deep-overlap recovery.
// The floor query also supports proved 8..11 intervals. Their separate slope
// movement responses, and remaining diagonal handlers, are Unsupported; no
// movement semantics are assigned from a primitive's numeric code.
RuntimePlayerContact runtime_player_contact_step(const RuntimeMap& map,
                                                size_t area_index,
                                                const L3PlayerCompatWork& player,
                                                RetailPlayerBounds bounds = {});

const char* runtime_contact_status_name(RuntimeContactStatus status);

} // namespace l3
