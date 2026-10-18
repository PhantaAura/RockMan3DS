#include "legends3ds/class20_03_action3.h"

namespace l3 {
namespace {
int16_t signed16(uint16_t value) noexcept {
    return value < 0x8000u ? static_cast<int16_t>(value)
        : static_cast<int16_t>(static_cast<int32_t>(value) - 65536);
}
int16_t decrement(int16_t value) noexcept {
    return signed16(static_cast<uint16_t>(static_cast<uint16_t>(value) - 1u));
}
Class20_03Action3Result hold(Class20_03Action3Status status) noexcept {
    Class20_03Action3Result result;
    result.status = status;
    return result;
}
}

Class20_03Action3Result class20_03_step_action3(
    Class20_03CallbackState& state, const Class20_03Action3Inputs& input) noexcept {
    using Status = Class20_03Action3Status;
    if (state.behavior.action_09 != 3) return hold(Status::UnknownAction);
    if (!input.packet_33c) return hold(Status::NeedsPacket);
    if (*input.packet_33c != state.packet_33c) return hold(Status::StalePacket);
    auto next = state;
    auto& b = next.behavior;
    Class20_03Action3Result result;
    if (*input.packet_33c & 0x02000000u) b.work_330 = 0;
    uint8_t phase = b.phase_0a;
    if (phase == 0) {
        if (!input.combat_record_byte_a) return hold(Status::NeedsCombatProfile);
        b.phase_0a = 1;
        b.animation_ac = 2;
        next.work_64 = 0;
        b.timer_32c = 5;
        b.work_330 = *input.combat_record_byte_a;
        phase = 1;
    }
    if (phase == 1) {
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c < 0) {
            b.phase_0a = 2;
            b.timer_32c = 5;
            phase = 2;
        }
    }
    if (phase == 2) {
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c <= 0) {
            if (!input.sound_consumer_ready) return hold(Status::NeedsSoundConsumer);
            b.phase_0a = 3;
            result.sound_8001a050 = 180;
            if (b.flags_328 & 0x20u) {
                if (!next.source_delay_0d) return hold(Status::NeedsDelay);
                b.timer_32c = *next.source_delay_0d;
                b.flags_328 &= 0xffdfu;
            } else {
                b.timer_32c = 45;
            }
        }
    } else if (phase == 3 || phase == 4) {
        if (phase == 3 && b.timer_32c < 4) {
            b.phase_0a = 4;
            next.work_64 = 256;
        }
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c < 0) {
            b.phase_0a = 5;
            b.animation_ac = static_cast<uint8_t>(b.animation_ac + 1u);
            b.timer_32c = 10;
            b.work_330 = 0;
            phase = 5;
        } else if (*input.packet_33c & 0x02000000u) {
            b.action_09 = 6;
            b.phase_0a = 0;
            next.aux_0b = 0;
        } else {
            if (!next.work_64) return hold(Status::NeedsTurnLimit);
            if (!input.target_heading_80031198) return hold(Status::NeedsTargetHeading);
            if (!input.prepared_motion) return hold(Status::NeedsMotion);
            const auto turn = class20_03_turn_step(*input.target_heading_80031198,
                                                  b.heading_56, *next.work_64);
            const auto heading = static_cast<uint16_t>(
                (static_cast<int32_t>(b.heading_56) + turn) & 0xfff);
            const auto& prepared = *input.prepared_motion;
            if (prepared.heading_after_turn != heading) return hold(Status::StaleMotion);
            b.work_36c = turn;
            b.heading_56 = heading;
            next.angle_54[1] = heading;
            next.raw_14 = prepared.raw_14;
            next.fixed_1c = prepared.fixed_1c;
            next.work_370 = prepared.source_contact_word;
            next.raw_14[1] = signed16(static_cast<uint16_t>(prepared.source_floor_word));
            const uint32_t fixed_y = static_cast<uint32_t>(prepared.source_floor_word) << 16;
            next.fixed_1c[1] = fixed_y <= 0x7fffffffu ? static_cast<int32_t>(fixed_y)
                : static_cast<int32_t>(static_cast<int64_t>(fixed_y) - 0x100000000LL);
            const int32_t change = static_cast<int32_t>(next.raw_14[1]) - next.previous_raw_2c[1];
            if (change >= 33 || change <= -33) {
                b.action_09 = 8;
                b.phase_0a = 0;
                next.aux_0b = 0;
                b.parameter_03 |= 0x40u;
                next.raw_14 = next.previous_raw_2c;
                next.fixed_1c = next.previous_fixed_34;
                b.timer_32c = -1;
            }
            if (prepared.source_contact_word & 0xf00u) b.timer_32c = -1;
        }
    }
    if (phase == 5) {
        if (b.parameter_03 & 0x20u) {
            if (!input.release_consumer_ready) return hold(Status::NeedsReleaseConsumer);
            result.release_80031824 = true;
            next.work_00 = 0;
        }
        b.timer_32c = decrement(b.timer_32c);
        if (b.timer_32c < 0) {
            next.work_35c = 30;
            b.flags_328 |= 1u;
        }
    }
    state = next;
    return result;
}
} // namespace l3
