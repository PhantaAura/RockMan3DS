// Family-C0 palette objects (0x80119398 / 0x80119800) on authored tables.
#include "legends3ds/source_palette_c0.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
namespace {
void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); } }
SourceEventPhase5Pool<16,36> fresh_pool() {
    SourceEventPhase5Pool<16,36> p;
    for (size_t i = 0; i < 16; ++i) { p.slots[i].known.fill(1); p.slots[i].raw[1] = uint8_t(0xC0 | i); }
    p.used[0] = 0; return p;
}
std::array<uint8_t,4> e(uint16_t index, uint8_t duration, uint8_t control) { return {{uint8_t(index), uint8_t(index >> 8), duration, control}}; }
uint16_t rgb(int r, int g, int b) { return uint16_t(r | (g << 5) | (b << 10)); }
}
int main() {
    SourcePaletteC0Profile prof; prof.loaded = true;
    // sub3 v0: palettes at word 0x100 -> 0x110 (4 steps) -> loop to entry 0.
    prof.sub3[0].base_token = 0x80110000u; prof.sub3[0].count = 3;
    prof.sub3[0].entries[0] = e(0x100, 4, 0); prof.sub3[0].entries[1] = e(0x110, 4, 0); prof.sub3[0].entries[2] = e(0x120, 2, 0x80);
    prof.sub3_destination[0] = {{12288, 1}};
    // sub3 v1 (unused by the blend checks below).
    prof.sub3[1].base_token = 0x80110100u; prof.sub3[1].count = 2;
    prof.sub3[1].entries[0] = e(0x170, 1, 0); prof.sub3[1].entries[1] = e(0x100, 1, 0xFF);
    prof.sub3_destination[1] = {{12304, 1}};
    // sub4 v0: 32 colours, snap mode (copy at step 0).
    prof.sub4[0].base_token = 0x80110200u; prof.sub4[0].count = 2;
    prof.sub4[0].entries[0] = e(0x200, 3, 0); prof.sub4[0].entries[1] = e(0x220, 3, 0x80);
    prof.sub4_record[0] = {{12416, 32, 0}};
    SourcePaletteBuffer buf;
    for (size_t i = 0; i < 0x300; ++i) buf.known[i] = 1;
    for (int i = 0; i < 16; ++i) { buf.words[0x100 + i] = rgb(0, 0, 0); buf.words[0x110 + i] = rgb(31, 16, 8); buf.words[0x120 + i] = rgb(4, 4, 4); }
    for (int i = 0; i < 32; ++i) { buf.words[0x200 + i] = uint16_t(0x8000 | i); buf.words[0x220 + i] = uint16_t(0x8100 | i); }
    buf.words[0x170] = 0x70;

    // 0x8002ED6C spawn of the area-0 record 01 C0 03 00 ...
    auto pool = fresh_pool();
    std::array<uint8_t,20> rec{}; rec[0] = 1; rec[1] = 0xC0; rec[2] = 3;
    require(spawn_source_c0_record_8002ed6c(pool, rec) == size_t(0) && *pool.used[0] == 0x80000000u, "spawn slot 0");
    const auto& w = pool.slots[0];
    require(w.raw[0] == 1 && w.raw[2] == 3 && w.raw[3] == 0 && w.raw[6] == 3 && w.raw[8] == 0, "spawn bytes");

    // State 0: src = entry0, tgt = entry1, duration = entry1[2]; then blends 4 steps.
    auto p = update_source_palette_c0_80017478(pool, buf, prof);
    require(p.updated == 1 && w.raw[8] == 1 && w.raw[0x0C] == 4 && w.raw[0x0D] == 0, "init");
    for (int step = 1; step <= 4; ++step) {
        update_source_palette_c0_80017478(pool, buf, prof);
        const uint16_t want = uint16_t((31 * step / 4) | ((16 * step / 4) << 5) | 0x8000 | ((8 * step / 4) << 10));
        require(buf.words[12288] == want && buf.words[12288 + 15] == want, "blend value");
    }
    require(buf.changed_rows & (uint64_t(1) << 48), "mirror row 48 marked");
    // Step 5 > duration 4: advance to entry 2 (src 0x110, tgt 0x120, duration 2), blend step 0 = src.
    update_source_palette_c0_80017478(pool, buf, prof);
    require(w.raw[0x0C] == 2 && w.raw[0x0D] == 0 && buf.words[12288] == uint16_t(rgb(31, 16, 8) | 0x8000), "advance");
    // Loop control 0x80 after entry 2 returns to entry 0.
    for (int i = 0; i < 3; ++i) update_source_palette_c0_80017478(pool, buf, prof);
    require(w.raw[0x0C] == 4 && w.raw[0] == 1, "loop to entry 0");

    // Variant switch: slot runs table 0, the scene sets +3 = 1 (record 7 does);
    // at the entry whose byte 0 is 0x70 it moves to table 1 in state 2, whose
    // -1 end releases (+0 = 0).
    SourcePaletteC0Profile sw = prof;
    sw.sub3[0].count = 3; sw.sub3[0].entries[0] = e(0x100, 1, 0); sw.sub3[0].entries[1] = e(0x170, 1, 0); sw.sub3[0].entries[2] = e(0x100, 1, 0x80);
    sw.sub3[1].count = 2; sw.sub3[1].entries[0] = e(0x100, 1, 0); sw.sub3[1].entries[1] = e(0x110, 1, 0xFF);
    auto pool1 = fresh_pool(); rec[3] = 0;
    require(spawn_source_c0_record_8002ed6c(pool1, rec) == size_t(0), "spawn v0");
    update_source_palette_c0_80017478(pool1, buf, sw);           // init: cursor at the 0x70 entry
    pool1.slots[0].raw[3] = 1;                                      // 0x801009B0 frame 120
    bool switched = false;
    for (int i = 0; i < 4 && !switched; ++i) { update_source_palette_c0_80017478(pool1, buf, sw); switched = pool1.slots[0].raw[8] == 2; }
    require(switched, "0x70 switch to state 2");
    // A -1 entry releases only when duration == step at the advance, which
    // states 1/2 never reach (they advance at step = duration + 1): the last
    // segment re-loads and the palette settles on its target.
    for (int i = 0; i < 8; ++i) require(update_source_palette_c0_80017478(pool1, buf, sw).released == 0, "-1 end does not release");
    require(pool1.slots[0].raw[0] == 1 && buf.words[12304] == uint16_t(rgb(31, 16, 8) | 0x8000), "-1 end holds the last palette");

    // Subtype 4 snap mode: step 0 copies count/16 blocks (32 colours) from the source.
    auto pool4 = fresh_pool(); rec[2] = 4; rec[3] = 0;
    require(spawn_source_c0_record_8002ed6c(pool4, rec) == size_t(0), "spawn sub4");
    update_source_palette_c0_80017478(pool4, buf, prof);   // init (no blend in state 0)
    for (int i = 0; i < 3; ++i) update_source_palette_c0_80017478(pool4, buf, prof);
    update_source_palette_c0_80017478(pool4, buf, prof);   // advance (loop): src = old target 0x220, step 0 -> copy
    require(buf.words[12416] == 0x8100 && buf.words[12416 + 31] == uint16_t(0x8100 | 31), "snap copy 32 colours");

    // Unknown source colours hold atomically.
    auto pool2 = fresh_pool(); rec[2] = 3; rec[3] = 0; spawn_source_c0_record_8002ed6c(pool2, rec);
    SourcePaletteBuffer empty; const auto before = pool2.slots[0];
    update_source_palette_c0_80017478(pool2, empty, prof);
    update_source_palette_c0_80017478(pool2, empty, prof);
    require(pool2.slots[0].raw[0x0D] == 0 && update_source_palette_c0_80017478(pool2, empty, prof).held == 1, "unknown palette holds");
    (void)before;
    static_assert(kSourcePaletteC0ProfileBytes == 7410 && kSourcePaletteBankBytes == 24596, "cooked sizes (cook_source_palette_c0.py, l3cook_palette_bank)");
    std::puts("source_palette_c0: spawn, init, blend, advance/loop/end, 0x70 switch, sub4 snap, holds OK");
}
