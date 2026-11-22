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

// 0x8004B6AC through the shared player port (runtime_player_contact_step): all
// primitive families it answers, the correction history and the deep-overlap
// recovery. A primitive the port does not answer that holds the actor in X/Z
// still rejects the query (Unsupported). Nine passes give the original
// 0x80000000 result, which the wrapper commits as its negative branch.
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
// raw-Y delta selection and 32767 sentinel. Rectangles 1/3/15, slopes 8..11 and
// the sloped diagonals 0x24..0x54 through the shared 0x8004A060 port; other
// diagonal families keep the strict envelope guard.
ActorFloorContact query_actor_floor_contact_80048ea4(
    const RuntimeMap& map, size_t area_index, const ActorContactWork& work,
    RetailPlayerBounds bounds) noexcept;

} // namespace l3
