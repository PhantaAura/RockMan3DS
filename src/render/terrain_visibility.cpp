#include "legends3ds/render/terrain_visibility.h"
#include "legends3ds/source_camera.h"
#include <algorithm>

namespace l3 {
namespace {
int16_t lo16(uint32_t v) noexcept { return static_cast<int16_t>(static_cast<uint16_t>(v & 0xffffu)); }
int16_t hi16(uint32_t v) noexcept { return static_cast<int16_t>(static_cast<uint16_t>(v >> 16)); }
}

TerrainFaceCamera make_terrain_face_camera(const std::array<uint32_t,8>& words,
    const std::array<uint8_t,40>* area) noexcept {
    TerrainFaceCamera c;
    for (size_t i = 0; i < 8; ++i) c.gte.control[i] = words[i];
    // CTC2 R4 keeps only RT33 (the pad halfword never reaches the GTE).
    c.gte.control[4] = static_cast<uint32_t>(static_cast<int32_t>(lo16(words[4])));
    c.gte.matrix_translation_bound = true;
    SourceProjectionControls owner;
    source_projection_init_geom_8007a4f0(owner);
    source_projection_offset_8007b118(owner, 160u, 120u);
    source_projection_h_8007b138(owner, 384u);
    if (area) {
        c.area_record = *area;
        const uint32_t distance = uint32_t((*area)[0x24]) | (uint32_t((*area)[0x25]) << 8);
        if (source_projection_fog_8007a1cc(owner, distance, 384u))
            c.far_color = source_far_color_8007b0f8({{(*area)[32], (*area)[33], (*area)[34]}});
    }
    (void)bind_source_projection_controls(c.gte, owner);
    return c;
}

std::optional<int32_t> terrain_view_depth_1f8001c0(const TerrainFaceCamera& camera,
    int16_t x, int16_t y, int16_t z) noexcept {
    auto gte = camera.gte;
    gte.data[0] = static_cast<uint16_t>(x) | (static_cast<uint32_t>(static_cast<uint16_t>(y)) << 16);
    gte.data[1] = static_cast<uint16_t>(z);
    // RT (MVMVA rotation + translation, sf=1) and RTPS share MAC1..3.
    if (source_gte_rtps_0180001(gte).status != SourcePlayerProxyStatus::SourceBackedPartial) return std::nullopt;
    return static_cast<int32_t>(gte.data[27]);
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
        if (v == 3) r.ir0 = static_cast<uint16_t>(gte.data[8]);   // IR0 after RTPS of vertex 3
        // A bit-29 negative face is a triangle (0x80021E74): RTPT only, so its
        // depth cue uses IR0 after vertex 2.
        if (v == 2 && negative_cell && (prefix & 0x20000000u)) { r.ir0 = static_cast<uint16_t>(gte.data[8]); break; }
    }
    const bool triangle = negative_cell && (prefix & 0x20000000u);
    if (triangle) { sx[3] = sx[2]; sy[3] = sy[2]; sz[3] = sz[2]; }
    r.rgb = prefix & 0x00ffffffu;
    if (camera.far_color && (negative_cell || !(prefix & 0x10000000u))) {
        SourceDpcsColor out;
        if (source_dpcs_0780010(prefix & 0x00ffffffu, *camera.far_color, r.ir0, out))
            r.rgb = out.rgb_code & 0x00ffffffu;
    }
    // NCLIP 0x4B400006 over SXY0..2.
    r.mac0 = int64_t(sx[0]) * sy[1] + int64_t(sx[1]) * sy[2] + int64_t(sx[2]) * sy[0]
           - int64_t(sx[0]) * sy[2] - int64_t(sx[1]) * sy[0] - int64_t(sx[2]) * sy[1];
    r.min_sz = *std::min_element(sz.begin(), sz.end());
    r.max_sz = *std::max_element(sz.begin(), sz.end());
    for (size_t v = 0; v < 4; ++v) { r.sx[v] = int16_t(sx[v]); r.sy[v] = int16_t(sy[v]); }
    const bool double_sided = (prefix & 0x40000000u) != 0;
    // 0x80025BE8..0x80025D38 (mode 2, after the MAC0 == 0 skip): unsigned
    // SX < 320 and SY < 240 for any vertex, else for a diagonal midpoint.
    if (!negative_cell && mode == 2 && r.mac0 != 0) {
        const auto on = [&](int32_t x, int32_t y) {
            return uint32_t(x - camera.screen_x_min) < camera.screen_width && uint32_t(y) < 240u;
        };
        const bool visible = on(sx[0], sy[0]) || on(sx[1], sy[1]) || on(sx[2], sy[2]) || on(sx[3], sy[3]) ||
            on((sx[0] + sx[3]) >> 1, (sy[0] + sy[3]) >> 1) || on((sx[1] + sx[2]) >> 1, (sy[1] + sy[2]) >> 1);
        if (!visible) { r.face = TerrainFace::Skip; r.off_screen = true; return r; }
    }
    if (!negative_cell) {
        const auto depth = terrain_face_depth_80025900(sz, prefix);
        if (depth.near_culled) { r.face = TerrainFace::Skip; return r; }
        r.otz = depth.otz;
        if (camera.area_record) r.ramp = terrain_clut_ramp_step(depth.otz, prefix, *camera.area_record, camera.lod_1f800074);
    }
    // 0x80024F30 / 0x80025900, after the depth cue: the screen box of SXY0..3
    // (int16) must overlap the centre region, and the face must lie nearer
    // than the player (max SZ < 0x1F8001C0).
    const auto see_through = [&] {
        if (!(camera.see_through_1f80007c & 0x02000000u)) return false;
        const bool gate = !(prefix & 0x20000000u) ||
            (r.min_sz < 0x181 && (mode == 2 || !(prefix & 0x01000000u)));
        if (!gate || int32_t(prefix) <= 0) return false;
        if (!(int32_t(*std::max_element(sz.begin(), sz.end())) < threshold)) return false;
        const auto [x0, x1] = std::minmax_element(sx.begin(), sx.end());
        const auto [y0, y1] = std::minmax_element(sy.begin(), sy.end());
        return *x1 > 0x96 && *x0 < 0xaa && *y1 > 0x78 && *y0 < 0xa0;
    };
    if (negative_cell || mode != 2) {
        r.face = (r.mac0 > 0 || double_sided) ? TerrainFace::Draw : TerrainFace::Skip;
        r.see_through = !negative_cell && mode == 1 && r.face == TerrainFace::Draw && see_through();
        return r;
    }
    if (r.mac0 == 0) { r.face = TerrainFace::Skip; return r; }
    if (r.mac0 > 0 || double_sided) { r.face = TerrainFace::Draw; r.see_through = see_through(); return r; }
    if (!(threshold < int32_t(r.min_sz)) || (prefix & 0x01000000u)) { r.face = TerrainFace::Skip; return r; }
    r.face = TerrainFace::Black;
    return r;
}

std::optional<TerrainSubdivision> terrain_subdivide_8002466c(const TerrainFaceCamera& camera,
    const std::array<int32_t,4>& x, const std::array<int32_t,4>& y, const std::array<int32_t,4>& z,
    const std::array<uint8_t,8>& uv, const TerrainFaceResult& face) noexcept {
    TerrainSubdivision s;
    for (size_t k = 0; k < 4; ++k) {
        s.world[k] = {{x[k], y[k], z[k]}};
        s.sx[k] = face.sx[k]; s.sy[k] = face.sy[k];
        s.u[k] = uv[2 * k]; s.v[k] = uv[2 * k + 1];
    }
    // SVECTOR halving: short inputs, int sum, arithmetic shift, short result.
    const auto half = [](int32_t a, int32_t b) { return int32_t(int16_t((int32_t(int16_t(a)) + int32_t(int16_t(b))) >> 1)); };
    const auto mid = [&](size_t a, size_t b) {
        return std::array<int32_t,3>{{half(s.world[a][0], s.world[b][0]), half(s.world[a][1], s.world[b][1]),
                                       half(s.world[a][2], s.world[b][2])}};
    };
    s.world[4] = mid(0, 1); s.world[5] = mid(0, 2); s.world[7] = mid(1, 3); s.world[8] = mid(2, 3);
    s.world[6] = mid(5, 7);
    const auto huv = [](uint8_t a, uint8_t b) { return uint8_t((unsigned(a) + unsigned(b)) >> 1); };
    s.u[4] = huv(s.u[0], s.u[1]); s.v[4] = huv(s.v[0], s.v[1]);
    s.u[5] = huv(s.u[0], s.u[2]); s.v[5] = huv(s.v[0], s.v[2]);
    s.u[7] = huv(s.u[1], s.u[3]); s.v[7] = huv(s.v[1], s.v[3]);
    s.u[8] = huv(s.u[2], s.u[3]); s.v[8] = huv(s.v[2], s.v[3]);
    s.u[6] = huv(s.u[4], s.u[8]); s.v[6] = huv(s.v[4], s.v[8]);
    for (size_t k = 4; k < 9; ++k) {
        auto gte = camera.gte;
        gte.data[0] = (static_cast<uint32_t>(s.world[k][0]) & 0xffffu) | (static_cast<uint32_t>(s.world[k][1]) << 16);
        gte.data[1] = static_cast<uint32_t>(s.world[k][2]) & 0xffffu;
        const auto p = source_gte_rtps_0180001(gte);
        if (p.status != SourcePlayerProxyStatus::SourceBackedPartial || p.depth == 0) return std::nullopt;
        s.sx[k] = lo16(p.screen_xy); s.sy[k] = hi16(p.screen_xy);
    }
    for (size_t t = 0; t < 4; ++t) {
        const auto& i = kSubdivisionTris80097CA0[t];
        const int64_t x0 = s.sx[i[0]], y0 = s.sy[i[0]], x1 = s.sx[i[1]], y1 = s.sy[i[1]], x2 = s.sx[i[2]], y2 = s.sy[i[2]];
        s.tri_drawn[t] = x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1 > 0;
    }
    return s;
}

} // namespace l3
