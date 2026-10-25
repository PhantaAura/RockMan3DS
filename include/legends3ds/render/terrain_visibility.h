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
//  - Mode 2 also skips a face lying wholly off screen, before the near cull
//    and the back-face test: no vertex SXY on screen and neither diagonal
//    midpoint (v0+v3, v1+v2) on screen (0x80025BE8..0x80025D38).
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
    // On-screen window of the mode-2 off-screen test (0x80025BE8..0x80025D38):
    // the original keeps SX in [0, 320) and SY in [0, 240) (unsigned compares).
    // The 3DS top screen is 400 wide at the same OFX-centred projection, so
    // the port widens SX to [-40, 360) (CANDIDATE, like the cell side cut).
    int16_t screen_x_min = 0;
    uint16_t screen_width = 320;
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
    // Skipped by the mode-2 off-screen test.
    bool off_screen = false;
    // GTE screen coordinates (SXY0..3, OFX/OFY 160/120) and max SZ, set once
    // all four vertices projected; the packet writers 0x80026158/0x80021B14
    // use them. A bit-29 negative face is a triangle: vertex 3 is not
    // projected (its slot repeats vertex 2) and ir0/rgb come from vertex 2.
    std::array<int16_t,4> sx{}, sy{};
    uint16_t max_sz = 0;
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

// 0x8002466C: one-level subdivision of a large positive face (chosen by
// terrain_needs_subdivision_8002602c). Nine points: corners 0..3, then
// m01 (4), m02 (5), centre (6) = (m02 + m13) / 2, m13 (7), m23 (8), each a
// signed halving of the world SVECTORs, projected by RTPT with the face
// camera. UVs halve the same way, except the centre UV = (uv m01 + uv m23) / 2.
// The original writes four POLY_FT4 (table 0x80097C90, copied to 0x1F800120)
// and, for each edge triangle of table 0x80097CA0 whose NCLIP > 0, a POLY_FT3
// (code & ~0x08); they are chained in that order and added at the parent's
// bucket. tri_drawn holds that NCLIP test.
constexpr std::array<std::array<uint8_t,4>,4> kSubdivisionQuads80097C90{{{{0,4,5,6}},{{4,1,6,7}},{{5,6,2,8}},{{6,7,8,3}}}};
constexpr std::array<std::array<uint8_t,3>,4> kSubdivisionTris80097CA0{{{{0,1,4}},{{1,3,7}},{{3,2,8}},{{2,0,5}}}};
struct TerrainSubdivision {
    std::array<int16_t,9> sx{}, sy{};
    std::array<uint8_t,9> u{}, v{};
    std::array<std::array<int32_t,3>,9> world{};
    std::array<bool,4> tri_drawn{};
};
// nullopt when a midpoint cannot be projected (the caller keeps the face 3D).
std::optional<TerrainSubdivision> terrain_subdivide_8002466c(const TerrainFaceCamera& camera,
    const std::array<int32_t,4>& x, const std::array<int32_t,4>& y, const std::array<int32_t,4>& z,
    const std::array<uint8_t,8>& uv, const TerrainFaceResult& face) noexcept;

} // namespace l3
