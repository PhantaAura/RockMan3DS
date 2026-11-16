#pragma once
// Stage loading beyond ST00 (docs/research_01516/STAGE_LOADING.md).
//
// The original changes stage when a hand-off writes 0x800C356C = 2 with a
// stage byte 356E that differs from the current stage 0x800C1B60 (ST00
// event 2: 356E = 2, 356F = 0, 3560..3565 = 0, 356A = 0). The stage loader
// then reloads the stage overlay, map and area set, sets 0x800C1B60/61 to the
// requested stage/area and spawns the player through 0x80015EE8 at the
// transition lanes. Story flags, life and the statistics selector live in
// globals the loader does not touch, so they survive the load.
//
// The port mirrors this by rebuilding the whole RuntimeSession (and the 3DS
// renderer resources) from the new stage's cooked data and carrying that
// state across. The main-phase bytes the original passes through on the way
// (0x800C1B10/11) are not traced; only the in-game value 03 02 is observed.
#include "legends3ds/save_state.h"
#include "legends3ds/source_event_pipeline.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace l3 {

// One stage-change request, read from the 0x800C3560.. transition lanes.
struct SourceStageRequest {
    uint8_t stage = 0;                 // 356E
    uint8_t area = 0;                  // 356F
    std::array<int16_t,3> spawn{};     // 3560..3565
    uint16_t heading = 0;              // 356A..356B
    uint32_t source_function = 0;      // the event handler that wrote the lanes
    uint64_t generation = 0;
};

// The 0x800C3560.. lanes as the post-win hand-off records them (raw[0..5]
// xyz, raw[10..11] heading, raw[14] stage, raw[15] area).
SourceStageRequest source_stage_request_from_lanes(const std::array<uint8_t,18>& raw,
                                                   uint32_t source_function, uint64_t generation) noexcept;

// What the stage loader leaves alone.
struct SourceStageCarry {
    std::array<uint8_t,kL3StoryFlagBytes> story_flags{};
    uint16_t hp = 0, hp_max = 0;
    uint32_t frames = 0;
    std::optional<uint8_t> stats_selector;
};

// Where the player starts in the new stage (0x80015EE8 with the lanes).
struct SourceStageArrival {
    uint8_t stage = 0, area = 0;       // 0x800C1B60 / 0x800C1B61 after the load
    std::array<int16_t,3> position{};
    uint16_t heading = 0;
};
SourceStageArrival source_stage_arrival(const SourceStageRequest& request) noexcept;

// Cooked-pack layout: ST00 lives at the pack root (historic names st00.*);
// every other stage under stages/stNN/ with generic names (map.l3map,
// render.l3render, init.l3init, flow.l3flow); ebd/ and materials/ keep their
// relative names. Returns the path relative to the pack root.
std::string stage_asset_path(uint8_t stage, const std::string& st00_relative);

} // namespace l3
