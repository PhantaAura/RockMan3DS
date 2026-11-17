#pragma once
// ST03 event 8 (docs/research_01516/STAGE_LOADING.md, "ST03 area 1 and
// event 8"): handler 0x80101D18 runs one of five scenes by event +4. Scenes 0 (the
// crash site, 0x80101D54) and 1 (the police car, 0x80102224) are ported. It runs on the shared
// scene runtime (timelines 0x80115FB4 / 0x8011600C, camera programs
// 0x80115FC4 / 0x80116034 from the cooked event8.l3cs) and reports its
// writes to the player, the two ST03 scene objects (0x8011CB30 / 0x8011CB34),
// the message window and the story flags for the runtime to apply.
// Original addresses are identity tokens.
#include "legends3ds/source_scene_runtime.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

struct St03Event8Profile {
    bool loaded = false;
    SourceSceneTimeline intro, crash;              // 0x80115FB4, 0x8011600C
    SourceSceneCameraProgram intro_camera, crash_camera;   // 0x80115FC4, 0x80116034
    bool police_loaded = false;                    // scene 1
    SourceSceneTimeline police;                    // 0x8011614C
    SourceSceneCameraProgram police_camera;        // 0x80116164
};
bool bind_st03_event8_profile(const SourceScenePackage& package, St03Event8Profile& out);

struct St03Event8 {
    bool active = false;          // event +0
    uint8_t scene = 0;            // +4
    uint8_t state = 0;            // +6
    uint8_t byte_07 = 0;          // +7
    int32_t countdown = 0;        // +0x0C
    const SourceSceneTimeline* timeline = nullptr;   // +0xA4 (record index below)
    size_t record = 0;
    SourceEventCounters counters; // +8/+9/+0x10..+0x1C
    const SourceSceneCameraProgram* camera_program = nullptr;   // 0x800BC7F0
    SourceCameraBlock camera;
    uint32_t ticks = 0;
};

// The player walk target 0x80048754 / 0x80048898 consume (player state 3/1).
struct St03PlayerTarget {
    std::array<int16_t,3> position{};   // +0x438 (and +0x428 as 16.16)
    std::optional<uint16_t> heading;    // +0x442, when written
    bool walk = false;                  // +8 = 3, +9 = 1, +0x440..+0x444 = 0, +0x448 = 48, +0xBD |= 0x10
    bool from_pose = false;             // scene 1: targets = the current pose (+0x14/+0x1C/+0x54), +0x448 = 48
};
struct St03ObjectWrite {
    std::optional<uint8_t> variant_04, state_08, state_09;   // +9 writes also clear +0x0A
    std::optional<uint16_t> heading_56;
    bool clear_0a_0b = false;            // halfword +0x0A = 0
};

struct St03Event8Inputs {
    bool player_ready = false;    // 0x80042238(player)
    bool subphase_2 = false;      // 0x800C1B11 == 2 (main-loop subphase; CANDIDATE mapping in the runtime)
    bool message_open = false;    // 0x80098A58 & 0x08000000 (slot-0 window)
    bool start_pressed = false;   // 0x8001DE84
    // Scene 1 (runtime-computed): player +0x16 equals 0x80048E6C(player)
    // (CANDIDATE mapping to the port's floor query), and 0x80031BEC's turn
    // delta from the player toward (-2240, 7000) at step 32.
    bool player_on_floor = false;
    int16_t turn_delta = 0;
};

struct St03Event8Step {
    std::optional<St03PlayerTarget> player;
    std::array<St03ObjectWrite,4> objects{};      // *(0x8011CB30 + 4 * i); 3 = the police car (0x8011CB3C)
    std::optional<uint8_t> player_anim_448;       // player +0x448
    std::optional<int16_t> player_heading_add;    // +0x442 = (+0x442 + n) & 0xFFF
    std::optional<uint16_t> sound_8001a008;       // 0x8001A008(n, 0, 0)
    bool player_heading_zero = false;             // +0x442 = 0
    bool flag_8009395c = false;                   // 0x8009395C = 1 without ending the event (scene 1)
    std::array<uint8_t,2> messages{};             // 0x80053A08(0, n) this tick, in order
    uint8_t message_count = 0;
    bool camera_reset_8001e6c0 = false;           // 0x8001E6C0(0, 0, 0)
    std::optional<uint16_t> flag_set;             // 0x8001DA8C
    bool skipped = false;                         // the Start path ran (0x80053A30, 0x80063BA8,
                                                  // 0x8001D494(0,1,0), 0x8001B314, 0x8001A008(264,0,0))
    bool player_release = false;                  // 0x80042314
    bool ended = false;                           // event +0 = 0 (scene 0 also 0x8009395C = 1)
    bool release_set = false;                     // 0x80103BA0
    std::optional<uint8_t> spawn_set;             // 0x80103AB0(n)
    bool unsupported_scene = false;               // scenes 2..4 are not ported
};

// 0x8001DDE4(8) with event +4 = scene.
St03Event8 start_st03_event8(uint8_t scene) noexcept;
// One tick of 0x80101D18.
St03Event8Step step_st03_event8(St03Event8& ev, const St03Event8Profile& profile, const St03Event8Inputs& in) noexcept;

// ST03 area 1's set-up 0x80100990, first pass: the object set it spawns and
// whether it starts event 8 scene 0, from story flags 12, 14, 13, 0, 4, 3.
// (The follow-up states are st03_area1_setup_step below.)
struct St03Area1Setup { uint8_t object_set = 0; bool start_event8 = false; };
template <class FlagFn>
St03Area1Setup st03_area1_setup_80100990(FlagFn flag) {
    if (flag(12)) return {5, false};
    if (flag(14)) return {4, false};
    if (flag(13)) return {3, false};
    if (flag(0)) return {2, false};
    if (flag(4)) return {1, false};
    if (flag(3)) return {0, false};
    return {0, true};
}

// The full 0x80100990 state machine over its byte +4, once per frame.
// State 0 is the first pass above (+4 = 255 for flags 12/14/13/0, 2 for 4,
// 1 for 3 or none; 0x80101918(n) is reported, not ported). State 1 is the
// trigger box (0x80100ADC): with flag 3 set and 4 / 0x605 clear, the player
// cell (raw x/z >> 9) within box 0x80114F2C (origin -5, 11; size 2 x 2)
// sets 0x605 and starts event 8 scene 1; flag 4 moves on to state 2. State
// 2 (0x80100B90): flag 0 moves on; otherwise flag 0x604 is cleared and event
// 8 scene 2 starts.
struct St03Area1SetupStep {
    std::optional<uint8_t> object_set;     // 0x80103AB0(n)
    std::optional<uint8_t> event8_scene;   // 0x8001DDE4(8) with 0x800C4C4C = scene
    std::optional<uint8_t> call_80101918;
    std::optional<uint16_t> flag_set, flag_clear;
};
template <class FlagFn>
St03Area1SetupStep st03_area1_setup_step(uint8_t& state_04, FlagFn flag, int16_t player_x, int16_t player_z) {
    St03Area1SetupStep out;
    switch (state_04) {
    case 0: {
        const auto first = st03_area1_setup_80100990(flag);
        out.object_set = first.object_set;
        if (flag(12)) state_04 = 255;
        else if (flag(14) || flag(13)) { state_04 = 255; out.call_80101918 = 1; }
        else if (flag(0)) { state_04 = 255; out.call_80101918 = 0; }
        else if (flag(4)) { state_04 = 2; out.call_80101918 = 0; }
        else { state_04 = 1; out.call_80101918 = 0; if (first.start_event8) out.event8_scene = 0; }
        break;
    }
    case 1:
        if (!flag(3)) break;
        if (flag(4)) { state_04 = 2; break; }
        if (flag(0x605)) break;
        if (uint32_t((player_x >> 9) - (-5)) < 2u && uint32_t((player_z >> 9) - 11) < 2u) {
            out.event8_scene = 1; out.flag_set = 0x605;
        }
        break;
    case 2:
        if (flag(0)) { state_04 = 3; break; }
        if (flag(0x604)) { out.flag_clear = 0x604; out.event8_scene = 2; }
        break;
    default: break;
    }
    return out;
}

} // namespace l3
