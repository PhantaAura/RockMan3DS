// Main phase 9, the game over (0x80016798): sub-phase order, fades 11/2/10,
// the closing curtain, the GAME OVER quad, Start or jingle end, front end.
#include "legends3ds/source_game_over.h"
#include <cassert>
#include <cstdio>

using namespace l3;

int main() {
    SourceGameOverState s; SourceGameOverInputs in;
    assert(!step_source_game_over_80016798(s, in).update_pass);       // inactive: nothing
    s.active = true;
    auto o = step_source_game_over_80016798(s, in);
    assert(o.update_pass && o.fade && *o.fade == 11 && o.endpoint_hold_1f800001 == 4 && s.sub == 1 && s.timer_08 == 120);
    // Curtain closes 2 per update while the fade runs; the world keeps updating.
    in.fade_busy_80098a72 = 1;
    for (int i = 0; i < 70; ++i) { o = step_source_game_over_80016798(s, in); assert(o.update_pass && o.curtain && s.sub == 1); }
    assert(s.timer_08 == 0);
    const auto bars = source_game_over_curtain_80016e90(0);
    assert(bars[0].y1 == 120 && bars[1].y0 == 119 && bars[1].y1 == 240 && bars[0].x1 == 320);
    const auto open = source_game_over_curtain_80016e90(120);
    assert(open[0].y1 == 0 && open[1].y0 == 239);
    in.fade_busy_80098a72 = 0; in.busy_800a8788 = 1;
    o = step_source_game_over_80016798(s, in); assert(s.sub == 1);      // 0x800A8788 still busy
    in.busy_800a8788 = 0;
    o = step_source_game_over_80016798(s, in); assert(s.sub == 2);
    // Sub 2: resets, GAMEOVER.BIN, jingle 0x41, fade 2.
    o = step_source_game_over_80016798(s, in);
    assert(!o.update_pass && o.reset_hud && o.stop_music && o.load_gameover && o.sound && *o.sound == 0x41);
    assert(o.fade && *o.fade == 2 && s.sub == 3 && s.timer_06 == 150);
    in.fade_busy_80098a72 = 1;
    o = step_source_game_over_80016798(s, in); assert(o.draw_quad && s.sub == 3);
    in.fade_busy_80098a72 = 0;
    o = step_source_game_over_80016798(s, in); assert(s.sub == 4);
    // Sub 4 waits for Start or the jingle's end, then fade 10.
    in.jingle_playing = true;
    for (int i = 0; i < 10; ++i) { o = step_source_game_over_80016798(s, in); assert(o.draw_quad && s.sub == 4); }
    in.start_pressed = true;
    o = step_source_game_over_80016798(s, in);
    assert(s.sub == 5 && o.fade && *o.fade == 10 && o.cd_8001d854);
    in.start_pressed = false; in.fade_busy_80098a72 = 1;
    o = step_source_game_over_80016798(s, in); assert(o.draw_quad && s.sub == 5);
    in.fade_busy_80098a72 = 0;
    o = step_source_game_over_80016798(s, in); assert(s.sub == 6);
    o = step_source_game_over_80016798(s, in); assert(o.clear_flag_1fe && o.to_front_end);
    // Jingle over without Start also continues.
    SourceGameOverState j; j.active = true; j.sub = 4;
    SourceGameOverInputs quiet;
    assert(step_source_game_over_80016798(j, quiet).fade && j.sub == 5);
    std::puts("game over: ok");
    return 0;
}
