#pragma once
#include "legends3ds/render/psx_gpu.h"
#include <array>
#include <cstdint>
#include <vector>

// Front end after the intro: the title (PRESS START) and the NEW GAME /
// CONTINUE / OPTION menu, from the ROCK_NEO thread 0x80013420 and its state
// table 0x80080950 (docs/research_01516/FRONTEND_TITLE.md). The original keeps
// this state at 0x80098AA8: +0 state, +1 substate, +4 timer, +0x0C selection,
// +0x0E back-from-menu halfword.
namespace l3 {

// PsyQ GetTPage(tp, abr, x, y) / GetClut(x, y).
constexpr uint16_t psx_get_tpage(unsigned tp, unsigned abr, unsigned x, unsigned y) noexcept {
    return uint16_t(((tp & 3u) << 7) | ((abr & 3u) << 5) | ((y & 0x100u) >> 4) | ((x & 0x3FFu) >> 6) |
                    ((y & 0x200u) << 2));
}
constexpr uint16_t psx_get_clut(unsigned x, unsigned y) noexcept { return uint16_t((y << 6) | (x >> 4)); }
constexpr uint32_t psx_material_word(uint16_t clut, uint16_t tpage) noexcept { return (uint32_t(clut) << 16) | tpage; }

// Logo, PRESS START, menu row, selected menu row, ruin wall (NTITLE3U), the
// Servbot model's one part word (DEMO_DAT.EBD class 0x20/0 bindings), and
// the OPTION/CONTINUE tile backdrop.
constexpr std::array<uint32_t, 7> kFrontendMaterialWords{{
    psx_material_word(psx_get_clut(0, 0x1F0), psx_get_tpage(1, 0, 320, 256)),
    psx_material_word(psx_get_clut(0, 0x1F1), psx_get_tpage(0, 0, 320, 256)),
    psx_material_word(psx_get_clut(16, 0x1F1), psx_get_tpage(0, 0, 320, 256)),
    psx_material_word(psx_get_clut(32, 0x1F1), psx_get_tpage(0, 0, 320, 256)),
    psx_material_word(psx_get_clut(0, 0x1F2), psx_get_tpage(1, 2, 576, 256)),
    0x3D88002Bu,
    psx_material_word(0x3E77, 0x000D),   // OPTION/CONTINUE tiles (0x80062368: E1 0x0D, SPRT CLUT 0x3E77)
}};

// One GP0 packet as the front end adds it: words, payload length and the
// original ordering-table entry (DAT_80098934 base): 30 text/menu (+0x78),
// 32 the logo list 0x800A4F50, 34 the wall-shadow strip (+0x88), 35 the wall
// and gradient (+0x8C). Adding packets in this order to a PsyQ OT reproduces
// the original draw order.
struct FrontendPacket {
    std::array<uint32_t, 9> words{};
    uint8_t length = 0;
    uint16_t bucket = 0;
};

// The POLY_FT4 packets (code 0x2C, rgb 0x808080, semi-transparency off).
psxgpu::PolyFt4 frontend_logo_80013fb8() noexcept;
// PRESS START BUTTON, then the two copyright lines (second packet).
std::array<psxgpu::PolyFt4, 2> frontend_press_start_80014394() noexcept;
// Rows 0..2 of 0x8001456C (NEW GAME, CONTINUE, OPTION); the selected row
// uses CLUT x 32, the others 16.
psxgpu::PolyFt4 frontend_menu_row_8001456c(unsigned row, int selection) noexcept;

// 0x80013FB8: logo, wall (two mirrored halves), shadow strip (Gouraud,
// semi-transparent, drawn with the E1 draw mode ABR 2 that follows it),
// gradient. The original then calls the scene draw 0x800137D4 (Servbots).
std::vector<FrontendPacket> frontend_backdrop_80013fb8();
// Port widescreen (400-px top screen): widens the backdrop in place.
void frontend_backdrop_widescreen(std::vector<FrontendPacket>& packets);
// 0x80062368: E1 0xE100000D, then 7 x 5 SPRT 64x64 (rgb 0x808080, uv
// (0xC0,0x8B), CLUT 0x3E77) at x/y -64 + 64i + (scroll & 63), one chain at
// OT entry 35. The translator draws polygons, so each SPRT is the equal
// POLY_FT4; returned in AddPrim order (the chain head drawn first).
std::vector<FrontendPacket> frontend_tiles_80062368(uint16_t scroll_1f800002);

enum class FrontendResult : uint8_t { None, NewGame, Continue, Option, Attract };

struct FrontendInput {
    uint16_t pressed = 0;     // 0x800C0C2A (PS1 pad bits, 1 = pressed this frame)
    int16_t fade_80098a72 = 0; // fade counter; nonzero while a fade runs
    int16_t saves_80098a70 = 0;
};

struct FrontendFrame {
    std::vector<FrontendPacket> packets;    // in AddPrim order
    std::vector<uint8_t> fades;             // 0x80017C30 arguments
    // 0x8001A008(id, a1, 0): {id, a1} (129..131 pass a1 = 1, 166..168 a1 = 0).
    std::vector<std::array<uint8_t, 2>> sounds;
    uint8_t calls_80053a30 = 0;
    uint8_t calls_8005382c = 0;             // OPTION message window 0x8005382C(0, 0x8008B69C, -1): not ported
    FrontendResult result = FrontendResult::None;
};

struct FrontendState {
    uint8_t state = 2, substate = 0;        // enters at the title (state 2)
    uint16_t timer = 0;                     // +4
    int8_t selection = 0;                   // +0x0C
    uint16_t back_0e = 0;                   // +0x0E
    bool option_row_800c1b28 = false;       // fourth menu row present
    bool option_wrap_800c1b2a = false;
    int8_t option_value_800c1b6b = 0;
    // 0x1F800002: the tile backdrop's scroll (& 63). Its writer is not
    // traced; it stays 0 (CANDIDATE).
    uint16_t scroll_1f800002 = 0;
    // 0x80098A58 bit 30, the other OPTION exit; writer not traced.
    bool exit_80098a58 = false;
};

// One front-end frame: the thread's Start/Cross check while in state 2, then
// the state function. States 2 (title), 4 (menu) and 6 (OPTION's frame:
// fade, tiles, scene, Start back) are ported; 5 (CONTINUE) and the
// attract hand-off are reported, not run.
FrontendFrame frontend_step_80013420(FrontendState& s, const FrontendInput& in);

} // namespace l3
