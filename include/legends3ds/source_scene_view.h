#pragma once
#include "legends3ds/source_camera.h"
#include "legends3ds/source_event_phase6.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
constexpr std::size_t kSourceSceneViewProfileBytes=2126;
// Owner-local scalar sine table80092348 differs from packed matrix trig.
// Original executable code is neither loaded nor interpreted by the runtime.
struct SourceSceneViewProfile {
    bool initialized=false;
    std::array<std::int16_t,1025> scalar_sine{};
    std::uint32_t boot_ofx=0,boot_ofy=0,boot_h=0;
};
SourceSceneViewProfile load_source_scene_view_profile(const std::vector<std::uint8_t>& bytes);
// Explicit fresh executable projection prefix80011F48..80011F58. Never use
// this operation as a warm/load missing-owner fallback. DQA/DQB are retained.
bool initialize_source_scene_projection_80011f48(SourceProjectionControls& owner,
    const SourceSceneViewProfile& profile) noexcept;
enum class SourceSceneViewStatus : std::uint8_t {
    SourceBackedPartial,NeedsActiveResources,NeedsProfile,NeedsReference,
    NeedsCameraMode,GameplayCameraOwned,NeedsScriptControl,NeedsSourceShift,
    UnsupportedSourceShift,NeedsCameraBytes,NeedsTrackedActor,
    NeedsTargetPointCamera,NeedsProjectionControls,UnsupportedOptics
};
struct SourceSceneViewInputs {
    const SourceSceneViewProfile* profile=nullptr;
    const ReferenceTables* reference=nullptr;
    const SourceEventPhase5Context* scene=nullptr;
    const SourceEventPhase6Context* script=nullptr;
    // Check the current active EBD witness in the calling session. This is an
    // immediate borrowed snapshot; no retained pointers/leases are returned.
    bool active_resources_admitted=false;
};
struct SourceSceneViewFrame {
    std::array<std::int16_t,9> rotation_q12{};
    std::array<std::int32_t,3> translation{};
    std::array<std::int16_t,3> eye{};
    std::array<std::int32_t,3> target{};
    std::int16_t distance=0,yaw=0,pitch=0;
    std::uint8_t source_shift=0;
    // Native coordinates are [raw X,-raw Y,raw Z]/512; camera forward is -Z.
    // A semantic matrix copy, not an original PS1 scratch pointer or padding.
    std::array<float,16> native_view_row_major{};
    float focal_pixels=0,center_x_pixels=0,center_y_pixels=0;
    float vertical_fov_degrees=0;
    // 320x240 source window is centered inside400x240 with extra side scenery.
    // The frontend's near/far/clipping/depth policy is separate and CANDIDATE.
};
struct SourceSceneViewResult {
    SourceSceneViewStatus status=SourceSceneViewStatus::NeedsActiveResources;
    std::optional<std::uint16_t> missing_camera_offset;
    std::optional<SourceSceneViewFrame> frame;
};
// Supported complete8001E550 nontracked radial branch through80038324 and
//80037FB4. Actor-tracked and target-point branches hold explicitly. The view
// does not consume/update camera program state, event counters or story flags.
// Every output is value-owned and valid only for this current input snapshot.
SourceSceneViewResult produce_source_scene_view_8001e550(const SourceSceneViewInputs& inputs) noexcept;
} // namespace l3
