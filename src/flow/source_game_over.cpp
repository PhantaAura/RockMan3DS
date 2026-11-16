#include "legends3ds/source_game_over.h"

namespace l3 {

SourceGameOverStep step_source_game_over_80016798(SourceGameOverState& s, const SourceGameOverInputs& in) noexcept {
    SourceGameOverStep out;
    if (!s.active) return out;
    const bool idle = in.fade_busy_80098a72 == 0 && in.busy_800a8788 == 0;   // 0x80016938
    switch (s.sub) {
    case 0:
        out.endpoint_hold_1f800001 = 4; out.update_pass = true;
        out.fade = 11; s.timer_08 = 0x78; ++s.sub;
        break;
    case 1:
        out.update_pass = true; out.curtain = true;
        s.timer_08 = int16_t(s.timer_08 - 2);
        if (s.timer_08 < 0) s.timer_08 = 0;
        if (idle) ++s.sub;
        break;
    case 2:
        out.endpoint_hold_1f800001 = 2;
        out.reset_hud = true; out.stop_music = true; out.load_gameover = true;
        s.timer_06 = 0x96; ++s.sub;
        out.sound = 0x41; out.fade = 2;
        break;
    case 3:
        out.draw_quad = true;
        if (in.fade_busy_80098a72 == 0) ++s.sub;                         // 0x80016D0C
        break;
    case 4:
        out.draw_quad = true;
        if (in.start_pressed || !in.jingle_playing) {
            ++s.sub; out.cd_8001d854 = true; out.fade = 10;
        }
        break;
    case 5:
        out.draw_quad = true;
        if (idle) ++s.sub;
        break;
    case 6:
        out.clear_flag_1fe = true; out.to_front_end = true;
        break;
    default:
        break;
    }
    return out;
}

std::array<SourceGameOverRect,2> source_game_over_curtain_80016e90(int16_t t) noexcept {
    return {{SourceGameOverRect{0, 0, 0x140, int16_t(0x78 - t)},
             SourceGameOverRect{0, int16_t(t + 0x77), 0x140, 0xf0}}};
}

} // namespace l3
