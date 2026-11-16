#pragma once
// ST00 post-win sequence (docs/research_01516/AREA3_BOSS_SOURCE_SLICE.md,
// "Post-win events 3 and 2"): event 3 (handler 0x80102058, started by poll
// flag 0x606 when the Area 3 boss falls) and event 2 (handler 0x8010194C,
// Area 4, poll flag 0x605 that Area 4's init 0x8010038C case 4 sets).
// Both run on the shared scene runtime (source_scene_runtime.h) over the
// cooked L3CS packages; the record callbacks are hand-ported here.
// Original addresses are identity tokens.
//
// The scene actors the events spawn (0x8001EAE8 records; byte 0 selects the
// event slot +0x24 + 4n) are not simulated here: the callbacks' writes to
// them are recorded per slot as commands for their owners.
#include "legends3ds/source_scene_runtime.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {

// The fields of a spawned scene actor the event callbacks write.
struct SourceSceneSlotCommands {
    bool spawned = false;
    uint8_t family = 0, object_class = 0, byte_04 = 0;   // record [3], [1], [4]
    std::array<int16_t,3> spawn{};                       // record [12..17]
    uint16_t spawn_heading = 0;                          // record [18..19]
    uint8_t flags_00 = 0;                                // |= 2 (drawn) by event 2 callbacks
    uint8_t state_09 = 0, sub_0a = 0, byte_0b = 0, byte_0c = 0, byte_128 = 0;
    std::optional<uint16_t> heading_56, colour_15e;
    std::optional<int32_t> fixed_1c, fixed_20, fixed_24;  // 16.16 position writes
    uint32_t writes = 0;                                 // count of callback writes (diagnostic)
};

// The 0x800C3558.. transition lanes a hand-off writes (offsets from 0x800C3560)
// in the shared intent shape: raw[0..5] xyz, [10..11] heading, [12] 356C,
// [13] 356D, [14] 356E stage, [15] 356F area, [16..17] 3570/3571.
struct SourcePostWinHandoff {
    std::array<uint8_t,18> raw{};
    uint8_t stage = 0, area = 0;
    uint16_t cleared_flag = 0;
};

// One requested object spawn (0x80031618 caller fills; not drawn natively yet).
struct SourcePostWinObject {
    uint8_t b00 = 0, b02 = 0, b0c = 0, b0d = 0, b0e = 0;
    std::array<int16_t,3> pos{};
};

struct SourcePostWinStep {
    uint32_t callback = 0;              // record callback run this tick (0 = none)
    uint8_t fade_type = 0;              // 0x80017C30 request
    uint32_t cd_request = 0;            // 0x8001D58C argument
    bool stop_sequences = false;        // 0x8001B33C
    bool player_hidden = false;         // player +0 &= 0xFC
    bool constructed = false;           // the constructor ran this tick
    std::array<uint8_t,2> spawned_slots{};  // 0x8001EAE8 this tick (slot indexes)
    uint8_t spawn_count = 0;
    std::optional<uint32_t> camera_program; // stored to 0x800BC7F0
    bool camera_reset_8001e6c0 = false; // 0x8001E6C0(0, 0, 0)
    // Event 3 smoke emitters 0x80116D60 (positions as passed) and their release.
    std::array<std::array<int16_t,3>,2> smoke{};
    uint8_t smoke_count = 0;
    bool smoke_release = false;         // 0x80031988 on the five handles
    std::optional<SourcePostWinObject> object;
    std::optional<std::array<uint16_t,2>> sound_8002ee90;
    // Event 2: 0x8011877C(pos, a1, a2), twice in one callback.
    std::array<std::array<int16_t,5>,2> fx_8011877c{};
    uint8_t fx_8011877c_count = 0;
    std::optional<SourcePostWinHandoff> handoff;   // the event ended this tick
};

struct SourcePostWinInputs {
    bool player_ready = false;   // 0x80042238(player) result
    bool cd_done = false;        // 0x8001D878
    uint16_t fade_busy = 0;      // 0x80098A72
    bool start_skip = false;     // 0x8001DE84
    // Positions of slot actors the callbacks read (+0x14..+0x18); a slot
    // without an owner reads its spawn position.
    std::array<std::optional<std::array<int16_t,3>>,5> slot_position{};
};

struct SourcePostWinEvent {
    uint8_t event = 0;            // 3 or 2 (event +1)
    bool active = false;          // event +0
    uint8_t phase = 0;            // +4
    uint32_t countdown = 0;       // +0x0C
    size_t record = 0;            // +0xA4 as a record index (valid once constructed)
    bool constructed = false;
    SourceEventCounters counters; // +8/+9/+0x10..+0x1C
    SourceCameraBlock camera;     // 0x800BC738 as the event's program drives it
    std::array<SourceSceneSlotCommands,8> slots{};  // event +0x24 + 4n
    uint32_t ticks = 0;
};

// The cooked tables an event needs, bound by original address.
struct SourcePostWinProfile {
    bool loaded = false;
    uint8_t event = 0;
    SourceSceneTimeline timeline;
    std::optional<SourceSceneCameraProgram> camera;     // event 2's program is not interpretable yet (op 0x80)
    std::vector<std::array<uint8_t,20>> spawns;
};
// Event 3: timeline 0x8011BE44, camera 0x8011BB3C, spawns 0x8011BB14/0x8011BB28.
// Event 2: timeline 0x8011BA4C, camera 0x8011B4B4 (optional), 14 spawns from
// 0x8011B388 (cook_cutscene.py --extra 8011B388:280).
bool bind_source_post_win_profile(const SourceScenePackage& package, uint8_t event, SourcePostWinProfile& out);

// 0x8001DDE4 for event 3 or 2: the work is zeroed and +0 = 1.
SourcePostWinEvent start_source_post_win_event(uint8_t event) noexcept;

// One tick of 0x80102058 (event 3) or 0x8010194C (event 2).
SourcePostWinStep step_source_post_win_event(SourcePostWinEvent& ev, const SourcePostWinProfile& profile,
                                             const SourcePostWinInputs& in) noexcept;

// The record callbacks, exposed for tests: run `callback` against the event's
// slots at the current +0x10.
void run_source_post_win_callback(SourcePostWinEvent& ev, uint32_t callback, const SourcePostWinInputs& in,
                                  SourcePostWinStep& out) noexcept;
} // namespace l3
