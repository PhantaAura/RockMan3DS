#include "legends3ds/class40_02_boss.h"
#include <limits>

namespace l3 {
namespace {
using Status = Class40_02Status;
int16_t s16(uint32_t v) noexcept { v &= 0xffffu; return v < 0x8000u ? int16_t(v) : int16_t(int32_t(v) - 65536); }
int32_t s32(uint32_t v) noexcept {
    return v <= uint32_t(std::numeric_limits<int32_t>::max()) ? int32_t(v) : int32_t(int64_t(v) - 0x100000000LL);
}
int16_t dec(int16_t v) noexcept { return s16(uint32_t(uint16_t(v)) - 1u); }
Class40_02Result hold(Status s) noexcept { Class40_02Result r; r.status = s; return r; }

// Shockwave offsets 0x8011C14C (charge) and 0x8011C168 (slam), forward units.
constexpr std::array<int16_t,3> kChargeShock{{12000, 9200, 6400}};
constexpr std::array<int16_t,3> kSlamShock{{12000, 9200, 6400}};

struct Body {
    Class40_02State& s;
    const Class40_02Inputs& in;
    Class40_02Result& r;
    uint8_t state74;
    Status failure = Status::SourceBackedPartial;

    bool sound(uint16_t id) {
        if (!in.sound_consumer_ready || r.sound_count >= r.sounds.size()) { failure = Status::NeedsSoundConsumer; return false; }
        r.sounds[r.sound_count++] = id; return true;
    }
    bool neighbors() const noexcept {
        const auto& map = *in.world->map;
        if (in.world->area_index >= map.areas.size()) return false;
        const auto& area = map.areas[in.world->area_index];
        const int16_t x = s.raw_14[0], z = s.raw_14[2];
        const int32_t nx = (uint16_t(x) & 511u) < 257u ? -512 : 512;
        const int32_t nz = (uint16_t(z) & 511u) < 257u ? -512 : 512;
        const std::array<std::array<int32_t,2>,4> d{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
        const auto shift9 = [](int32_t v) { return v >= 0 ? v / 512 : -((-v + 511) / 512); };
        for (const auto& o : d) {
            const auto gx = shift9(int32_t(x) + o[0] + 32768) - area.start_x;
            const auto gz = shift9(int32_t(z) + o[1] + 32768) - area.start_z;
            if (gx < 0 || gz < 0 || gx >= area.width || gz >= area.height) return false;
            const uint64_t local = uint64_t(gz) * area.width + uint64_t(gx);
            if (local >= area.cell_count || uint64_t(area.cell_first) + local >= map.cells.size()) return false;
        }
        return true;
    }
    ActorContactWork contact() const noexcept {
        ActorContactWork c;
        c.raw = s.raw_14; c.fixed = s.fixed_1c; c.previous_raw = s.previous_raw_2c; c.previous_fixed = s.previous_fixed_34;
        c.velocity = {{s.velocity_44, 0, 0}}; c.object_class = in.world->object_class; c.state74 = state74;
        return c;
    }
    // 0x8002FEA4(work, forward, 0, 0).
    void move(int16_t forward) noexcept {
        const auto d = reference_player_motion_delta(*in.world->references, s.heading_56, forward, 0, 0);
        const std::array<int32_t,3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
        for (size_t a = 0; a < 3; ++a) {
            s.fixed_1c[a] = s32(uint32_t(s.fixed_1c[a]) + uint32_t(delta[a]));
            s.raw_14[a] = reference_fixed_to_short_80030150(s.fixed_1c[a]);
        }
    }
    // 0x8004B414; returns the wall word.
    bool wall(uint32_t& word) noexcept {
        if (!neighbors()) { failure = Status::UnsupportedBoundary; return false; }
        auto c = contact();
        const auto w = query_actor_wall_contact_8004b6ac(*in.world->map, in.world->area_index, c, in.world->bounds);
        if (!apply_actor_contact_result_8004b414(c, w)) { failure = Status::UnsupportedWall; return false; }
        s.raw_14 = c.raw; s.fixed_1c = c.fixed; s.velocity_44 = c.velocity[0]; state74 = c.state74;
        word = uint32_t(w.result);
        return true;
    }
    // 0x80048E6C: the floor height under the current position.
    bool floor_value(int16_t& y) noexcept {
        if (!neighbors()) { failure = Status::UnsupportedBoundary; return false; }
        const auto f = query_actor_floor_contact_80048ea4(*in.world->map, in.world->area_index, contact(), in.world->bounds);
        if (f.status != RuntimeContactStatus::SourceBackedPartial) { failure = Status::UnsupportedFloor; return false; }
        y = s16(uint32_t(f.result));
        return true;
    }
    // ... stored to +0x16 / +0x20.
    bool floor() noexcept {
        int16_t y = 0; if (!floor_value(y)) return false;
        s.raw_14[1] = y; s.fixed_1c[1] = s32(uint32_t(uint16_t(y)) << 16);
        return true;
    }
    void restore() noexcept { s.raw_14 = s.previous_raw_2c; s.fixed_1c = s.previous_fixed_34; }
    int32_t rise() const noexcept {
        const int32_t d = int32_t(s.raw_14[1]) - s.previous_raw_2c[1];
        return d < 0 ? -d : d;
    }
    // 0x80031DCC (behind=false) / 0x80031E60 (behind=true): heading a->b
    // within +-half_a of head_a, and within +-half_b of head_b (minus 0x800
    // for 0x80031DCC).
    bool cone(const std::array<int32_t,3>& a, uint16_t head_a, uint16_t half_a,
              const std::array<int32_t,3>& b, uint16_t head_b, uint16_t half_b, bool behind, bool& ok) noexcept {
        const auto h = source_heading_800310cc(a[0], a[2], b[0], b[2], in.angle_steps);
        if (!h) { failure = Status::NeedsAngle; return false; }
        const auto within = [](uint32_t value, uint16_t half) {
            return int32_t((uint32_t(half) + value) & 0xfffu) <= int32_t(int32_t(int16_t(half)) << 16 >> 15);
        };
        const uint32_t hb = behind ? uint32_t(head_b) : uint32_t(head_b) - 0x800u;
        ok = within(*h - head_a, half_a) && within(*h - hb, half_b);
        return true;
    }
    std::array<int32_t,3> fixed() const noexcept { return {{s.fixed_1c[0], s.fixed_1c[1], s.fixed_1c[2]}}; }
    bool facing_player(uint16_t half, bool& ok) noexcept {             // 0x80031DCC(+0x1C, +0x56, half, player, ..., 0x800)
        return cone(fixed(), s.heading_56, half, in.player_fixed, in.player_heading, 0x800, false, ok);
    }
    // 0x80031BEC(+0x14, player +0x14, heading, max).
    bool turn_toward_player(int16_t max, int32_t& step) noexcept {
        const auto t = source_heading_80031198(s.raw_14[0], s.raw_14[2], in.player_raw[0], in.player_raw[2], in.angle_steps);
        if (!t) { failure = Status::NeedsAngle; return false; }
        step = class20_03_turn_step(uint16_t(*t), s.heading_56, max);
        return true;
    }
    bool frame_a4(uint8_t& v) noexcept {
        if (!in.frame_a4) { failure = Status::NeedsAnimationCue; return false; }
        v = *in.frame_a4; return true;
    }
    void choose(uint8_t substate, uint8_t saved) noexcept {
        s.substate_09 = substate; s.saved_0d = saved; s.phase_0a = 0; s.aux_0b = 0;
    }
    // Three 0x80031618 shockwaves, each 0x8002FEA4(offset) from the position
    // at that moment, then the whole +0x14..+0x28 restored from +0x2C.
    bool shockwaves(const std::array<int16_t,3>& offsets, bool floor_y) noexcept {
        for (size_t i = 0; i < offsets.size(); ++i) {
            move(offsets[i]);
            Class40_02Shockwave w; w.point_38 = s.raw_14;
            if (floor_y) { int16_t y = 0; if (!floor_value(y)) return false; w.point_38[1] = y; }
            r.shockwaves[r.shockwave_count++] = w;
            restore();
        }
        r.camera_shake_800398f8 = true;
        return true;
    }
};

bool substate0(Body& b) noexcept {                                     // 0x80107F40
    auto& s = b.s;
    if (s.phase_0a == 0) { s.phase_0a = 1; s.animation_ac = 0; s.attack_4e8 = 0; }
    else if (s.phase_0a != 1) return true;
    if ((s.flags_4e0 & 0x20u) || (s.packet_4f4 & 0xffffffu)) s.flags_4e0 |= 1u;
    return true;
}

bool substate1(Body& b) noexcept {                                     // 0x80107FAC
    auto& s = b.s; const auto& in = b.in;
    if (s.phase_0a == 0) { s.phase_0a = 1; s.animation_ac = 1; s.timer_4e4 = 0x50; s.attack_4e8 = 0; }
    else if (s.phase_0a != 1) return true;
    if (!in.work_80) { b.failure = Status::NeedsWork80; return false; }
    if (*in.work_80 < 0xc8000u) {
        s.animation_ac = s.previous_animation_ad;
        bool ok = false; if (!b.facing_player(0x80, ok)) return false;
        if (!ok) b.choose(3, 6); else b.choose(6, s.saved_0d);
        return true;
    }
    s.timer_4e4 = dec(s.timer_4e4);
    if (s.timer_4e4 < 0) {
        bool ok = false; if (!b.facing_player(0x80, ok)) return false;
        if (!ok) b.choose(3, 2); else b.choose(2, s.saved_0d);
        s.timer_4e6 = 0x3c;
    }
    int32_t step = 0; if (!b.turn_toward_player(0x20, step)) return false;
    s.heading_56 = uint16_t((uint32_t(s.heading_56) + uint32_t(step)) & 0xfffu);
    b.move(0x100);
    uint32_t w = 0; if (!b.wall(w)) return false;
    uint8_t a4 = 0; if (!b.frame_a4(a4)) return false;
    if ((a4 & 3u) == 0 && !b.sound(200)) return false;
    if (!b.floor()) return false;
    if (b.rise() < 0x21) {
        if (w & 0xf00u) { b.choose(3, 1); s.animation_ac = s.previous_animation_ad; }
    } else {
        b.choose(3, 1); s.animation_ac = s.previous_animation_ad; b.restore();
    }
    return true;
}

bool substate2(Body& b) noexcept {                                     // 0x801084D4
    auto& s = b.s; const auto& in = b.in;
    if (s.phase_0a == 0) { s.phase_0a = 1; s.animation_ac = 2; s.attack_4e8 = 0; s.timer_4e4 = s.timer_4e6; }
    else if (s.phase_0a != 1) return true;
    if (!in.work_80) { b.failure = Status::NeedsWork80; return false; }
    s.timer_4e4 = dec(s.timer_4e4);
    if (s.timer_4e4 < 0 || (s.packet_4f4 & 0x2000000u) || *in.work_80 <= 0x47fffu) { s.flags_4e0 |= 1u; return true; }
    b.move(0x200);
    uint32_t w = 0; if (!b.wall(w)) return false;
    if (!b.floor()) return false;
    if (b.rise() < 0x21) {
        if ((w & 0xf00u) == 0) {
            bool away = false;
            if (!b.cone(in.player_fixed, in.player_heading, 0x800, b.fixed(), s.heading_56, 0x400, true, away)) return false;
            if (!away) {
                if (!in.world->source_a6) { b.failure = Status::NeedsAnimationCue; return false; }
                if (*in.world->source_a6 != 1) return true;
                if (!b.shockwaves(kChargeShock, true)) return false;
                return b.sound(0xc2);
            }
            b.choose(3, 1);
        } else b.choose(3, 2);
        s.animation_ac = s.previous_animation_ad;
    } else {
        b.choose(3, 2); s.animation_ac = s.previous_animation_ad; b.restore();
    }
    s.timer_4e6 = s.timer_4e4;
    return true;
}

bool substate3(Body& b) noexcept {                                     // 0x80108C18
    auto& s = b.s;
    if (s.phase_0a == 0) {
        s.phase_0a = 1; s.animation_ac = 3;
        int32_t full = 0; if (!b.turn_toward_player(0x800, full)) return false;
        s.timer_4e4 = 0xc; s.attack_4e8 = 0;
        int32_t t = int32_t(int16_t(full / 0xc));
        s.turn_66 = int16_t(t < 0 ? -t : t);
    } else if (s.phase_0a != 1) return true;
    s.timer_4e4 = dec(s.timer_4e4);
    if (s.timer_4e4 < 1) s.flags_4e0 |= 1u;
    int32_t step = 0; if (!b.turn_toward_player(s.turn_66, step)) return false;
    s.heading_56 = uint16_t((uint32_t(s.heading_56) + uint32_t(step)) & 0xfffu);
    uint8_t a4 = 0; if (!b.frame_a4(a4)) return false;
    if ((a4 & 1u) && !b.sound(200)) return false;
    return true;
}

bool substate4(Body& b) noexcept {                                     // 0x80109038
    auto& s = b.s;
    if (s.phase_0a == 0) {
        s.phase_0a = 1; s.animation_ac = 4; s.velocity_44 = 0; s.timer_4e4 = 0x1c; s.attack_4e8 = 0;
        s.flags_4e0 &= 0xfffbu;
    } else if (s.phase_0a != 1) return true;
    s.timer_4e4 = dec(s.timer_4e4);
    if (s.timer_4e4 < 1) s.flags_4e0 |= 5u;
    b.move(int16_t(int32_t(s.velocity_44) << 3));
    uint32_t w = 0; if (!b.wall(w)) return false;
    if (!b.floor()) return false;
    if (b.rise() > 0x20) b.restore();
    return true;
}

bool substate6(Body& b) noexcept {                                     // 0x80109638
    auto& s = b.s;
    const auto arms_pulse = [&] {
        for (auto& a : s.arms) { a.substate_09 = 3; a.phase_0a = 0; }
    };
    switch (s.phase_0a) {
    case 0:
        s.phase_0a = 1; s.animation_ac = 0xb; s.timer_4e4 = 0xd; s.attack_4e8 = 0;
        if (!b.sound(0xbf)) return false;
        [[fallthrough]];
    case 1:
        s.timer_4e4 = dec(s.timer_4e4);
        if (s.timer_4e4 >= 0) return true;
        s.phase_0a = 2; s.timer_4e4 = 10; arms_pulse();
        [[fallthrough]];
    case 2:
        s.timer_4e4 = dec(s.timer_4e4);
        if (s.timer_4e4 >= 0) return true;
        s.phase_0a = 3; s.animation_ac = uint8_t(s.animation_ac + 1); s.timer_4e4 = 0x14;
        if (!b.sound(0xc0)) return false;
        [[fallthrough]];
    case 3:
        s.timer_4e4 = dec(s.timer_4e4);
        if (s.timer_4e4 >= 0) {
            if (s.timer_4e4 == 3) {
                if (!b.shockwaves(kSlamShock, false)) return false;
                return b.sound(0xc1);
            }
            return true;
        }
        s.phase_0a = 4; s.timer_4e4 = 10;
        [[fallthrough]];
    case 4:
        s.timer_4e4 = dec(s.timer_4e4);
        if (s.timer_4e4 >= 0) return true;
        s.phase_0a = 5; s.animation_ac = uint8_t(s.animation_ac + 1); s.timer_4e4 = 0xf;
        [[fallthrough]];
    case 5:
        s.timer_4e4 = dec(s.timer_4e4);
        if (s.timer_4e4 < 0) s.flags_4e0 |= 1u;
        return true;
    default:
        return true;
    }
}

// 0x8010A448.
bool hit(Body& b) noexcept {
    auto& s = b.s; const auto& in = b.in;
    uint32_t damage = s.packet_4f4 & 0xffffffu;
    if (damage == 0) return true;
    if ((s.packet_4f4 & 0x18000000u) == 0) return b.sound(0xa3);
    if (!in.record) { b.failure = Status::NeedsRecord; return false; }
    const int16_t before = s.health_6e;
    const int32_t after = int32_t(uint16_t(before)) - int32_t(damage);
    s.health_6e = s16(uint32_t(after)); s.previous_health_b2 = before;
    if (int32_t(uint32_t(after) << 16) < 1) {
        if (in.stage_800c1b60 == 0) {
            // 0x80042208: alive player (HP >= 0) locks 0x800B55F9 and returns 1.
            if (!in.player_hp_800b521e) { b.failure = Status::NeedsPlayerHealth; return false; }
            if (*in.player_hp_800b521e >= 0) {
                b.r.player_lock_800b55f9 = true;
                s.health_6e = 0; s.flags_4e0 &= 0xfffdu;
            } else s.health_6e = 1;
        } else {
            s.health_6e = 0; s.flags_4e0 = uint16_t((s.flags_4e0 & 0xfffdu) | 4u);
            s.substate_09 = 4; s.phase_0a = 0; s.aux_0b = 0;
            damage = in.record->byte09_stagger;
        }
    }
    const uint32_t limit = uint32_t(in.record->byte09_stagger) << 2;
    uint16_t id = 0xa2;
    if (!(s.flags_4e0 & 4u)) {
        if (damage < limit) { id = 0xa0; if (int32_t(limit) <= s.stagger_4ea) id = 0xa2; }
    } else {
        s.stagger_4ea = s16(uint32_t(uint16_t(s.stagger_4ea)) + (damage > 9 ? damage : 10u));
        if (limit <= damage || int32_t(limit) <= s.stagger_4ea) {
            if (!b.sound(0xa2)) return false;
            s.substate_09 = 4; s.phase_0a = 0; s.aux_0b = 0; s.stagger_4ea = 0;
            id = 0;
        } else id = 0xa0;
    }
    if (id && !b.sound(id)) return false;
    s.shade_15e = 0x7fff;
    for (auto& a : s.arms) a.flags_0e |= 0x20u;
    return true;
}

// Arm 0x8011032C for this variant: flash, substate 0 / 3, done bit.
void arm_update(Class40_02Arm& a) noexcept {
    if (!a.allocated) return;
    a.flags_0e &= 0xdfu;                                                // flash consumed (+0xB4 = 0xFF)
    bool done = false;
    if (a.substate_09 == 3) {                                           // 0x80110D24
        switch (a.phase_0a) {
        case 0:
            a.phase_0a = 1; a.timer_c0 = 10; a.scale_y_f8 = 0x1000;
            [[fallthrough]];
        case 1:
            a.timer_c0 = dec(a.timer_c0);
            if (a.timer_c0 >= 0) { a.scale_y_f8 += 0x199; break; }
            a.phase_0a = 2; a.timer_c0 = 0x1e; a.scale_y_f8 = 0x2000;
            [[fallthrough]];
        case 2:
            a.timer_c0 = dec(a.timer_c0);
            if (a.timer_c0 >= 0) break;
            a.phase_0a = 3; a.timer_c0 = 0xf;
            [[fallthrough]];
        case 3:
            a.timer_c0 = dec(a.timer_c0);
            if (a.timer_c0 < 0) { a.scale_y_f8 = 0x1000; done = true; break; }
            a.scale_y_f8 -= 0x111;
            break;
        default: break;
        }
    }
    if (done) { a.substate_09 = 0; a.phase_0a = 0; }
}
} // namespace

Class40_02Choice choose_class40_02_80107b48(int16_t hp, uint8_t substate, uint8_t saved_0d,
                                            uint16_t flags_4e0, uint8_t stage_800c1b60) noexcept {
    Class40_02Choice c; c.substate = substate; c.saved_0d = saved_0d; c.flags_4e0 = flags_4e0;
    if (!(flags_4e0 & 1u)) return c;
    c.reset_phase = true;
    if (hp < 1) {
        c.substate = 0;
        if (stage_800c1b60 == 0 && !(c.flags_4e0 & 0x40u)) { c.flags_4e0 |= 0x40u; c.defeat_started = true; }
    } else if (substate == 0 || substate == 4) {
        c.substate = 3; c.saved_0d = 1;
    } else if (substate == 3) {
        c.substate = saved_0d;
    } else if (substate == 2 || (flags_4e0 & 0x20u)) {
        c.substate = 1;
    } else c.substate = 0;
    c.flags_4e0 &= 0xfffeu;
    return c;
}

std::optional<uint8_t> source_camera_shake_800398f8(int32_t n, uint32_t near_distance, uint32_t far_distance,
                                                    uint32_t distance) noexcept {
    if (n <= 0 || far_distance < distance || far_distance <= near_distance) return std::nullopt;
    // The near clamp writes a dead stack slot; far - distance is used as is.
    return uint8_t((uint32_t(far_distance - distance) * uint32_t(n)) / (far_distance - near_distance));
}

Class40_02Result step_class40_02_801076e8(Class40_02State& state, const Class40_02Inputs& in) noexcept {
    if (state.released) return hold(Status::Released);
    if (state.state_08 == 3) return hold(Status::UnknownState);         // null table entry
    if (state.state_08 == 4) return hold(Status::UnsupportedState);     // parameter 0 only
    if (state.state_08 > 4) return hold(Status::UnknownState);
    if (!in.random_sample) return hold(Status::NeedsRandomSample);
    if (!in.world || !in.world->map || !in.world->references) return hold(Status::NeedsWorld);
    auto n = state;
    Class40_02Result r;
    Body b{n, in, r, in.world->state74};
    n.previous_animation_ad = n.animation_ac;
    n.previous_raw_2c = n.raw_14; n.previous_fixed_34 = n.fixed_1c;
    n.shade_15e = 0x4210;
    n.random_4e2 = *in.random_sample;
    if (n.state_08 == 0) {                                              // 0x801078B8
        if (n.parameter_03 != 1) return hold(Status::UnsupportedState);
        if (!in.arms_allocated) return hold(Status::NeedsArms);
        if (!in.record) return hold(Status::NeedsRecord);
        if (!in.floor_sample_80048e6c) return hold(Status::NeedsFloor);
        n.arms[0] = {}; n.arms[0].allocated = true; n.arms[0].part_0d = 2; n.arms[0].flags_0e = 0x40;
        n.arms[1] = {}; n.arms[1].allocated = true; n.arms[1].part_0d = 4; n.arms[1].flags_0e = 1;
        n.work_00 |= 2u;
        n.flags_4e0 = 0x1e; n.timer_4e4 = 0; n.timer_4e6 = 0; n.attack_4e8 = 0; n.stagger_4ea = 0;
        n.packet_4f4 = 0;
        n.health_6e = n.health_max_b0 = n.previous_health_b2 = int16_t(uint16_t(in.record->byte08_hp) << 5);
        n.raw_14[1] = s16(*in.floor_sample_80048e6c);
        n.fixed_1c[1] = s32(uint32_t(*in.floor_sample_80048e6c) << 16);
        n.state_08 = 1; n.substate_09 = 0; n.phase_0a = 0; n.aux_0b = 0;
        r.animation = Class40_02Animation::Restart;                    // 0x800322A8(work, 0, 0)
        n.gauge_registered = true; r.register_gauge_8005d620 = true;
        n.shade_15e = 0x4210;
    } else if (n.state_08 == 2) {                                       // 0x80107F00
        r.released = true; r.release_arms = true;
        n.released = true; n.gauge_registered = false;
        state = n;
        r.status = Status::Released;
        return r;
    } else {                                                            // 0x80107B48
        if (!in.detected_8003648c) return hold(Status::NeedsDetection);
        n.flags_4e0 = *in.detected_8003648c ? uint16_t(n.flags_4e0 | 0x20u) : uint16_t(n.flags_4e0 & 0xffdfu);
        if ((n.flags_4e0 & 2u) && !hit(b)) return hold(b.failure);
        bool ok = false;
        switch (n.substate_09) {
        case 0: ok = substate0(b); break;
        case 1: ok = substate1(b); break;
        case 2: ok = substate2(b); break;
        case 3: ok = substate3(b); break;
        case 4: ok = substate4(b); break;
        case 6: ok = substate6(b); break;
        default: return hold(Status::UnsupportedSubstate);
        }
        if (!ok) return hold(b.failure);
        if (n.flags_4e0 & 1u) {
            const auto c = choose_class40_02_80107b48(n.health_6e, n.substate_09, n.saved_0d, n.flags_4e0, in.stage_800c1b60);
            n.substate_09 = c.substate; n.saved_0d = c.saved_0d; n.flags_4e0 = c.flags_4e0;
            n.phase_0a = 0; n.aux_0b = 0;
            if (c.defeat_started) {
                r.set_flag_606 = true; r.unregister_gauge_8005d6e8 = true; n.gauge_registered = false;
            }
        }
        if (n.stagger_4ea) n.stagger_4ea = dec(n.stagger_4ea);
        if (!(n.flags_4e0 & 0x10u)) return hold(Status::UnsupportedState); // packed-pose path, never selected here
        r.animation = n.previous_animation_ad == n.animation_ac ? Class40_02Animation::Advance : Class40_02Animation::Restart;
    }
    for (auto& a : n.arms) arm_update(a);
    if ((n.work_00 & 1u) && (n.flags_4e0 & 8u)) {
        r.register_proxy = true; r.proxy_word_18 = uint32_t(n.attack_4e8) | 0x04880008u;
    }
    n.packet_4f4 = 0;
    state = n;
    return r;
}

} // namespace l3
