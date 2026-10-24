#include "legends3ds/source_box_e005.h"

namespace l3 {

SourceE005Object source_e005_init_80113f2c(std::array<int16_t,3> position, uint8_t parameter,
                                           uint16_t tail_word) noexcept {
    SourceE005Object o;
    o.parameter = parameter; o.position = position;
    o.flags_2c = 0x0E; o.color_40 = 0x808080;
    o.hp_44 = (parameter & 0xF0u) == 0 ? int16_t(0xA0) : int16_t(0x22);
    o.flag_46 = tail_word;
    return o;
}

SourceE005Step source_e005_step(SourceE005Object& o, uint32_t& packet,
                                std::optional<uint16_t> random_sample) noexcept {
    SourceE005Step out;
    if (o.released) { packet = 0; return out; }
    if (o.flags_2c & 8u) o.color_40 = 0x808080;
    if (random_sample) o.random_2e = random_sample;
    if (o.flags_2c & 2u) {                                              // 0x80114370
        const uint32_t damage = packet & 0xFFFFFFu;
        if (damage) {
            if ((packet & 0x1C000000u) == 0) out.sound = 0xA3;
            else {
                const int32_t after = int32_t(uint16_t(o.hp_44)) - int32_t(damage);
                o.hp_44 = int16_t(uint16_t(uint32_t(after)));
                out.sound = 0xA0;
                if (int32_t(uint32_t(after) << 16) < 1) {
                    out.sound = 0xA2; o.action_09 = 1; o.phase_0a = 0; o.counter_0c = 0;
                    o.flags_2c &= 0xFFFDu;
                }
                o.color_40 = 0xFFFFFF;
            }
        }
    }
    if (o.action_09 == 1) {                                             // 0x80114170
        if (o.phase_0a == 0) {
            o.phase_0a = 1; o.counter_0c = 1; o.color_40 = 0x808080; o.flags_2c &= 0xFFF7u;
            // 0x80052528 clears ride links (+0x76/+0xC0) that name the box; the
            // port has none. The 0x80031618 effect goes to the caller.
            out.break_effect = true;
        }
        if (o.phase_0a == 1) {
            const uint8_t before = o.counter_0c--;
            if (before == 1) { o.phase_0a = 2; o.counter_0c = 1; o.color_40 = 0x2E808080; }
        }
        if (o.phase_0a == 2) {                                          // LAB_80114278
            if (++o.counter_0c == 5) {
                o.released = true; out.released = true;
                const uint8_t kind = o.parameter & 0xFu;
                if (kind == 1) { out.reward_record = 2; out.set_flag = o.flag_46; }
                else {
                    if (kind == 2) out.set_flag = o.flag_46;
                    if (o.random_2e) out.reward_record = uint8_t(*o.random_2e & 1u);
                    else out.reward_needs_random = true;
                }
            } else {
                o.color_40 += 0xFFDFDFE0u;                              // 0x8011430C
                o.scale_cb08 += int32_t(o.counter_0c) * 20;
                o.matrix_scale_18 = (o.matrix_scale_18 * o.scale_cb08) >> 12;   // 0x8007ADE8
            }
        }
    }
    if (!o.released && (o.flags_2c & 4u)) {
        out.register_proxy = true;
        out.proxy_point = {{o.position[0], int16_t(o.position[1] - 0x80), o.position[2]}};
        // 0x8011CB00 / 0x8011CAF8 indexed by parameter >> 4 (0 in the tutorial).
        static constexpr std::array<uint32_t,4> kModes{{0x8000u, 0x2000u, 0x1000u, 0x1000u}};
        static constexpr std::array<uint32_t,2> kWords{{0x04080000u, 0x04000000u}};
        out.proxy_mode = kModes[(o.parameter >> 4) & 3u];
        out.proxy_word_18 = kWords[(o.parameter >> 4) & 1u];
    }
    packet = 0;                                                         // 0x80113EE0 +0x30 = 0
    return out;
}

} // namespace l3
