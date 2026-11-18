// The 0x80065064 save block on an authored EXE excerpt (no retail bytes).
#include "legends3ds/source_save_block.h"
extern "C" {
#include "mml_retail_save_v2.h"
}
#include <cassert>
#include <cstring>
#include <iostream>

using namespace l3;

int main() {
    ExeExcerpt exe;
    auto range = [&](uint32_t a, size_t n, uint8_t fill) { ExeExcerpt::Range r; r.addr = a; r.bytes.assign(n, fill); exe.ranges.push_back(r); return &exe.ranges.back(); };
    range(kSaveIcon8008f9bc, 0x80, 0x11); range(kSaveClut8008fa3c, 0x20, 0x22);
    range(kSaveTitle8008fa64, 0x40, 0x40);
    { auto* r = range(kSaveTimeGlyphs8008d028, 10, 0); for (int i = 0; i < 10; ++i) r->bytes[size_t(i)] = uint8_t(0x50 + i); }
    { auto* r = range(kSaveSjisDigits80098124, 22, 0); for (int i = 0; i < 11; ++i) { r->bytes[size_t(2 * i)] = 0x82; r->bytes[size_t(2 * i + 1)] = uint8_t(0x4F + i); } }

    L3PlayerCompatWork pl{}; for (size_t i = 0; i < sizeof pl.bytes; ++i) pl.bytes[i] = uint8_t(i * 7);
    SourceSaveBlockInputs in; in.player = pl; in.slot = 0;
    const uint32_t t = 30u * (15u * 60u + 37u);
    for (int i = 0; i < 4; ++i) in.globals_800c1b10[size_t(0x0C + i)] = uint8_t(t >> (8 * i));
    in.globals_800c1b10[0x50] = 3; in.globals_800c1b10[0x51] = 1; in.globals_800c1b10[0x1C] = 0x2A;
    in.flags_800be378[0xC1] = 0x80;   // flag 0x608
    in.seeds = {1, 2, 3, 4, 5, 6};
    std::array<uint8_t,kSourceSaveBlockBytes> b{};
    assert(build_source_save_block_80065064(exe, in, b));
    assert(b[0] == 'S' && b[1] == 'C' && b[2] == 0x11 && b[3] == 0x01);
    assert(b[4 + 0x20] == 0x82 && b[4 + 0x21] == 0x50);                    // slot 1
    const uint8_t hhmmss[6] = {0x4F, 0x4F, 0x50, 0x54, 0x52, 0x56};      // 00 15 37
    const size_t at[6] = {0x24, 0x26, 0x2A, 0x2C, 0x30, 0x32};
    for (int i = 0; i < 6; ++i) assert(b[4 + at[i] + 1] == hhmmss[i]);
    assert(b[0x60] == 0x22 && b[0x80] == 0x11 && b[0x44] == 0);
    assert(b[0x100] == 3 && b[0x101] == 0x50 && b[0x103] == 0x13 && b[0x104] == 0x51 && b[0x105] == 0x55 && b[0x107] == 0x53 && b[0x108] == 0x57);
    assert(b[0x184] == 0x2A && b[0x194] == 3 && b[0x195] == 1);
    assert(std::memcmp(b.data() + 0x1C0, pl.bytes + 0x14, 8) == 0 && std::memcmp(b.data() + 0x1C8, pl.bytes + 0x56, 2) == 0);
    assert(std::memcmp(b.data() + 0x22C, pl.bytes + 0x4F4, 256) == 0 && std::memcmp(b.data() + 0x1C80, pl.bytes + 0x474, 128) == 0);
    assert(b[0x380 + 0xC1] == 0x80);
    assert(mml_retail_save_verify(b.data(), kSourceSaveTransferBytes) == MML_RETAIL_SAVE_OK);
    assert(source_save_block_valid(b.data(), b.size()));
    b[0x380] ^= 1; assert(!source_save_block_valid(b.data(), b.size()));
    assert(mml_retail_save_verify(b.data(), kSourceSaveTransferBytes) != MML_RETAIL_SAVE_OK);
    ExeExcerpt none; assert(!build_source_save_block_80065064(none, in, b));
    (void)pl;
    std::cout << "source_save_block: ok\n";
    return 0;
}
