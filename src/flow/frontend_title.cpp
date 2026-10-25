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
} // namespace

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
    const auto logo = [&] { out.draws.push_back(frontend_logo_80013fb8()); };
    const auto menu = [&] { for (unsigned r = 0; r < 3; ++r) out.draws.push_back(frontend_menu_row_8001456c(r, s.selection)); };
    const auto fade = [&](uint8_t n) { out.fades.push_back(n); };
    const auto sound = [&](uint8_t n) { out.sounds.push_back(n); };

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
                for (const auto& p : frontend_press_start_80014394()) out.draws.push_back(p);
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
