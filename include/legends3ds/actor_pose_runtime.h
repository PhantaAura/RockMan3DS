#pragma once
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

struct ActorPoseCollisionInputs {
    const ReferenceTables* reference = nullptr;
    uint16_t heading_56 = 0;
    std::array<int16_t, 3> raw_14{};
    std::array<int16_t, 3> scale_158{};
    // Decoded signed12 channels from the selected original pose window.
    std::array<int16_t, 3> root_translation{};
    uint8_t flags_06 = 0;
    std::optional<uint8_t> scratch_shift_1f800074;
    // Only read when work06&40. Class20 uses source bytes388/389/38A.
    std::optional<std::array<uint8_t, 3>> root_scale_388;
    // The producer writes180/182/184 only. Fresh pool clear proves zero;
    // allocation of a recycled slot does not clear this retained halfword.
    std::optional<int16_t> preserved_186;
};
enum class ActorPoseCollisionStatus : uint8_t {
    SourceBackedPartial, NeedsReference, NeedsScratchShift,
    NeedsRootScale, NeedsPreservedHalfword
};
struct ActorPoseCollisionResult {
    ActorPoseCollisionStatus status = ActorPoseCollisionStatus::NeedsReference;
    std::optional<std::array<int16_t, 4>> point_180;
    // Original heading matrix after8007ADE8; diagnostic/source state only.
    std::array<int16_t, 9> scaled_heading_160{};
};

// Root collision-point writes from8003346C→80033598, including original
// RotMatrix8007B5A8, ScaleMatrix8007ADE8 and MAC outputs8007AD38. Child
// rotations/positions, render matrices and general GTE flags are excluded.
// All products retain original low32 wrapping before arithmetic shifts;
// coordinates narrow to signed16 and lane186 is preserved, never fabricated.
ActorPoseCollisionResult produce_actor_pose_collision_point_8003346c(
    const ActorPoseCollisionInputs& inputs) noexcept;

} // namespace l3
