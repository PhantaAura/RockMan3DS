// Front-end title and menu (ROCK_NEO 0x80013420 / 0x800136B8 / 0x8001389C):
// packet geometry from the disassembly and the state flow. Values authored.
#include "legends3ds/frontend_title.h"
#include <cassert>
#include <iostream>

using namespace l3;

int main() {
    // GetTPage / GetClut and the four material words.
    assert(psx_get_tpage(1, 0, 320, 256) == 0x95 && psx_get_tpage(0, 0, 320, 256) == 0x15);
    assert(psx_get_clut(0, 0x1F0) == 0x7C00 && psx_get_clut(32, 0x1F1) == 0x7C42);
    assert(kFrontendMaterialWords[0] == 0x7C000095u && kFrontendMaterialWords[3] == 0x7C420015u);

    const auto logo = frontend_logo_80013fb8();
    assert(logo.code_rgb == 0x2C808080u && logo.x[0] == 32 && logo.x[1] == 287 && logo.y[2] == 168);
    assert(logo.u[1] == 255 && logo.v[2] == 160 && logo.clut == 0x7C00 && logo.tpage == 0x95);
    const auto start = frontend_press_start_80014394();
    assert(start[0].x[0] == 56 && start[0].x[1] == 264 && start[0].y[0] == 176 && start[0].v[0] == 160 && start[0].u[1] == 208);
    assert(start[1].x[0] == 32 && start[1].x[1] == 288 && start[1].y[2] == 232 && start[1].v[2] == 200 && start[1].u[1] == 255);
    const auto row1 = frontend_menu_row_8001456c(1, 1), row2 = frontend_menu_row_8001456c(2, 1);
    assert(row1.clut == 0x7C42 && row2.clut == 0x7C41 && row1.u[0] == 112 && row2.x[0] == 120 && row2.y[2] == 196);

    // Backdrop 0x80013FB8: logo, mirrored wall halves, strip + E1 (ABR 2), gradient.
    const auto back = frontend_backdrop_80013fb8();
    assert(back.size() == 6 && back[0].bucket == 32 && back[1].bucket == 35 && back[3].bucket == 34);
    assert(back[1].words[2] == ((uint32_t(0x7C80) << 16) | 0x0000u) && back[2].words[2] == ((uint32_t(0x7C80) << 16) | 160u));
    assert((back[3].words[0] >> 24) == 0x3A && back[3].words[4] == 0xFFFFFFu && back[4].words[0] == 0xE10000C0u);
    assert((back[5].words[0] >> 24) == 0x38 && back[5].words[6] == 0xE0FFC0u && back[5].words[7] == ((240u << 16) | 320u));
    assert(kFrontendMaterialWords[4] == 0x7C8000D9u);
    {   // Widescreen: two wall extensions, strip and gradient span x -40..360.
        auto wide = frontend_backdrop_80013fb8();
        frontend_backdrop_widescreen(wide);
        assert(wide.size() == 8 && wide[1].bucket == 35 && int16_t(wide[1].words[1]) == -40);
        assert(int16_t(wide[5].words[1]) == -40 && uint16_t(wide[5].words[7]) == 360);
        assert(int16_t(wide[7].words[1]) == -40 && uint16_t(wide[7].words[3]) == 360);
    }

    // Title: fade 2 then the 900-frame PRESS START loop, then attract.
    FrontendState s;
    FrontendInput idle;
    auto f = frontend_step_80013420(s, idle);
    assert(f.fades.size() == 1 && f.fades[0] == 2 && s.substate == 2 && s.timer == 900 && f.packets.size() == 6);
    f = frontend_step_80013420(s, idle);   // fade done
    assert(s.substate == 3);
    f = frontend_step_80013420(s, idle);
    assert(f.packets.size() == 8 && s.timer == 899);
    for (int i = 0; i < 898; ++i) frontend_step_80013420(s, idle);
    f = frontend_step_80013420(s, idle);
    assert(s.substate == 4 && f.fades.size() == 1 && f.fades[0] == 10);
    f = frontend_step_80013420(s, idle);
    assert(s.state == 3);
    frontend_step_80013420(s, idle);
    f = frontend_step_80013420(s, idle);
    assert(s.state == 0 && f.result == FrontendResult::Attract);

    // Start on the title opens the menu (sound 130, no saves -> substate 1).
    FrontendState m;
    frontend_step_80013420(m, idle); frontend_step_80013420(m, idle);
    FrontendInput start_press; start_press.pressed = 0x0008;
    f = frontend_step_80013420(m, start_press);
    assert(f.sounds.size() == 1 && f.sounds[0][0] == 130 && f.sounds[0][1] == 1 && m.state == 4 && m.substate == 2 && f.packets.size() == 10);
    frontend_step_80013420(m, idle);           // fade done -> input substate
    assert(m.substate == 3);
    FrontendInput down; down.pressed = 0x40;
    f = frontend_step_80013420(m, down);
    assert(m.selection == 1 && f.sounds.size() == 1 && f.sounds[0][0] == 129);
    frontend_step_80013420(m, down);
    f = frontend_step_80013420(m, down);       // no fourth row: stays on OPTION
    assert(m.selection == 2 && f.sounds.empty());
    FrontendInput up; up.pressed = 0x10;
    frontend_step_80013420(m, up); frontend_step_80013420(m, up);
    f = frontend_step_80013420(m, up);
    assert(m.selection == 0 && f.sounds.empty());
    FrontendInput cross; cross.pressed = 0x4000;
    f = frontend_step_80013420(m, cross);       // NEW GAME
    assert(m.substate == 5 && f.fades.size() == 1 && f.fades[0] == 22 && f.sounds[0][0] == 166 && f.sounds[0][1] == 0);
    f = frontend_step_80013420(m, idle);
    assert(f.result == FrontendResult::NewGame);

    // Triangle in the menu returns to the title (substate 1).
    FrontendState t; t.state = 4; t.substate = 3;
    FrontendInput tri; tri.pressed = 0x1000;
    f = frontend_step_80013420(t, tri);
    assert(t.state == 2 && t.substate == 1 && t.back_0e == 1 && f.sounds[0][0] == 131);
    std::cout << "frontend title ok\n";
    return 0;
}
