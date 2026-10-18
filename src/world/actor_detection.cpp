#include "legends3ds/actor_detection.h"
#include <limits>

namespace l3 {
namespace {
int16_t short_bits(uint32_t bits) noexcept {
    bits &= UINT32_C(0xffff);
    return bits <= static_cast<uint32_t>(std::numeric_limits<int16_t>::max())
        ? static_cast<int16_t>(bits)
        : static_cast<int16_t>(static_cast<int32_t>(bits) - 65536);
}
int32_t arithmetic_shift(int32_t value, unsigned shift) noexcept {
    if (value >= 0) return value / static_cast<int32_t>(UINT32_C(1) << shift);
    return -static_cast<int32_t>((-static_cast<int64_t>(value) +
        (INT64_C(1) << shift) - 1) / (INT64_C(1) << shift));
}
int16_t add_short(int16_t a, int32_t b) noexcept {
    return short_bits(static_cast<uint32_t>(static_cast<int32_t>(a)) + static_cast<uint32_t>(b));
}
bool near_player(int16_t point, int16_t player) noexcept {
    const uint32_t delta = (static_cast<uint32_t>(static_cast<int32_t>(point)) -
        static_cast<uint32_t>(static_cast<int32_t>(player)) + 100U) & UINT32_C(0xffff);
    return delta < 200;
}
ActorDetectionResult hold(ActorDetectionStatus status) noexcept {
    ActorDetectionResult result; result.status = status; return result;
}
} // namespace

ActorDetectionGlobals initialize_actor_detection_globals_8005c258() noexcept {
    return {};
}
void tick_actor_detection_globals_8005c2b8(ActorDetectionGlobals& globals) noexcept {
    if (globals.halfword_800bbd94 > 0) --globals.halfword_800bbd94;
    if (globals.halfword_800bbd96 > 0) --globals.halfword_800bbd96;
}
void set_actor_detection_countdown_8005ebb4(ActorDetectionGlobals& globals,
                                           uint16_t bits) noexcept {
    globals.halfword_800bbd96 = short_bits(bits);
}
void notify_actor_detection_8005d4c0(ActorDetectionGlobals& globals) noexcept {
    globals.byte_800bbd84 = 1;
}

std::optional<bool> actor_detection_behind_80030628(
    const std::array<int16_t, 3>& actor_raw,
    const std::array<int16_t, 3>& player_raw, uint16_t heading56,
    const ReferenceTables* reference) noexcept {
    if (reference == nullptr) return std::nullopt;
    const int32_t angle = 2048 - short_bits(heading56);
    const uint32_t index = static_cast<uint32_t>(angle < 0 ? -angle : angle) & 4095U;
    const auto& trig = reference->trig[index];
    const int32_t matrix20 = angle < 0 ? trig.sin_q12 : -static_cast<int32_t>(trig.sin_q12);
    const int16_t dx = short_bits(static_cast<uint32_t>(static_cast<int32_t>(player_raw[0])) -
        static_cast<uint32_t>(static_cast<int32_t>(actor_raw[0])));
    const int16_t dz = short_bits(static_cast<uint32_t>(static_cast<int32_t>(player_raw[2])) -
        static_cast<uint32_t>(static_cast<int32_t>(actor_raw[2])));
    // CPU rotation of the identity matrix stores signed16 entries. In the
    // original trig profile each value is within Q12 [-4096,4096]. We retain
    // the same halfword narrowing for malformed/authored table values too.
    const int64_t mac3 = static_cast<int64_t>(short_bits(static_cast<uint32_t>(matrix20))) * dx +
        static_cast<int64_t>(trig.cos_q12) * dz;
    // Arithmetic shift by12 does not change sign. MAC3 never exceeds44 bits
    // for two signed16 products, so the GTE overflow flag cannot alter it.
    return mac3 < 0;
}

ActorDetectionResult step_actor_detection_800364b0(
    ActorDetectionWork& work, const ActorDetectionInputs& input) noexcept {
    if (!input.profile) return hold(ActorDetectionStatus::NeedsProfile);
    ActorDetectionWork next = work;
    ActorDetectionResult result; result.status = ActorDetectionStatus::SourceBackedPartial;
    const auto& profile = *input.profile;
    if (input.distance_work80 >= (static_cast<uint32_t>(profile.byte2) << 20)) {
        next.phase_353 = 0; work = next; return result;
    }
    if (next.phase_353 == 1) {
        for (size_t i = 0; i < 3; ++i) {
            const int32_t difference = static_cast<int32_t>(input.player_raw[i]) - next.origin_34c[i];
            const uint32_t bounded = static_cast<uint32_t>(difference) + 26750U;
            if (bounded > 53500U) { next.phase_353 = 0; work = next; return result; }
        }
        if (!input.phase1) return hold(ActorDetectionStatus::NeedsPhase1Helpers);
        const auto& prepared = *input.phase1;
        // Both helper results are retained in the contract: the normalized
        // vector is80033F04's first input, the previous vector its second.
        next.vector_354 = prepared.interpolated_80033f04;
        for (size_t i = 0; i < 3; ++i) {
            const int32_t step = arithmetic_shift(static_cast<int32_t>(next.vector_354[i]) * 25, 10);
            next.origin_34c[i] = add_short(next.origin_34c[i], step);
        }
        if (prepared.segment0_80049c54 != 0 || prepared.segment1_80049c54 != 0) {
            next.phase_353 = 0; work = next; return result;
        }
        next.countdown_35a = static_cast<uint8_t>(next.countdown_35a - 1U);
        if (next.countdown_35a == 0) { next.phase_353 = 0; work = next; return result; }
        for (size_t i = 0; i < 3; ++i) {
            if (!near_player(next.origin_34c[i], input.player_raw[i])) { work = next; return result; }
        }
        if (!input.scratch_1f800075) return hold(ActorDetectionStatus::NeedsScratchMode);
        if (!input.global_800bbd96) return hold(ActorDetectionStatus::NeedsPlayerGlobal);
        next.phase_353 = 2; next.countdown_35a = profile.byte1;
        result.sound156_intent = *input.scratch_1f800075 == 2;
        result.detected = *input.global_800bbd96 < 1;
        work = next; return result;
    }
    if (next.phase_353 == 2) {
        if (!input.global_800bbd96) return hold(ActorDetectionStatus::NeedsPlayerGlobal);
        if (*input.global_800bbd96 > 0) {
            next.countdown_35a = 0; next.phase_353 = 0; work = next; return result;
        }
        const auto behind = actor_detection_behind_80030628(input.actor_raw, input.player_raw,
                                                           input.heading56, input.reference);
        if (!behind) return hold(ActorDetectionStatus::NeedsReference);
        result.notify_global_800bbd84 = true;
        if (*behind) {
            next.countdown_35a = static_cast<uint8_t>(next.countdown_35a - 1U);
            if (next.countdown_35a == 0) { next.phase_353 = 0; work = next; return result; }
        } else next.countdown_35a = profile.byte1;
        result.detected = true; work = next; return result;
    }
    const auto behind = actor_detection_behind_80030628(input.actor_raw, input.player_raw,
                                                       input.heading56, input.reference);
    if (!behind) return hold(ActorDetectionStatus::NeedsReference);
    if (*behind) return result;
    next.phase_353 = 1;
    next.origin_34c = input.actor_raw;
    next.origin_34c[1] = add_short(next.origin_34c[1], static_cast<int32_t>(next.offset_35b) * 16);
    for (size_t i = 0; i < 3; ++i)
        next.vector_354[i] = short_bits(static_cast<uint32_t>(static_cast<int32_t>(input.player_raw[i])) -
                                      static_cast<uint32_t>(static_cast<int32_t>(next.origin_34c[i])));
    next.countdown_35a = profile.byte0;
    work = next; return result;
}
} // namespace l3
