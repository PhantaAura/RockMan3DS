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
    // Shake byte 0x800A395F, set by 0x800398E0 (positive arguments only).
    // The follow tail (0x80038FE4) lifts the target by it, then drops it by
    // 10 per frame. Not reset by the init (the original never clears it there).
    uint8_t shake_800a395f = 0;
    // 0x800A39B4: target height above the player. The init paths write 0x96;
    // a scripted mode (0xD2) is not ported.
    int32_t height_800a39b4 = 0x96;
    // Blend (0x80039FFC focus / 0x80039E50 return, played by 0x80038894 mode 0 at 0x800389C8):
    // counter 0x800A3962 (-1 idle; 0x8003A13C is busy while it is >= 0), step 0x800A3961,
    // mode 0x800A3959 (1 focus, 2 return), start 0x800A397C/80/84 + point 0x800A39C0,
    // end 0x800A3994/98/9C + point 0x800A39E0.
    int8_t blend_counter_800a3962 = -1;
    int8_t blend_step_800a3961 = -4;
    uint8_t blend_mode_800a3959 = 0;
    int32_t start_distance_800a397c = 0, start_yaw_800a3980 = 0, start_pitch_800a3984 = 0;
    int32_t end_distance_800a3994 = 0, end_yaw_800a3998 = 0, end_pitch_800a399c = 0;
    std::array<int32_t,3> start_point_800a39c0{}, end_point_800a39e0{};
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
// no script/first-person), with the 0x800A395F shake. Reads player work +0x14/+0x18/+0x56/+0x44D.
void source_gameplay_camera_update_80038894(SourceGameplayCamera& camera,
    const ReferenceTables& refs, const L3PlayerCompatWork& player,
    const RuntimeMap& map, size_t area_index) noexcept;

// 0x80039FFC: blend from the current view to (point, distance, yaw, pitch) in 16 updates.
void source_camera_focus_80039ffc(SourceGameplayCamera& camera, const std::array<int32_t,3>& point,
    int32_t distance, uint32_t yaw, int32_t pitch) noexcept;
// 0x80039E50: blend back to the follow view behind the player (distance 0x200, pitch 0x30,
// height 0x96, yaw = player +0x56 + 0x800). The caller clears player +0x44D (0x800B55FD).
void source_camera_return_80039e50(SourceGameplayCamera& camera, const L3PlayerCompatWork& player) noexcept;
// 0x8003A13C: 1 while a blend runs.
inline bool source_camera_busy_8003a13c(const SourceGameplayCamera& c) noexcept { return c.blend_counter_800a3962 >= 0; }
// 0x80038894 with a blend running (0x800389C8..0x80038B9C): event_active is 0x800C4C48 != 0
// (the end point follows the player only without an event). Returns false when no blend runs.
bool source_gameplay_camera_blend_80038894(SourceGameplayCamera& camera, const ReferenceTables& refs,
    const L3PlayerCompatWork& player, bool event_active) noexcept;

// 0x80037FB4 eye: target - (d*sin(yaw)*cos(pitch), d*sin(pitch),
// d*cos(yaw)*cos(pitch)) with original Q12 truncation and signed16 narrowing.
std::array<int32_t,3> source_camera_eye_80037fb4(const ReferenceTables& refs,
    const std::array<int32_t,3>& target, int32_t distance, int32_t yaw, int32_t pitch) noexcept;

} // namespace l3
