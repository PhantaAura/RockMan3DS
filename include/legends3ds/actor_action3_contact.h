#pragma once
#include "legends3ds/actor_contact.h"
#include "legends3ds/class20_03_action3.h"
#include "legends3ds/reference_tables.h"

namespace l3 {

enum class ActorAction3ContactStatus : uint8_t {
    SourceBackedPartial,
    NotMovingBranch,
    StaleWork,
    NeedsTurnLimit,
    NeedsTargetHeading,
    UnsupportedBoundary,
    UnsupportedWall,
    UnsupportedFloor
};

struct ActorAction3ContactPreparation {
    ActorAction3ContactStatus status = ActorAction3ContactStatus::NotMovingBranch;
    ActorContactWork before, after_wall;
    ActorWallContact wall;
    ActorFloorContact floor;
    std::optional<Class20_03PreparedAction3> inputs;
    uint8_t initial_action = 0, initial_phase = 0;
    int16_t initial_timer = 0;
    std::optional<int16_t> initial_work64;
    uint32_t initial_packet = 0;
    uint16_t initial_heading = 0;
    std::optional<uint16_t> target_heading;
    int16_t effective_turn_limit = 0, source_turn = 0;
};

// Read-only source action3 preparation. The caller stages current raw/fixed
// into the previous callback snapshots first, as original8010226C does. Both
// mirrors must match the supplied contactwork. Forward768 and source turn64
// feed resident original trig; actual wall mutation/floor query must succeed.
// Boundary cases where the inherited contact candidate gather is unproved are
// explicitly held. Source reducer owns floor stores and full snapshot rollback.
// No allocations and no mutation of state, map, or active motion.
ActorAction3ContactPreparation prepare_actor_action3_contact(
    const Class20_03CallbackState& state, const ActorContactWork& motion,
    const RuntimeMap& map, size_t area_index, const ReferenceTables& references,
    RetailPlayerBounds bounds, std::optional<uint16_t> target_heading) noexcept;

// Exact two little-endian32-bit words read by8001A050 at the source176/180
// producers. Pass pre-action raw14..1B: fourthlane is retained, and action2's
// later movement must not change the queued position payload.
std::array<uint32_t, 2> actor_source_sound_position_words(
    const std::array<int16_t, 4>& pre_action_raw) noexcept;

} // namespace l3
