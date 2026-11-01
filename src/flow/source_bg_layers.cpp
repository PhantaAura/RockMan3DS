#include "legends3ds/source_bg_layers.h"

namespace l3 {
namespace {
uint16_t rd16(const uint8_t* p) noexcept { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t* p) noexcept { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
uint32_t crc32(const uint8_t* p, size_t n) noexcept {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
// MIPS div truncates toward zero; the renderer biases negatives by 31 first.
int div32(int v) noexcept { return (v < 0 ? v + 31 : v) >> 5; }
// 0x8002366C: wrap a 16.16 position into [0, cells * 32 << 16).
int32_t wrap(int32_t v, int16_t cells) noexcept {
    const int32_t span = int32_t(uint32_t(int32_t(cells)) << 21);
    if (span <= 0) return v;
    while (v >= span) v -= span;
    while (v < 0) v += span;
    return v;
}
} // namespace

bool load_source_bg_profile(const std::vector<uint8_t>& b, SourceBgProfile& out) {
    if (b.size() != kSourceBgProfileBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'B' || b[3] != 'G' ||
        rd16(&b[4]) != 2 || rd16(&b[6]) != 16 || rd32(&b[8]) != b.size() || rd32(&b[12]) != crc32(b.data() + 16, b.size() - 16))
        return false;
    SourceBgProfile p{};
    const uint8_t* r = b.data() + 16 + 32;
    for (auto& d : p) {
        d.present = r[0] == 1;
        for (size_t k = 0; k < 11; ++k) d.init[k] = rd16(r + 4 + 2 * k);
        if (d.present) {
            const size_t cells = size_t(d.init[1]) * d.init[2];
            if (!cells || cells > kSourceBgMaxCells) return false;
            for (size_t k = 0; k < cells; ++k) d.tiles.push_back(rd32(r + 26 + 4 * k));
        }
        r += 4 + 22 + 4 * kSourceBgMaxCells + 2;
    }
    out = std::move(p);
    return true;
}

void init_source_bg_layers_80022fd4(const SourceBgProfile& profile, SourceBgLayers& layers) noexcept {
    for (size_t i = 0; i < kSourceBgLayers; ++i) {
        const auto& d = profile[i];
        auto& l = layers[i];
        if (!d.present) { l = SourceBgLayer{}; continue; }
        const auto s = [&](size_t k) { return int16_t(d.init[k]); };
        l.present = true;
        l.flags = uint8_t(d.init[0]); l.mode = uint8_t(d.init[0] >> 8);
        l.width = s(1); l.height = s(2); l.tpage = d.init[3]; l.tile_add = d.init[4];
        l.vx = s(5); l.vy = 0; l.x28 = s(6); l.y30 = s(7);
        l.colour = uint32_t(d.init[8]) | (uint32_t(d.init[9]) << 16);
        l.ot_entry = uint8_t(d.init[9] >> 8);
        l.semi = uint8_t(d.init[10]); l.abr = uint8_t(d.init[10] >> 8);
        l.tiles = d.tiles;
        // Velocity layers start at (+0x1C, +0x1E) << 16 (0x80023108..0x80023124).
        if (l.mode) { l.x16 = int32_t(uint32_t(int32_t(l.x28)) << 16); l.y16 = int32_t(uint32_t(int32_t(l.y30)) << 16); }
    }
}

SourceBgUpdate update_source_bg_layers_80023144(SourceBgLayers& layers) noexcept {
    SourceBgUpdate out;
    for (auto& l : layers) {
        if (!l.present || !(l.flags & 1u)) continue;
        if (!l.mode) { ++out.camera_held; continue; }   // camera scroll 0x8002330C..0x8002340C
        l.x16 = int32_t(uint32_t(l.x16) + (uint32_t(int32_t(l.vx)) << 8));
        l.y16 = int32_t(uint32_t(l.y16) + (uint32_t(int32_t(l.vy)) << 8));
        l.x16 = wrap(l.x16, l.width); l.y16 = wrap(l.y16, l.height);
        l.x = int16_t(l.x16 >> 16); l.y = int16_t(l.y16 >> 16);
        ++out.updated;
    }
    return out;
}

std::vector<FrontendPacket> draw_source_bg_layers_80023144(const SourceBgLayers& layers, int margin) {
    std::vector<FrontendPacket> out;
    for (size_t i = kSourceBgLayers; i-- > 0;) {
        const auto& l = layers[i];
        if (!l.present || !(l.flags & 2u) || l.width <= 0 || l.height <= 0 ||
            l.tiles.size() != size_t(l.width) * size_t(l.height)) continue;
        const uint32_t tpage = uint32_t(l.tpage) | (uint32_t(l.abr) << 5);
        const uint32_t code = 0x2C000000u | (uint32_t(l.semi) << 25) | (l.colour & 0xFFFFFFu);
        const int cols = div32(l.x), rows = div32(l.y);
        const int x0 = -int16_t(l.x - cols * 32), y0 = -int16_t(l.y - rows * 32);
        int row = rows;
        for (int y = y0; y < 240; y += 32) {
            // The original starts at x0, column x / 32; the margin adds whole
            // columns to the left (wrapping) and runs past 320.
            const int extra = (margin + 31) / 32;
            int col = cols - extra;
            while (col < 0) col += l.width;
            for (int x = x0 - 32 * extra; x < 320 + margin; x += 32) {
                const int r = ((row % l.height) + l.height) % l.height, c = col % l.width;
                const uint32_t t = l.tiles[size_t(r) * size_t(l.width) + size_t(c)] + (uint32_t(l.tile_add) << 14);
                const uint32_t u = t & 0xFFu, v = (t >> 8) & 0xFFu, clut = t >> 16;
                // The sprite shows texels u..u+31; the port samples (u + 0.5) / 256
                // per vertex, so u + 32 would bleed the next tile's column.
                const uint32_t u1 = (u + 31) & 0xFFu, v1 = (v + 31) & 0xFFu;
                const uint32_t xa = uint16_t(x), xb = uint16_t(x + 32), ya = uint16_t(y), yb = uint16_t(y + 32);
                FrontendPacket p; p.length = 9; p.bucket = uint16_t(28 + l.ot_entry);
                p.words = {{code, (ya << 16) | xa, (clut << 16) | (v << 8) | u,
                            (ya << 16) | xb, (tpage << 16) | (v << 8) | u1,
                            (yb << 16) | xa, (v1 << 8) | u,
                            (yb << 16) | xb, (v1 << 8) | u1}};
                out.push_back(p);
                if (++col >= l.width) col = 0;
            }
            ++row;
        }
    }
    return out;
}
} // namespace l3
