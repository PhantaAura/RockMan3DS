#pragma once
// The opening's class-40 Mega Man face: two channels that copy UVs from a
// hidden frame part onto the visible face part (eyes: mesh part 17 -> 16,
// mouth: 19 -> 18) through 0x8002D320, timed by cue tables against the event
// timeline (0x8010C820) and stepped by looping scripts (0x8010C6B8).
#include "legends3ds/actor_resource.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
constexpr size_t kSourceFaceCueCapacity = 32, kSourceFaceScripts = 24, kSourceFaceScriptCapacity = 32;
struct SourceFaceCue { uint16_t timeline = 0, frame = 0, script = 0; };   // event+8 halfword, event+0x1C, index
struct SourceFaceCueTable { uint8_t count = 0; std::array<SourceFaceCue,kSourceFaceCueCapacity> cues{}; };
struct SourceFaceScript { uint8_t count = 0; std::array<std::array<uint8_t,4>,kSourceFaceScriptCapacity> entries{}; };
// Cooked into L3C0: cue tables 0x8011C338 (eyes) / 0x8011C3A4 (mouth) and
// script tables 0x8011C2A8 / 0x8011C2E0.
struct SourceSceneFaceProfile {
    bool loaded = false;
    std::array<SourceFaceCueTable,2> cues{};
    std::array<std::array<SourceFaceScript,kSourceFaceScripts>,2> scripts{};
};
bool parse_source_scene_face_profile(const uint8_t* bytes, size_t size, SourceSceneFaceProfile& out) noexcept;

struct SourceSceneFaceState {
    std::array<uint8_t,2> cue_position{};
    std::array<bool,2> running{};
    std::array<uint8_t,2> script{}, entry{};
    std::array<uint16_t,2> counter{};
};
struct SourceFaceUvCopy { uint8_t destination_part = 0, source_part = 0; uint32_t offset = 0; };
// 0x8002D404(frame, 42, 51, 6): grid column/row -> packed u|v<<8|u<<16|v<<24.
uint32_t source_face_offset_8002d404(uint32_t frame, uint32_t du, uint32_t dv, uint32_t per_row) noexcept;
// One original frame: 0x8010C820 (cue match for both channels, then
// 0x8010C6B8 script stepping). Returns the UV copies to apply, in order.
// A channel steps only after its first cue (the uncued cursors are BSS).
size_t step_source_scene_face_8010c820(SourceSceneFaceState& state, const SourceSceneFaceProfile& profile,
    uint16_t event_halfword_08, uint32_t event_word_1c, std::array<SourceFaceUvCopy,4>& out) noexcept;
// 0x8002D320 on a decoded draw slot: source UV words come from the EBD mesh
// part records (Near mesh = descriptor+0x70), each word + offset (32-bit add).
bool apply_source_face_uv_8002d320(PbdPart& destination, const uint8_t* ebd, size_t ebd_size, uint32_t load_base,
    uint32_t descriptor_token, uint8_t source_part, uint32_t offset) noexcept;
} // namespace l3
