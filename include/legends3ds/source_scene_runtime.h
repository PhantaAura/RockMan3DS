#pragma once
// Shared cutscene runtime: the cooked L3CS package (tools/cook_cutscene.py)
// and the engine services every event uses, the timeline consumer 0x8001F070
// and the camera program interpreter 0x8001EC0C over the camera block
// 0x800BC738. Per-event handlers and record callbacks stay per scene.
// Original addresses are identity tokens.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
struct SourceSceneTimeline {
    uint32_t token = 0;                              // original address of record 0
    std::vector<std::array<uint32_t,2>> records;    // {header, callback}
};
struct SourceSceneCameraProgram {
    uint32_t token = 0;
    std::vector<uint32_t> words;
};
struct SourceSceneSpawnRecord { uint32_t token = 0; std::array<uint8_t,20> bytes{}; };
struct SourceSceneAreaRequest { uint32_t callback = 0; int32_t area = 0; };
struct SourceSceneData { uint32_t token = 0; std::vector<uint8_t> bytes; };
struct SourceScenePackage {
    bool loaded = false;
    uint16_t event = 0, stage = 0;
    uint32_t handler = 0, bg_table = 0;
    std::vector<SourceSceneTimeline> timelines;
    std::vector<SourceSceneCameraProgram> cameras;
    std::vector<SourceSceneSpawnRecord> spawns;
    std::vector<uint32_t> cd_requests;
    std::vector<SourceSceneAreaRequest> area_requests;
    std::vector<SourceSceneData> data;
    const SourceSceneTimeline* timeline(uint32_t token) const noexcept;
    const SourceSceneCameraProgram* camera(uint32_t token) const noexcept;
    const SourceSceneData* data_at(uint32_t token) const noexcept;
};
constexpr size_t kSourceScenePackageMaxBytes = 64 * 1024;
bool load_source_scene_package(const std::vector<uint8_t>& bytes, SourceScenePackage& out);

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
void step_source_camera_program_8001ec0c(SourceCameraBlock& camera, const SourceSceneCameraProgram& program,
                                         const SourceEventCounters& event) noexcept;
struct SourceCameraView {
    std::array<int32_t,3> target{};
    int16_t yaw = 0, pitch = 0, distance = 0;
    uint8_t shift = 0;
};
SourceCameraView source_camera_view(const SourceCameraBlock& camera) noexcept;
// One 0x8001F070 pass: counters +1; at the record's tick count, the next
// record's event+8/+9 (resetting +0x18/+0x1C on a change), +0x10 = 0, next.
void step_source_timeline_8001f070(SourceEventCounters& event, size_t& record, const SourceSceneTimeline& timeline) noexcept;
} // namespace l3
