#pragma once
#include "legends3ds/pbd_animation.h"
#include <array>
#include <cstdint>

namespace l3 {

// Backend-neutral row-major affine matrix. Translation lives in m[3], m[7], m[11].
struct PlayerPoseMatrix {
    std::array<float, 16> m{{
        1.f,0.f,0.f,0.f,
        0.f,1.f,0.f,0.f,
        0.f,0.f,1.f,0.f,
        0.f,0.f,0.f,1.f
    }};
};

struct PlayerPosePoint { float x=0.f, y=0.f, z=0.f; };

struct PlayerPoseMatrices {
    std::array<PlayerPoseMatrix, kPbdMainPosePartCount> local{};
    std::array<PlayerPoseMatrix, kPbdMainPosePartCount> world{};
};

// Converts the game's 0..4095 turn domain to radians.
float pbd_angle12_to_radians(uint16_t angle);

// Psy-Q RotMatrix-compatible axis order: X * Y * Z. The public PSn00bSDK implementation
// independently matches this order. Float math is intentional here; bit-exact PS1 Q12 rounding
// is not required by the 3DS renderer and remains a separate validation target.
PlayerPoseMatrix player_pose_rotation_xyz(uint16_t x, uint16_t y, uint16_t z);
PlayerPoseMatrix player_pose_mul(const PlayerPoseMatrix& a, const PlayerPoseMatrix& b);
PlayerPosePoint player_pose_transform_point(const PlayerPoseMatrix& m, PlayerPosePoint p);

// Structural player-pose bridge. Proven inputs: 15-part parent tree, channel-to-part mapping,
// action-0 base translation table, and X*Y*Z rotation order. Part 0 uses animated root translation;
// child parts use their action-0 base/local translation. This is suitable for the native renderer,
// while exact PS1 fixed-point rounding/overflow remains intentionally outside this float bridge.
PlayerPoseMatrices build_player_pose_matrices(const PbdMainSkeleton& skeleton,
                                               const PbdMainPoseBank& pose);

} // namespace l3
