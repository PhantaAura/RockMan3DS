#pragma once
#include "legends3ds/class20_03_behavior.h"

namespace l3 {

// Values owned by original turn/motion, wall, and floor preparation. These are
// after 8002FEA4/8004B414 and before 80048E6C stores; no motion is fabricated.
struct Class20_03PreparedAction3 {
    std::array<int16_t, 4> raw_14{};
    std::array<int32_t, 4> fixed_1c{};
    int32_t source_floor_word = 0;
    uint32_t source_contact_word = 0;
    uint16_t heading_after_turn = 0;
};

struct Class20_03Action3Inputs {
    std::optional<uint32_t> packet_33c;
    // Selected original combat-record byte A is required only on phase0.
    std::optional<uint8_t> combat_record_byte_a;
    std::optional<uint16_t> target_heading_80031198;
    std::optional<Class20_03PreparedAction3> prepared_motion;
    bool sound_consumer_ready = false;
    // Acknowledges original class20 pool-bitmap update. The scalar body clears
    // work00, but this flag never implements allocator/resource cleanup itself.
    bool release_consumer_ready = false;
};

enum class Class20_03Action3Status : uint8_t {
    SourceBackedPartial,
    UnknownAction,
    NeedsPacket,
    StalePacket,
    NeedsCombatProfile,
    NeedsDelay,
    NeedsTurnLimit,
    NeedsTargetHeading,
    NeedsMotion,
    StaleMotion,
    NeedsSoundConsumer,
    NeedsReleaseConsumer
};

struct Class20_03Action3Result {
    Class20_03Action3Status status = Class20_03Action3Status::SourceBackedPartial;
    std::optional<uint16_t> sound_8001a050;
    bool release_80031824 = false;
};

// Original ST00 80102C9C action3 body, excluding common state1 tail and outer
// callback. Original halfword wrap, fallthrough, complete snapshot rollback,
// requests2/3 and source consumer ordering are retained. Unknown dependencies
// hold every field and return no intents. No full AI or combat parity implied.
Class20_03Action3Result class20_03_step_action3(
    Class20_03CallbackState& state, const Class20_03Action3Inputs& inputs) noexcept;

} // namespace l3
