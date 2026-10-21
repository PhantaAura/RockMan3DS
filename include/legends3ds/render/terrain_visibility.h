#pragma once
#include "legends3ds/source_player_proxy.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// Per-quad face test of the original map renderers, after RTPT/NCLIP over
// vertices 0..2 and RTPS of vertex 3 with the 0x1F800008 camera.
//  - Positive mode 2 (scratch 0x1F800075 == 2 -> 0x800267AC group 2, family
//    0x80025900): MAC0 == 0 skips; MAC0 > 0 or prefix bit 30 draws; a back
//    face is skipped when its minimum SZ is <= the scratch word 0x1F8001C0 or
//    prefix bit 24 is set, otherwise drawn as a 0x2C010101 (near-black) quad.
//  - Positive modes 0/1/3 (families 0x80023EA8/0x80024F30/0x80026B04) and the
//    negative family 0x80021B14: MAC0 > 0 or bit 30 draws, else skipped.
// Measured in PCSX-Redux at the Area 1 control start: mode 2, threshold 522;
// the start wall's back face (min SZ 284/270) is skipped and farther back
// faces are drawn black. The threshold's writer is untraced (OBSERVED).
enum class TerrainFace : uint8_t { Draw, Black, Skip, Unsupported };

constexpr int32_t kObservedArea1BackFaceThreshold = 522; // 0x1F8001C0, OBSERVED

struct TerrainFaceCamera {
    SourceGteRtpsState gte{}; // matrix/translation 0x1F800008 + OFX/OFY/H
};
// OFX/OFY 160/120 and H 384 as installed by 0x8007A4F0 + 0x80011F48.
TerrainFaceCamera make_terrain_face_camera(const std::array<uint32_t,8>& camera_1f800008) noexcept;

struct TerrainFaceResult {
    TerrainFace face = TerrainFace::Unsupported;
    int64_t mac0 = 0;
    uint16_t min_sz = 0;
};
// Vertices are raw world coordinates in the primitive's original order.
// Vertices outside the signed16 range or behind the camera (SZ 0) return
// Unsupported: the caller keeps drawing them as before.
TerrainFaceResult classify_terrain_face(const TerrainFaceCamera& camera,
    const std::array<int32_t,4>& x, const std::array<int32_t,4>& y, const std::array<int32_t,4>& z,
    uint32_t prefix, bool negative_cell, uint8_t positive_mode_1f800075,
    int32_t back_face_threshold_1f8001c0) noexcept;

} // namespace l3
