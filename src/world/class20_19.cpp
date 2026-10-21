#include "legends3ds/class20_19.h"
#include <limits>

namespace l3 {
namespace {
using Status = Class20_19Status;
int16_t s16(uint32_t v) noexcept { v &= 0xffffu; return v < 0x8000u ? int16_t(v) : int16_t(int32_t(v) - 65536); }
int32_t s32(uint32_t v) noexcept {
    return v <= uint32_t(std::numeric_limits<int32_t>::max()) ? int32_t(v) : int32_t(int64_t(v) - 0x100000000LL);
}
int16_t dec(int16_t v) noexcept { return s16(uint32_t(uint16_t(v)) - 1u); }
Class20_19Result hold(Status s) noexcept { Class20_19Result r; r.status = s; return r; }

struct Body {
    Class20_19State& s;
    const Class20_19Inputs& in;
    Class20_19Result& r;
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
        c.velocity = s.velocity_44; c.object_class = in.world->object_class; c.state74 = state74;
        return c;
    }
    // 0x8002FEA4(work, forward, vertical, 0).
    void move(int16_t forward, int16_t vertical) noexcept {
        const auto d = reference_player_motion_delta(*in.world->references, s.heading_56, forward, vertical, 0);
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
        s.raw_14 = c.raw; s.fixed_1c = c.fixed; s.velocity_44 = c.velocity; state74 = c.state74;
        word = uint32_t(w.result);
        return true;
    }
    // 0x80048E6C into +0x16 / +0x20.
    bool floor() noexcept {
        if (!neighbors()) { failure = Status::UnsupportedBoundary; return false; }
        const auto f = query_actor_floor_contact_80048ea4(*in.world->map, in.world->area_index, contact(), in.world->bounds);
        if (f.status != RuntimeContactStatus::SourceBackedPartial) { failure = Status::UnsupportedFloor; return false; }
        s.raw_14[1] = s16(uint32_t(f.result));
        s.fixed_1c[1] = s32(uint32_t(uint16_t(f.result)) << 16);
        return true;
    }
    void restore() noexcept { s.raw_14 = s.previous_raw_2c; s.fixed_1c = s.previous_fixed_34; }
    int32_t rise() const noexcept { return int32_t(s.raw_14[1]) - s.previous_raw_2c[1]; }
    // Step with wall and floor, then undo a step of more than 32 either way.
    bool step(int16_t forward) noexcept {
        move(forward, 0);
        uint32_t w = 0; if (!wall(w)) return false;
        if (!floor()) return false;
        int32_t d = rise(); if (d < 0) d = -d;
        if (d > 0x20) restore();
        return true;
    }
    // 0x80031DCC: player within +-half of the heading, facing back within +-half2.
    bool cone(uint16_t half, uint16_t half2, bool& ok) noexcept {
        const auto a = source_heading_800310cc(s.fixed_1c[0], s.fixed_1c[2], in.player_fixed[0], in.player_fixed[2], in.angle_steps);
        if (!a) { failure = Status::NeedsAngle; return false; }
        const auto within = [](uint32_t value, uint16_t h) {
            return int32_t((uint32_t(h) + value) & 0xfffu) <= int32_t(int32_t(int16_t(h)) << 16 >> 15);
        };
        ok = within(*a - s.heading_56, half) && within(*a - (uint32_t(in.player_heading) - 0x800u), half2);
        return true;
    }
    bool detected() noexcept {
        if (!(s.flags_348 & 0x20u)) return true;
        if (!in.detected_8003648c) { failure = Status::NeedsDetection; return false; }
        s.flags_348 = *in.detected_8003648c ? uint16_t(s.flags_348 | 0x10u) : uint16_t(s.flags_348 & 0xffefu);
        return true;
    }
};

// Shared wander/chase tail (0x80104E64 / 0x801050E8 after the floor sample).
// Returns true when the floor dropped by more than 32 (the caller hops).
bool floor_tail(Body& b) noexcept {
    const int32_t d = b.rise();
    if (d >= 0x21) { b.restore(); return true; }
    if (d < 0 && -d > 0x20) { b.s.heading_56 = uint16_t((b.s.heading_56 + 0x800u) & 0xfffu); b.restore(); }
    return false;
}

bool action0(Body& b) noexcept {                                       // 0x80104DE8
    auto& s = b.s;
    if (s.phase_0a == 0) { s.phase_0a = 1; s.animation_ac = 0; s.timer_34c = 0x3C; s.attack_350 = 0; s.flags_348 &= 0xffdfu; }
    else if (s.phase_0a != 1) return true;
    s.timer_34c = dec(s.timer_34c);
    if (s.timer_34c < 0) s.flags_348 |= 1u;
    return true;
}

bool action1(Body& b) noexcept {                                       // 0x80104E64
    auto& s = b.s;
    if (s.phase_0a == 0) {
        s.phase_0a = 1; s.animation_ac = 1; s.velocity_44[0] = 0x30; s.accel_4c = 4;
        s.timer_34c = 0x32; s.timer_34e = 0x19; s.attack_350 = 0; s.flags_348 &= 0xffdfu;
    } else if (s.phase_0a != 1) return true;
    s.timer_34e = dec(s.timer_34e);
    if (s.timer_34e < 0) { s.timer_34e = 0x19; s.velocity_44[0] = 0x30; }
    s.heading_56 = uint16_t((uint32_t(s.heading_56) + uint32_t(int32_t(s.turn_66))) & 0xfffu);
    b.move(s.velocity_44[0], 0);
    s.velocity_44[0] = s16(uint32_t(int32_t(s.velocity_44[0]) + s.accel_4c));
    uint32_t w = 0; if (!b.wall(w)) return false;
    if (!b.floor()) return false;
    if (w & 0xf00u) { s.heading_56 = uint16_t((s.heading_56 + 0x800u) & 0xfffu); b.restore(); }
    if (floor_tail(b)) { s.action_09 = 4; s.phase_0a = 0; s.aux_0b = 0; return true; }
    s.timer_34c = dec(s.timer_34c);
    if (s.timer_34c < 0) s.flags_348 |= 1u;
    return true;
}

bool action2(Body& b) noexcept {                                       // 0x801050E8
    auto& s = b.s; const auto& in = b.in;
    if (s.phase_0a == 0) {
        s.phase_0a = 1; s.animation_ac = 2; s.velocity_44[0] = 0x100; s.accel_4c = 0x10;
        s.timer_34c = 0xC; s.attack_350 = 0;
    } else if (s.phase_0a != 1) return true;
    s.timer_34c = dec(s.timer_34c);
    if (s.timer_34c < 0) { s.velocity_44[0] = 0x100; s.timer_34c = 0xC; }
    // 0x80031BEC(+0x14, player +0x14, heading, 0x30).
    const auto target = source_heading_80031198(s.raw_14[0], s.raw_14[2], in.player_raw[0], in.player_raw[2], in.angle_steps);
    if (!target) { b.failure = Status::NeedsAngle; return false; }
    s.heading_56 = uint16_t((uint32_t(s.heading_56) + uint32_t(int32_t(class20_03_turn_step(uint16_t(*target), s.heading_56, 0x30)))) & 0xfffu);
    if (!in.world->source_a6) { b.failure = Status::NeedsAnimationCue; return false; }
    b.move(int16_t(uint8_t(*in.world->source_a6)), 0);
    s.velocity_44[0] = s16(uint32_t(int32_t(s.velocity_44[0]) + s.accel_4c));
    uint32_t w = 0; if (!b.wall(w)) return false;
    if (!b.floor()) return false;
    if (w & 0xf00u) { s.heading_56 = uint16_t((s.heading_56 + 0x800u) & 0xfffu); b.restore(); }
    const int32_t d = b.rise();
    if (d >= 0x21) {
        b.restore(); s.action_09 = 4; s.phase_0a = 0; s.aux_0b = 0; s.animation_ac = s.previous_animation_ad;
        return true;
    }
    if (d < 0 && -d > 0x20) { s.heading_56 = uint16_t((s.heading_56 + 0x800u) & 0xfffu); b.restore(); }
    if (!in.work_80) { b.failure = Status::NeedsWork80; return false; }
    if (*in.work_80 < 0x1E848u) {
        bool ok = false; if (!b.cone(0x40, 0x400, ok)) return false;
        if (ok) { s.action_09 = 5; s.phase_0a = 0; s.aux_0b = 0; s.animation_ac = s.previous_animation_ad; return true; }
    }
    if (!(s.flags_348 & 0x10u)) s.flags_348 = uint16_t((s.flags_348 & 0xffdfu) | 1u);
    return true;
}

bool action3(Body& b) noexcept {                                       // 0x801053B8
    auto& s = b.s;
    switch (s.phase_0a) {
    case 0:
        s.phase_0a = 1; s.animation_ac = 7; s.timer_34c = 0xC; s.attack_350 = 0;
        s.detection_phase_373 = 0; b.r.reset_detection = true;          // 0x80036470
        [[fallthrough]];
    case 1: {
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c >= 0) { if (s.timer_34c > 2) return b.step(int16_t(-0x50)); return true; }
        s.phase_0a = 2; s.animation_ac = uint8_t((s.random_34a & 1u) + 0xC); s.timer_34c = 6; s.flags_348 |= 0x20u;
        [[fallthrough]];
    }
    case 2: {
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c >= 0) return true;
        s.phase_0a = 3; s.animation_ac = uint8_t(s.animation_ac + 2); s.timer_34c = int16_t((s.random_34a & 7u) + 8u);
        bool ok = false; if (!b.cone(0x400, 0x800, ok)) return false;
        s.timer_34e = ok ? int16_t((s.random_34a & 3u) + 1u) : int16_t((s.random_34a & 7u) + 3u);
        [[fallthrough]];
    }
    case 3:
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c >= 0) { if (s.flags_348 & 0x10u) s.timer_34e = 0; return true; }
        s.timer_34e = dec(s.timer_34e);
        if (s.timer_34e >= 0) {
            s.animation_ac ^= 1u; s.timer_34c = int16_t((s.random_34a & 7u) + 7u); return true;
        }
        s.phase_0a = 4; s.animation_ac = uint8_t(s.animation_ac + 2); s.timer_34c = 0xD;
        [[fallthrough]];
    case 4:
        if (!b.step(0x33)) return false;
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c < 0) s.flags_348 |= 1u;
        return true;
    default:
        return true;
    }
}

bool action4(Body& b) noexcept {                                       // 0x80105740
    auto& s = b.s;
    const auto start = [&] {
        s.phase_0a = 1; s.animation_ac = 4;
        if (!(s.flags_348 & 0x40u)) {
            s.velocity_44[0] = 0x200; s.velocity_44[1] = int16_t(-0x100); s.accel_4e = 0x40;
            s.heading_56 &= 0xfffu;
        } else {                                                        // the dormant drop-in
            s.velocity_44[0] = int16_t((s.random_34a & 3u) << 7);
            s.velocity_44[1] = s16(uint32_t(1 - int32_t(s.random_34a & 0x30u)) * 0x10u);
            s.accel_4e = int16_t((((s.random_34a >> 3) & 6u) + 1u) * 0x10u);
            s.flags_348 &= 0xffbfu;
        }
        s.timer_34c = 0x10; s.timer_34e = 0; s.attack_350 = 0;
        s.flags_348 &= 0xffd7u;
    };
    switch (s.phase_0a) {
    case 1: break;
    case 0: start(); [[fallthrough]];
    case 2:
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c < 0) {
            s.phase_0a = 1; s.velocity_44[0] = 0x88; s.velocity_44[1] = int16_t(-0xF0); s.accel_4e = 0x30;
        }
        break;
    case 0xFF:
        if (!b.in.wake_flag_set) return true;
        s.work_00 |= 2u; s.flags_348 |= 6u;
        start();
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c < 0) { s.phase_0a = 1; s.velocity_44[0] = 0x88; s.velocity_44[1] = int16_t(-0xF0); s.accel_4e = 0x30; }
        break;
    default:
        return true;
    }
    b.move(s.velocity_44[0], s.velocity_44[1]);
    s.velocity_44[1] = s16(uint32_t(int32_t(s.velocity_44[1]) + s.accel_4e));
    uint32_t w = 0; if (!b.wall(w)) return false;
    if (w & 0xf00u) s.velocity_44[0] = 0;
    if (w & 0x1000u) {
        s.timer_34e = s16(uint32_t(uint16_t(s.timer_34e)) + 1u);
        if (s.timer_34e == 1) {
            s.phase_0a = 2; s.animation_ac = 5; s.velocity_44[0] = 0xF0; s.velocity_44[1] = 0; s.accel_4e = 0;
            s.timer_34c = 2;
        } else s.flags_348 |= 9u;
    }
    return true;
}

bool action5(Body& b) noexcept {                                       // 0x80105974
    auto& s = b.s; const auto& in = b.in;
    if (s.packet_35c & 0x2000000u) s.attack_350 = 0;
    switch (s.phase_0a) {
    case 0:
        if (!in.record) { b.failure = Status::NeedsRecord; return false; }
        s.phase_0a = 1; s.animation_ac = 7; s.timer_34c = 9; s.flags_348 &= 0xffdfu;
        s.attack_350 = in.record->byte0a_attack;
        [[fallthrough]];
    case 1:
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c >= 0) return b.step(int16_t(-0x50));
        s.phase_0a = 2; s.animation_ac = 8; s.timer_34c = 0xB;
        [[fallthrough]];
    case 2: {
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c >= 0) return true;
        s.phase_0a = 3; s.animation_ac = 9; s.timer_34c = int16_t(-3);
        b.move(0x640, int16_t(-0x5A0));
        uint32_t w = 0; if (!b.wall(w)) return false;
        return b.sound(0xBA);
    }
    case 3: {
        s.timer_34c = s16(uint32_t(uint16_t(s.timer_34c)) + 1u);
        const int32_t t = s.timer_34c;
        s.raw_14[1] = s16(uint32_t(int32_t(s.raw_14[1]) + (t < 0 ? -4 * t * t : 4 * t * t)));
        s.fixed_1c[1] = s32(uint32_t(int32_t(s.raw_14[1])) << 16);
        b.move(0x370, 0);
        uint32_t w = 0; if (!b.wall(w)) return false;
        if (w & 0xf00u) s.velocity_44[0] = 0;
        if (w & 0x1000u) { s.phase_0a = 4; s.animation_ac = 10; s.timer_34c = 10; s.attack_350 = 0; }
        if (s.packet_35c & 0x2000000u) {
            s.action_09 = 4; s.phase_0a = 0; s.aux_0b = 0;
            s.heading_56 = uint16_t((s.heading_56 + 0x800u) & 0xfffu); s.attack_350 = 0;
        }
        return true;
    }
    case 4:
        if (!b.step(0xA0)) return false;
        s.timer_34c = dec(s.timer_34c);
        if (s.timer_34c < 1) s.flags_348 |= 1u;
        return true;
    default:
        return true;
    }
}

// 0x80105DEC.
bool hit(Body& b) noexcept {
    auto& s = b.s; const auto& in = b.in;
    uint32_t damage = s.packet_35c & 0xffffffu;
    if (damage == 0) return true;
    if ((s.packet_35c & 0x38000000u) == 0) return b.sound(0xA3);
    if (!in.record) { b.failure = Status::NeedsRecord; return false; }
    const int16_t before = s.health_6e.value_or(0);
    const int32_t after = int32_t(uint16_t(before)) - int32_t(damage);
    s.health_6e = s16(uint32_t(after)); s.previous_health_b2 = before;
    if (int32_t(uint32_t(after) << 16) < 1) {
        s.flags_348 = uint16_t((s.flags_348 & 0xfffdu) | 0x88u);
        damage = in.record->byte09_stagger;
    }
    const bool strong = (s.packet_35c & 0x20000000u) != 0;
    uint16_t id = 0;
    if (!(s.flags_348 & 8u)) {
        id = (strong || int16_t(in.record->byte09_stagger) <= s.stagger_352) ? 0xA2 : 0xA0;
    } else {
        s.stagger_352 = s16(uint32_t(uint16_t(s.stagger_352)) + damage);
        if (strong || int32_t(in.record->byte09_stagger) <= s.stagger_352) {
            if (!b.sound(0xA2)) return false;
            // Knockback away from the hit (0x80031198 to +0x364).
            if (!(s.hit_word_36a & 1u)) {
                const auto a = source_heading_80031198(in.hit_reference_801000cc[0], in.hit_reference_801000cc[2],
                                                       s.hit_point_364[0], s.hit_point_364[2], in.angle_steps);
                if (!a) { b.failure = Status::NeedsAngle; return false; }
                s.heading_56 = uint16_t(*a);
            } else {
                const auto a = source_heading_80031198(s.raw_14[0], s.raw_14[2], s.hit_point_364[0], s.hit_point_364[2], in.angle_steps);
                if (!a) { b.failure = Status::NeedsAngle; return false; }
                const uint16_t h = uint16_t(*a);
                s.heading_56 = ((((int32_t(int16_t(s.heading_56)) - int16_t(h)) + 0x400u) & 0xfffu) > 0x7ffu)
                    ? uint16_t((h + 0x800u) & 0xfffu) : h;
            }
            s.action_09 = 4; s.phase_0a = 0; s.aux_0b = 0; s.stagger_352 = 0;
            s.shade_15e = 0x7fff;
            return true;
        }
        id = 0xA0;
    }
    if (!b.sound(id)) return false;
    s.shade_15e = 0x7fff;
    return true;
}

// 0x80105D10.
void select_action(Class20_19State& s) noexcept {
    static constexpr std::array<uint8_t,4> kWeights{{4, 6, 3, 3}};   // 0x8011BF5C (CANDIDATE: overlay data)
    const int r = int(s.random_34a & 0xfu);
    int sum = 0, pick = 3;
    for (int i = 0; i < 4; ++i) { sum += kWeights[size_t(i)]; if (r < sum) { pick = i; break; } }
    s.action_09 = 1;
    if (pick == 0) { s.action_09 = 3; return; }
    // Pick 1 stores 0 and then 0x10 through the branch delay slot (0x80105DD0).
    s.turn_66 = pick == 3 ? int16_t(-0x10) : int16_t(0x10);
}
} // namespace

Class20_19Result step_class20_19_80104864(Class20_19State& state, const Class20_19Inputs& in) noexcept {
    if (state.released || state.state_08 == 2) return hold(Status::Released);
    if (state.state_08 > 2) return hold(Status::UnknownState);
    if (!in.random_sample) return hold(Status::NeedsRandomSample);
    if (!in.world || !in.world->map || !in.world->references) return hold(Status::NeedsWorld);
    auto n = state;
    Class20_19Result r;
    Body b{n, in, r, in.world->state74};
    n.previous_animation_ad = n.animation_ac;
    n.previous_raw_2c = n.raw_14; n.previous_fixed_34 = n.fixed_1c;
    n.shade_15e = 0x4210;
    n.random_34a = *in.random_sample;
    if (n.state_08 == 0) {                                              // 0x80104A08
        if (!in.record) return hold(Status::NeedsRecord);
        n.work_00 |= 2u;
        n.wake_flag_37c = uint16_t(n.raw_14[3]); n.raw_14[3] = 0;   // +0x1A -> +0x37C
        n.flags_348 = 0xE; n.timer_34c = 0; n.timer_34e = 0; n.attack_350 = 0; n.stagger_352 = 0; n.packet_35c = 0;
        n.defeat_timer_37e = 4;
        n.health_6e = int16_t(in.record->byte08_hp); n.previous_health_b2 = int16_t(in.record->byte08_hp);
        n.velocity_44 = {}; n.turn_66 = 0;
        const bool dormant = n.parameter_03 == 1 && !in.wake_flag_set;
        if (!dormant) {
            if (!in.floor_sample_80048e6c) return hold(Status::NeedsFloor);
            n.raw_14[1] = s16(*in.floor_sample_80048e6c);
            n.fixed_1c[1] = s32(uint32_t(*in.floor_sample_80048e6c) << 16);
        }
        n.state_08 = 1; n.aux_0b = 0; n.animation_ac = 0;
        if (n.parameter_03 == 0) { n.action_09 = 0; n.phase_0a = 0; }
        else if (dormant) {
            n.action_09 = 4; n.phase_0a = 0xFF; n.work_00 &= 0xfdu;
            n.flags_348 = uint16_t((n.flags_348 & 0xfff9u) | 0x40u);
        } else { n.action_09 = 1; n.phase_0a = 0; }
        n.shade_15e = 0x4210;
        r.animation = Class20_19Animation::Restart;
    } else {                                                            // 0x80104BE4
        if (!b.detected()) return hold(b.failure);
        if (n.flags_348 & 2u) { if (!hit(b)) return hold(b.failure); }
        bool ok = false;
        switch (n.action_09) {
        case 0: ok = action0(b); break;
        case 1: ok = action1(b); break;
        case 2: ok = action2(b); break;
        case 3: ok = action3(b); break;
        case 4: ok = action4(b); break;
        case 5: ok = action5(b); break;
        default: return hold(Status::UnknownAction);
        }
        if (!ok) return hold(b.failure);
        if (n.flags_348 & 1u) {
            const uint8_t a = n.action_09;
            if (a == 4 || a == 5) n.action_09 = 3;
            else if (a == 3 && (n.flags_348 & 0x10u)) n.action_09 = 2;
            else select_action(n);
            n.phase_0a = 0; n.aux_0b = 0;
            n.flags_348 &= 0xfffeu;
        }
        if (n.stagger_352) n.stagger_352 = dec(n.stagger_352);
        if (n.flags_348 & 0x80u) {
            n.defeat_timer_37e = dec(n.defeat_timer_37e);
            if (n.defeat_timer_37e < 0) {
                if (!b.sound(0xA1)) return hold(b.failure);
                r.defeat_effect_128_65 = true; r.defeat_reward_80047148 = true; r.released_80031824 = true;
                n.released = true;
            }
        }
        r.animation = n.previous_animation_ad == n.animation_ac ? Class20_19Animation::Advance : Class20_19Animation::Restart;
    }
    if ((n.work_00 & 1u) && (n.flags_348 & 4u) && !n.released) {
        r.register_proxy = true; r.proxy_word_18 = uint32_t(n.attack_350) | 0x04880000u;
    }
    n.packet_35c = 0;
    r.state74_after = b.state74;
    state = n;
    return r;
}

} // namespace l3
