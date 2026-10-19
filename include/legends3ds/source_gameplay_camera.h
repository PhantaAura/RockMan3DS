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
    // Matrix half of 0x80037FB4: the eight words copied to 0x1F800008 and the
    // rotation stored at 0x800A3A10. Valid once init has run.
    std::array<uint32_t,8> camera_1f800008{};
    std::array<int16_t,3> rotation_800a3a10{};
    // Scratch shift 0x1F800074 the producer reads. The caller owns it.
    uint8_t scratch_shift_1f800074 = 0;
    // Halfword 0x1F8000F2 sits between the transposed coefficients and the
    // translation, so 0x80037FB4 copies whatever an earlier scratch user left
    // there into the high half of word 4. Its live value is UNKNOWN: it is
    // retained here (initially 0) and never reaches the GTE, because CTC2 R4
    // keeps only the low half (RT33) and libgte MATRIX users skip the pad.
    uint16_t retained_pad_1f8000f2 = 0;
    // Frames that used the packed RotMatrix table for rsin/rcos because the
    // scalar quarter table was not loaded (L3RF v1). Not source behaviour.
    uint32_t packed_trig_substitutions = 0;
};

struct SourceGameplayCameraFrame {
    std::array<uint32_t,8> camera_1f800008{};
    std::array<int16_t,3> rotation_800a3a10{};
    std::array<int32_t,3> eye_800a39f0{};
    bool packed_trig_substituted = false;
};
// Whole persistent output of 0x80037FB4: RotMatrixYXZ 0x8007B838 on
// [(-pitch)&FFF, yaw&FFF, 0], TransposeMatrix 0x8007B568, the translation
// (-t<<shift + (d<<shift)*trig) through ApplyMatrixLV 0x8007A9B8, the
// 32-byte copy to 0x1F800008, then the narrowed eye.
SourceGameplayCameraFrame source_gameplay_camera_80037fb4(const ReferenceTables& refs,
    const std::array<int32_t,3>& target, int32_t distance, int32_t yaw, int32_t pitch,
    uint8_t scratch_shift_1f800074, uint16_t retained_pad_1f8000f2) noexcept;

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
