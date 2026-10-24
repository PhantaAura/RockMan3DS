#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/player_collision.h"
#include "legends3ds/reference_tables.h"
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

// USA 0x80049C54 with identical start and end points (the gameplay camera's
// eye test): is a point inside solid collision? Free (raw 0) when every
// candidate cell lets it through; Embedded with the original nonzero raw
// result otherwise (0x40000000 = no cell at the point; low16 = packed cell,
// bits16..19 = primitive flags). Candidates come from 0x8004B1E0 mode 1
// (empty cells included, gather stops at the grid edge). Primitives whose
// height interval needs an unported 0x8004A060 case return Unsupported.
enum class RuntimePointEmbedding : uint8_t { Free, Embedded, Unsupported };
struct RuntimePointEmbeddingResult {
    RuntimePointEmbedding status = RuntimePointEmbedding::Unsupported;
    uint32_t raw = 0;
};
RuntimePointEmbeddingResult runtime_point_embedded_80049c54(const RuntimeMap& map,
    size_t area_index, int16_t x, int16_t y, int16_t z, RetailPlayerBounds bounds,
    bool skip_flag_8000) noexcept;

// USA 0x8004920C: the floor under a point, used by effects and pickups.
// The cell is 0x80048C60 of (x, z) only (no neighbours). A cell <= 0 gives
// -((cell & 0x1F00) >> 1) - 1. Otherwise every primitive of every family-0/1
// group whose rectangle (zero bounds) holds the point, and whose bottom lies
// below y, offers its top; the smallest top wins, 0x7FFF when none does.
// Nullopt when a group needs an unported shape (families 2..5) or interval.
std::optional<int32_t> runtime_point_floor_8004920c(const RuntimeMap& map,
    size_t area_index, int16_t x, int16_t y, int16_t z) noexcept;

const char* runtime_contact_status_name(RuntimeContactStatus status);

// Event primitive found by USA 0x800514FC (contact code > 0xFF). Coordinates
// are the source group/primitive centres; type is code>>8, flags primitive +6.
struct RuntimeEventPrimitiveSample {
    int16_t x = 0, y = 0, z = 0;
    uint16_t heading = 0; // player +0x56, copied unchanged
    uint16_t type = 0;
    uint16_t flags = 0;
    uint32_t cell_index = 0;
    uint32_t primitive_index = 0;
};

enum class RuntimeEventQueryStatus : uint8_t {
    NoData,      // no candidate cell
    Miss,        // candidates searched, no event primitive contains the point
    Hit,
    Unsupported, // an event group needs an unported shape family or interval
    NeedsTrig,   // flag 0x8000 needs the RotMatrixY front point
};

struct RuntimeEventPrimitiveQuery {
    RuntimeEventQueryStatus status = RuntimeEventQueryStatus::NoData;
    std::optional<RuntimeEventPrimitiveSample> sample;
    uint32_t candidate_cells = 0;
    uint32_t event_groups_tested = 0;
    uint16_t unsupported_code = 0;
};

// Exact USA 0x800514FC for the player (a1 = player +0x78 bounds). Candidates
// come from 0x8004B1E0 and are searched last to first. Only groups whose first
// primitive code exceeds 0xFF are tested; flag 0x8000 tests the point 64 units
// ahead (RotMatrixY(+0x56) applied to (0,0,-64)). Supported: shape families
// 0/1 (0x8004E304 strict expanded rectangle) and the default 0x8004A060
// interval (low byte outside 8..85), whose second bounds subtraction is kept.
// Anything else stops the search as Unsupported rather than guessing a miss.
// Allocation-free.
RuntimeEventPrimitiveQuery runtime_player_event_primitive_800514fc(
    const RuntimeMap& map, size_t area_index, const L3PlayerCompatWork& player,
    const ReferenceTables* trig, RetailPlayerBounds bounds = {});

} // namespace l3
