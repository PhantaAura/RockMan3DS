// Background layers 0x80022FD4 / 0x80023144 / 0x8002366C and the ST01
// callback 0x801002E4, on authored descriptors.
#include "legends3ds/source_bg_layers.h"
#include "legends3ds/source_intro_crawl.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace l3;
namespace {
void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); } }
SourceBgDescriptor desc(uint16_t flags, int16_t w, int16_t h, uint16_t tpage, uint16_t add, int16_t vx, uint16_t ot) {
    SourceBgDescriptor d; d.present = true;
    d.init = {{flags, uint16_t(w), uint16_t(h), tpage, add, uint16_t(vx), 0, 0, 0x8080, uint16_t((ot << 8) | 0x80), 0}};
    for (int i = 0; i < w * h; ++i) d.tiles.push_back(0x78010000u | uint32_t(i % 8) * 32u);
    return d;
}
uint32_t crc(const uint8_t* p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
// An empty-layer L3BG package (v2, or v3 with an area record).
std::vector<uint8_t> package(uint16_t version, bool record) {
    std::vector<uint8_t> b(kSourceBgProfileBytes + (record ? 40 : 0), 0);
    b[0] = 'L'; b[1] = '3'; b[2] = 'B'; b[3] = 'G'; b[4] = uint8_t(version); b[6] = 16;
    const uint32_t n = uint32_t(b.size());
    for (int i = 0; i < 4; ++i) b[8 + i] = uint8_t(n >> (8 * i));
    if (record) { b[kSourceBgProfileBytes + 32] = 0x40; b[kSourceBgProfileBytes + 0x24] = 0x33; }
    const uint32_t c = crc(b.data() + 16, b.size() - 16);
    for (int i = 0; i < 4; ++i) b[12 + i] = uint8_t(c >> (8 * i));
    return b;
}
}
int main() {
    {
        SourceBgProfile p;
        require(load_source_bg_profile(package(2, false), p) && !source_bg_area_record(package(2, false)), "L3BG v2: no record");
        const auto v3 = package(3, true);
        const auto r = source_bg_area_record(v3);
        require(load_source_bg_profile(v3, p) && r && (*r)[32] == 0x40 && (*r)[0x24] == 0x33, "L3BG v3: the area record");
        require(!load_source_bg_profile(package(2, true), p) && !load_source_bg_profile(package(3, false), p), "version/size must agree");
    }
    SourceBgProfile prof{};
    prof[0] = desc(0x0100, 20, 8, 0x1C, 0x1F20, 0, 6);   // hidden, no update (the lighthouse)
    prof[1] = desc(0x0103, 20, 8, 0x1D, 0x1F20, 0, 6);
    prof[3] = desc(0x0103, 20, 8, 0x1C, 0x1F20, -512, 6);
    SourceBgLayers L{};
    init_source_bg_layers_80022fd4(prof, L);
    require(L[0].present && !L[2].present && L[3].mode == 1 && L[3].vx == -512 && L[3].ot_entry == 6 &&
            L[3].colour == 0x06808080u && L[3].x16 == 0, "init fields");
    // Velocity -512/256 = -2 px per update; wraps into [0, 640).
    auto u = update_source_bg_layers_80023144(L);
    require(u.updated == 2 && L[3].x == 638 && L[3].x16 == (638 << 16) && L[0].x == 0, "velocity scroll wraps");
    // Layer 3 at x 638: first column 638/32 = 19, starts at x -30; 11 columns x 8 rows.
    auto pk = draw_source_bg_layers_80023144(L);
    require(pk.size() == 11 * 8 + 10 * 8, "layer 3 (x -30) 11 columns, layer 1 (x 0) 10 columns, 8 rows");
    require(pk[0].bucket == 34 && int16_t(pk[0].words[1]) == -30 && int16_t(pk[0].words[1] >> 16) == 0, "layer 3 first, at x -30");
    const uint32_t t = 0x78010000u + 3u * 32u + (0x1F20u << 14);   // map cell 19 -> i % 8 = 3
    require(pk[0].words[2] == ((t >> 16) << 16 | (t & 0xFFFFu)) && (pk[0].words[4] >> 16) == 0x1C && pk[0].words[0] == 0x2C808080u,
            "tile word + (0x28 << 14), tpage, colour");
    require(int16_t(pk[11 * 8].words[1]) == 0 && (pk[11 * 8].words[4] >> 16) == 0x1D, "layer 1 drawn after layer 3 (AddPrim order)");
    // Widescreen margin 40: two more columns on the left, x up to 360.
    require(draw_source_bg_layers_80023144(L, 40).size() == (15 + 14) * 8, "margin columns");
    // Callback: tick 0 hides layer 0 (already hidden), x = 130, vx changes; 2200 shows it;
    // then x -1 every 3 ticks to -11.
    SourceBgLayers C{}; prof[2] = desc(0x0103, 20, 8, 0x1E, 0x1F20, 0, 6);
    init_source_bg_layers_80022fd4(prof, C);
    SourceIntroEvent71State ev; ev.phase = 5;
    auto st = step_source_intro_event71_8010049c(ev, 0, &C);
    require(st.airship_action1 && C[0].x == 130 && C[1].vx == -16 && C[2].vx == -56 && ev.countdown_0c == 3, "callback tick 0");
    while (ev.timeline_ticks <= 2200) step_source_intro_event71_8010049c(ev, 0, &C);
    require((C[0].flags & 2u) && C[0].x == 130, "layer 0 shown at 2200");
    for (int i = 0; i < 3; ++i) step_source_intro_event71_8010049c(ev, 0, &C);
    require(C[0].x == 129, "slide 1 px per 3 ticks");
    while (!ev.byte_06 && ev.timeline_ticks < kSourceIntroTimelineTicks) step_source_intro_event71_8010049c(ev, 0, &C);
    require(C[0].x == -11 && ev.byte_06 == 1 && ev.timeline_ticks == 2201 + 3 * 141, "slide ends at -11");
    std::puts("source_bg_layers: init, velocity wrap, tile walk, draw order, margin, ST01 callback OK");
}
