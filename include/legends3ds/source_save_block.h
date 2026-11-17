#pragma once
// The memory-card save block of USA ROCK_NEO.EXE (docs/research_01516/DATA_MENU.md):
// 0x80065064 fills the 8 KiB block at 0x800B5DB0 (header 0x80064CB8, play-time text
// 0x800679B8 / 0x80067D10, field and region copies), 0x80065384 the check words
// (src/p2/mml_retail_save_v2.c, verified against an oracle-written card).
// Original addresses are identity tokens.
#include "legends3ds/mml_message_window.h"
#include "legends3ds/player_runtime.h"
#include <array>
#include <cstdint>

namespace l3 {

constexpr size_t kSourceSaveBlockBytes = 0x2000;
constexpr size_t kSourceSaveTransferBytes = 0x1F00;   // the part 0x80065384 checks and the card write covers
// EXE ranges the block needs (cooked into frontend/option.l3ex).
constexpr uint32_t kSaveIcon8008f9bc = 0x8008F9BC;    // 128 bytes -> +0x80
constexpr uint32_t kSaveClut8008fa3c = 0x8008FA3C;    // 32 bytes  -> +0x60
constexpr uint32_t kSaveTitle8008fa64 = 0x8008FA64;   // 64 bytes  -> +0x04 (Shift-JIS, digits patched)
constexpr uint32_t kSaveTimeGlyphs8008d028 = 0x8008D028;   // digit -> message glyph
constexpr uint32_t kSaveSjisDigits80098124 = 0x80098124;   // digit -> Shift-JIS u16

struct SourceSaveBlockInputs {
    uint8_t slot = 0;                                   // gp+0x11A0 (0..4)
    std::array<uint8_t,0x180> globals_800c1b10{};       // 0x800C1B10.. (+0x0C play time, +0x50 stage)
    L3PlayerCompatWork player{};                        // 0x800B51B0 (+0x6E life, +0xB0 max life)
    std::array<uint8_t,0x100> flags_800be378{};
    std::array<uint8_t,0xE00> map_800bd3b8{};           // per-stage table (0x8003B140 updates it first)
    std::array<uint8_t,0xA00> table_800bc9b8{};
    std::array<uint32_t,6> seeds{};                     // 0x80065384: (rand << 16) | rand each
};

// false when the excerpt lacks a needed range.
bool build_source_save_block_80065064(const ExeExcerpt& exe, const SourceSaveBlockInputs& in,
                                      std::array<uint8_t,kSourceSaveBlockBytes>& out);

} // namespace l3
