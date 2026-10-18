#pragma once
#include "legends3ds/player_contact_runtime.h"
#include <array>
#include <vector>

namespace l3 {

struct ActorDetectionSegment {
    RuntimeContactStatus status = RuntimeContactStatus::Unsupported;
    int32_t result = 0;
    uint32_t candidate_cells = 0;
    uint32_t groups_tested = 0;
    uint32_t primitives_tested = 0;
    uint32_t unsupported_primitives = 0;
    // Resident map indices and primitive code at the first held query. These
    // scalar diagnostics do not require a hot-loop allocation or owner bytes.
    uint32_t unsupported_cell_index = UINT32_MAX;
    uint32_t unsupported_group_index = UINT32_MAX;
    uint32_t unsupported_primitive_index = UINT32_MAX;
    uint32_t unsupported_code = UINT32_MAX;
};

// Source80049C54(a0=startSH4,a1=endSH4,a2=bounds,a3=class). This is the
// original local step query, not a generic line/ray intersection algorithm.
// Classifiers use startpoint occupancy; end also supplies the original
// wrap/sign-cross rejection. Source includezero1 gathering is bounded4 cells.
// Families0..6 preserve classifier truthiness, original short-wrap and signed
// product/division rules. Family7 admits only its original outer rejection.
// Plain-Y low8 dispatch and packednegativeY are supported. Intersecting slope
// Y branches and family7 interior hold; no guessed clear/blocked, no allocations.
ActorDetectionSegment query_actor_detection_segment_80049c54(
    const RuntimeMap& map, size_t area_index,
    const std::array<int16_t, 4>& start, const std::array<int16_t, 4>& end,
    RetailPlayerBounds bounds, uint32_t source_class_argument = 0xffffu) noexcept;

// Neutral owner-local L3DB format:32Bheader+sixsigned16 bounds+CRC32. Header
// source identity80097D0C identifies the proved six-field contract; it is not
// source payload. Order x0,x1,z0,z1,y0,y1, matching scratch124..12F.
std::vector<uint8_t> encode_actor_detection_segment_bounds(RetailPlayerBounds bounds);
RetailPlayerBounds load_actor_detection_segment_bounds(const std::vector<uint8_t>& bytes);

} // namespace l3
