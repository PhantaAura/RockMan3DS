#include "legends3ds/render/terrain_visibility.h"
#include "legends3ds/source_camera.h"
#include <algorithm>

namespace l3 {
namespace {
int16_t lo16(uint32_t v) noexcept { return static_cast<int16_t>(static_cast<uint16_t>(v & 0xffffu)); }
int16_t hi16(uint32_t v) noexcept { return static_cast<int16_t>(static_cast<uint16_t>(v >> 16)); }
}

TerrainFaceCamera make_terrain_face_camera(const std::array<uint32_t,8>& words) noexcept {
    TerrainFaceCamera c;
    for (size_t i = 0; i < 8; ++i) c.gte.control[i] = words[i];
    // CTC2 R4 keeps only RT33 (the pad halfword never reaches the GTE).
    c.gte.control[4] = static_cast<uint32_t>(static_cast<int32_t>(lo16(words[4])));
    c.gte.matrix_translation_bound = true;
    SourceProjectionControls owner;
    source_projection_init_geom_8007a4f0(owner);
    source_projection_offset_8007b118(owner, 160u, 120u);
    source_projection_h_8007b138(owner, 384u);
    (void)bind_source_projection_controls(c.gte, owner);
    return c;
}

TerrainFaceResult classify_terrain_face(const TerrainFaceCamera& camera,
    const std::array<int32_t,4>& x, const std::array<int32_t,4>& y, const std::array<int32_t,4>& z,
    uint32_t prefix, bool negative_cell, uint8_t mode, int32_t threshold) noexcept {
    TerrainFaceResult r;
    std::array<int32_t,4> sx{}, sy{};
    std::array<uint16_t,4> sz{};
    for (size_t v = 0; v < 4; ++v) {
        if (x[v] < INT16_MIN || x[v] > INT16_MAX || y[v] < INT16_MIN || y[v] > INT16_MAX ||
            z[v] < INT16_MIN || z[v] > INT16_MAX) return r;
        auto gte = camera.gte;
        gte.data[0] = (static_cast<uint32_t>(x[v]) & 0xffffu) | (static_cast<uint32_t>(y[v]) << 16);
        gte.data[1] = static_cast<uint32_t>(z[v]) & 0xffffu;
        const auto p = source_gte_rtps_0180001(gte);
        if (p.status != SourcePlayerProxyStatus::SourceBackedPartial || p.depth == 0) return r;
        sx[v] = lo16(p.screen_xy); sy[v] = hi16(p.screen_xy); sz[v] = p.depth;
    }
    // NCLIP 0x4B400006 over SXY0..2.
    r.mac0 = int64_t(sx[0]) * sy[1] + int64_t(sx[1]) * sy[2] + int64_t(sx[2]) * sy[0]
           - int64_t(sx[0]) * sy[2] - int64_t(sx[1]) * sy[0] - int64_t(sx[2]) * sy[1];
    r.min_sz = *std::min_element(sz.begin(), sz.end());
    const bool double_sided = (prefix & 0x40000000u) != 0;
    if (negative_cell || mode != 2) {
        r.face = (r.mac0 > 0 || double_sided) ? TerrainFace::Draw : TerrainFace::Skip;
        return r;
    }
    if (r.mac0 == 0) { r.face = TerrainFace::Skip; return r; }
    if (r.mac0 > 0 || double_sided) { r.face = TerrainFace::Draw; return r; }
    if (!(threshold < int32_t(r.min_sz)) || (prefix & 0x01000000u)) { r.face = TerrainFace::Skip; return r; }
    r.face = TerrainFace::Black;
    return r;
}

} // namespace l3
