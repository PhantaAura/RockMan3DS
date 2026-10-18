#pragma once
#include "legends3ds/actor_detection.h"
#include "legends3ds/actor_detection_math.h"
#include "legends3ds/actor_detection_segment.h"
#include <array>
#include <cstddef>
#include <optional>

namespace l3 {
struct ActorDetectionPreparationInputs {
    ActorDetectionInputs source;
    const RuntimeMap* map = nullptr;
    size_t area_index = 0;
    const SourceNormalizationLookup* normalization_lookup = nullptr;
    std::optional<RetailPlayerBounds> segment_bounds;
};
enum class ActorDetectionPreparationStatus {
    Ready,
    DetectorHeld,
    NeedsMap,
    NeedsBounds,
    NeedsNormalizationLookup,
    SourceMathRejected,
    SegmentUnsupported,
};
struct ActorDetectionPreparation {
    ActorDetectionPreparationStatus status = ActorDetectionPreparationStatus::DetectorHeld;
    ActorDetectionWork before;
    ActorDetectionWork next;
    ActorDetectionInputs source;
    ActorDetectionResult detector;
    std::array<int32_t, 3> difference{};
    std::array<int16_t, 3> step{};
    std::array<int16_t, 4> updated_point{};
    std::array<int16_t, 4> midpoint{};
    ActorDetectionSegment segment0;
    ActorDetectionSegment segment1;
    bool phase1_math_prepared = false;
    bool segment0_queried = false;
    bool segment1_queried = false;
};

// Source800364B0 composition with original phase1 normalization/interpolation
// and bounded80049C54 occupancy. Never mutates input work. Range/delta rejection
// and non-phase1 branches do not require helpers they do not call. Externally
// supplied phase1 premises are discarded; this function computes its own.
// Only Ready may publish next; hold statuses retain before/next identically.
// Sound/notification result intents still require their real external owners.
ActorDetectionPreparation prepare_actor_detection_800364b0(
    const ActorDetectionWork& work, const ActorDetectionPreparationInputs& input) noexcept;

// Check all persistent original detector fields before publishing a prepared
// result. Prepared source actor/player inputs and map lifetime remain caller-owned.
bool actor_detection_preparation_matches(const ActorDetectionPreparation& prepared,
                                         const ActorDetectionWork& current) noexcept;
} // namespace l3
