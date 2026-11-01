// ST01 narration crawl (0x801008C0 / 0x801008F8 / 0x80100918 / 0x80100A70) on a C0 slot.
#include "legends3ds/source_intro_crawl.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
namespace {
void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); } }
int scroll(const SourceC0Work& w) { return int16_t(uint16_t(w.raw[0x1A] | (w.raw[0x1B] << 8))); }
}
int main() {
    SourceC0Work w;
    require(update_source_intro_crawl_801008c0(w).held, "unknown state holds");
    w.raw[8] = 0; w.known[8] = 1;
    auto f = update_source_intro_crawl_801008c0(w);
    require(!f.held && f.count == 0 && w.raw[8] == 1 && scroll(w) == -190 && w.raw[0x0B] == 0, "init: scroll -190, state 1, no draw");
    // Oracle: +0xB counts 1,2,3->0 per tick and +0x1A steps once per 3 ticks.
    f = update_source_intro_crawl_801008c0(w);
    require(f.count == 0 && w.raw[0x0B] == 1 && scroll(w) == -190, "first update: nothing visible yet");
    update_source_intro_crawl_801008c0(w); update_source_intro_crawl_801008c0(w);
    require(w.raw[0x0B] == 0 && scroll(w) == -189, "third update steps the scroll");
    // Scroll 0: all 176 strips; fade rows and colours as 0x80100918 builds them.
    w.raw[0x1A] = 0; w.raw[0x1B] = 0;
    f = update_source_intro_crawl_801008c0(w);
    require(f.count == 176, "full window");
    require(f.strips[0].y == 32 && f.strips[0].v == 0 && f.strips[0].colour == 0x02000000u, "first fade-in strip black");
    require(f.strips[15].y == 47 && f.strips[15].colour == (0x02000000u | (15u << 19) | (15u << 11) | (15u << 3)), "last fade-in strip");
    require(f.strips[16].y == 48 && f.strips[16].v == 16 && f.strips[16].colour == 0x02808080u, "body strip");
    require(f.strips[160].y == 192 && f.strips[160].v == 160 && f.strips[160].colour == (0x02000000u | (16u << 19) | (16u << 11) | (16u << 3)), "fade-out start");
    // Near the end only rows < 1024 draw; 1024 releases.
    w.raw[0x1A] = uint8_t(1000 & 0xFF); w.raw[0x1B] = uint8_t(1000 >> 8);
    f = update_source_intro_crawl_801008c0(w);
    require(f.count == 16 + 8 && !f.released, "rows past 1023 skipped");
    w.raw[0x1A] = uint8_t(1023 & 0xFF); w.raw[0x1B] = uint8_t(1023 >> 8); w.raw[0x0B] = 2;
    f = update_source_intro_crawl_801008c0(w);
    require(f.released && scroll(w) == 1024, "release at 1024");
    std::puts("source_intro_crawl: init, 3-tick scroll, strip window, fades, release OK");
}
