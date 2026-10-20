#pragma once
#include "legends3ds/actor_contact.h"
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/reference_tables.h"

namespace l3 {

// World owners for the ST00 action bodies that move inside the body itself.
// The body runs after the hit consumer, so its motion cannot be prepared
// beforehand without going stale when a hit changes the action.
struct Class20_03MotionWorld {
    const RuntimeMap* map = nullptr;
    size_t area_index = 0;
    const ReferenceTables* references = nullptr;
    RetailPlayerBounds bounds{};
    uint8_t object_class = 0x20;
    uint8_t state74 = 0;                 // wall wrapper byte, in/out
    std::optional<int8_t> source_a6;     // current animation cue
};

enum class Class20_03LateActionStatus : uint8_t {
    SourceBackedPartial, UnknownAction, NeedsWorld, NeedsCombatProfile,
    NeedsTargetHeading, NeedsAnimationCue, NeedsSoundConsumer,
    UnsupportedBoundary, UnsupportedWall, UnsupportedFloor
};

struct Class20_03LateActionInputs {
    const Class20_03MotionWorld* world = nullptr;
    // Combat record +0x0A for the actor's own variant (0x800A8844 table).
    std::optional<uint8_t> combat_record_byte_a;
    // 0x80031BEC turn target: actor-to-player angle of 0x80031198.
    std::optional<uint16_t> target_heading_80031198;
    bool sound_consumer_ready = false;
};

struct Class20_03LateActionResult {
    Class20_03LateActionStatus status = Class20_03LateActionStatus::SourceBackedPartial;
    uint8_t sound_count = 0;
    std::array<uint16_t,2> sounds{};
    std::array<std::array<uint32_t,2>,2> sound_words{};
    uint8_t state74_after = 0;
};

// ST00 jump table 0x8011BEA8 entries 5 (0x8010324C), 6 (0x80103554) and 10
// (0x80103DFC). All-or-nothing: on any hold `state` is unchanged.
//  5: damaged bounce; rsin/rcos launch, gravity 16, wall reflection, sound
//     181/222, then phase2 sets HP 0 and flags 0x41 (the defeat tail).
//  6: hit knockback; vertical 0x46<<7 through 0x8002FEA4, ends on contact 0x1000.
// 10: lunge; animation 4 for 45 frames, turn32 + forward52, floor with the
//     +/-33 rollback, then cooldown 0x35E=45.
Class20_03LateActionResult class20_03_step_late_action(
    Class20_03CallbackState& state, const Class20_03LateActionInputs& inputs) noexcept;

bool class20_03_is_late_action(uint8_t action) noexcept;

} // namespace l3
