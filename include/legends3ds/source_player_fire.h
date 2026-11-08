#pragma once
// Mega Buster firing chain of player work (USA ROCK_NEO.EXE):
//   fire input 0x80040B68 -> 0x80040E00 -> fire action 0x80040E9C
//   Buster lane 0x80042734 -> 0x80042F78 (+0x178 states, raises +0x174 bit 0x20)
//   overlay ("upper body") animation track 0x800321C8 / 0x800323FC, mirrored
//   from the main track by 0x8003C418 while the player is not firing
//   pose selector +0x106 used by 0x800325D4.
// Only the +0x171 (Buster) lane is ported. The +0x170 special-weapon lane
// (0x80042770 family, 0x80037440) and the buffered +0x176 writers are not.
#include "legends3ds/pbd_animation.h"
#include "legends3ds/player_runtime.h"
#include "legends3ds/source_buster_profile.h"
#include <cstdint>
#include <optional>

namespace l3 {

// 0x800423DC: left-arm request 0x60..0x63 from action +9 and stance +0x112.
uint8_t source_left_arm_request_800423dc(const L3PlayerCompatWork& work) noexcept;

// Context passed by the action handlers of table 0x80089FF8 (+9):
// 0,1,4,6,14 -> 0; 2 -> 1; 7,8 -> 2. Other actions never call 0x80040B68.
std::optional<uint8_t> source_fire_context_for_action(uint8_t action_09) noexcept;

// Fire input 0x80040B68, Buster (+0x171) branches only. pad_11c is the pad
// word the handler sees at +0x11C; +0x13A is the Buster button mask. Writes
// +0x174/+0x176/+0x177/+0x178/+0x104/+0x105/+0x100 and may request a main
// animation through 0x80041DDC. Returns the original return value.
// counter_171 is the Buster lane's +0x4FA + (+0x171)*8 counter (kept by the
// port in SourceBusterPlayer::lanes[1]).
bool source_fire_input_80040b68(L3PlayerCompatWork& work, uint8_t context, uint16_t pad_11c,
                                int16_t counter_171, const SourceBusterProfile& profile) noexcept;

// Overlay track (+0xF8/+0xFC pointers, +0x100..+0x103 current key, +0x104
// request, +0x105 started request, +0x106 selector). Keys are the S records of
// the bank resolved by request group; only 0x00..0x3F (main) and 0x60..0x7F
// (left arm, L00 or L04 by +0x171) are supported.
struct SourcePlayerAnimationBanks {
    const PbdAnimation* main = nullptr;
    const PbdAnimation* left_arm_00 = nullptr;   // PL00L00S/M (+0x171 == 0)
    const PbdAnimation* left_arm_04 = nullptr;   // PL00L04S/M (+0x171 != 0)
};
struct SourceOverlayTrack {
    uint8_t request = 0;
    uint8_t record = 0;
    bool valid = false;
};
// The S action and the M (pose) action for a request, as 0x800321C8 and
// 0x800329D4 resolve them. Main requests use M action request + 1.
struct SourceAnimationActions {
    const PbdAnimationAction* keys = nullptr;
    const PbdAnimationAction* poses = nullptr;
};
SourceAnimationActions source_player_animation_actions(const SourcePlayerAnimationBanks& banks,
    uint8_t request, uint8_t selection_171) noexcept;

// One 0x8003C418 overlay step (after the action handler). Returns false when
// the request's bank is not available (the track is then left unchanged).
bool source_overlay_update_8003c418(L3PlayerCompatWork& work, SourceOverlayTrack& track,
                                    const SourcePlayerAnimationBanks& banks) noexcept;

// Current overlay pose frame (60-byte frame; the decoder reads channels 0..20).
const std::array<uint16_t, kPbdPackedAngleChannels>* source_overlay_frame(
    const L3PlayerCompatWork& work, const SourceOverlayTrack& track,
    const SourcePlayerAnimationBanks& banks) noexcept;

// Buster lane 0x80042F78, run from 0x8003CA00 when (+0x174 & 0xC0) == 0x80.
// Returns true if it ran.
// Returns false (no change) when a needed table entry is missing.
bool source_buster_lane_80042f78(L3PlayerCompatWork& work, SourceOverlayTrack& overlay,
                                 const SourcePlayerAnimationBanks& banks, uint16_t pad_11c,
                                 int16_t counter_171, const SourceBusterProfile& profile) noexcept;

// 0x800325D4 part selection: +0x106 = 1 takes parts 0..4, 2 takes parts 0, 1,
// 5..7 and 3 takes parts 0..7 from the overlay bank; the root stays main.
PbdMainPoseBank source_merge_overlay_pose_800325d4(const PbdMainPoseBank& main,
                                                   const PbdMainPoseBank& overlay,
                                                   uint16_t selector_106) noexcept;

} // namespace l3
