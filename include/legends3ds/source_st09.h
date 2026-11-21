#pragma once
// Stage 9 (the first ruin, S5; docs/research_01516/STAGE_LOADING.md).
// The area-setup routine 0x80100398 ends with 0x801005F4, which sets flag
// 0x16 on every area load. ST09 has no area-enter / 0x1FF gate: its
// per-frame dispatcher 0x8010061C runs the area's update directly. Area 0's
// update 0x80100670: flag 0x7B0 starts event 0 (the door event, already run
// by the runtime's door path); byte +4 = 0: without flag 12 it spawns set 0
// (0x80100B20(0): one record at 0x8011DD68, the trapped junk-shop man, class
// 0x13 variant 1, ST09 code 0x80107CDC, action 0 ported below) and goes to 1, else 255;
// state 1 waits for flag 0x60C, then starts event 0x17 (reported) and goes
// to 255. Original addresses are identity tokens.
#include "legends3ds/source_st0c.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

constexpr uint32_t kSt09Set0Record = 0x8011DD68u;
constexpr uint32_t kSt09TrappedManBox = 0x8011DF64u;   // {min x, min z, width, depth} in 512 tiles

// The trapped man (class 0x13 variant 1, callback 0x80107CDC; states
// 0x8011DF34: 0 init 0x80107D28, 1 tick 0x80107E54, 2 release 0x80108098;
// actions 0x8011DF40: 0 0x801080B8 (ported), 1 0x80108384 and
// 2 0x80108430 (event 0x17's rescue and exit; reported).
// Init: request 20, +0x34A from 0x8007FE50, target (22789, 0, 9890) at
// +0x37C..+0x380, +0x6E = 128, check box 0x8011DF4C, floor bounds 0x8011DF58.
// Tick: 0x80051B68(work, 3, -1, 0) registers the talk (kind 3), the hit
// proxy 0x800341BC (+0x348 bit 1; not ported: shots at him), shadow 48/512.
// Action 0 phase 0: phase 1, request 20, timer +0x34C = 60. Phase 1, each
// frame:
//  - scan the 16 class-20 pool slots (mask 0x80098768) for an object other
//    than class 0x13 whose tile ((x >> 9) - min x) & 0xFF < width and
//    likewise z: none -> +0x348 bit 3 set, else clear;
//  - timer 0x34C counts down to 0;
//  - the player's talk target (+0x184) is me: clear it; with bit 3 and no
//    window open (0x80098A58 >= 0) set flag 0x60C and +0x56 = 1492; with
//    enemies and no window: timer 450, window 4 message +0x0C + 4;
//  - else a hit (+0x35C & 0xFFFFFF, not ported): timer 450, message + 5;
//  - else the player inside the box (unsigned, unmasked tile differences):
//    entering (bit 2 clear) sets bit 2, clears flags 0x60A and 0x60B and
//    shows message +0; outside, when the timer is 0: timer
//    450, bit 2 cleared, message +0 (neither of the last two checks the window).
// Window 4 is 0x8005382C's fifth slot, which shows alongside the others
// (opening any other slot clears it); the port shows these lines through
// the non-gating dialogue (CANDIDATE: slot 0's frame and position).
struct St09TrappedMan {
    uint8_t action_09 = 0, phase_0a = 0;
    uint8_t request_ac = 20;
    uint16_t flags_348 = 0;                   // bit 2 player in the box, bit 3 no enemy in the box
    int16_t timer_34c = 0;
    uint8_t message_0c = 0;                   // work +0x0C (record [8])
    std::optional<uint16_t> heading_56;       // 1492 once talked to with the box clear
};
struct St09TrappedManInputs {
    std::array<uint8_t,4> box{};              // 0x8011DF64
    int16_t player_x = 0, player_z = 0;
    std::vector<std::array<int16_t,2>> enemies;   // live class-20 objects other than class 0x13: x, z
    bool player_targets_me = false;           // player +0x184 == work
    bool window_open = false;                 // 0x80098A58 < 0 (any window)
    bool hit = false;                         // +0x35C & 0xFFFFFF (not ported: false)
};
struct St09TrappedManTick {
    std::optional<uint8_t> request;           // +0xAC this tick (0x800322A8 when changed)
    bool clear_player_target = false;
    std::optional<uint16_t> flag_set;         // 0x8001DA8C
    std::array<uint16_t,2> flag_clears{};     // 0x8001DCD0, in order (0 = none)
    std::optional<uint8_t> message;           // 0x80053A08(4, n)
    bool enemies_in_box = false;
    bool action_unported = false;             // action 1 / 2 (event 0x17)
};
St09TrappedManTick step_st09_trapped_man_801080b8(St09TrappedMan& m, const St09TrappedManInputs& in) noexcept;

struct St09Area0Step {
    std::optional<uint8_t> object_set;
    std::optional<uint8_t> event_unported;   // 0x8001DDE4(0x17)
};
template <class FlagFn>
St09Area0Step st09_area0_update_80100670(uint8_t& state_04, FlagFn flag) {
    St09Area0Step out;
    if (state_04 == 0) {
        if (flag(12)) state_04 = 255;
        else { out.object_set = 0; state_04 = 1; }
    } else if (state_04 == 1 && flag(0x60C)) {
        out.event_unported = 0x17; state_04 = 255;
    }
    return out;
}

// Set 0 (the one record at 0x8011DD68) from the cooked stage.l3sd.
bool bind_st09_profile(const std::vector<StageDataBlock>& blocks, St03Event8Profile& out);

} // namespace l3
