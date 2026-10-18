#pragma once

#include "legends3ds/reference_tables.h"
#include "legends3ds/source_player_proxy.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// Each retained control has its own source owner. Calling one setter does not
// admit the other four controls or a current camera matrix.
struct SourceProjectionControls {
    std::optional<std::uint32_t> ofx, ofy, h, dqa, dqb;
};
void source_projection_offset_8007b118(SourceProjectionControls& owner,
    std::uint32_t x, std::uint32_t y) noexcept;
void source_projection_h_8007b138(SourceProjectionControls& owner,
    std::uint32_t h) noexcept;
void source_projection_dqa_8007b0c0(SourceProjectionControls& owner,
    std::uint32_t dqa) noexcept;
void source_projection_dqb_8007b0cc(SourceProjectionControls& owner,
    std::uint32_t dqb) noexcept;
// Exact8007A1CC signed division with original low32 numerator wrapping.
// Source break domains (zero divisor and INT_MIN/-1) hold both controls.
bool source_projection_fog_8007a1cc(SourceProjectionControls& owner,
    std::uint32_t distance, std::uint32_t denominator) noexcept;
// Atomic admission of all five original retained controls. Missing any field
// retains every GTE field; a viewport does not establish source ownership.
bool bind_source_projection_controls(SourceGteRtpsState& state,
    const SourceProjectionControls& owner) noexcept;

struct SourceCameraDirectInputs {
    const ReferenceTables* reference = nullptr;
    std::optional<std::array<std::int32_t,3>> eye;
    std::optional<std::int32_t> yaw, pitch;
    // Transpose8007B568 writes18 coefficient bytes; halfword at1F8000F2
    // survives before the32-byte copy. Slot/scratch reuse cannot assume zero.
    std::optional<std::uint16_t> retained_matrix_padding_1f8000f2;
};
enum class SourceCameraStatus : std::uint8_t {
    SourceBackedPartial, NeedsReference, NeedsEye, NeedsAngles,
    NeedsRetainedMatrixPadding
};
struct SourceCameraDirectResult {
    SourceCameraStatus status = SourceCameraStatus::NeedsReference;
    std::optional<std::array<std::uint32_t,8>> scratch_camera_1f800008;
    std::optional<std::array<std::int16_t,3>> scratch_eye_1f800028;
    std::optional<std::array<std::int32_t,3>> eye_800a39f0;
    std::array<std::int16_t,3> rotation_800a3a10{};
};

// Persistent output slice of complete original800384F0: RotMatrixYX8007B838,
// transpose8007B568 and split-vector ApplyMatrixLV8007A9B8. Packed owner-local
// trig and retained padding are explicit premises. No live update, obstruction,
// interpolation or transient GTE FLAG/control ownership is inferred here.
SourceCameraDirectResult produce_source_camera_800384f0(
    const SourceCameraDirectInputs& inputs) noexcept;

} // namespace l3
