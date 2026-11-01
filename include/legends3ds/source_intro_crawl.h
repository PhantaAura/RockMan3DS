#pragma once
// ST01 (pre-title narration) text crawl: a family-C0 object (oracle: pool
// 0x800A2D08 slot 0, spawned by the area loader) whose update is 0x801008C0:
// state 0 = 0x801008F8, state 1 = 0x80100918. The text is prerendered in
// OPENG0..3 (four 4-bit 256x256 pages side by side from VRAM x 320, y 256,
// CLUT 0x7C00); 0x80100A70 emits one 256-pixel scanline strip of it.
// Original addresses are identity tokens.
#include "legends3ds/source_event_phase5.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace l3 {
using SourceC0Work = SourceEventPhase5Bytes<36>;

// One POLY_FT4 strip from 0x80100A70: x 32..287, y..y+1, u 0..255, v = v & 0xFF,
// tpage 21 + (v >> 8) with semi-transparency mode 1 (0x20), CLUT 0x7C00,
// GP0 code 0x2C | (colour >> 24); modulation colour = colour & 0xFFFFFF.
struct SourceIntroCrawlStrip {
    int16_t y = 0;
    uint16_t v = 0;          // 0..1023 (page = v >> 8, row = v & 0xFF)
    uint32_t colour = 0;     // a3 as passed (bit 25 = semi-transparent)
};
constexpr size_t kSourceIntroCrawlMaxStrips = 16 + 144 + 16;
struct SourceIntroCrawlFrame {
    std::array<SourceIntroCrawlStrip,kSourceIntroCrawlMaxStrips> strips{};
    size_t count = 0;
    bool released = false;   // 0x800319F4 after scroll reaches 1024
    bool held = false;       // work fields unknown: nothing drawn or written
};
// 0x801008C0 dispatch on +8. State 0 (0x801008F8): +9..+0xB = 0, +0x1A = -190,
// +8 = 1 (no draw). State 1 (0x80100918): draw with the current +0x1A, then
// +0xB += 1; at 3 it resets and +0x1A += 1; +0x1A >= 1024 releases.
SourceIntroCrawlFrame update_source_intro_crawl_801008c0(SourceC0Work& w) noexcept;
} // namespace l3
