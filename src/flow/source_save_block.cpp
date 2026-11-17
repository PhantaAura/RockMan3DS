#include "legends3ds/source_save_block.h"
#include <cstring>

namespace l3 {
namespace {
bool copy_exe(const ExeExcerpt& exe, uint32_t addr, uint8_t* dst, size_t n) {
    for (size_t i = 0; i < n; ++i) { const uint8_t* p = exe.at(uint32_t(addr + i)); if (!p) return false; dst[i] = *p; }
    return true;
}
uint32_t rd32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }

// 0x80065384's check words: six blocks {data, first word index, words, seed, check} and a
// master over them (same tables as src/p2/mml_retail_save_v2.c, verified on an oracle card).
enum class Base { Add, Not, Xor };
struct CheckBlock { uint32_t data_off, first_index, word_count, seed_off, check_off; Base mode0; uint32_t key0, key1, key2; };
constexpr CheckBlock kChecks[6] = {
    {0x000, 0, 92, 0x170, 0x174, Base::Add, 0, 0xE1E1E1E1u, 0x57575757u},
    {0x180, 96, 12, 0x1B0, 0x1B4, Base::Not, 0, 0x33333333u, 0xCCCCCCCCu},
    {0x1C0, 112, 12, 0x1F0, 0x1F4, Base::Xor, 0x69696969u, 0x2B2B2B2Bu, 0xB8B8B8B8u},
    {0x200, 128, 92, 0x370, 0x374, Base::Add, 0, 0x1E1E1E1Eu, 0x75757575u},
    {0x380, 224, 1644, 0x1D30, 0x1D34, Base::Not, 0, 0x33333333u, 0xCCCCCCCCu},
    {0x1D40, 1872, 104, 0x1EE0, 0x1EE4, Base::Xor, 0x96969696u, 0xB2B2B2B2u, 0x8B8B8B8Bu}};
void wr32(uint8_t* p, uint32_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); p[2] = uint8_t(v >> 16); p[3] = uint8_t(v >> 24); }
uint32_t rol16(uint32_t x) { return (x << 16) | (x >> 16); }
void generate_checks(uint8_t* b, const std::array<uint32_t,6>& seeds) {
    for (unsigned i = 0; i < 6; ++i) {
        const auto& k = kChecks[i];
        wr32(b + k.seed_off, seeds[i]);
        uint32_t s0 = 0, s1 = 0, s2 = 0; const uint32_t seed = seeds[i];
        for (uint32_t n = 0; n < k.word_count; ++n) {
            const uint32_t idx = k.first_index + n, w = rd32(b + k.data_off + n * 4);
            s0 += rol16(w);
            if (idx & 1u) s1 += w ^ (seed + idx); else s2 += w ^ (seed + idx);
        }
        const uint32_t o0 = k.mode0 == Base::Add ? seed + s0 : k.mode0 == Base::Not ? (~seed) + s0 : (seed ^ k.key0) + s0;
        wr32(b + k.check_off, o0); wr32(b + k.check_off + 4, (seed ^ k.key1) + s1); wr32(b + k.check_off + 8, (seed ^ k.key2) + s2);
    }
    auto r = [&](uint32_t o) { return rd32(b + o); };
    const uint32_t sa = r(0x170), sb = r(0x1B0), sc = r(0x1F0), sd = r(0x370), se = r(0x1D30), sf = r(0x1EE0);
    const uint32_t x = ((r(0x174) + r(0x1B4)) ^ sa) + ((r(0x1F4) + r(0x374)) ^ sc) + ((r(0x1D34) + r(0x1EE4)) ^ se);
    const uint32_t y = ((r(0x178) + r(0x1B8)) ^ sb) + ((r(0x1F8) + r(0x378)) ^ sd) + ((r(0x1D38) + r(0x1EE8)) ^ sf);
    const uint32_t z = ((r(0x17C) + r(0x1BC)) ^ sa) + ((r(0x1FC) + r(0x37C)) ^ sb) + ((r(0x1D3C) + r(0x1EEC)) ^ sc);
    wr32(b + 0x1EF0, (~x) + (y ^ 0xAAAAAAAAu) + (z ^ 0x55555555u)); wr32(b + 0x1EF4, x); wr32(b + 0x1EF8, y); wr32(b + 0x1EFC, z);
}
} // namespace

bool build_source_save_block_80065064(const ExeExcerpt& exe, const SourceSaveBlockInputs& in,
                                      std::array<uint8_t,kSourceSaveBlockBytes>& out) {
    out.fill(0);
    uint8_t* b = out.data();
    const uint8_t* c = in.globals_800c1b10.data();
    const uint8_t* p = in.player.bytes;

    // 0x800679B8: play time 0x800C1B1C (30 per second) -> digits 0x800A8800..05 and the
    // message glyphs 0x80097FD4.. (FD6 / FD9 keep the colon glyph 0x13).
    const uint32_t t = rd32(c + 0x0C);
    const uint32_t secs = t / 30u, mins_total = secs / 60u, s = secs - mins_total * 60u;
    const uint32_t hours = mins_total / 60u, m = mins_total - hours * 60u;
    const uint8_t digit[6] = {uint8_t(hours / 10u), uint8_t(hours % 10u), uint8_t(m / 10u), uint8_t(m % 10u),
                              uint8_t(s / 10u), uint8_t(secs % 10u)};
    uint8_t glyph[10]{};
    if (!copy_exe(exe, kSaveTimeGlyphs8008d028, glyph, 10)) return false;
    uint8_t sjis[22]{};
    if (!copy_exe(exe, kSaveSjisDigits80098124, sjis, 22)) return false;
    auto g = [&](uint8_t d) { return d < 10 ? glyph[d] : uint8_t(0); };

    // 0x80067D10 patches the title (0x8008FA64): slot digit at +0x20, h h : m m : s s at +0x24..+0x32.
    uint8_t title[0x40]{};
    if (!copy_exe(exe, kSaveTitle8008fa64, title, 0x40)) return false;
    auto put_sjis = [&](size_t off, unsigned d) { if (d <= 10) { title[off] = sjis[2 * d]; title[off + 1] = sjis[2 * d + 1]; } };
    put_sjis(0x20, unsigned(in.slot) + 1u);   // 0x80098126[slot]
    put_sjis(0x24, digit[0]); put_sjis(0x26, digit[1]);
    put_sjis(0x2A, digit[2]); put_sjis(0x2C, digit[3]);
    put_sjis(0x30, digit[4]); put_sjis(0x32, digit[5]);

    // 0x80064CB8: "SC" 0x11 0x01, title, zero +0x44..+0x5F, icon +0x80, CLUT +0x60.
    b[0] = 'S'; b[1] = 'C'; b[2] = 0x11; b[3] = 0x01;
    std::memcpy(b + 4, title, 0x40);
    if (!copy_exe(exe, kSaveIcon8008f9bc, b + 0x80, 0x80) || !copy_exe(exe, kSaveClut8008fa3c, b + 0x60, 0x20)) return false;

    // 0x80065064.
    b[0x100] = c[0x50];                                   // stage 0x800C1B60
    const uint8_t time_glyphs[8] = {g(digit[0]), g(digit[1]), 0x13, g(digit[2]), g(digit[3]), 0x13, g(digit[4]), g(digit[5])};
    std::memcpy(b + 0x101, time_glyphs, 8);               // from 0x80097FD4
    auto cp = [&](size_t dst, const uint8_t* src, size_t n) { std::memcpy(b + dst, src, n); };
    cp(0x128, c + 0x10, 8); cp(0x130, c + 0x18, 4);
    cp(0x180, c + 0x0C, 4); cp(0x184, c + 0x1C, 12);
    cp(0x1D00, c + 0x28, 24); cp(0x1D18, c + 0x40, 12);
    cp(0x190, c + 0x4C, 8);                                // +0x4C word, +0x50..+0x53 bytes
    cp(0x1D0, c + 0x60, 2); cp(0x1D2, c + 0x62, 2); cp(0x1E0, c + 0x64, 4);
    cp(0x204, c + 0x78, 4); cp(0x200, c + 0x7C, 2);
    cp(0x198, c + 0x54, 3); cp(0x19C, c + 0x58, 4);
    cp(0x1C0, p + 0x14, 8);                                // position (+ pad)
    cp(0x1C8, p + 0x56, 2); cp(0x1CC, p + 0x6E, 2); cp(0x1CE, p + 0xB0, 2);
    cp(0x1D4, p + 0x16A, 3); cp(0x1D8, p + 0x170, 4);
    b[0x1DC] = p[0x44C]; b[0x1DE] = p[0x44E]; b[0x1DF] = p[0x44F];
    cp(0x208, p + 0x450, 4);
    cp(0x20C, p + 0x454, 32);                              // 0x800176DC counts 32-byte units
    cp(0x1C80, p + 0x474, 128);
    cp(0x22C, p + 0x4F4, 256);
    cp(0x380, in.flags_800be378.data(), 256);
    cp(0x480, in.map_800bd3b8.data(), 0xE00);
    cp(0x1280, in.table_800bc9b8.data(), 0xA00);
    cp(0x1D40, c + 0x84, 240);                             // 0x800176A4 counts 16-byte units

    // 0x80065384: seeds and check words over the first 0x1F00 bytes.
    generate_checks(b, in.seeds);
    return true;
}

} // namespace l3
