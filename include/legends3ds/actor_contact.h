#pragma once
#include "legends3ds/player_contact_runtime.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// The actor work fields read/written by USA 8004B414. Padding mirrors are
// retained because a negative class20 result copies all eight/sixteen bytes.
struct ActorContactWork {
    std::array<int16_t, 4> raw{};            // +14..1B
    std::array<int32_t, 4> fixed{};          // +1C..2B
    std::array<int16_t, 4> previous_raw{};   // +2C..33
    std::array<int32_t, 4> previous_fixed{}; // +34..43
    std::array<int16_t, 3> velocity{};       // +44/+46/+48
    uint8_t object_class = 0x20;
    uint8_t state74 = 0;
};

struct ActorWallContact {
    RuntimeContactStatus status = RuntimeContactStatus::Unsupported;
    int32_t result = 0;
    L3RetailContactScratch scratch{};
    uint32_t candidate_cells = 0;
    uint32_t groups_tested = 0;
    uint32_t primitives_tested = 0;
    uint32_t unsupported_primitives = 0;
    uint8_t iterations = 0;
};

struct ActorFloorContact {
    RuntimeContactStatus status = RuntimeContactStatus::Unsupported;
    // Original signed return, including the valid 32767 no-floor sentinel.
    int32_t result = 32767;
    std::optional<uint32_t> cell_index;
    std::optional<uint32_t> primitive_index;
    uint32_t candidate_cells = 0;
    uint32_t unsupported_primitives = 0;
};

// Allocation-free, bounded source subset. Candidate gathering, class filter,
// list selection, rectangle1/3/15 responses, packed negative-cell Y response,
// and final core-result masks retain original integer behavior. Unknown
// families can only be excluded using the separately proved strict envelope
// guard. Intersecting unknown families, deep overlap and arithmetic outside
// the admitted bounds reject the complete query.
ActorWallContact query_actor_wall_contact_8004b6ac(
    const RuntimeMap& map, size_t area_index, const ActorContactWork& work,
    RetailPlayerBounds bounds) noexcept;

// Exact work-mutation half of 8004B414, also usable with an explicitly owned
// external core result. Only SourceBackedPartial input commits. A zero result
// preserves every work field. Negative results choose class-dependent prior
// pose restoration. Positive results apply original masks and state74 rule.
bool apply_actor_contact_result_8004b414(
    ActorContactWork& work, const ActorWallContact& core) noexcept;

// Generic source floor query after wall mutation, including original signed
// raw-Y delta selection and 32767 sentinel. Supports rectangle1/3/15.
// Slope8..11 and unknown visits reject the query without a substituted current
// Y. Their scalar transform evidence does not prove the full wrapper contract.
ActorFloorContact query_actor_floor_contact_80048ea4(
    const RuntimeMap& map, size_t area_index, const ActorContactWork& work,
    RetailPlayerBounds bounds) noexcept;

} // namespace l3
