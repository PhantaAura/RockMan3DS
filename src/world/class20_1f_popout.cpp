#include "legends3ds/class20_1f_popout.h"
#include <limits>

namespace l3 {
namespace {
using Status = Class20_1FStatus;
int32_t s32(uint32_t v) noexcept {
    return v <= uint32_t(std::numeric_limits<int32_t>::max()) ? int32_t(v) : int32_t(int64_t(v) - 0x100000000LL);
}
int16_t s16(uint32_t v) noexcept { v &= 0xFFFFu; return v < 0x8000u ? int16_t(v) : int16_t(int32_t(v) - 65536); }
Class20_1FResult hold(Status s) noexcept { Class20_1FResult r; r.status = s; return r; }
// 0x8002FEA4(work, forward, 0, 0): heading-relative fixed add, then 0x80030150.
void move_forward(Class20_1FState& s, const ReferenceTables& refs, int16_t forward) noexcept {
    const auto d = reference_player_motion_delta(refs, s.heading_56, forward, 0, 0);
    const std::array<int32_t, 3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
    for (size_t a = 0; a < 3; ++a) {
        s.fixed_1c[a] = s32(uint32_t(s.fixed_1c[a]) + uint32_t(delta[a]));
        s.raw_14[a] = reference_fixed_to_short_80030150(s.fixed_1c[a]);
    }
}
}

Class20_1FResult step_class20_1f_801062ec(Class20_1FState& state, const Class20_1FInputs& in) noexcept {
    if (state.state_08 > 2) return hold(Status::UnknownState);
    if (state.state_08 == 2) return hold(Status::Released);      // 0x801066B8 -> 0x80031824
    if (!in.random_sample) return hold(Status::NeedsRandomSample);
    if (!in.references) return hold(Status::NeedsReference);
    auto n = state;
    Class20_1FResult r;
    // 0x801062EC prologue.
    n.previous_animation_ad = n.animation_ac;
    n.previous_raw_2c = n.raw_14; n.previous_fixed_34 = n.fixed_1c;
    n.random_34a = *in.random_sample;
    if (n.state_08 == 0) {
        // 0x8010648C. Presentation-only stores (scale 0x200, matrix +37C, shade)
        // and the 0x8002F9C4 resource lookup belong to the actor binding.
        n.work_00 |= 2;
        n.flags_348 = 0x2C;
        n.timer_34c = 0; n.work_350 = 0; n.work_352 = 0; n.scale_z_384 = 0;
        n.health_6e = in.initial_health;
        n.state_08 = 1; n.action_09 = 0; n.phase_0a = 0;
    } else {
        // 0x80106584.
        if (n.flags_348 & 0x40u) return hold(Status::NeedsDetection);  // 0x8003648C not admitted here
        n.flags_348 &= 0xFFEFu;
        if (n.flags_348 & 2u) {                                        // 0x80106800
            const uint32_t damage = in.packet_35c & 0xFFFFFFu;
            if (damage) {
                if (!in.sound_consumer_ready) return hold(Status::NeedsSoundConsumer);
                r.sound_8001a050 = 0xA3;
                if (in.packet_35c & 0x18000000u) {
                    const int16_t before = n.health_6e.value_or(0);
                    const int32_t after = int32_t(uint16_t(before)) - int32_t(damage);
                    n.health_6e = s16(uint32_t(after));
                    if (int32_t(uint32_t(after) << 16) < 1) {
                        n.action_09 = 1; n.phase_0a = 0;
                        n.flags_348 = uint16_t((n.flags_348 & 0xFFFDu) | 4u);
                    }
                }
            }
        }
        if (n.action_09 == 0) {                                        // 0x801066D8
            if (n.phase_0a == 0) { n.phase_0a = 1; n.animation_ac = 0; n.work_350 = 0; }
            if (n.phase_0a == 1) {
                if (!in.work_80) return hold(Status::NeedsWork80);
                if (*in.work_80 < 0x4E201u) n.flags_348 |= 1u;
            }
        } else if (n.action_09 == 1) {                                 // 0x8010673C
            if (n.phase_0a == 0) { n.phase_0a = 1; n.animation_ac = 0; n.timer_34c = 8; n.work_350 = 0; }
            if (n.phase_0a == 1) {
                n.timer_34c = s16(uint32_t(uint16_t(n.timer_34c)) - 1u);
                if (n.timer_34c < 0) { move_forward(n, *in.references, 0x200); n.scale_z_384 += 0x800; n.flags_348 |= 1u; }
                else { move_forward(n, *in.references, 0x40); n.scale_z_384 += 0x100; }
            }
        } else return hold(Status::UnknownState);
        if (n.work_352) n.work_352 = s16(uint32_t(uint16_t(n.work_352)) - 1u);
        if (n.flags_348 & 1u) {
            if (n.action_09 == 0) n.action_09 = 1;
            else if (n.action_09 == 1) {
                n.morph_to_class20_03 = true;
                n.state_08 = 0; n.action_09 = 0;
            }
            n.phase_0a = 0;
            n.flags_348 &= 0xFFFEu;
        }
    }
    r.animation = n.previous_animation_ad == n.animation_ac ? Class20_1FAnimation::Advance : Class20_1FAnimation::Restart;
    r.proxy_800341bc = (n.flags_348 & 8u) != 0;
    state = n;
    return r;
}

} // namespace l3
