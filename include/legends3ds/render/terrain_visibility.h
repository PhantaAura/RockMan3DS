#pragma once
#include "legends3ds/render/source_depth_cue.h"
#include "legends3ds/render/terrain_clut_ramp.h"
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
//  - Every positive family skips a bit31-clear face whose max SZ < 0xC1
//    (near cull, terrain_face_depth_80025900).
// Measured in PCSX-Redux at the Area 1 control start: mode 2, threshold 522;
// the start wall's back face (min SZ 284/270) is skipped and farther back
// faces are drawn black. Its writer is 0x800238BC: gte_rt of the player's
// +0x14 position with the 0x1F800008 camera, stored by stlvnl to 0x1F8001B8,
// so 0x1F8001C0 is the player's view-space depth (MAC3) each frame
// (see terrain_view_depth_1f8001c0).
enum class TerrainFace : uint8_t { Draw, Black, Skip, Unsupported };

constexpr int32_t kObservedArea1BackFaceThreshold = 522; // 0x1F8001C0, OBSERVED

// 0x1F80007C, ORed into the code word of a see-through face: 0x02000000 (ABE,
// semi-transparent) while the event byte 0x800C4C48 is 0, else 0.
constexpr uint32_t terrain_see_through_code_1f80007c(uint8_t event_active_800c4c48) noexcept {
    return event_active_800c4c48 == 0 ? 0x02000000u : 0u;
}

struct TerrainFaceCamera {
    SourceGteRtpsState gte{}; // matrix/translation 0x1F800008 + OFX/OFY/H/DQA/DQB
    // FarColor from the area configuration (0x800371A0 -> 0x8007B0F8), present
    // only when the area record was supplied: enables the depth cue below.
    std::optional<SourceFarColorControls> far_color;
    // Area record (ramp bytes +0..31) and the 0x1F800074 LOD shift, for the
    // positive-family depth ramp; absent without the record.
    std::optional<std::array<uint8_t,40>> area_record;
    uint8_t lod_1f800074 = 0;
    // 0x1F80007C for this frame (terrain_see_through_code_1f80007c).
    uint32_t see_through_1f80007c = 0x02000000u;
};
// OFX/OFY 160/120 and H 384 as installed by 0x8007A4F0 + 0x80011F48. With the
// 40-byte area record (0x8011AC54 + area * 40, cooked as the L3SF setup blob),
// 0x800371A0 also runs SetFogNear 0x8007A1CC(+0x24, 384) for DQA/DQB and
// FarColor from bytes +32/+33/+34.
TerrainFaceCamera make_terrain_face_camera(const std::array<uint32_t,8>& camera_1f800008,
    const std::array<uint8_t,40>* area_record = nullptr) noexcept;

struct TerrainFaceResult {
    TerrainFace face = TerrainFace::Unsupported;
    int64_t mac0 = 0;
    uint16_t min_sz = 0;
    // Depth cue: IR0 of the face's last RTPS (vertex 3) and the primitive RGB
    // after DPCS toward FarColor. Every positive family (0x80023EA8,
    // 0x80024F30, 0x80025900, 0x80026B04) applies it unless prefix bit 28 is
    // set; the negative family 0x80021B14 always does. Without a far colour,
    // rgb is the prefix RGB unchanged.
    uint16_t ir0 = 0;
    uint32_t rgb = 0x808080;
    // Positive faces: ordering depth (otz) and CLUT ramp step 0..7.
    int32_t otz = 0;
    uint8_t ramp = 0;
    // See-through rule of families 0x80024F30 (mode 1) and 0x80025900 (mode 2):
    // a drawn face with prefix > 0 (bit 31 clear), max SZ < 0x1F8001C0 and a
    // screen box overlapping SX 0x97..0xA9 / SY 0x79..0x9F gets 0x1F80007C
    // ORed into its code word, so it is drawn semi-transparent (ABR from its
    // TPAGE). Faces that take the bit-29 path (bit 29 with min SZ >= 0x181;
    // mode 1 also with bit 24) are excluded. Modes 0/3 never set it.
    bool see_through = false;
};
// 0x1F8001C0 for this frame: view-space depth (MAC3) of the player position.
std::optional<int32_t> terrain_view_depth_1f8001c0(const TerrainFaceCamera& camera,
    int16_t x, int16_t y, int16_t z) noexcept;
// Vertices are raw world coordinates in the primitive's original order.
// Vertices outside the signed16 range or behind the camera (SZ 0) return
// Unsupported: the caller keeps drawing them as before.
TerrainFaceResult classify_terrain_face(const TerrainFaceCamera& camera,
    const std::array<int32_t,4>& x, const std::array<int32_t,4>& y, const std::array<int32_t,4>& z,
    uint32_t prefix, bool negative_cell, uint8_t positive_mode_1f800075,
    int32_t back_face_threshold_1f8001c0) noexcept;

} // namespace l3
