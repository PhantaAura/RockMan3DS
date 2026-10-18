#pragma once
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/source_reaction_profile.h"
namespace l3 {
enum class Class20_03ReactionStatus:uint8_t {
    SourceBackedPartial,NotReaction,NeedsProfile,NeedsHealth,NeedsCombatByteB,
    NeedsTableOwner,UnsupportedTableIndex,NeedsMotion,StaleMotion,
    NeedsSoundConsumer,NeedsTargetHeading
};
struct Class20_03ReactionPlan {
    Class20_03ReactionStatus status=Class20_03ReactionStatus::NotReaction;
    bool moves=false;
    int16_t forward=0,vertical=0;
    std::array<int16_t,3> velocity_before_wall{};
};
struct Class20_03ReactionInputs {
    const SourceReactionProfile* profile=nullptr;
    // Exact selected record+0B, required only by action8/parameter40 phase0.
    // Matching actor_variant is checked; a base record is never borrowed.
    std::optional<uint8_t> combat_byte_b;
    std::optional<int16_t> combat_record_variant;
    std::optional<Class20_03PreparedReaction> motion;
    std::optional<uint16_t> target_heading_80031198;
    bool sound_consumer_ready=false;
};
struct Class20_03ReactionResult {
    Class20_03ReactionStatus status=Class20_03ReactionStatus::NotReaction;
    std::optional<uint16_t> sound_8001a050;
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
};
// Exact action7/8 control and scalar writes. Motion/wall work is an admitted
// receipt, not a new knockback or physics rule. Missing inputs hold all state.
Class20_03ReactionPlan class20_03_plan_reaction(
    const Class20_03CallbackState&,const Class20_03ReactionInputs&) noexcept;
Class20_03ReactionResult class20_03_step_reaction(
    Class20_03CallbackState&,const Class20_03ReactionInputs&) noexcept;
} // namespace l3
