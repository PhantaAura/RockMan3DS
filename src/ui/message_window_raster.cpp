#include "legends3ds/message_window_raster.h"
#include <algorithm>
#include <cmath>

namespace l3 {

void MessageFont::add(const MaterialCache& cache) {
    // One page per TPAGE per cache: two caches may hold different images for
    // the same TPAGE (SUB01 / SUB02U both load to page 12); lookups then go
    // by TPAGE and CLUT.
    const size_t first = pages.size();
    for (const auto& e : cache.entries) {
        if (!e.resolved || e.palette_ps1.size() != 16 || e.indices.size() != 256u * 256u) continue;
        const uint16_t tpage = uint16_t(e.material_word);
        Page* page = nullptr;
        for (size_t i = first; i < pages.size(); ++i) if (pages[i].tpage == tpage) page = &pages[i];
        if (!page) { pages.push_back({}); page = &pages.back(); page->tpage = tpage; page->indices = e.indices; }
        std::array<uint16_t, 16> pal{};
        std::copy(e.palette_ps1.begin(), e.palette_ps1.end(), pal.begin());
        page->palettes.emplace_back(uint16_t(e.material_word >> 16), pal);
    }
}

MessageFont message_font_from_cache(const MaterialCache& cache) {
    MessageFont f; f.add(cache); return f;
}

namespace {
struct Vtx { float x, y, u, v, r, g, b; };

int16_t sx(uint32_t w) { return int16_t(uint16_t(w)); }
int16_t sy(uint32_t w) { return int16_t(uint16_t(w >> 16)); }

// One triangle, pixel centres, gouraud colour and affine UV. shade(px, py, v)
// writes the pixel.
template <typename Shade> void triangle(const Vtx& a, const Vtx& b, const Vtx& c, Shade shade) {
    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::fabs(area) < 1e-6f) return;
    const int x0 = std::max(0, int(std::floor(std::min({a.x, b.x, c.x}))));
    const int x1 = std::min(320, int(std::ceil(std::max({a.x, b.x, c.x}))));
    const int y0 = std::max(0, int(std::floor(std::min({a.y, b.y, c.y}))));
    const int y1 = std::min(240, int(std::ceil(std::max({a.y, b.y, c.y}))));
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const float px = x + 0.5f, py = y + 0.5f;
            float w0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) / area;
            float w1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) / area;
            float w2 = 1.f - w0 - w1;
            const float eps = -1e-4f;
            if (w0 < eps || w1 < eps || w2 < eps) continue;
            Vtx v{px, py, w0 * a.u + w1 * b.u + w2 * c.u, w0 * a.v + w1 * b.v + w2 * c.v,
                  w0 * a.r + w1 * b.r + w2 * c.r, w0 * a.g + w1 * b.g + w2 * c.g, w0 * a.b + w1 * b.b + w2 * c.b};
            shade(x, y, v);
        }
}

Vtx vtx(uint32_t xy, uint32_t rgb, float u = 0, float v = 0) {
    return {float(sx(xy)), float(sy(xy)), u, v, float(rgb & 255), float((rgb >> 8) & 255), float((rgb >> 16) & 255)};
}
} // namespace

size_t rasterize_message_windows(const std::vector<FrontendPacket>& packets, const MessageFont& font, uint8_t* rgb) {
    if (!rgb) return 0;
    uint16_t max_bucket = 0;
    for (const auto& p : packets) max_bucket = std::max(max_bucket, p.bucket);
    unsigned abr = 0;
    size_t drawn = 0;
    const auto put = [&](int x, int y, int r, int g, int b) {
        uint8_t* d = rgb + (size_t(y) * 320u + size_t(x)) * 3u;
        d[0] = uint8_t(std::clamp(r, 0, 255)); d[1] = uint8_t(std::clamp(g, 0, 255)); d[2] = uint8_t(std::clamp(b, 0, 255));
    };
    for (int bucket = max_bucket; bucket >= 0; --bucket)
        for (size_t i = packets.size(); i-- > 0;) {
            const auto& p = packets[i];
            if (p.bucket != bucket || !p.length) continue;
            const uint32_t* w = p.words.data();
            const uint8_t code = uint8_t(w[0] >> 24);
            if (code == 0xE1) { abr = (w[0] >> 5) & 3u; continue; }
            if ((code & 0xFC) == 0x2C && p.length == 9) {          // textured quad (glyph)
                const uint16_t clut = uint16_t(w[2] >> 16), tpage = uint16_t(w[4] >> 16);
                const MessageFont::Page* page = nullptr;
                const std::array<uint16_t, 16>* pal = nullptr;
                for (const auto& pg : font.pages) {
                    if (pg.tpage != tpage) continue;
                    for (const auto& [c, colours] : pg.palettes) if (c == clut) { page = &pg; pal = &colours; }
                    if (pal) break;
                }
                if (!pal) continue;
                const uint32_t rgbm = w[0] & 0xFFFFFF;
                const auto tv = [&](unsigned k) {   // xy words 1,3,5,7; uv words 2,4,6,8
                    return vtx(w[1 + 2 * k], rgbm, float(uint8_t(w[2 + 2 * k])), float(uint8_t(w[2 + 2 * k] >> 8)));
                };
                const Vtx v0 = tv(0), v1 = tv(1), v2 = tv(2), v3 = tv(3);
                const auto shade = [&](int x, int y, const Vtx& v) {
                    const int u = std::clamp(int(v.u), 0, 255), t = std::clamp(int(v.v), 0, 255);
                    const uint16_t c = (*pal)[page->indices[size_t(t) * 256u + size_t(u)] & 15u];
                    if (!c) return;   // PS1: colour 0 is transparent
                    const int r = (c & 31) * 8, g = ((c >> 5) & 31) * 8, b = ((c >> 10) & 31) * 8;
                    put(x, y, r * int(v.r) / 128, g * int(v.g) / 128, b * int(v.b) / 128);
                };
                triangle(v0, v1, v2, shade); triangle(v1, v3, v2, shade);
                ++drawn; continue;
            }
            const bool semi = code & 2;
            const auto blend = [&](int x, int y, const Vtx& v) {
                uint8_t* d = rgb + (size_t(y) * 320u + size_t(x)) * 3u;
                const int f[3] = {int(v.r), int(v.g), int(v.b)};
                int o[3];
                for (int k = 0; k < 3; ++k) {
                    const int bk = d[k];
                    o[k] = !semi ? f[k] : abr == 0 ? (bk + f[k]) / 2 : abr == 1 ? bk + f[k] : abr == 2 ? bk - f[k] : bk + f[k] / 4;
                }
                put(x, y, o[0], o[1], o[2]);
            };
            if ((code & 0xFC) == 0x28 && p.length == 5) {           // flat quad
                const uint32_t c = w[0] & 0xFFFFFF;
                const Vtx v0 = vtx(w[1], c), v1 = vtx(w[2], c), v2 = vtx(w[3], c), v3 = vtx(w[4], c);
                triangle(v0, v1, v2, blend); triangle(v1, v3, v2, blend);
                ++drawn; continue;
            }
            if ((code & 0xFC) == 0x38 && p.length == 8) {           // gouraud quad
                const Vtx v0 = vtx(w[1], w[0] & 0xFFFFFF), v1 = vtx(w[3], w[2]), v2 = vtx(w[5], w[4]), v3 = vtx(w[7], w[6]);
                triangle(v0, v1, v2, blend); triangle(v1, v3, v2, blend);
                ++drawn; continue;
            }
        }
    return drawn;
}

} // namespace l3
