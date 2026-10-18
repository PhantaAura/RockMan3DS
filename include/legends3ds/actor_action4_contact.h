#pragma once
#include "legends3ds/actor_contact.h"
#include "legends3ds/class20_03_action4.h"

namespace l3 {
enum class ActorAction4ContactStatus : uint8_t {
    SourceBackedPartial,NotMovingBranch,StaleWork,
    UnsupportedBoundary,UnsupportedWall,UnsupportedFloor
};
struct ActorAction4ContactPreparation {
    ActorAction4ContactStatus status=ActorAction4ContactStatus::NotMovingBranch;
    ActorContactWork before,after_wall;
    ActorWallContact wall;
    ActorFloorContact floor;
    std::optional<Class20_03PreparedAction4> inputs;
    uint8_t initial_action=0,initial_phase=0,initial_request=0;
    uint16_t initial_heading=0;
    int16_t initial_timer=0,initial_variant=0;
    Class20_03Action4Plan plan;
};
// Distinct original motion branch. No cue, angle, hit result or fabricated
// floor required. Caller stages all four previous raw/fixed lanes first.
ActorAction4ContactPreparation prepare_actor_action4_contact(
    const Class20_03CallbackState&,const ActorContactWork&,
    const RuntimeMap&,size_t area_index,const ReferenceTables&,
    RetailPlayerBounds) noexcept;
} // namespace l3
