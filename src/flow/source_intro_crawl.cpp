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
} // namespace l3
