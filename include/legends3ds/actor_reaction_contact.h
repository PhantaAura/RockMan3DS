#pragma once
#include "legends3ds/actor_contact.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/class20_03_reaction.h"
#include "legends3ds/class20_03_hit.h"
namespace l3 {
enum class ActorReactionContactStatus:uint8_t {
    SourceBackedPartial,NotMovingBranch,PlanHeld,StaleWork,
    UnsupportedBoundary,UnsupportedWall
};
struct ActorReactionContactPreparation {
    ActorReactionContactStatus status=ActorReactionContactStatus::NotMovingBranch;
    Class20_03ReactionPlan plan;
    ActorContactWork before,after_wall;
    ActorWallContact wall;
    std::optional<Class20_03PreparedReaction> inputs;
    uint8_t initial_action=0,initial_phase=0,initial_aux=0,initial_parameter=0;
    int16_t initial_timer=0;
    uint16_t initial_heading=0;
    std::optional<uint32_t> initial_table;
    uint32_t incoming_packet=0;
    std::optional<Class20_03HitStatus> staged_hit_status;
};
//8002FEA4 motion followed by8004B414 wall work. Neither reaction calls a
// separate floor helper; ground contact is the original result bit1000.
ActorReactionContactPreparation prepare_actor_reaction_contact(
    const Class20_03CallbackState&,const Class20_03ReactionInputs&,
    const ActorContactWork&,const RuntimeMap&,size_t,const ReferenceTables&,
    RetailPlayerBounds) noexcept;
} // namespace l3
