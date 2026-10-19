#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/player_collision.h"
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>

namespace l3 {

// Original gameplay camera controller, USA 0x800A3958.. block. Field names
// keep their original addresses; only the ordinary player-follow path is
// ported. Verified against the R3000 instructions of 0x80038648, 0x800397E8,
// 0x80038894 (mode 0 path), 0x8003A154, 0x8003A218, 0x80037FB4 and 0x80049C54.
struct SourceGameplayCamera {
    bool initialized = false;
    std::array<int32_t,3> target_800a39a0{};   // look-at point
    int32_t distance_800a3964 = 0x200;
    int32_t yaw_800a3968 = 0;
    int32_t pitch_800a396c = 0x30;
    int32_t distance_goal_80097d2c = 0x200;
    int32_t pull_in_delay_80097d48 = 0x3c;
    int32_t yaw_rate_80097d28 = 0;
    uint32_t follow_frames_80098668 = 0;
    std::array<int32_t,3> eye_800a39f0{};      // signed16-narrowed eye
    std::array<int32_t,3> previous_eye_800a3a00{};
    // Native counters: frames where the eye-embedding test met geometry the
    // native port cannot classify (distance then holds; never a guess).
    uint32_t unsupported_eye_tests = 0;
};

// 0x80038648 + 0x800397E8: fresh controller at the player's XZ. The original
// also clears player +0x44D (0x800B55FD); the caller owns that store.
void source_gameplay_camera_init_80038648(SourceGameplayCamera& camera,
    const ReferenceTables& refs, int16_t player_x, int16_t player_z) noexcept;

// 0x80038894 ordinary follow (controller state 1, mode 0, settled blend,
// no script/first-person/shake). Reads player work +0x14/+0x18/+0x56/+0x44D.
void source_gameplay_camera_update_80038894(SourceGameplayCamera& camera,
    const ReferenceTables& refs, const L3PlayerCompatWork& player,
    const RuntimeMap& map, size_t area_index) noexcept;

// 0x80037FB4 eye: target - (d*sin(yaw)*cos(pitch), d*sin(pitch),
// d*cos(yaw)*cos(pitch)) with original Q12 truncation and signed16 narrowing.
std::array<int32_t,3> source_camera_eye_80037fb4(const ReferenceTables& refs,
    const std::array<int32_t,3>& target, int32_t distance, int32_t yaw, int32_t pitch) noexcept;

} // namespace l3
