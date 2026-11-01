#pragma once
// ST01 (pre-title narration) text crawl: a family-C0 object (oracle: pool
// 0x800A2D08 slot 0, spawned by the area loader) whose update is 0x801008C0:
// state 0 = 0x801008F8, state 1 = 0x80100918. The text is prerendered in
// OPENG0..3 (four 4-bit 256x256 pages side by side from VRAM x 320, y 256,
// CLUT 0x7C00); 0x80100A70 emits one 256-pixel scanline strip of it.
// Original addresses are identity tokens.
#include "legends3ds/frontend_title.h"
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

// The four tpage/CLUT words the strips use: tpage 21..24 (4-bit, abr 1,
// x 320/384/448/512, y 256) with CLUT 0x7C00 = (0, 496).
constexpr std::array<uint32_t,4> kSourceIntroCrawlMaterialWords{{
    psx_material_word(0x7C00, psx_get_tpage(0, 1, 320, 256)),
    psx_material_word(0x7C00, psx_get_tpage(0, 1, 384, 256)),
    psx_material_word(0x7C00, psx_get_tpage(0, 1, 448, 256)),
    psx_material_word(0x7C00, psx_get_tpage(0, 1, 512, 256)),
}};
// The GP0 words 0x80100A70 writes (9-word POLY_FT4, OT entry 30 of 0x800A8934).
constexpr uint16_t kSourceIntroCrawlOtEntry = 30;
FrontendPacket source_intro_crawl_packet_80100a70(const SourceIntroCrawlStrip& strip) noexcept;

// ST01 event 71 handler 0x8010049C (phase table 0x80100004), one update per
// tick. Ported: the phase order, the 5-tick countdown, fade requests 2 and 10,
// the 3100-tick timeline record 0x80100F7C ending on its 0xFF terminator, and
// the phase-10 wait for the fade (0x800A8A72 == 0). Not ported: the CD command
// 0x2000001E and its wait 0x8001D878 (taken as one tick; the oracle shows
// phase 1 -> 5 in 8 ticks = 1 + 1 + 5 + 1), the camera, the callback's
// background fade, the 0x8001DE84 Start skip.
struct SourceIntroEvent71State {
    uint8_t phase = 0;
    int32_t countdown_0c = 0;
    uint32_t timeline_ticks = 0;   // 0x8001F070 ticks of the single record
    bool done = false;             // phase 10 ended: the title thread starts
};
struct SourceIntroEvent71Step {
    uint8_t fade_type = 0;         // 0x80017C30 request this tick (0 = none)
    bool cd_command = false;       // 0x8001D58C(0x2000001E) issued this tick
};
constexpr uint32_t kSourceIntroTimelineTicks = 0x0C1C;   // record 0x80100F7C
SourceIntroEvent71Step step_source_intro_event71_8010049c(SourceIntroEvent71State& st, int16_t fade_busy_80098a72) noexcept;
} // namespace l3
