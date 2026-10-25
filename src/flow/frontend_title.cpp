#include "legends3ds/frontend_title.h"

namespace l3 {
namespace {
constexpr uint16_t kUp = 0x10, kRight = 0x20, kDown = 0x40, kLeft = 0x80;
constexpr uint16_t kTriangle = 0x1000, kStartCross = 0x4008;

psxgpu::PolyFt4 quad(uint32_t word, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint8_t u0, uint8_t v0, uint8_t u1, uint8_t v1) noexcept {
    psxgpu::PolyFt4 p;
    p.code_rgb = 0x2C808080u;   // +7 = 0x2C, & 0xFD: no semi-transparency
    p.x = {{x0, x1, x0, x1}};
    p.y = {{y0, y0, y1, y1}};
    p.u = {{u0, u1, u0, u1}};
    p.v = {{v0, v0, v1, v1}};
    p.clut = uint16_t(word >> 16);
    p.tpage = uint16_t(word);
    return p;
}
FrontendPacket ft4(const psxgpu::PolyFt4& q, uint16_t bucket) noexcept {
    FrontendPacket p; p.length = 9; p.bucket = bucket;
    psxgpu::write_poly_ft4(p.words.data(), q);
    return p;
}
// POLY_G4 (0x38, | 2 semi-transparent): rgb per vertex, xy (x0,y0)-(x1,y1).
FrontendPacket g4(bool semi, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  uint32_t top_rgb, uint32_t bottom_rgb, uint16_t bucket) noexcept {
    FrontendPacket p; p.length = 8; p.bucket = bucket;
    const auto xy = [](int16_t x, int16_t y) { return uint32_t(uint16_t(x)) | (uint32_t(uint16_t(y)) << 16); };
    p.words = {{(semi ? 0x3A000000u : 0x38000000u) | top_rgb, xy(x0, y0), top_rgb, xy(x1, y0),
                bottom_rgb, xy(x0, y1), bottom_rgb, xy(x1, y1), 0}};
    return p;
}
} // namespace

std::vector<FrontendPacket> frontend_backdrop_80013fb8() {
    std::vector<FrontendPacket> out;
    out.push_back(ft4(frontend_logo_80013fb8(), 32));
    const uint32_t wall = kFrontendMaterialWords[4];
    out.push_back(ft4(quad(wall, 0, 0, 160, 160, 0, 0, 160, 160), 35));
    out.push_back(ft4(quad(wall, 160, 0, 320, 160, 160, 0, 0, 160), 35));   // mirrored
    out.push_back(g4(true, 0, 144, 320, 160, 0x000000u, 0xFFFFFFu, 34));
    FrontendPacket mode; mode.length = 1; mode.bucket = 34;
    mode.words[0] = 0xE1000000u | psx_get_tpage(1, 2, 0, 0);   // SetDrawMode(0,0,GetTPage(1,2,0,0))
    out.push_back(mode);
    out.push_back(g4(false, 0, 162, 320, 240, 0x000000u, 0xE0FFC0u, 35));   // bottom rgb (192,255,224)
    return out;
}

psxgpu::PolyFt4 frontend_logo_80013fb8() noexcept {
    return quad(kFrontendMaterialWords[0], 0x20, 8, 0x11F, 0xA8, 0, 0, 0xFF, 0xA0);
}
std::array<psxgpu::PolyFt4, 2> frontend_press_start_80014394() noexcept {
    return {{quad(kFrontendMaterialWords[1], 0x38, 0xB0, 0x108, 0xC0, 0, 0xA0, 0xD0, 0xB0),
             quad(kFrontendMaterialWords[1], 0x20, 0xD0, 0x120, 0xE8, 0, 0xB0, 0xFF, 0xC8)}};
}
psxgpu::PolyFt4 frontend_menu_row_8001456c(unsigned row, int selection) noexcept {
    const uint32_t word = kFrontendMaterialWords[int(row) == selection ? 3 : 2];
    switch (row) {
        case 0: return quad(word, 104, 148, 216, 164, 0, 200, 112, 216);     // NEW GAME
        case 1: return quad(word, 104, 164, 216, 180, 112, 200, 224, 216);   // CONTINUE
        default: return quad(word, 120, 180, 200, 196, 0, 216, 80, 232);     // OPTION
    }
}

FrontendFrame frontend_step_80013420(FrontendState& s, const FrontendInput& in) {
    FrontendFrame out;
    const auto logo = [&] { for (const auto& p : frontend_backdrop_80013fb8()) out.packets.push_back(p); };
    const auto menu = [&] {
        for (unsigned r = 0; r < 3; ++r) out.packets.push_back(ft4(frontend_menu_row_8001456c(r, s.selection), 30));
        // The box behind the rows: POLY_F4 | 2, black; it takes the current
        // texture page's blend (the logo's ABR 0, 0.5B + 0.5F).
        FrontendPacket box; box.length = 5; box.bucket = 30;
        box.words = {{0x2A000000u, (144u << 16) | 96u, (144u << 16) | 224u, (200u << 16) | 96u, (200u << 16) | 224u, 0, 0, 0, 0}};
        out.packets.push_back(box);
    };
    const auto fade = [&](uint8_t n) { out.fades.push_back(n); };
    const auto sound = [&](uint8_t n) { out.sounds.push_back({{n, uint8_t(n >= 129 && n <= 131 ? 1 : 0)}}); };

    // 0x800134AC..0x80013524: Start/Cross on the title opens the menu.
    if (s.state == 2 && (in.pressed & kStartCross)) {
        sound(130);
        s.substate = (in.fade_80098a72 > 0 || (in.fade_80098a72 == 0 && in.saves_80098a70 == 0)) ? 1 : 0;
        s.state = 4;
    }
    if (s.state == 2) {
        // 0x800136B8, substates from 0x80010070.
        switch (s.substate) {
            case 0:
                if (in.fade_80098a72 != 0) return out;
                fade(2); ++s.substate;
                [[fallthrough]];
            case 1:
                ++out.calls_80053a30; s.timer = 900; ++s.substate; logo();
                return out;
            case 2:
                logo();
                if (in.fade_80098a72 == 0) ++s.substate;   // 0x80013F60
                return out;
            case 3:
                logo();
                for (const auto& p : frontend_press_start_80014394()) out.packets.push_back(ft4(p, 30));
                if (--s.timer == 0) { ++s.substate; fade(10); }
                return out;
            case 4:
                ++out.calls_80053a30; logo();
                if (in.fade_80098a72 == 0) { s.substate = 0; ++s.state; }   // 0x80013F8C
                return out;
            default: return out;
        }
    }
    if (s.state == 3) { s.state = 0; s.substate = 0; return out; }   // 0x80013890
    if (s.state == 0) { out.result = FrontendResult::Attract; return out; }
    if (s.state == 4) {
        // 0x8001389C, substates from 0x80010088.
        switch (s.substate) {
            case 0:
                if (in.fade_80098a72 != 0) return out;
                fade(1); ++s.substate;
                [[fallthrough]];
            case 1:
                ++out.calls_80053a30; s.timer = 900; ++s.substate;
                logo(); menu();
                return out;
            case 2:
                logo(); menu();
                if (in.fade_80098a72 == 0) ++s.substate;
                return out;
            case 3: break;
            case 5:
                logo(); menu(); ++out.calls_80053a30;
                out.result = FrontendResult::NewGame;   // 0x800C1B92 = 0, thread 0x800155A4
                return out;
            default: return out;
        }
        // Substate 3 (0x8001396C): draw, then one input branch.
        logo(); menu();
        const uint16_t p = in.pressed;
        if ((p & kUp) && s.selection > 0) { sound(129); --s.selection; return out; }
        if (p & kDown) {
            const int limit = s.option_row_800c1b28 ? 3 : 2;
            if (s.selection < limit) { sound(129); ++s.selection; return out; }
        }
        if ((p & kLeft) && s.selection == 3) {
            sound(129);
            const int8_t v = int8_t(s.option_value_800c1b6b - 1);
            s.option_value_800c1b6b = v;
            if (s.option_wrap_800c1b2a ? v < 0 : v <= 0) s.option_value_800c1b6b = 2;
        }
        if ((p & kRight) && s.selection == 3) {
            sound(129);
            const int8_t v = int8_t(s.option_value_800c1b6b + 1);
            s.option_value_800c1b6b = v;
            if (v >= 3) s.option_value_800c1b6b = s.option_wrap_800c1b2a ? 0 : 1;
        }
        if (p & kStartCross) {
            switch (s.selection) {
                case 0: s.substate = uint8_t(s.substate + 2); fade(22); sound(166); break;
                case 1: s.state = 5; s.substate = 0; fade(9); sound(167); out.result = FrontendResult::Continue; break;
                case 2: s.state = 6; s.substate = 0; fade(9); sound(168); out.result = FrontendResult::Option; break;
                case 3: {
                    const int8_t v = int8_t(s.option_value_800c1b6b + 1);
                    s.option_value_800c1b6b = v;
                    if (v >= 3) s.option_value_800c1b6b = s.option_wrap_800c1b2a ? 0 : 1;
                    sound(129);
                    break;
                }
                default: break;
            }
        } else if (p & kTriangle) {
            sound(131); s.back_0e = 1; s.state = 2; s.substate = 1;
        }
        return out;
    }
    if (s.state == 5) out.result = FrontendResult::Continue;
    if (s.state == 6) out.result = FrontendResult::Option;
    return out;
}

} // namespace l3
