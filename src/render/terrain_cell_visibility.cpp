#include "legends3ds/render/terrain_cell_visibility.h"
#include "legends3ds/source_camera.h"
#include <algorithm>
#include <cstdlib>

namespace l3 {
namespace {
int16_t lo16(uint32_t v) noexcept { return static_cast<int16_t>(static_cast<uint16_t>(v & 0xffffu)); }
int16_t hi16(uint32_t v) noexcept { return static_cast<int16_t>(static_cast<uint16_t>(v >> 16)); }
// MIPS div and the compiler's "add 2^n - 1 when negative, then sra" both
// truncate toward zero.
int32_t tdiv(int32_t v, int32_t d) noexcept { return v / d; }

// Far tables 0x80097C78 / 0x80097C84 (three words each).
constexpr std::array<uint16_t,3> kFar97C78{{0x0E00, 0x0680, 0x0300}};
constexpr std::array<uint16_t,3> kFar97C84{{0x0D80, 0x0620, 0x02C0}};

struct Walk {
    const RuntimeMap& map;
    const RuntimeMapArea& area;
    const TerrainCellViewInput& in;
    TerrainCellVisibility& out;
    std::array<int16_t,9> r{};
    uint16_t far = 0;
    int32_t x0 = 0, z0 = 0, x1 = 0, z1 = 0;          // 0x1F800040..43
    int32_t nx0 = 0, nz0 = 0, nx1 = 0, nz1 = 0;      // 0x1F800100..106

    // The wedge runs can leave the area when the search rectangle lies outside
    // it (the original then reads past the grid); those cells count as empty.
    int16_t packed(int32_t x, int32_t z) const noexcept {
        if (x < x0 || x >= x1 || z < z0 || z >= z1) return 0;
        const uint32_t local = uint32_t(z - z0) * area.width + uint32_t(x - x0);
        return map.cells[area.cell_first + local];
    }
    // 0x80021834.
    void test(int32_t x, int32_t z, int16_t p, int32_t radius) noexcept {
        ++out.cells_tested;
        const int32_t vx = int16_t(x * 0x200 - 0x7F00), vz = int16_t(z * 0x200 - 0x7F00);
        int32_t vy;
        if (p < 1) vy = int16_t(-int16_t((int32_t(p) & 0x1f00) >> 1));
        else {
            if (size_t(p) >= map.idx.size()) return;
            vy = map.idx[size_t(p)].y;
        }
        // gte_rt (sf=1) + stlvnl: MAC1..3.
        const auto mac = [&](int row) {
            const int64_t t = int64_t(in.translation_1f800030[size_t(row)]) * 4096 +
                int64_t(r[size_t(3*row)]) * vx + int64_t(r[size_t(3*row+1)]) * vy + int64_t(r[size_t(3*row+2)]) * vz;
            return int32_t(t >> 12);
        };
        const int32_t mx = mac(0), mz = mac(2);
        int32_t z4 = mz >> 2;
        if (z4 >= int32_t(far)) return;
        int32_t z8 = z4 << 3;
        if (z4 < 0) { z4 = 0; z8 = 0; }
        int32_t side = (z8 * 15 + z4 * -14) >> 6;
        if (in.widen_for_400) side = side * 5 / 4;   // CANDIDATE: 200/384 for the 400-pixel view
        side += radius;
        if (mx < -side || mx > side) return;
        if (p < 0) {
            const uint32_t local = uint32_t(z - z0) * area.width + uint32_t(x - x0);
            if (!out.negative_visible[local]) { out.negative_visible[local] = 1; ++out.negative_cells; }
        } else if (!out.idx_visible[size_t(p)]) {
            out.idx_visible[size_t(p)] = 1; ++out.positive_cells;
        }
    }
    bool in_near(int32_t x, int32_t z) const noexcept {
        return !(x < nx0 || x >= nx1 || z < nz0 || z >= nz1);
    }
    // 0x80021258: one column per x, a z run between the two wedge edges.
    void walk_columns(int32_t rx0, int32_t rx1, int32_t rz0, int32_t rz1, int16_t sa, int16_t sb) noexcept {
        const bool flip = ((int32_t(in.yaw_800a3a12) - 0x200) & 0x800) != 0;
        const int32_t a = tdiv(in.eye_x_1f800028 + 0x100, 512) - rx0 + (flip ? 0x41 : 0x3E);
        const int32_t s_hi = flip ? sb : sa, s_lo = flip ? sa : sb;
        const int32_t base = tdiv(in.eye_z_1f80002c + 0x100, 512);
        int32_t hi_acc = a * s_hi, lo_acc = a * s_lo;
        for (int32_t x = rx0; x < rx1; ++x) {
            int32_t lo = base + tdiv(lo_acc, 4096) + 0x3D, hi = base + tdiv(hi_acc, 4096) + 0x42;
            lo = lo < rz0 ? rz0 : (lo > rz1 ? rz1 : lo);
            hi = hi < rz0 ? rz0 : (hi > rz1 ? rz1 : hi);
            for (int32_t z = lo; z < hi; ++z) {
                if (in_near(x, z)) continue;
                if (const int16_t p = packed(x, z)) test(x, z, p, 400);
            }
            hi_acc -= s_hi; lo_acc -= s_lo;
        }
    }
    // 0x80021548: one row per z, an x run between the two wedge edges.
    void walk_rows(int32_t rx0, int32_t rx1, int32_t rz0, int32_t rz1, int16_t sa, int16_t sb) noexcept {
        const bool flip = ((int32_t(in.yaw_800a3a12) - 0x200) & 0x800) != 0;
        const int32_t a = tdiv(in.eye_z_1f80002c + 0x100, 512) - rz0 + (flip ? 0x3E : 0x41);
        const int32_t s_hi = flip ? sb : sa, s_lo = flip ? sa : sb;
        const int32_t base = tdiv(in.eye_x_1f800028 + 0x100, 512);
        int32_t hi_acc = a * s_hi, lo_acc = a * s_lo;
        for (int32_t z = rz0; z < rz1; ++z) {
            int32_t lo = base + tdiv(lo_acc, 4096) + 0x3D, hi = base + tdiv(hi_acc, 4096) + 0x42;
            lo = lo < rx0 ? rx0 : (lo > rx1 ? rx1 : lo);
            hi = hi < rx0 ? rx0 : (hi > rx1 ? rx1 : hi);
            for (int32_t x = lo; x < hi; ++x) {
                if (in_near(x, z)) continue;
                if (const int16_t p = packed(x, z)) test(x, z, p, 400);
            }
            hi_acc -= s_hi; lo_acc -= s_lo;
        }
    }
};
} // namespace

std::array<int32_t,3> terrain_cell_translation_1f800030(const std::array<uint32_t,8>& w,
    const std::array<int32_t,3>& eye) noexcept {
    const std::array<int16_t,9> m{{lo16(w[0]), hi16(w[0]), lo16(w[1]), hi16(w[1]), lo16(w[2]), hi16(w[2]),
        lo16(w[3]), hi16(w[3]), lo16(w[4])}};
    const auto t = source_apply_matrix_lv_8007a9b8(m, {{0u - uint32_t(eye[0]), 0u - uint32_t(eye[1]), 0u - uint32_t(eye[2])}});
    return {{int32_t(t[0]), int32_t(t[1]), int32_t(t[2])}};
}

uint16_t terrain_cell_far_1f800076(uint8_t lod, uint8_t flags) noexcept {
    if (lod > 2) return 0;
    return (flags & 4) ? kFar97C84[lod] : kFar97C78[lod];
}

void compute_terrain_cell_visibility(const RuntimeMap& map, size_t area_index,
    const ReferenceTables& refs, const TerrainCellViewInput& in, TerrainCellVisibility& out) {
    out.valid = false;
    out.cells_tested = out.positive_cells = out.negative_cells = 0;
    if (area_index >= map.areas.size()) return;
    const auto& area = map.areas[area_index];
    if (size_t(area.cell_first) + size_t(area.width) * area.height > map.cells.size()) return;
    out.idx_visible.assign(map.idx.size(), 0);
    out.negative_visible.assign(size_t(area.width) * area.height, 0);
    Walk w{map, area, in, out};
    const auto& c = in.camera_1f800008;
    w.r = {{lo16(c[0]), hi16(c[0]), lo16(c[1]), hi16(c[1]), lo16(c[2]), hi16(c[2]), lo16(c[3]), hi16(c[3]), lo16(c[4])}};
    w.far = terrain_cell_far_1f800076(in.lod_1f800074, in.flags_1f800078);
    w.x0 = area.start_x; w.z0 = area.start_z;
    w.x1 = area.start_x + area.width; w.z1 = area.start_z + area.height;
    const auto rsin = [&](int32_t a) { return reference_rsin_8007a060(refs, a); };
    const auto rcos = [&](int32_t a) { return reference_rcos_8007a12c(refs, a); };

    if (in.mode_1f800075 == 1) {
        // 0x80020A4C: the whole area, radius 800.
        for (int32_t z = w.z0; z < w.z1; ++z)
            for (int32_t x = w.x0; x < w.x1; ++x)
                if (const int16_t p = w.packed(x, z)) w.test(x, z, p, 800);
        out.valid = true;
        return;
    }

    // 0x80020BB4: search rectangle 2*|cos pitch| ahead along the yaw.
    const int32_t yaw = in.yaw_800a3a12, pitch = in.pitch_800a3a10;
    const auto clamp_fff = [](int32_t v) { return v == 0x1000 ? 0xfff : (v == -0x1000 ? -0xfff : v); };
    const int32_t sy = clamp_fff(rsin(yaw)), cy = clamp_fff(rcos(yaw));
    const int32_t cp = std::abs(rcos(pitch));
    const int32_t cx = tdiv(in.eye_x_1f800028 + tdiv(sy * cp * 2, 4096), 512) + 0x40;
    const int32_t cz = tdiv(in.eye_z_1f80002c + tdiv(cy * cp * 2, 4096), 512) + 0x40;
    const int32_t half = cp >> 8;
    const int32_t rx0 = std::max(cx - half, w.x0), rx1 = std::min(cx + half, w.x1);
    const int32_t rz0 = std::max(cz - half, w.z0), rz1 = std::min(cz + half, w.z1);

    // Near window 0x1F800100..106 around the camera target.
    const int32_t sp = std::abs(tdiv(rsin(pitch), 0x160));
    const int32_t tx = tdiv(in.target_x_800a39a0 + 0x100, 512), tz = tdiv(in.target_z_800a39a8 + 0x100, 512);
    w.nx0 = int16_t(tx - sp + 0x3D); if (w.nx0 < w.x0) w.nx0 = w.x0;
    w.nx1 = int16_t(tx + sp + 0x43); if (w.nx1 > w.x1) w.nx1 = w.x1;
    w.nz0 = int16_t(tz - sp + 0x3D); if (w.nz0 < w.z0) w.nz0 = w.z0;
    w.nz1 = int16_t(tz + sp + 0x43); if (w.nz1 > w.z1) w.nz1 = w.z1;

    // Wedge edge slopes 0x1F800108/10A from yaw -/+ 0x100.
    const int32_t a0 = (yaw - 0x100) & 0xfff, a1 = (yaw + 0x100) & 0xfff;
    const int32_t s0 = rsin(a0), s1 = rsin(a1), c0 = rcos(a0), c1 = rcos(a1);
    const bool columns = ((yaw - 0x200) & 0x400) == 0;
    const int32_t n0 = columns ? (c0 + 1) * 0x1000 : (s0 + 1) * 0x1000;
    const int32_t d0 = columns ? -(s0 + 1) : -(c0 + 1);
    const int32_t n1 = columns ? (c1 + 1) * 0x1000 : (s1 + 1) * 0x1000;
    const int32_t d1 = columns ? -(s1 + 1) : -(c1 + 1);
    if (d0 == 0 || d1 == 0) return;   // the original traps (break 7)
    const int16_t sa = int16_t(n0 / d0), sb = int16_t(n1 / d1);
    if (columns) w.walk_columns(rx0, rx1, rz0, rz1, sa, sb);
    else w.walk_rows(rx0, rx1, rz0, rz1, sa, sb);

    // The near window itself, radius 600.
    for (int32_t z = w.nz0; z < w.nz1; ++z)
        for (int32_t x = w.nx0; x < w.nx1; ++x)
            if (const int16_t p = w.packed(x, z)) w.test(x, z, p, 600);
    out.valid = true;
}

} // namespace l3
