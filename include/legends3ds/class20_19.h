#pragma once
#include "legends3ds/class20_03_late_actions.h"
#include "legends3ds/source_angle.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// ST00 class 0x20 subtype 0x19: the Area 1 trap-room Reaverbots.
// Callback 0x80104864, states 0x8011BF2C (0 init 0x80104A08, 1 active
// 0x80104BE4, 2 release 0x80104DC8), actions 0x8011BF38:
//  0 0x80104DE8 idle 60 frames      3 0x801053B8 back off, look, advance
//  1 0x80104E64 wander               4 0x80105740 hop (drop-in, knockback)
//  2 0x801050E8 chase                5 0x80105974 lunge
// Selector 0x80105D10 (weights 4/6/3/3 at 0x8011BF5C over rand&15), hit
// 0x80105DEC, floor bounds 0x8011BF50 {-32,32,-32,32,-32,0}.
// Parameter 1 with its +0x1A wake flag clear (the trap room: 0x607) starts
// hidden in action 4 phase 0xFF and drops in once the flag is set.
struct Class20_19State {
    uint8_t work_00 = 0;               // bit 0 proxy owner, bit 1 visible
    uint8_t parameter_03 = 0;
    uint8_t state_08 = 0, action_09 = 0, phase_0a = 0, aux_0b = 0;
    uint8_t animation_ac = 0, previous_animation_ad = 0;
    std::array<int16_t,4> raw_14{}, previous_raw_2c{};
    std::array<int32_t,4> fixed_1c{}, previous_fixed_34{};
    std::array<int16_t,3> velocity_44{};   // +44 forward, +46 vertical, +48
    int16_t accel_4c = 0, accel_4e = 0;
    uint16_t heading_56 = 0;
    int16_t turn_66 = 0;
    std::optional<int16_t> health_6e;
    int16_t previous_health_b2 = 0;
    uint16_t shade_15e = 0;
    uint16_t flags_348 = 0;
    uint16_t random_34a = 0;
    int16_t timer_34c = 0, timer_34e = 0;
    uint16_t attack_350 = 0;
    int16_t stagger_352 = 0;
    uint32_t packet_35c = 0;
    std::array<int16_t,3> hit_point_364{};
    uint16_t hit_word_36a = 0;
    uint8_t detection_phase_373 = 0;   // +0x36C + 7 (0x80036470 clears it)
    uint16_t wake_flag_37c = 0;        // copied from +0x1A
    int16_t defeat_timer_37e = 4;
    bool released = false;
};

// Statistics record of subtype 0x19 (0x80098844[0x19], index 0).
struct Class20_19Record {
    uint8_t byte08_hp = 0;             // +6E/+B0/+B2 directly (not x16)
    uint8_t byte09_stagger = 0;        // stagger limit and lethal stagger
    uint8_t byte0a_attack = 0;         // lunge +0x350
    uint8_t byte0f_detection = 0;      // 0x80036478 profile
    uint32_t word00_reward = 0;        // 0x80047148 -> 0x80046B34
};

struct Class20_19Inputs {
    const Class20_03MotionWorld* world = nullptr;  // map/refs/bounds/state74/source_a6
    std::optional<uint16_t> random_sample;         // RA 0x801048F0
    std::optional<Class20_19Record> record;
    std::optional<uint32_t> work_80;               // distance to the player
    std::array<int16_t,3> player_raw{};            // 0x800B51C4
    std::array<int32_t,3> player_fixed{};          // 0x800B51CC
    uint16_t player_heading = 0;                   // 0x800B5206
    const SourceAngleSteps* angle_steps = nullptr;
    std::optional<bool> detected_8003648c;         // read only with flags 0x20
    bool wake_flag_set = false;                    // story flag +0x37C
    std::array<int16_t,3> hit_reference_801000cc{}; // overlay words 0x801000CC/D0
    std::optional<uint16_t> floor_sample_80048e6c; // init only (parameter 0 / awake)
    bool sound_consumer_ready = false;
};

enum class Class20_19Status : uint8_t {
    SourceBackedPartial, UnknownState, UnknownAction, NeedsRandomSample, NeedsRecord,
    NeedsWorld, NeedsWork80, NeedsAngle, NeedsDetection, NeedsAnimationCue, NeedsFloor,
    NeedsSoundConsumer, UnsupportedBoundary, UnsupportedWall, UnsupportedFloor, Released
};
enum class Class20_19Animation : uint8_t { Advance, Restart };

struct Class20_19Result {
    Class20_19Status status = Class20_19Status::SourceBackedPartial;
    Class20_19Animation animation = Class20_19Animation::Advance; // 0x80032488 / 0x800322A8
    uint8_t sound_count = 0;
    std::array<uint16_t,3> sounds{};
    bool register_proxy = false;       // 0x800341BC (radius 0x30) this frame
    uint32_t proxy_word_18 = 0;        // +0x350 | 0x04880000
    bool reset_detection = false;      // 0x80036470
    bool defeat_effect_128_65 = false; // 0x80031618 kind 5, 0x80/0x41 at +0x14
    bool defeat_reward_80047148 = false;
    bool released_80031824 = false;
    uint8_t state74_after = 0;
};

// One 0x80104864 callback. All-or-nothing: a hold leaves `state` unchanged.
Class20_19Result step_class20_19_80104864(Class20_19State& state, const Class20_19Inputs& in) noexcept;

} // namespace l3
