#include "legends3ds/class20_03_late_actions.h"
#include <limits>

namespace l3 {
namespace {
using Status = Class20_03LateActionStatus;
int16_t s16(uint32_t v) noexcept {
    v &= 0xffffu;
    return v < 0x8000u ? int16_t(v) : int16_t(int32_t(v) - 65536);
}
int32_t s32(uint32_t v) noexcept {
    return v <= uint32_t(INT32_MAX) ? int32_t(v) : int32_t(int64_t(v) - 0x100000000LL);
}
Class20_03LateActionResult hold(Status s) noexcept {
    Class20_03LateActionResult r; r.status = s; return r;
}
std::array<uint32_t,2> words(const std::array<int16_t,4>& raw) noexcept {
    return {{uint32_t(uint16_t(raw[0])) | (uint32_t(uint16_t(raw[1])) << 16),
             uint32_t(uint16_t(raw[2])) | (uint32_t(uint16_t(raw[3])) << 16)}};
}
bool push_sound(Class20_03LateActionResult& r, bool ready, uint16_t id,
                const std::array<int16_t,4>& raw) noexcept {
    if (!ready || r.sound_count >= r.sounds.size()) return false;
    r.sounds[r.sound_count] = id; r.sound_words[r.sound_count] = words(raw); ++r.sound_count;
    return true;
}
bool neighbors(const RuntimeMap& map, size_t area_index, int16_t x, int16_t z) noexcept {
    if (area_index >= map.areas.size()) return false;
    const auto& area = map.areas[area_index];
    const int32_t nx = (uint16_t(x) & 511u) < 257u ? -512 : 512;
    const int32_t nz = (uint16_t(z) & 511u) < 257u ? -512 : 512;
    const std::array<std::array<int32_t,2>,4> delta{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
    const auto shift9 = [](int32_t v) { return v >= 0 ? v / 512 : -((-v + 511) / 512); };
    for (const auto& o : delta) {
        const auto gx = shift9(int32_t(x) + o[0] + 32768) - area.start_x;
        const auto gz = shift9(int32_t(z) + o[1] + 32768) - area.start_z;
        if (gx < 0 || gz < 0 || gx >= area.width || gz >= area.height) return false;
        const uint64_t local = uint64_t(gz) * area.width + uint64_t(gx);
        if (local >= area.cell_count || uint64_t(area.cell_first) + local >= map.cells.size()) return false;
    }
    return true;
}
ActorContactWork contact_work(const Class20_03CallbackState& s, const Class20_03MotionWorld& w) noexcept {
    ActorContactWork c;
    c.raw = s.raw_14; c.fixed = s.fixed_1c;
    c.previous_raw = s.previous_raw_2c; c.previous_fixed = s.previous_fixed_34;
    c.velocity = s.velocity_44; c.object_class = w.object_class; c.state74 = w.state74;
    return c;
}
void take_contact(Class20_03CallbackState& s, const ActorContactWork& c, uint8_t& state74) noexcept {
    s.raw_14 = c.raw; s.fixed_1c = c.fixed; s.velocity_44 = c.velocity; state74 = c.state74;
}
// 0x8002FEA4(work, forward, vertical, strafe): heading-relative fixed add,
// then the 0x80030150 raw conversion of the three lanes.
void move_8002fea4(Class20_03CallbackState& s, const ReferenceTables& refs,
                   int16_t forward, int16_t vertical) noexcept {
    const auto d = reference_player_motion_delta(refs, s.behavior.heading_56, forward, vertical, 0);
    const std::array<int32_t,3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
    for (size_t a = 0; a < 3; ++a) {
        s.fixed_1c[a] = s32(uint32_t(s.fixed_1c[a]) + uint32_t(delta[a]));
        s.raw_14[a] = reference_fixed_to_short_80030150(s.fixed_1c[a]);
    }
}
// 0x8004B414 on the current work. Returns false when the contact is outside
// the admitted geometry; the caller then holds the whole body.
bool wall_8004b414(Class20_03CallbackState& s, const Class20_03MotionWorld& w,
                   uint8_t& state74, uint32_t& word, Status& failure) noexcept {
    if (!neighbors(*w.map, w.area_index, s.raw_14[0], s.raw_14[2])) { failure = Status::UnsupportedBoundary; return false; }
    auto c = contact_work(s, w); c.state74 = state74;
    const auto wall = query_actor_wall_contact_8004b6ac(*w.map, w.area_index, c, w.bounds);
    if (!apply_actor_contact_result_8004b414(c, wall)) { failure = Status::UnsupportedWall; return false; }
    take_contact(s, c, state74);
    word = uint32_t(wall.result);
    s.work_370 = word;
    return true;
}
bool floor_80048e6c(const Class20_03CallbackState& s, const Class20_03MotionWorld& w,
                    uint8_t state74, int32_t& floor, Status& failure) noexcept {
    if (!neighbors(*w.map, w.area_index, s.raw_14[0], s.raw_14[2])) { failure = Status::UnsupportedBoundary; return false; }
    auto c = contact_work(s, w); c.state74 = state74;
    const auto f = query_actor_floor_contact_80048ea4(*w.map, w.area_index, c, w.bounds);
    if (f.status != RuntimeContactStatus::SourceBackedPartial) { failure = Status::UnsupportedFloor; return false; }
    floor = f.result;
    return true;
}
int16_t decrement(int16_t v) noexcept { return s16(uint32_t(uint16_t(v)) - 1u); }

Class20_03LateActionResult action5(Class20_03CallbackState& state, const Class20_03LateActionInputs& in) noexcept {
    auto n = state; auto& b = n.behavior; const auto& w = *in.world;
    Class20_03LateActionResult r; uint8_t state74 = w.state74; Status failure{};
    if (n.packet_33c & 0x02000000u) b.work_330 = 0;
    uint8_t phase = b.phase_0a;
    if (phase == 0) {
        if (!in.combat_record_byte_a) return hold(Status::NeedsCombatProfile);
        b.phase_0a = 1; b.animation_ac = 4;
        n.velocity_44[0] = s16(uint32_t(-reference_rsin_8007a060(*w.references, s16(b.heading_56))) >> 7);
        n.velocity_44[1] = 32;
        n.work_4e = 16;
        n.velocity_44[2] = s16(uint32_t(-reference_rcos_8007a12c(*w.references, s16(b.heading_56))) >> 7);
        b.timer_32c = 115;
        b.flags_328 &= 0xfff7u;
        b.work_330 = *in.combat_record_byte_a;
        phase = 1;
    }
    if (phase == 1) {
        if (!w.source_a6) return hold(Status::NeedsAnimationCue);
        if (*w.source_a6 == 1 && !push_sound(r, in.sound_consumer_ready, 181, n.raw_14))
            return hold(Status::NeedsSoundConsumer);
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c < 0 || (n.packet_33c & 0x02000000u)) {
            b.work_330 = 0; b.phase_0a = 2; b.timer_32c = 2;
            phase = 2;
        } else {
            if (!n.health_6e) return hold(Status::NeedsCombatProfile);
            if (*n.health_6e <= 0) {
                b.flags_328 |= 1u;
            } else {
                if (b.timer_32c < 100) {
                    n.fixed_1c[0] = s32(uint32_t(n.fixed_1c[0]) + (uint32_t(uint16_t(n.velocity_44[0])) << 16));
                    const int16_t old_y = n.raw_14[1];
                    int32_t floor = 0;
                    if (!floor_80048e6c(n, w, state74, floor, failure)) return hold(failure);
                    if (int32_t(old_y) != floor) {
                        n.fixed_1c[1] = s32(uint32_t(n.fixed_1c[1]) + (uint32_t(uint16_t(n.velocity_44[1])) << 16));
                        n.velocity_44[1] = s16(uint32_t(uint16_t(n.velocity_44[1])) + uint16_t(n.work_4e));
                    } else {
                        n.velocity_44[1] = 32;
                    }
                    n.fixed_1c[2] = s32(uint32_t(n.fixed_1c[2]) + (uint32_t(uint16_t(n.velocity_44[2])) << 16));
                    for (size_t a = 0; a < 3; ++a) n.raw_14[a] = reference_fixed_to_short_80030150(n.fixed_1c[a]);
                }
                uint32_t contact = 0;
                if (!wall_8004b414(n, w, state74, contact, failure)) return hold(failure);
                if (contact) {
                    if ((contact & 0xf00u) && !push_sound(r, in.sound_consumer_ready, 222, n.raw_14))
                        return hold(Status::NeedsSoundConsumer);
                    const auto mag = [](int16_t v) { return int16_t(v < 0 ? -v : v); };
                    if (contact & 0x100u) n.velocity_44[0] = int16_t(-mag(n.velocity_44[0]));
                    else if (contact & 0x200u) n.velocity_44[0] = mag(n.velocity_44[0]);
                    if (contact & 0x400u) n.velocity_44[2] = int16_t(-mag(n.velocity_44[2]));
                    else if (contact & 0x800u) n.velocity_44[2] = mag(n.velocity_44[2]);
                }
            }
            r.state74_after = state74; state = n; return r;
        }
    }
    if (phase == 2) {
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c < 0) {
            n.health_6e = 0;
            n.defeat_timer_360 = 0;
            b.flags_328 |= 0x41u;
        }
    }
    r.state74_after = state74; state = n; return r;
}

Class20_03LateActionResult action6(Class20_03CallbackState& state, const Class20_03LateActionInputs& in) noexcept {
    auto n = state; auto& b = n.behavior; const auto& w = *in.world;
    Class20_03LateActionResult r; uint8_t state74 = w.state74; Status failure{};
    if (b.phase_0a > 2) { r.state74_after = state74; return r; }
    if (b.phase_0a == 0) {
        b.phase_0a = 1; b.animation_ac = 6; b.timer_32c = 10; b.work_330 = 0;
    }
    if (b.phase_0a == 1) {
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c < 0) b.phase_0a = 2;
        else n.velocity_44[1] = s16(uint32_t(10 - int32_t(b.timer_32c)));
    }
    move_8002fea4(n, *w.references, 0, s16(uint32_t(int32_t(n.velocity_44[1])) << 7));
    uint32_t contact = 0;
    if (!wall_8004b414(n, w, state74, contact, failure)) return hold(failure);
    if ((contact & 0x1000u) && b.timer_32c < 0) b.flags_328 |= 1u;
    r.state74_after = state74; state = n; return r;
}

Class20_03LateActionResult action10(Class20_03CallbackState& state, const Class20_03LateActionInputs& in) noexcept {
    auto n = state; auto& b = n.behavior; const auto& w = *in.world;
    Class20_03LateActionResult r; uint8_t state74 = w.state74; Status failure{};
    if (b.phase_0a > 1) { r.state74_after = state74; return r; }
    if (b.phase_0a == 0) {
        if (!in.combat_record_byte_a) return hold(Status::NeedsCombatProfile);
        b.phase_0a = 1; b.animation_ac = 4; b.timer_32c = 45;
        b.work_330 = *in.combat_record_byte_a;
    }
    if (!w.source_a6) return hold(Status::NeedsAnimationCue);
    if (*w.source_a6 == 1 && !push_sound(r, in.sound_consumer_ready, 181, n.raw_14))
        return hold(Status::NeedsSoundConsumer);
    b.timer_32c = decrement(b.timer_32c);
    if (b.timer_32c <= 0) { n.work_35e = 45; b.flags_328 |= 1u; }
    if (!in.target_heading_80031198) return hold(Status::NeedsTargetHeading);
    b.work_36c = class20_03_turn_step(*in.target_heading_80031198, b.heading_56, 32);
    b.heading_56 = uint16_t((int32_t(b.heading_56) + b.work_36c) & 4095);
    move_8002fea4(n, *w.references, 52, 0);
    uint32_t contact = 0;
    if (!wall_8004b414(n, w, state74, contact, failure)) return hold(failure);
    int32_t floor = 0;
    if (!floor_80048e6c(n, w, state74, floor, failure)) return hold(failure);
    n.raw_14[1] = s16(uint32_t(floor));
    n.fixed_1c[1] = s32(uint32_t(floor) << 16);
    const int32_t dy = int32_t(n.raw_14[1]) - n.previous_raw_2c[1];
    if (dy >= 33 || dy <= -33) { n.raw_14 = n.previous_raw_2c; n.fixed_1c = n.previous_fixed_34; }
    r.state74_after = state74; state = n; return r;
}
} // namespace

bool class20_03_is_late_action(uint8_t action) noexcept {
    return action == 5 || action == 6 || action == 10;
}

Class20_03LateActionResult class20_03_step_late_action(
    Class20_03CallbackState& state, const Class20_03LateActionInputs& in) noexcept {
    if (!class20_03_is_late_action(state.behavior.action_09)) return hold(Status::UnknownAction);
    if (!in.world || !in.world->map || !in.world->references) return hold(Status::NeedsWorld);
    switch (state.behavior.action_09) {
    case 5: return action5(state, in);
    case 6: return action6(state, in);
    default: return action10(state, in);
    }
}

} // namespace l3
