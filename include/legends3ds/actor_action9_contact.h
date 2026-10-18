#pragma once
#include "legends3ds/actor_contact.h"
#include "legends3ds/class20_03_action9.h"
#include "legends3ds/reference_tables.h"

namespace l3 {
enum class ActorAction9ContactStatus : uint8_t {
    SourceBackedPartial,
    NotMovingBranch,
    NeedsWork80,
    StaleWork,
    NeedsTurnTarget,
    NeedsAnimationCue,
    UnsupportedBoundary,
    UnsupportedWall,
    UnsupportedFloor
};

struct ActorAction9ContactPreparation {
    ActorAction9ContactStatus status=ActorAction9ContactStatus::NotMovingBranch;
    ActorContactWork before,after_wall;
    ActorWallContact wall;
    ActorFloorContact floor;
    std::optional<Class20_03PreparedAction9> inputs;
    uint8_t initial_action=0,initial_phase=0;
    uint16_t initial_flags=0,initial_heading=0;
    std::optional<uint32_t> work80;
    std::optional<uint16_t> turn_target;
    std::optional<int8_t> source_a6;
    int16_t source_turn=0;
};

// Independently admitted action9 turn32, forward52 producer. Source8002FEA4
// executes the accumulator/rounding path;8002FEDC is its call delay slot, not
// a separate function. Caller stages previous callback snapshots first.
// Missing inputs, unsupported contact or stale mirrors publish no prepared
// values. Raw/fixed fourth lanes and wall-owned velocity/state74 are retained.
ActorAction9ContactPreparation prepare_actor_action9_contact(
    const Class20_03CallbackState& state,const ActorContactWork& motion,
    const RuntimeMap& map,size_t area_index,const ReferenceTables& references,
    RetailPlayerBounds bounds,std::optional<uint32_t> work80,
    std::optional<uint16_t> turn_target,std::optional<int8_t> source_a6) noexcept;

} // namespace l3
