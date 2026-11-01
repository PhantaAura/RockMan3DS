#include "legends3ds/source_intro_crawl.h"

namespace l3 {
namespace {
bool known(const SourceC0Work& w, size_t at, size_t n) noexcept { for (size_t i = 0; i < n; ++i) if (!w.known[at + i]) return false; return true; }
void put(SourceC0Work& w, size_t at, size_t n, uint32_t v) noexcept {
    for (size_t i = 0; i < n; ++i) { w.raw[at + i] = uint8_t(v >> (8 * i)); w.known[at + i] = 1; }
}
// 0x80100A70: strips outside 0..1023 are skipped (bltz / slti 1024).
void strip(SourceIntroCrawlFrame& f, int y, int v, uint32_t colour) noexcept {
    if (v < 0 || v >= 1024 || f.count >= f.strips.size()) return;
    f.strips[f.count++] = {int16_t(y), uint16_t(v), colour};
}
} // namespace

SourceIntroCrawlFrame update_source_intro_crawl_801008c0(SourceC0Work& w) noexcept {
    SourceIntroCrawlFrame f;
    if (!known(w, 8, 1)) { f.held = true; return f; }
    if (w.raw[8] == 0) {   // 0x801008F8
        put(w, 9, 1, 0); put(w, 10, 1, 0); put(w, 11, 1, 0);
        put(w, 0x1A, 2, uint16_t(int16_t(-190)));
        put(w, 8, 1, 1);
        return f;
    }
    if (w.raw[8] != 1 || !known(w, 0x0B, 1) || !known(w, 0x1A, 2)) { f.held = true; return f; }
    const int scroll = int16_t(uint16_t(w.raw[0x1A] | (w.raw[0x1B] << 8)));
    for (int i = 0; i < 16; ++i)   // fade in: grey i*8, semi-transparent
        strip(f, 32 + i, scroll + i, 0x02000000u | (uint32_t(i) << 19) | (uint32_t(i) << 11) | (uint32_t(i) << 3));
    for (int i = 0; i < 144; ++i)
        strip(f, 48 + i, scroll + 16 + i, 0x02808080u);
    for (int i = 0; i < 16; ++i) { // fade out: grey (16 - i)*8
        const uint32_t g = uint32_t(16 - i);
        strip(f, 192 + i, scroll + 160 + i, 0x02000000u | (g << 19) | (g << 11) | (g << 3));
    }
    uint8_t counter = uint8_t(w.raw[0x0B] + 1);
    int next = scroll;
    if (counter >= 3) { counter = 0; ++next; put(w, 0x1A, 2, uint16_t(int16_t(next))); }
    put(w, 0x0B, 1, counter);
    if (next >= 1024) f.released = true;
    return f;
}

FrontendPacket source_intro_crawl_packet_80100a70(const SourceIntroCrawlStrip& s) noexcept {
    FrontendPacket p; p.length = 9; p.bucket = kSourceIntroCrawlOtEntry;
    const uint32_t y0 = uint16_t(s.y), y1 = uint16_t(s.y + 1), v = s.v & 0xFFu;
    const uint32_t tpage = uint32_t((s.v >> 8) + 21) | 0x20u;
    p.words = {{0x2C000000u | s.colour,
                (y0 << 16) | 32u, (0x7C00u << 16) | (v << 8) | 0u,
                (y0 << 16) | 287u, (tpage << 16) | (v << 8) | 255u,
                (y1 << 16) | 32u, (v << 8) | 0u,
                (y1 << 16) | 287u, (v << 8) | 255u}};
    return p;
}

namespace {
void callback_801002e4(SourceIntroEvent71State& st, SourceIntroEvent71Step& out, SourceBgLayers* layers) noexcept {
    const uint32_t t = st.timeline_ticks;
    if (t == 0) {
        if (layers) {
            (*layers)[0].x = 130; (*layers)[0].flags = uint8_t((*layers)[0].flags & ~2u);
            (*layers)[2].vx = int16_t((*layers)[2].vx - 56);
            (*layers)[1].vx = int16_t((*layers)[1].vx - 16);
        }
        out.airship_action1 = true;
        st.countdown_0c = 3;
    }
    if (t == 2200 && layers) (*layers)[0].flags = uint8_t((*layers)[0].flags | 2u);
    if (st.byte_06 == 0 && t > 2200 && --st.countdown_0c == 0) {
        st.countdown_0c = 3;
        if (layers) {
            auto& x = (*layers)[0].x;
            x = int16_t(x - 1);
            if (x == -11) ++st.byte_06;
        }
    }
}
} // namespace

SourceIntroEvent71Step step_source_intro_event71_8010049c(SourceIntroEvent71State& st, int16_t busy, SourceBgLayers* layers) noexcept {
    SourceIntroEvent71Step out;
    const auto timeline = [&] { if (st.timeline_ticks < kSourceIntroTimelineTicks) ++st.timeline_ticks; };
    switch (st.phase) {
    case 0: ++st.phase; break;                                   // 0x801004D8
    case 1: out.cd_command = true; ++st.phase; break;            // 0x80100504
    case 2: st.countdown_0c = 5; ++st.phase; break;              // 0x80100520
    case 3: if (--st.countdown_0c == 0) ++st.phase; break;       // 0x8010053C
    case 4: out.fade_type = 2; ++st.phase; break;                // 0x80100564 (+ constructor 0x80100400)
    case 5:                                                      // 0x80100584
        if (busy == 0 && st.timeline_ticks >= kSourceIntroTimelineTicks) { out.fade_type = 10; st.phase = 10; }
        if (st.timeline_ticks < kSourceIntroTimelineTicks) callback_801002e4(st, out, layers);   // 0x8010045C
        timeline();
        break;
    case 10:                                                     // 0x801005D0
        if (st.timeline_ticks < kSourceIntroTimelineTicks) callback_801002e4(st, out, layers);
        timeline();
        if (busy == 0) st.done = true;
        break;
    default: break;
    }
    return out;
}
} // namespace l3
