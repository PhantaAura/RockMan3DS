#pragma once
// ST07 event 10, the title attract scene (docs/research_01516/ATTRACT_EVENT10.md):
// handler 0x80100330 (phases 0x8010036C / 0x80100598 / 0x801006D8), the
// timeline consumer 0x8001F070, the camera program interpreter 0x8001EC0C
// over the camera block 0x800BC738, and the record callbacks' background and
// area effects. Actor spawns/releases in the callbacks are not ported yet;
// the steps report them. Original addresses are identity tokens.
#include "legends3ds/source_bg_layers.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
struct SourceAttractTimeline {
    uint32_t token = 0;                       // original address of record 0
    std::vector<std::array<uint32_t,2>> records;   // {header, callback}
};
struct SourceAttractCameraProgram {
    uint32_t token = 0;
    std::vector<uint32_t> words;
};
struct SourceAttractProfile {
    bool loaded = false;
    std::array<SourceAttractTimeline,2> timelines;      // 0x80105E1C, 0x801060D4
    std::array<SourceAttractCameraProgram,2> cameras;   // 0x80105DD0, 0x80105E2C
    std::array<std::array<uint8_t,8>,8> bg_rows{};      // 0x80106124
};
constexpr size_t kSourceAttractProfileBytes = 16 + 32 + 2 * (8 + 8 * 16) + 2 * (8 + 4 * 256) + 64;
bool load_source_attract_profile(const std::vector<uint8_t>& bytes, SourceAttractProfile& out);

// The 0x800BC738 fields 0x8001EC0C reads and writes (16.16 words).
struct SourceCameraBlock {
    std::array<int32_t,3> target{}, target_velocity{};   // +8, +0x18
    std::array<int32_t,3> angles{}, angle_velocity{};    // +0x38 (yaw/pitch/distance in the high halves), +0x48
    std::array<int32_t,3> third{}, third_velocity{};     // +0x68, +0x78
    std::array<int32_t,3> word_a8{};                     // +0xA8 (op 20)
    std::array<int32_t,3> offset_98{};                   // +0x98 (0x8001E6C0 args << 16)
    uint8_t mode = 0, tracked_slot = 0, tracked = 0, shift = 0;
    uint32_t pc = 0;                                     // +0xB8: word index into the program
    bool unsupported = false;
};
struct SourceEventCounters {
    uint8_t e8 = 0, e9 = 0;
    uint32_t c10 = 0, c14 = 0, c18 = 0, c1c = 0;
};
// One 0x8001EC0C pass: commands from pc until a wait fails (pc keeps its
// place), then target/angles/third += their velocities.
void step_source_camera_program_8001ec0c(SourceCameraBlock& camera, const SourceAttractCameraProgram& program,
                                         const SourceEventCounters& event) noexcept;
struct SourceCameraView {
    std::array<int32_t,3> target{};
    int16_t yaw = 0, pitch = 0, distance = 0;
    uint8_t shift = 0;
};
SourceCameraView source_camera_view(const SourceCameraBlock& camera) noexcept;

// One 0x8001F070 pass: counters +1; at the record's tick count, the next
// record's event+8/+9 (resetting +0x18/+0x1C on a change), +0x10 = 0, next.
void step_source_timeline_8001f070(SourceEventCounters& event, size_t& record, const SourceAttractTimeline& timeline) noexcept;

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
