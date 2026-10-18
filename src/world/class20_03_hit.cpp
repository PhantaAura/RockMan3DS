#include "legends3ds/class20_03_hit.h"

namespace l3 {
namespace {
int16_t signed16(uint16_t value) noexcept {
    return value < 0x8000u ? static_cast<int16_t>(value)
        : static_cast<int16_t>(static_cast<int32_t>(value) - 65536);
}
Class20_03HitResult hold(Class20_03HitStatus status) noexcept {
    Class20_03HitResult result; result.status = status; return result;
}
std::array<uint32_t,2> pointed_words(const std::array<int16_t,4>& raw) noexcept {
    return {{uint32_t(uint16_t(raw[0])) | (uint32_t(uint16_t(raw[1])) << 16),
             uint32_t(uint16_t(raw[2])) | (uint32_t(uint16_t(raw[3])) << 16)}};
}
} // namespace

Class20_03HitResult class20_03_consume_hit_801041b0(
    Class20_03CallbackState& state, const Class20_03HitInputs& inputs) noexcept {
    using Status = Class20_03HitStatus;
    if (!inputs.packet_33c || *inputs.packet_33c != state.packet_33c) return hold(Status::NeedsPacket);
    const uint32_t packet = *inputs.packet_33c;
    const uint32_t low24 = packet & 0x00ffffffu;
    if (low24 == 0) return {};
    auto next = state;
    auto& behavior = next.behavior;
    uint16_t cue = 163;
    if (packet & 0x38000000u) {
        if (!next.health_6e) return hold(Status::NeedsHealth);
        const int16_t previous_health = *next.health_6e;
        const int16_t health = signed16(static_cast<uint16_t>(
            uint32_t(uint16_t(previous_health)) - low24));
        next.health_6e = health;
        next.previous_health_b2 = previous_health;
        uint32_t increment = low24;
        const auto byte09 = [&]() -> std::optional<uint8_t> {
            if (!inputs.selected_record ||
                inputs.selected_record->actor_variant_32e != next.variant_32e)
                return std::nullopt;
            return inputs.selected_record->unsigned_byte09;
        };
        if (health <= 0) {
            const auto selected = byte09();
            if (!selected) return hold(Status::NeedsCombatProfile);
            behavior.flags_328 = static_cast<uint16_t>((behavior.flags_328 & 0xfffdu) | 0x48u);
            increment = *selected;
        }
        if (behavior.flags_328 & 8u) {
            next.work_332 = signed16(static_cast<uint16_t>(uint32_t(uint16_t(next.work_332)) + increment));
            bool below_selected = false;
            if (!(packet & 0x20000000u)) {
                const auto selected = byte09();
                if (!selected) return hold(Status::NeedsCombatProfile);
                below_selected = next.work_332 < *selected;
            }
            if (below_selected) {
                if (next.variant_32e == 0) {
                    if (behavior.action_09 != 6) { behavior.phase_0a = 0; next.aux_0b = 0; }
                    behavior.action_09 = 6;
                }
                cue = 160;
            } else if (behavior.action_09 == 5) {
                cue = 160;
            } else {
                if (!inputs.hit_word_34a) return hold(Status::NeedsHitContext);
                if (!inputs.angle_80031198) return hold(Status::NeedsAngle);
                const uint16_t angle = static_cast<uint16_t>(*inputs.angle_80031198);
                behavior.work_36c = signed16(angle);
                const bool absolute_origin = (*inputs.hit_word_34a & 1u) != 0;
                const int32_t delta = absolute_origin
                    ? int32_t(signed16(behavior.heading_56)) - int32_t(signed16(angle))
                    : int32_t(signed16(angle)) - int32_t(signed16(behavior.heading_56));
                const bool first_half = ((delta + 1024) & 4095) < 2048;
                behavior.action_09 = absolute_origin ? (first_half ? 7 : 8) : (first_half ? 8 : 7);
                behavior.heading_56 = first_half ? angle : static_cast<uint16_t>((uint32_t(angle) + 2048u) & 4095u);
                behavior.phase_0a = 0; next.aux_0b = 0; next.work_332 = 0;
                cue = 162;
            }
        } else {
            bool below_selected = false;
            if (!(packet & 0x20000000u)) {
                const auto selected = byte09();
                if (!selected) return hold(Status::NeedsCombatProfile);
                below_selected = next.work_332 < *selected;
            }
            cue = below_selected || behavior.action_09 == 5 ? 160 : 162;
        }
        next.work_15e = 32767;
        behavior.parameter_03 &= 0xbfu;
    }
    if (!inputs.sound_consumer_ready) return hold(Status::NeedsSoundConsumer);
    Class20_03HitResult result;
    result.sound_8001a050 = cue;
    result.sound_pointed_words = pointed_words(state.raw_14);
    state = next;
    return result;
}
} // namespace l3
