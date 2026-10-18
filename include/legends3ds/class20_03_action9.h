#pragma once
#include "legends3ds/class20_03_behavior.h"

namespace l3 {

// Distinct source8002FEA4(work,52,0,0) producer after80031BEC turn32 and
//8004B414 wall response, before80048E6C floor stores. The signed animation
// byteA6 belongs to the admitted source animation cursor, never a default cue.
struct Class20_03PreparedAction9 {
    std::array<int16_t,4> raw_14{};
    std::array<int32_t,4> fixed_1c{};
    int32_t source_floor_word = 0;
    uint32_t source_contact_word = 0;
    uint16_t heading_after_turn = 0;
    std::optional<int8_t> source_a6;
};

struct Class20_03Action9Inputs {
    std::optional<uint32_t> work_80;
    // First source angle getter receives player position then actor position.
    std::optional<uint16_t> target_angle_player_to_actor_80031198;
    std::optional<uint16_t> player_heading_800b5206;
    // Later source turn helper receives actor position then player position.
    std::optional<uint16_t> turn_target_actor_to_player_80031198;
    std::optional<Class20_03PreparedAction9> prepared_motion;
    bool sound_consumer_ready = false;
};

enum class Class20_03Action9Status : uint8_t {
    SourceBackedPartial,
    UnknownAction,
    NeedsPlayerToActorAngle,
    NeedsPlayerHeading,
    NeedsWork80,
    NeedsTurnTarget,
    NeedsMotion,
    StaleMotion,
    NeedsAnimationCue,
    NeedsSoundConsumer
};

struct Class20_03Action9Result {
    Class20_03Action9Status status = Class20_03Action9Status::SourceBackedPartial;
    std::optional<uint16_t> sound_8001a050;
    // Source176 reads the post-wall position before floor stores/rollback.
    // It differs from action2's pre-motion sound position.
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
};

// Original ST00 80103B58 action9 body only. Always retains the unsigned
// player-heading difference, complete phase0 fallthrough, source turn32,
// floor33-unit/full-lane rollback, contact and action3/4/10 boundaries.
// Missing dependencies hold every state field and suppress all cue intents.
// BIOS RNG, common tail, child attacks and outer callback are separate gates.
Class20_03Action9Result class20_03_step_action9(
    Class20_03CallbackState& state,const Class20_03Action9Inputs& input) noexcept;

} // namespace l3
