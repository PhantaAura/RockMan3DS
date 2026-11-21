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
#include "legends3ds/source_st03_objects.h"
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
    int16_t speed_44 = 0, vy_46 = 0, accel_4e = 0;   // action 2's run
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

// State 1 (0x80107E54) around the three actions. Action 1 (0x80108384,
// set by event 0x17): phase 0 waits for animation frame +0xA4 == 15 or 35,
// then request 21 and the talk 0x80063BC8(work + 0x14, 2, 0, +0x0C + 6);
// phase 1 sets +0x348 bit 0 once no window is open. Action 2 (0x80108430):
// phase 0 request 2, +0x44 = +0x46 = 384, +0x4E = 192, timer 8, flag 0x60D;
// phase 1 turns toward (22789, 9890) (step 96), moves 0x8002FEA4(+0x44,
// +0x46, 0), and within distance^2 2048 sets bit 0. After the action, bit 0
// moves action 0 -> 1, 1 -> 2, and 2 releases him (0x80031824) with flag 12.
// Not run (CANDIDATE gaps): 0x8004B414's floor/wall pass, so his hops
// (+0x46 += +0x4E until floor contact 0x1000, landing sound 279) are not
// reproduced and he keeps his y; the dust (0x80031618 every frame once the
// timer is out) is reported only. Sound 278 plays on +0xA6 == 1.
struct St09TrappedManWorldInputs {
    St09TrappedManInputs action0;
    uint8_t frame_a4 = 0, anim_a6 = 0;        // the animation cursor's +0xA4 / +0xA6
    const ReferenceTables* refs = nullptr;    // 0x8002FEA4 trig
    const SourceAngleSteps* angle_steps = nullptr;   // 0x80031198
};
struct St09TrappedManStep {
    St09TrappedManTick action0;               // action 0's outputs (empty for actions 1 / 2)
    std::optional<uint8_t> talk;              // 0x80063BC8(work + 0x14, 2, 0, n): the talk window
    std::optional<uint16_t> flag_set;         // 0x8001DA8C (0x60D, 12)
    std::optional<uint16_t> sound;            // 0x8001A008(n, 0, 0)
    bool moved = false, dust = false, released = false, refs_missing = false;
};
constexpr std::array<int16_t,2> kSt09TrappedManExit{{22789, 9890}};   // init: +0x37C / +0x380
St09TrappedManStep step_st09_trapped_man_80107e54(St09TrappedMan& m, St03Npc& n, const St09TrappedManWorldInputs& in) noexcept;

// Event 0x17 (handler 0x8010074C -> scene 0x80100788, the rescue). Data:
// timeline 0x8011DD7C (one record, handler 0x80100AF8 = 0x8001EC0C +
// 0x8001F070), camera program 0x8011DD8C; the man is *(0x8011EA60), set
// 0's slot 0. State 0: 0x80042238(player), 0x8001F1DC, 0x8001E6C0(0,0,0),
// the player walks to (22626, -577, 7563) facing 3540 (+8 = 3, +9 = 1,
// +0x448 = 48), timer 30. State 1: timer out -> 36, man +9 = 1 (action 1).
// State 2: with flag 0x60D: +0x448 = 13 at 36, then 0x80031BEC turns +0x442
// toward the man (step 32) until the timer runs out, +0x448 = 48, state 3.
// States 2/3: flag 12 releases the player (0x80042314), 0x8009395C = 1,
// event +0 = 0. States 1..2 run the timeline record and, on Start, skip:
// 0x80100BF0 (release set 0's objects), close the windows, flag 12,
// state 3, the player at the same point facing 2401. 0x8002EE90 (states 0
// and the end) is reported. Ending sets 0x380 + 0x17 (0x8001D9F0).
constexpr uint32_t kSt09RescueTimeline = 0x8011DD7Cu, kSt09RescueCamera = 0x8011DD8Cu;
constexpr uint32_t kSt09RescueRecordCallback = 0x80100AF8u;
struct St09Event17Inputs {
    bool player_ready = false;                // 0x80042238(player)
    bool start_pressed = false;               // 0x8001DE84
    bool flag_60d = false, flag_12 = false;
    int16_t turn_delta = 0;                   // 0x80031BEC(player +0x14, man +0x14, +0x442, 32)
};
St03Event8Step step_st09_event17_80100788(St03Event8& ev, const St03Event8Profile& p, const St09Event17Inputs& in) noexcept;

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
// Event 0x17's timeline and camera program from the cooked event23.l3cs.
bool bind_st09_event17(const SourceScenePackage& package, St03Event8Profile& out);

} // namespace l3
