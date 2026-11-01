#pragma once
// ST07 event 10, the title attract scene (docs/research_01516/ATTRACT_EVENT10.md):
// handler 0x80100330 (phases 0x8010036C / 0x80100598 / 0x801006D8) on the
// shared scene runtime (source_scene_runtime.h), and the record callbacks'
// background and area effects. Actor spawns/releases in the callbacks are not ported yet;
// the steps report them. Original addresses are identity tokens.
#include "legends3ds/source_bg_layers.h"
#include "legends3ds/source_scene_runtime.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// The event's tables, bound from the cooked L3CS package by original address:
// timelines 0x80105E1C / 0x801060D4, camera programs 0x80105DD0 / 0x80105E2C,
// the background rows 0x80106124 (cook_cutscene.py --extra) and the
// importer's callback -> area requests (0x80100CD4).
struct SourceAttractProfile {
    bool loaded = false;
    std::array<SourceSceneTimeline,2> timelines;
    std::array<SourceSceneCameraProgram,2> cameras;
    std::array<std::array<uint8_t,8>,8> bg_rows{};
    std::vector<SourceSceneAreaRequest> area_requests;
};
bool bind_source_attract_profile(const SourceScenePackage& package, SourceAttractProfile& out);

struct SourceAttractEvent10State {
    uint8_t phase = 0, sub = 0;
    uint8_t flags_0a = 0, count_0b = 0;
    uint32_t callback_counter_80108b0c = 0;
    uint8_t timeline = 0, camera_program = 0;   // which cooked table is current
    size_t record = 0;
    SourceEventCounters event;
    SourceCameraBlock camera;
    bool done = false;
};
struct SourceAttractEvent10Step {
    uint8_t fade_type = 0;          // 0x80017C30 request this tick
    bool cd_command = false;        // 0x8001D58C(0x20000005)
    int8_t area_request = -1;       // 0x80100CD4(area)
    uint8_t actor_callbacks = 0;    // callbacks whose actor spawns/releases are not ported
    uint32_t callback = 0;          // record callback run this tick (0 = none)
};
// One tick of 0x80100330. cd_done: 0x8001D878; fade_busy: 0x800A8A72;
// start: pad bit 8 (0x800C0C2A). layers: the 0x800BBA90 records (area 0
// set already initialised by the stage init).
SourceAttractEvent10Step step_source_attract_event10_80100330(SourceAttractEvent10State& st,
    const SourceAttractProfile& profile, SourceBgLayers& layers, bool cd_done, int16_t fade_busy, bool start) noexcept;
} // namespace l3
