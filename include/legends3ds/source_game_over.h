#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// Main phase 9, the game over (ROCK_NEO 0x80016798, sub-phases 0x800101A8),
// entered by player state 2 (0x80041AB0: phase 9, sub 0 on its first frame).
//  0 0x800167D4  update pass, preload file 0x19, fade 11, +8 = 120
//  1 0x80016808  update pass, closing curtain 0x80016E90, +8 -= 2 (floor 0);
//                next once the fade and 0x800A8788 are idle
//  2 0x8001683C  HUD/message reset, music stop, Cd_read_comb(0x19) =
//                GAMEOVER.BIN, +6 = 150, jingle 0x41, fade 2
//  3 0x800168C4  GAME OVER quad 0x80016DAC; next when the fade is idle
//  4 0x800168DC  quad; Start or the jingle over -> 0x8001D854(0x10), fade 10
//  5 0x80016930  quad; next once the fade and 0x800A8788 are idle
//  6 0x80016974  clear flag 0x1FE, 0x800A8AA8 = 1, front end 0x80013420
struct SourceGameOverState {
    bool active = false;
    int8_t sub = 0;               // 0x800C1B11
    int16_t timer_08 = 0;         // 0x800C1B18 (curtain)
    int16_t timer_06 = 0;         // 0x800C1B16
};
struct SourceGameOverInputs {
    int16_t fade_busy_80098a72 = 0;
    uint8_t busy_800a8788 = 0;
    bool start_pressed = false;   // 0x800C0C2A & 8
    bool jingle_playing = false;  // 0x8001A0A8(0) != 0
};
struct SourceGameOverStep {
    bool update_pass = false;     // 0x80015DB4 (the world keeps running)
    std::optional<uint8_t> fade;  // 0x80017C30 type
    uint8_t endpoint_hold_1f800001 = 0;
    bool curtain = false;         // 0x80016E90 with timer_08
    bool reset_hud = false;       // 0x80053A30 / 0x80063BA8 / 0x80063EF0
    bool stop_music = false;      // 0x8001B33C / 0x8001B314
    bool load_gameover = false;   // Cd_read_comb(0x19)
    std::optional<uint16_t> sound;  // 0x8001A008
    bool draw_quad = false;       // 0x80016DAC
    bool cd_8001d854 = false;     // 0x8001D854(0x10)
    bool clear_flag_1fe = false;
    bool to_front_end = false;    // 0x80012F78(0x80013420)
};
SourceGameOverStep step_source_game_over_80016798(SourceGameOverState&, const SourceGameOverInputs&) noexcept;

// 0x80016E90: two black POLY_F4 bars over the 320x240 screen; top
// (0, 0)-(320, 120 - t), bottom (0, t + 119)-(320, 240).
struct SourceGameOverRect { int16_t x0 = 0, y0 = 0, x1 = 0, y1 = 0; };
std::array<SourceGameOverRect,2> source_game_over_curtain_80016e90(int16_t timer_08) noexcept;

// 0x80016DAC: POLY_FT4 (32, 56)-(287, 184), UV (0, 0)-(255, 128), colour
// 0x808080, word GetTPage(0,0,320,256) | GetClut(0,0x1F0) << 16.
constexpr uint32_t kSourceGameOverMaterialWord = 0x7c000015u;
constexpr SourceGameOverRect kSourceGameOverQuad{32, 56, 287, 184};

} // namespace l3
