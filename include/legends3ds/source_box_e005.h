#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// ST00 E0/05 (0x80113EE0, states 0x8011CAE4, actions 0x8011CAF0): the Area 2
// breakable box. Init 0x80113F2C: flags +0x2C 0x0E, HP +0x44 0xA0 (0x22 when
// parameter & 0xF0), flag +0x46 = the record's tail word (0x620 in Area 2).
// State 1 0x80113FEC: rand() into +0x2E every frame (RA 0x80114020); hit
// 0x80114370 while flags & 2 (0xA3 without 0x1C000000, else HP -= damage, 0xA0,
// or at HP <= 0 0xA2 and the break action); then a 0x800341BC proxy at
// (+0x38, +0x3A - 0x80, +0x3C), radius 0xA0, mode 0x8000 / producer 0x04080000
// (parameter >> 4 == 0). Action 1 0x80114170 breaks it over 4 frames and
// releases; parameter & 0xF: 1 -> reward record 2 and the flag, 2 -> the flag
// and reward record (+0x2E & 1), else reward record (+0x2E & 1). Each fade
// frame adds counter * 20 to the static scale vector 0x8011CB08 (0x1000 in the
// overlay image) and applies ScaleMatrix 0x8007ADE8 to the +0x18 matrix.
struct SourceE005Object {
    uint8_t parameter = 0;
    std::array<int16_t,3> position{};   // +0x38
    uint8_t action_09 = 0, phase_0a = 0, counter_0c = 0;
    uint16_t flags_2c = 0x0E;
    int16_t hp_44 = 0;
    uint16_t flag_46 = 0;
    uint32_t color_40 = 0x808080;
    int32_t scale_cb08 = 0x1000;        // 0x8011CB08 (static; one box per stage, CANDIDATE)
    int32_t matrix_scale_18 = 0x1000;   // uniform diagonal of the +0x18 matrix
    std::optional<uint16_t> random_2e;
    bool released = false;
};
struct SourceE005Step {
    uint16_t sound = 0;
    bool register_proxy = false;
    std::array<int16_t,3> proxy_point{};
    uint32_t proxy_mode = 0x8000, proxy_word_18 = 0x04080000;
    std::optional<uint16_t> set_flag;
    std::optional<uint8_t> reward_record;  // 0x80047228(this, +0x38, record)
    bool reward_needs_random = false;      // released without a +0x2E sample
    bool break_effect = false;             // phase 0: 0x80031618 5/0x80/0x42 at +0x38, y - 0x80
    bool released = false;
};

SourceE005Object source_e005_init_80113f2c(std::array<int16_t,3> position, uint8_t parameter,
                                           uint16_t tail_word_3e) noexcept;
// One callback; `packet` is +0x30 (cleared after the callback, as 0x80113EE0 does).
SourceE005Step source_e005_step(SourceE005Object& o, uint32_t& packet,
                                std::optional<uint16_t> random_sample) noexcept;

} // namespace l3
