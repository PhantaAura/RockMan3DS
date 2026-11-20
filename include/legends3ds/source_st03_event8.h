#pragma once
// ST03 event 8 (docs/research_01516/STAGE_LOADING.md, "ST03 area 1 and
// event 8"): handler 0x80101D18 runs one of five scenes by event +4. Scenes 0 (the
// crash site, 0x80101D54) and 1 (the police car, 0x80102224) are ported. It runs on the shared
// scene runtime (timelines 0x80115FB4 / 0x8011600C, camera programs
// 0x80115FC4 / 0x80116034 from the cooked event8.l3cs) and reports its
// writes to the player, the two ST03 scene objects (0x8011CB30 / 0x8011CB34),
// the message window and the story flags for the runtime to apply.
// Original addresses are identity tokens.
#include "legends3ds/source_door_locks.h"
#include "legends3ds/source_scene_runtime.h"
#include "legends3ds/source_st03_objects.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

struct St03Event8Profile {
    bool loaded = false;
    SourceSceneTimeline intro, crash;              // 0x80115FB4, 0x8011600C
    SourceSceneCameraProgram intro_camera, crash_camera;   // 0x80115FC4, 0x80116034
    bool inspector_loaded = false;                 // scene 2
    SourceSceneTimeline inspector;                 // 0x801161D0 (event +0xA4)
    SourceSceneCameraProgram inspector_camera;     // 0x801161F8 (0x800BC7F0)
    bool police_loaded = false;                    // scene 1
    SourceSceneTimeline police;                    // 0x8011614C
    SourceSceneCameraProgram police_camera;        // 0x80116164
    std::array<std::vector<St03ObjectRecord>,40> object_sets;   // 0x80116578 (cooked extras; ST04 uses up to 34)
    std::optional<St03GenericClasses> generic_classes;           // other stages: classes whose variant 0 is the generic NPC
    bool idle_timers_loaded = false;
    std::array<uint8_t,64> idle_timers{};          // 0x80116E68
    bool walk_loaded = false;
    std::array<uint8_t,112> walk_speeds{};         // 0x80116DAC
    std::array<uint8_t,112> walk_tables{};         // 0x80116ED4
    bool doors_loaded = false;
    std::array<uint8_t,8> doors{};                 // 0x801152E4: ST03's door list
    std::array<std::array<uint8_t,8>,15> door_states{};   // 0x801152EC: rows by 0x800C1B62 (12..14: flag overrides)
    bool car_paths_loaded = false;
    std::array<uint8_t,88> car_paths{};            // 0x80117274
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
    bool clear_0b = false;               // byte +0x0B = 0
    std::optional<uint8_t> byte_0c_from, byte_0d_from;   // +0x0C / +0x0D = slot k's +1 (its id)
    std::optional<uint8_t> byte_0f_or;   // +0x0F |= n
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
    // Scene 2: 0x8003A13C() (the focus camera still moving; CANDIDATE in the runtime),
    // the flags it polls, and the two positions state 0 reads.
    bool camera_busy = false;
    bool flag_606 = false;
    std::array<int16_t,3> barrell{};              // *(0x8011CB34) +0x14
    uint16_t heading_barrell_to_player = 0;       // 0x80031198(Barrell, player)
};

struct St03Event8Step {
    std::optional<St03PlayerTarget> player;
    std::array<St03ObjectWrite,6> objects{};      // *(0x8011CB30 + 4 * i) (set 0: 3 = the police car; set 1: 5)
    uint16_t release_slots = 0;                   // 0x80103CC4(i) for each bit i
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
    bool unsupported_scene = false;               // scenes 3..4 are not ported
    // Scene 2:
    struct CameraFocus { std::array<int16_t,3> point{}; int16_t distance = 0; uint16_t yaw = 0; };
    std::optional<CameraFocus> camera_focus;      // 0x80039FFC(point, distance, yaw, *(0x8009396C) + 97)
    bool camera_restore_80039e50 = false;
    bool player_hide = false, player_show = false;   // player +0 &= ~2 / |= 2 (with +0xBD bit 0x40)
    std::optional<uint16_t> player_heading_56;    // player +0x56
    std::optional<uint16_t> player_heading_442;   // player +0x442
    std::array<uint16_t,2> flag_clears{};         // 0x8001DCD0, in order
    uint8_t flag_clear_count = 0;
};

constexpr uint32_t kSt03Doors = 0x801152E4u;   // + 15 state rows of 8 bytes

// ST03's stage init 0x801007B4: the door locks from row 0x800C1B62 and, when
// that byte is 0, again from row 14 / 13 / 12 when flag 2 / 14 / 0 is set.
template <class FlagFn, class SetFn>
void st03_stage_init_door_locks_801007b4(const St03Event8Profile& p, uint8_t byte_800c1b62, FlagFn flag, SetFn set_flag) {
    if (!p.doors_loaded || byte_800c1b62 >= p.door_states.size()) return;
    source_door_locks_8001e4c4(p.doors, p.door_states[byte_800c1b62], set_flag);
    if (byte_800c1b62 != 0) return;
    if (flag(2)) source_door_locks_8001e4c4(p.doors, p.door_states[14], set_flag);
    else if (flag(14)) source_door_locks_8001e4c4(p.doors, p.door_states[13], set_flag);
    else if (flag(0)) source_door_locks_8001e4c4(p.doors, p.door_states[12], set_flag);
}

// ST03's area-enter call 0x8010089C, once per area load (the area loader
// 0x8001F554 clears 0x1FF): while 0x1FF is clear, 0x80115124[area] runs.
// Areas 0..2: with block +4 == 0 and flag 0x235, locks 0x7E2/3/4/8; areas
// 0..6 then set 0x1FF (area 0 with block +4 == 0xFF does not). Areas 7 and 8
// (0x238 / 0x239 paths, 0x80101B70 / 0x8001EB98) are reported, not run;
// they set 0x1FF too. Returns false when the area function is unported.
template <class FlagFn, class SetFn>
bool st03_area_enter_8010089c(uint8_t area, uint8_t block_04, FlagFn flag, SetFn set_flag) {
    if (flag(0x1FF)) return true;
    if (area == 0 && block_04 == 0xFFu) return true;
    if (area <= 2 && block_04 == 0 && flag(0x235))
        for (uint16_t f : {uint16_t(0x7E2), uint16_t(0x7E3), uint16_t(0x7E4), uint16_t(0x7E8)}) set_flag(f, true);
    set_flag(0x1FF, true);
    return area <= 6;
}

// ST03 area 2's per-frame set-up 0x80100BF8 (byte +4), first pass (state
// 0). By the first set flag of 6 / 23 / (1 and 2) / 26 / 12 / 14 / 13 / 4:
// set 16 (state 255); set 12 (state 2); event 8 scene 3 (state 2); set 10
// (255); set 9 (255; with 34 and not 69 it also re-spawns slot 0 from
// 0x80116754, reported); set 8 (255); set 7 + flag 0x60A (255); set 6 +
// 0x60A (255); none: set 6 + 0x60A (255). States 2..5 (scene 4 on 0x604,
// message 103 + set 10 on flag 26, events 0x60 / 0x61) are reported, not run.
struct St03Area2SetupStep {
    std::optional<uint8_t> object_set;
    std::optional<uint8_t> event8_scene;
    std::optional<uint16_t> flag_set;
    bool respawn_slot0_unported = false;
    bool state_unported = false;
};
template <class FlagFn>
St03Area2SetupStep st03_area2_setup_step(uint8_t& state_04, FlagFn flag) {
    St03Area2SetupStep out;
    if (state_04 == 255) return out;
    if (state_04 != 0) { out.state_unported = true; return out; }
    if (flag(6)) { out.object_set = 16; state_04 = 255; }
    else if (flag(23)) { out.object_set = 12; state_04 = 2; }
    else if (flag(1) && flag(2)) { state_04 = 2; out.event8_scene = 3; }
    else if (flag(26)) { out.object_set = 10; state_04 = 255; }
    else if (flag(12)) { out.object_set = 9; state_04 = 255; out.respawn_slot0_unported = flag(34) && !flag(69); }
    else if (flag(14)) { out.object_set = 8; state_04 = 255; }
    else if (flag(13)) { out.object_set = 7; out.flag_set = 0x60A; state_04 = 255; }
    else { out.object_set = 6; out.flag_set = 0x60A; state_04 = 255; }   // flag 4 or none
    return out;
}

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
