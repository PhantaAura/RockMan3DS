#pragma once
// ST03 event 8 (docs/research_01516/STAGE_LOADING.md, "ST03 area 1 and
// event 8"): handler 0x80101D18 runs one of five scenes by event +4. Only
// scene 0 (the crash site, 0x80101D54) is ported. It runs on the shared
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
};
struct St03ObjectWrite {
    std::optional<uint8_t> variant_04, state_08, state_09;   // +9 writes also clear +0x0A
    std::optional<uint16_t> heading_56;
};

struct St03Event8Inputs {
    bool player_ready = false;    // 0x80042238(player)
    bool subphase_2 = false;      // 0x800C1B11 == 2 (main-loop subphase; CANDIDATE mapping in the runtime)
    bool message_open = false;    // 0x80098A58 & 0x08000000 (slot-0 window)
    bool start_pressed = false;   // 0x8001DE84
};

struct St03Event8Step {
    std::optional<St03PlayerTarget> player;
    std::array<St03ObjectWrite,2> objects{};
    std::array<uint8_t,2> messages{};             // 0x80053A08(0, n) this tick, in order
    uint8_t message_count = 0;
    bool camera_reset_8001e6c0 = false;           // 0x8001E6C0(0, 0, 0)
    std::optional<uint16_t> flag_set;             // 0x8001DA8C
    bool skipped = false;                         // the Start path ran (0x80053A30, 0x80063BA8,
                                                  // 0x8001D494(0,1,0), 0x8001B314, 0x8001A008(264,0,0))
    bool player_release = false;                  // 0x80042314
    bool ended = false;                           // event +0 = 0, 0x8009395C = 1
    bool release_set = false;                     // 0x80103BA0
    std::optional<uint8_t> spawn_set;             // 0x80103AB0(n)
    bool unsupported_scene = false;               // scenes 1..4 are not ported
};

// 0x8001DDE4(8) with event +4 = scene.
St03Event8 start_st03_event8(uint8_t scene) noexcept;
// One tick of 0x80101D18.
St03Event8Step step_st03_event8(St03Event8& ev, const St03Event8Profile& profile, const St03Event8Inputs& in) noexcept;

// ST03 area 1's set-up 0x80100990, first pass: the object set it spawns and
// whether it starts event 8 scene 0, from story flags 12, 14, 13, 0, 4, 3.
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

} // namespace l3
