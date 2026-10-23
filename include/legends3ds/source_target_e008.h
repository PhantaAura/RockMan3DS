#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

// ST00 E0/08 (0x80115E70, states 0x8011CD2C), the Area 2 targets and doors.
//  - Parameter < 0x80: target. Init (0x80115EAC) releases when its flag is set
//    (0x608 for parameter & 0x1F == 0, else 0x609); 100 HP at +0x0A, check box
//    0x8011CD14, heading (parameter & 0x60) << 5, y -= 0x20. State 1
//    (0x801161D4): the shared hit record (0x80122620 + 16 * (parameter & 1))
//    costs HP (sound 0xA0); HP < damage or a 0x20000000 hit destroys it (sound
//    0xA2, its flag, state 2). Player +0x184 == this sets 0x60E and clears the
//    check; 0x61D clears it too. States 2..4 break it apart, 9 releases.
//  - Parameter >= 0x80: door. Init releases when 0x608 and 0x609 are both set;
//    check box 0x8011CD20; parameter & 0x4F != 0 owns its IDX cell (key
//    0x8011CD0C[parameter & 0xF]: 0x71400 / 0xF0900). State 5 (0x80116750)
//    writes the keyed descriptor once both flags are set, fades 8 frames (6)
//    and releases (9). Sounds 0x11A / 0x11B every 25 / 31 frames (param & 0x40).
struct SourceE008Box { int16_t x0 = 0, x1 = 0, z0 = 0, z1 = 0, y0 = 0, y1 = 0; };
struct SourceE008Object {
    uint8_t parameter = 0;
    bool target = false;
    std::array<int16_t,3> position{};   // +0x38/+0x3A/+0x3C
    uint8_t state = 0;                  // +8
    uint8_t blink_09 = 0, hp_0a = 0, timer_0b = 0;
    int32_t heading_30 = 0;
    uint32_t color_40 = 0x808080;
    uint32_t counter_48 = 0;
    std::optional<SourceE008Box> check_box;   // +0x10 (cleared once checked)
    std::optional<uint16_t> idx_cell;         // +0x44
    uint16_t closed_descriptor = 0;           // +0x0C
    uint16_t open_descriptor = 0;             // +0x2C
    bool released = false;
    uint8_t record() const noexcept { return uint8_t(parameter & 1u); }
};
// 0x80122620 + 16 * index: packet word +0, point +8 (hit direction).
struct SourceE008HitRecord {
    uint32_t packet = 0;
    std::array<int16_t,3> point{};
    uint16_t word_0e = 0;
};
struct SourceE008Inputs {
    bool flag_608 = false, flag_609 = false, flag_61d = false;
    bool checked_by_player = false;       // player +0x184 == this object
    std::optional<uint16_t> random_sample; // 0x8007FE50 (blink / door pulse)
};
struct SourceE008Step {
    bool set_608 = false, set_609 = false, set_60e = false, clear_player_check = false;
    std::optional<uint16_t> descriptor_write;
    uint16_t sound = 0;
    bool register_proxy = false;          // 0x800341BC this frame
    std::array<int16_t,3> proxy_point{};  // (+0x38, +0x3A - 0x28, +0x3C)
    uint8_t debris_effects = 0;           // state 2 end: 3 x 0x80031618 kind 1/0x83/2
    bool burst_effect = false;            // state 4: 0x80031618 kind 5/0x80/0x82
    bool needs_random = false;            // hold: a 0x8007FE50 draw is required
};

std::optional<SourceE008Object> source_e008_init_80115eac(const RuntimeMap& map, size_t area,
    std::array<int16_t,3> position, uint8_t parameter, bool flag_608, bool flag_609);
// Consumes and clears record.packet like the original (+0 stored back 0).
SourceE008Step source_e008_step(SourceE008Object& o, SourceE008HitRecord& record,
    const SourceE008Inputs& in) noexcept;

// 0x800411DC/0x800414F0 for check-box candidates with zero rotation (every
// E0 object; class 20/40/60 pools carry no check data in the tutorial).
// Probes 0x50/0x80/0xB0 ahead of the player with tolerances 12/24/36, keeps
// the candidate with the smallest 0x80030374 horizontal distance below 208.
// CANDIDATE: 0x8007A578 (table SquareRoot0) is an exact integer sqrt here.
struct SourceActionCheckCandidate { std::array<int16_t,3> position{}; SourceE008Box box{}; };
struct SourceActionCheckResult { std::optional<size_t> index; uint16_t distance_182 = 0; };
SourceActionCheckResult source_action_check_800411dc(const ReferenceTables& refs,
    std::array<int16_t,3> player, uint16_t heading,
    const std::vector<SourceActionCheckCandidate>& candidates) noexcept;

} // namespace l3
