#pragma once
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// ST00 class 0x20 subtype 0x1F, variant 0: the tutorial's hidden "small one".
// Callback 0x801062EC over state table 0x8011C000 (0 init 0x8010648C,
// 1 active 0x80106584, 2 release 0x801066B8) and actions 0x8011C00C
// (0 wait 0x801066D8, 1 lunge 0x8010673C). When the lunge ends it rewrites
// itself into subtype 3 with parameter 0x40 (the airborne Reaverbot entrance).
struct Class20_1FState {
    uint8_t work_00 = 0;          // +0 (state 0 sets bit 1, visible)
    uint8_t state_08 = 0, action_09 = 0, phase_0a = 0;
    uint8_t animation_ac = 0, previous_animation_ad = 0;
    uint16_t flags_348 = 0;
    int16_t timer_34c = 0;
    uint16_t work_350 = 0;
    int16_t work_352 = 0;
    uint16_t random_34a = 0;
    std::optional<int16_t> health_6e; // +6E/+B0/+B2 from the combat record byte +8
    uint16_t heading_56 = 0;
    std::array<int16_t, 4> raw_14{};
    std::array<int32_t, 4> fixed_1c{};
    std::array<int16_t, 4> previous_raw_2c{};
    std::array<int32_t, 4> previous_fixed_34{};
    int32_t scale_z_384 = 0;      // +37C {0x1000,0x1000,z} Q12 ScaleMatrix 0x8007ADE8 (flags 0x20)
    bool morph_to_class20_03 = false; // +2 = 3, +3 = 0x40, +8..+B = 0
};

struct Class20_1FInputs {
    const ReferenceTables* references = nullptr;
    std::optional<uint16_t> random_sample;   // rand() at RA 0x80106378
    std::optional<uint32_t> work_80;         // 0x8003606C distance
    std::optional<int16_t> initial_health;   // combat record +8 (state 0)
    uint32_t packet_35c = 0;                 // hit packet (read only with flags bit 2)
    bool sound_consumer_ready = false;
};

enum class Class20_1FStatus : uint8_t {
    SourceBackedPartial, UnknownState, NeedsRandomSample, NeedsReference,
    NeedsWork80, NeedsDetection, NeedsSoundConsumer, Released
};
enum class Class20_1FAnimation : uint8_t { Advance, Restart };

struct Class20_1FResult {
    Class20_1FStatus status = Class20_1FStatus::SourceBackedPartial;
    Class20_1FAnimation animation = Class20_1FAnimation::Advance; // 0x80032488 / 0x800322A8
    std::optional<uint16_t> sound_8001a050;                       // 0xA3 on a hit packet
    bool proxy_800341bc = false;                                  // flags bit 8
};

// One original callback. All-or-nothing: on a hold `state` is unchanged.
Class20_1FResult step_class20_1f_801062ec(Class20_1FState& state, const Class20_1FInputs& in) noexcept;

} // namespace l3
