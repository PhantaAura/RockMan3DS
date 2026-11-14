#pragma once
#include "legends3ds/class20_03_late_actions.h"
#include "legends3ds/source_angle.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// ST00 class 0x40 subtype 0x02: the Area 3 tutorial boss (parameter 1).
// Wrapper 0x801076E8, states 0x8011C054 (0 init 0x801078B8, 1 action
// 0x80107B48, 2 release 0x80107F00, 3 null, 4 0x80107D04), substates
// 0x8011C068:
//  0 0x80107F40 wait for detection or a hit     4 0x80109038 stagger
//  1 0x80107FAC walk toward the player, decide  6 0x80109638 slam (stomp)
//  2 0x801084D4 charge (footstep shockwaves)
//  3 0x80108C18 12-tick turn, then substate +0x0D
// Hit 0x8010A448. Parameter 1 dispatches state 1 only; substates 5 and 7..14
// belong to state 4 / parameter 0 (another stage's variant) and are held,
// never invented. Two class-60 subtype-0x0A arms (0x801101F8) follow limbs 2
// and 4; in this variant they only receive the slam pulse (substate 3,
// 0x80110D24) and the hit flash.
struct Class40_02Arm {
    bool allocated = false;
    uint8_t part_0d = 0;                // 2 or 4: the boss limb they follow
    uint8_t flags_0e = 0;               // bit 0x20: flash on the next update
    uint8_t substate_09 = 0, phase_0a = 0;
    int16_t timer_c0 = 0;
    int32_t scale_y_f8 = 0x1000;        // Q12 (+0xF4/+0xF8/+0xFC), x/z stay 0x1000
};

struct Class40_02State {
    uint8_t work_00 = 0;                // bit 0 proxy owner, bit 1 visible
    uint8_t parameter_03 = 1;
    uint8_t state_08 = 0, substate_09 = 0, phase_0a = 0, aux_0b = 0;
    uint8_t saved_0d = 0;
    uint8_t animation_ac = 0, previous_animation_ad = 0;
    std::array<int16_t,4> raw_14{}, previous_raw_2c{};
    std::array<int32_t,4> fixed_1c{}, previous_fixed_34{};
    int16_t velocity_44 = 0;
    uint16_t heading_56 = 0;
    int16_t turn_66 = 0;
    int16_t health_6e = 0, health_max_b0 = 0, previous_health_b2 = 0;
    uint16_t shade_15e = 0x4210;
    uint16_t flags_4e0 = 0;             // 1 done, 2 hittable, 4 stagger mode,
                                        // 8 proxy, 0x10 animated, 0x20 detected,
                                        // 0x40 defeat handled
    uint16_t random_4e2 = 0;
    int16_t timer_4e4 = 0, timer_4e6 = 0;
    uint16_t attack_4e8 = 0;
    int16_t stagger_4ea = 0;
    uint32_t packet_4f4 = 0;            // proxy target written by the bank
    std::array<Class40_02Arm,2> arms{}; // +0x514 / +0x518
    bool gauge_registered = false;      // 0x8005D620 / 0x8005D6E8
    bool released = false;
};

// Statistics record 0x8009884C[2] + parameter*16 (Area 3: 06 05 04 03 ...
// 20 50 10 00 00 00 00 05, PSXRecomp RAM read).
struct Class40_02Record {
    uint8_t byte08_hp = 0;              // HP = byte08 << 5
    uint8_t byte09_stagger = 0;         // stagger limit = byte09 << 2
    uint8_t byte0f_detection = 0;       // 0x80036478 profile for +0x504
};

struct Class40_02Inputs {
    const Class20_03MotionWorld* world = nullptr; // bounds 0x8011C0A4, class 0x40
    std::optional<uint16_t> random_sample;        // RA 0x80107774
    std::optional<Class40_02Record> record;
    std::optional<uint32_t> work_80;              // distance to the player
    std::array<int16_t,3> player_raw{};           // 0x800B51C4
    std::array<int32_t,3> player_fixed{};         // 0x800B51CC
    uint16_t player_heading = 0;                  // 0x800B5206
    const SourceAngleSteps* angle_steps = nullptr;
    std::optional<bool> detected_8003648c;        // stepped every state-1 update
    uint8_t stage_800c1b60 = 0;
    std::optional<int16_t> player_hp_800b521e;    // read by 0x80042208 at defeat
    std::optional<uint8_t> frame_a4;              // animation cursor +0xA4 (footstep cue)
    bool arms_allocated = false;                  // two 0x80031518 receipts
    bool sound_consumer_ready = false;
};

enum class Class40_02Status : uint8_t {
    SourceBackedPartial, Released, UnknownState, UnsupportedState, UnsupportedSubstate,
    NeedsRandomSample, NeedsRecord, NeedsWorld, NeedsWork80, NeedsAngle, NeedsDetection,
    NeedsFloor, NeedsAnimationCue, NeedsSoundConsumer, NeedsArms, NeedsPlayerHealth,
    UnsupportedBoundary, UnsupportedWall, UnsupportedFloor
};
enum class Class40_02Animation : uint8_t { Advance, Restart };

struct Class40_02Shockwave {               // 0x80031618: work 5, subtype 0x80, param 0x42
    std::array<int16_t,4> point_38{};
};

struct Class40_02Result {
    Class40_02Status status = Class40_02Status::SourceBackedPartial;
    Class40_02Animation animation = Class40_02Animation::Advance; // 0x80032488 / 0x800322A8
    uint8_t sound_count = 0;
    std::array<uint16_t,4> sounds{};
    bool register_proxy = false;        // 0x800341BC radius 0xD0
    uint32_t proxy_word_18 = 0;         // +0x4E8 | 0x04880008
    uint8_t shockwave_count = 0;
    std::array<Class40_02Shockwave,3> shockwaves{};
    bool camera_shake_800398f8 = false; // (0x1E, 2000, 5000, +0x14)
    bool register_gauge_8005d620 = false;
    bool unregister_gauge_8005d6e8 = false;
    bool set_flag_606 = false;          // 0x8001DA8C(0x606)
    bool player_lock_800b55f9 = false;  // 0x80042208 side effect
    bool release_arms = false;          // state 2: 0x800318D8 for both arms
    bool released = false;              // state 2: 0x8003187C
};

// One 0x801076E8 callback plus the two arms' updates. All-or-nothing: a hold
// leaves `state` unchanged.
Class40_02Result step_class40_02_801076e8(Class40_02State& state, const Class40_02Inputs& in) noexcept;

// 0x80107B48 post-dispatch chooser (pure).
struct Class40_02Choice {
    uint8_t substate = 0, saved_0d = 0;
    uint16_t flags_4e0 = 0;
    bool reset_phase = false, defeat_started = false;
};
Class40_02Choice choose_class40_02_80107b48(int16_t hp, uint8_t substate, uint8_t saved_0d,
                                            uint16_t flags_4e0, uint8_t stage_800c1b60) noexcept;

// 0x800398F8(n, near, far, point): camera shake byte from the camera's
// distance, or nothing when farther than `far`.
std::optional<uint8_t> source_camera_shake_800398f8(int32_t n, uint32_t near_distance, uint32_t far_distance,
                                                    uint32_t distance) noexcept;

} // namespace l3
