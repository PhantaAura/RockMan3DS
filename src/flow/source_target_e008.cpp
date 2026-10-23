#include "legends3ds/source_target_e008.h"
#include "legends3ds/source_door_event.h"
#include <cmath>
#include <limits>

namespace l3 {
namespace {
// Check boxes at 0x8011CD14 (target) and 0x8011CD20 (door): x, z, y pairs.
constexpr SourceE008Box kTargetBox{-80, 80, -64, 64, -96, 32};
constexpr SourceE008Box kDoorBox{-256, 256, -16, 16, -384, 32};
// IDX descriptor keys at 0x8011CD0C, indexed by parameter & 0xF.
constexpr std::array<uint32_t,2> kDoorKeys{{0x71400u, 0xF0900u}};
uint32_t pulse(uint16_t r, uint32_t mask, uint32_t base) noexcept {
    const uint32_t v = (uint32_t(r) & mask) + base;
    return (v << 16) | (v << 8) | 0x2000000u | v;
}
}

std::optional<SourceE008Object> source_e008_init_80115eac(const RuntimeMap& map, size_t area,
    std::array<int16_t,3> position, uint8_t parameter, bool f608, bool f609) {
    SourceE008Object o;
    o.parameter = parameter; o.position = position;
    if (parameter < 0x80) {
        if ((parameter & 0x1Fu) == 0 ? f608 : f609) return std::nullopt;   // 0x80031A4C
        o.target = true; o.state = 1; o.hp_0a = 100; o.check_box = kTargetBox;
        o.position[1] = int16_t(o.position[1] - 0x20);
        o.heading_30 = int32_t((parameter & 0x60u) << 5);
        return o;
    }
    if (f608 && f609) return std::nullopt;
    o.state = 5; o.check_box = kDoorBox;
    if (parameter & 0x4Fu) {
        if ((parameter & 0xFu) >= kDoorKeys.size()) return std::nullopt;
        const auto cell = runtime_map_find_cell(map, area, position[0], position[2]);
        if (!cell || cell->packed_cell <= 0 || size_t(cell->packed_cell) >= map.idx.size()) return std::nullopt;
        const auto open = source_descriptor_by_word0_8003ac24(map, kDoorKeys[parameter & 0xFu]);
        if (!open) return std::nullopt;
        o.idx_cell = uint16_t(cell->packed_cell);
        o.closed_descriptor = map.idx[*o.idx_cell].descriptor_index;
        o.open_descriptor = *open;
    }
    return o;
}

SourceE008Step source_e008_step(SourceE008Object& o, SourceE008HitRecord& rec, const SourceE008Inputs& in) noexcept {
    SourceE008Step out;
    if (o.released) return out;
    const uint16_t r = in.random_sample.value_or(0);   // CANDIDATE when absent (presentation only)
    switch (o.state) {
    case 1: {                                                           // 0x801161D4
        const uint32_t damage = rec.packet & 0xFFFFFFu;
        if (damage == 0) {
            if (--o.blink_09 == 0) {
                o.blink_09 = uint8_t(((r & 8u) + 1u) * 2u); o.color_40 = 0xFFC080; out.sound = 0x11C;
            } else o.color_40 = 0x808080;
        } else o.color_40 = 0xFFFFFF;
        if (in.checked_by_player) { out.set_60e = true; out.clear_player_check = true; o.check_box.reset(); }
        if (in.flag_61d) { out.clear_player_check = true; o.check_box.reset(); }
        if (int32_t(o.hp_0a) - int32_t(damage) < 0 || (rec.packet & 0x20000000u)) {
            if (rec.word_0e == 0)   // falling direction from the hit (visual only)
                o.heading_30 = -int32_t(std::lround(std::atan2(double(rec.point[0]), double(-rec.point[2])) * 4096.0 / 6.283185307179586));
            o.timer_0b = 0x10; o.color_40 = 0x808080; o.state = 2; out.sound = 0xA2;
            if ((o.parameter & 0x1Fu) == 0) out.set_608 = true; else out.set_609 = true;
        } else {
            o.hp_0a = uint8_t(o.hp_0a - damage); rec.packet = 0;
            if (damage) out.sound = 0xA0;
        }
        out.register_proxy = true;
        out.proxy_point = {{o.position[0], int16_t(o.position[1] - 0x28), o.position[2]}};
        break;
    }
    case 2:                                                             // 0x801164A8
        if (--o.timer_0b == 0) { o.timer_0b = 8; o.state = 3; out.debris_effects = 3; }
        break;
    case 3:                                                             // 0x80116658
        if (--o.timer_0b == 0) o.state = 4;
        break;
    case 4:                                                             // 0x8011668C
        out.burst_effect = true; o.state = 9;
        break;
    case 5: {                                                           // 0x80116750
        ++o.counter_48;
        uint16_t sound = 0; uint32_t period = 0;
        if (!in.flag_608 && !in.flag_609) {
            o.color_40 = pulse(r, 0x3F, 0x40); period = 0x19; sound = 0x11A;
        } else {
            if (in.flag_608 && in.flag_609) {
                if ((o.parameter & 0x4Fu) && o.idx_cell) out.descriptor_write = o.open_descriptor;
                o.timer_0b = 8; o.state = 6;
            }
            o.color_40 = pulse(r, 0x1F, 0x10); period = 0x1F; sound = 0x11B;
        }
        if (o.counter_48 >= period && (o.parameter & 0x40u)) { out.sound = sound; o.counter_48 = 0; }
        break;
    }
    case 6: {                                                           // 0x80116A08
        const uint32_t t = o.timer_0b;
        o.color_40 = (t << 19) | (t << 11) | 0x2000000u | (t << 3);
        if (--o.timer_0b == 0) o.state = 9;
        break;
    }
    case 9:                                                             // 0x80116A7C
        o.released = true;
        break;
    default:
        break;
    }
    return out;
}

SourceActionCheckResult source_action_check_800411dc(const ReferenceTables& refs,
    std::array<int16_t,3> p, uint16_t heading,
    const std::vector<SourceActionCheckCandidate>& candidates) noexcept {
    SourceActionCheckResult out;
    if (!refs.scalar_quarter_known) return out;
    const int32_t s = reference_rsin_8007a060(refs, heading), c = reference_rcos_8007a12c(refs, heading);
    const auto trunc12 = [](int32_t v) { return (v < 0 ? v + 0xFFF : v) >> 12; };
    uint32_t best = 0x7FFF;
    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& k = candidates[i];
        const auto& b = k.box;
        const int32_t mid_x = (int32_t(b.x0) + b.x1) / 2, mid_z = (int32_t(b.z0) + b.z1) / 2;
        // Box centre in 16.16: position << 16 + (lo + hi) * 0x8000, high half.
        const int32_t cx = int32_t((uint32_t(int32_t(k.position[0]) << 16) + uint32_t((int32_t(b.x0) + b.x1) * 0x8000)) >> 16);
        const int32_t cz = int32_t((uint32_t(int32_t(k.position[2]) << 16) + uint32_t((int32_t(b.z0) + b.z1) * 0x8000)) >> 16);
        const int16_t cx16 = int16_t(cx), cz16 = int16_t(cz);
        uint32_t score = 0;
        for (int step = 0, tol = 12, d = 0x50; step < 3; ++step, tol += 12, d += 0x30) {
            const int16_t px = int16_t(uint16_t(p[0]) - uint16_t(trunc12(s * d)));
            const int16_t pz = int16_t(uint16_t(p[2]) - uint16_t(trunc12(c * d)));
            if (px - tol - (b.x1 - mid_x) <= cx16 && cx16 <= px + tol - (b.x0 - mid_x) &&
                pz - tol - (b.z1 - mid_z) <= cz16 && cz16 <= pz + tol - (b.z0 - mid_z) &&
                int32_t(p[1]) - b.y1 <= k.position[1] && k.position[1] <= int32_t(p[1]) - b.y0) {
                const int64_t dx = int64_t(cx16) - p[0], dz = int64_t(cz16) - p[2];
                score = uint32_t(std::sqrt(double(dx * dx + dz * dz)));   // 0x80030374 / 0x8007A578
                break;
            }
        }
        if (score != 0 && score < best) { best = score; out.index = i; }
    }
    if (best < 0xD0 && out.index) out.distance_182 = uint16_t(best);
    else out.index.reset();
    return out;
}

} // namespace l3
