#include "legends3ds/actor_detection_preparation.h"

namespace l3 {
namespace {
int16_t narrow(int32_t value) noexcept {
    const uint16_t bits = static_cast<uint16_t>(value);
    return bits < 32768 ? static_cast<int16_t>(bits)
        : static_cast<int16_t>(static_cast<int32_t>(bits) - 65536);
}
int32_t shift(int32_t value, unsigned amount) noexcept {
    if (value >= 0) return value / static_cast<int32_t>(1U << amount);
    return -static_cast<int32_t>((-static_cast<int64_t>(value) +
        (INT64_C(1) << amount) - 1) / (INT64_C(1) << amount));
}
bool same(const ActorDetectionWork& a, const ActorDetectionWork& b) noexcept {
    return a.origin_34c == b.origin_34c && a.profile_352 == b.profile_352 &&
        a.phase_353 == b.phase_353 && a.vector_354 == b.vector_354 &&
        a.countdown_35a == b.countdown_35a && a.offset_35b == b.offset_35b;
}
ActorDetectionPreparation reduce(ActorDetectionPreparation result) noexcept {
    result.next = result.before;
    result.detector = step_actor_detection_800364b0(result.next, result.source);
    if (result.detector.status == ActorDetectionStatus::SourceBackedPartial)
        result.status = ActorDetectionPreparationStatus::Ready;
    else {
        result.status = ActorDetectionPreparationStatus::DetectorHeld;
        result.next = result.before;
    }
    return result;
}
} // namespace

ActorDetectionPreparation prepare_actor_detection_800364b0(
    const ActorDetectionWork& work, const ActorDetectionPreparationInputs& input) noexcept {
    ActorDetectionPreparation result;
    result.before = work; result.next = work; result.source = input.source;
    result.source.phase1.reset();
    if (!result.source.profile || work.phase_353 != 1 ||
        result.source.distance_work80 >= (static_cast<uint32_t>(result.source.profile->byte2) << 20))
        return reduce(result);
    for (size_t i = 0; i != 3; ++i) {
        result.difference[i] = static_cast<int32_t>(result.source.player_raw[i]) - work.origin_34c[i];
        if (static_cast<uint32_t>(result.difference[i]) + 26750U > 53500U)
            return reduce(result);
    }
    // A phase1 dependency hold is distinct from a missing profile. The high
    // level status identifies the exact missing helper or unsupported geometry.
    result.detector.status=ActorDetectionStatus::NeedsPhase1Helpers;
    if (input.map == nullptr) { result.status = ActorDetectionPreparationStatus::NeedsMap; return result; }
    if (!input.segment_bounds || input.segment_bounds->x0 >= input.segment_bounds->x1 ||
        input.segment_bounds->z0 >= input.segment_bounds->z1 || input.segment_bounds->y0 >= input.segment_bounds->y1) {
        result.status = ActorDetectionPreparationStatus::NeedsBounds; return result;
    }
    const auto normalized = source_normalize_8007a694(result.difference, input.normalization_lookup);
    if (!normalized) {
        result.status = input.normalization_lookup == nullptr
            ? ActorDetectionPreparationStatus::NeedsNormalizationLookup
            : ActorDetectionPreparationStatus::SourceMathRejected;
        return result;
    }
    const auto interpolated = source_detection_interpolate_80033f04(*normalized, work.vector_354,
                                                                   input.normalization_lookup);
    if (!interpolated) {
        result.status = input.normalization_lookup == nullptr
            ? ActorDetectionPreparationStatus::NeedsNormalizationLookup
            : ActorDetectionPreparationStatus::SourceMathRejected;
        return result;
    }
    result.phase1_math_prepared = true;
    ActorDetectionPhase1 phase1;
    phase1.normalized_8007a694 = *normalized;
    phase1.interpolated_80033f04 = *interpolated;
    for (size_t i = 0; i != 3; ++i) {
        result.step[i] = narrow(shift(static_cast<int32_t>((*interpolated)[i]) * 25, 10));
        result.updated_point[i] = narrow(static_cast<int32_t>(work.origin_34c[i]) + result.step[i]);
        result.midpoint[i] = narrow(static_cast<int32_t>(work.origin_34c[i]) + shift(result.step[i], 1));
    }
    // First original SH4 lane3 is the packed profile/phase bytes atwork+352.
    // Second lane3 is an unused scratch halfword;80049C54 does not read it.
    result.updated_point[3] = narrow(static_cast<int32_t>(work.profile_352) |
                                    static_cast<int32_t>(work.phase_353) << 8);
    result.midpoint[3] = 0;
    result.segment0_queried = true;
    result.segment0 = query_actor_detection_segment_80049c54(*input.map, input.area_index,
        result.updated_point, result.updated_point, *input.segment_bounds, 0xffffu);
    if (result.segment0.status != RuntimeContactStatus::SourceBackedPartial) {
        result.status = ActorDetectionPreparationStatus::SegmentUnsupported; return result;
    }
    phase1.segment0_80049c54 = result.segment0.result;
    if (result.segment0.result == 0) {
        result.segment1_queried = true;
        result.segment1 = query_actor_detection_segment_80049c54(*input.map, input.area_index,
            result.midpoint, result.midpoint, *input.segment_bounds, 0xffffu);
        if (result.segment1.status != RuntimeContactStatus::SourceBackedPartial) {
            result.status = ActorDetectionPreparationStatus::SegmentUnsupported; return result;
        }
        phase1.segment1_80049c54 = result.segment1.result;
    }
    // A blocked first query skips the second in the original. Its unused zero
    // field is never consumed by the reducer's first-result rejection branch.
    result.source.phase1 = phase1;
    return reduce(result);
}
bool actor_detection_preparation_matches(const ActorDetectionPreparation& prepared,
                                         const ActorDetectionWork& current) noexcept {
    return same(prepared.before, current);
}
} // namespace l3
