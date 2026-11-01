#pragma once
// ST07 event 10, the title attract scene (docs/research_01516/ATTRACT_EVENT10.md):
// handler 0x80100330 (phases 0x8010036C / 0x80100598 / 0x801006D8) on the
// shared scene runtime (source_scene_runtime.h), and the record callbacks'
// background and area effects. Actor spawns/releases in the callbacks are not ported yet;
// the steps report them. Original addresses are identity tokens.
#include "legends3ds/source_bg_layers.h"
#include "legends3ds/reference_tables.h"
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
    std::vector<std::array<uint8_t,20>> records;   // 0x80105B8C.. (29 spawn records)
    std::vector<uint32_t> servbot_behaviours;      // 0x8010663C[id] (class 0x20/0x00 by +0xC)
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
    uint8_t release_slots = 0;      // 0x80031824 on event+0x24.. slots 0..n-1 this tick
    int8_t spawn_first = -1;        // 0x8001EAE8 on records [spawn_first, spawn_first + spawn_count)
    uint8_t spawn_count = 0;
    uint32_t callback = 0;          // record callback run this tick (0 = none)
};
// One tick of 0x80100330. cd_done: 0x8001D878; fade_busy: 0x800A8A72;
// start: pad bit 8 (0x800C0C2A). layers: the 0x800BBA90 records (area 0
// set already initialised by the stage init).
SourceAttractEvent10Step step_source_attract_event10_80100330(SourceAttractEvent10State& st,
    const SourceAttractProfile& profile, SourceBgLayers& layers, bool cd_done, int16_t fade_busy, bool start) noexcept;

// A Servbot (family 0x20 class 0x00, per-type table 0x801052DC[4] =
// 0x8010205C). Spawn 0x8001E968: +0x14/+0x1C position from rec[12..17],
// heading +0x56 = rec[18..19]; 0x8001EAE8: +0xC..+0xF = rec[8..11] (id,
// animation request, size flag, +0xF).
struct SourceAttractServbot {
    uint8_t slot = 0, id = 0, request = 0, big = 0, byte_0f = 0;
    std::array<int16_t,3> raw{};        // +0x14
    std::array<int32_t,3> fixed{};      // +0x1C (16.16)
    uint16_t heading = 0;               // +0x56
    uint8_t state = 0, started = 0;     // +8, +9
    uint32_t behaviour = 0;             // 0x8010663C[id]
    std::array<int16_t,3> scale() const noexcept { const int16_t v = big == 1 ? 1024 : 512; return {{v, v, v}}; }
};
bool spawn_source_attract_servbot(const std::array<uint8_t,20>& record, const SourceAttractProfile& profile,
                                  SourceAttractServbot& out) noexcept;
struct SourceAttractAnimation { bool reset = false; uint8_t request = 0; bool step = false; bool shadow = false; };
// One update of 0x8010205C: state 0 runs the init 0x8010212C (the floor query
// 0x80048E6C is the caller's), then every update saves nothing the port uses
// and runs the behaviour 0x8010663C[+0xC] with event+0x10 (after the
// timeline's increment this tick).
SourceAttractAnimation update_source_attract_servbot_8010205c(SourceAttractServbot& servbot, uint32_t event_10,
                                                              const ReferenceTables& refs) noexcept;

// The bridge's static cast (family 0x20, per-class table [+4 = 10]): class
// 0x28 (0x80102D84) and class 0x2B (0x80102EA8). Init: drawn flag, model,
// scale +0x158 = (512, 512, 512), floor query (y += floor), animation request
// 19 (0x28) or 0 (0x2B); then 0x80032488 each update (shadow 0x8002C8CC with
// radius 48 not ported). Placement as 0x8001E968.
struct SourceAttractStatic {
    uint8_t object_class = 0, slot = 0, request = 0;
    std::array<int16_t,3> raw{};
    uint16_t heading = 0;
};
bool spawn_source_attract_static(const std::array<uint8_t,20>& record, SourceAttractStatic& out) noexcept;
} // namespace l3
