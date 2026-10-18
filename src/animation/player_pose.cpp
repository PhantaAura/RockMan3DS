#include "legends3ds/player_pose.h"
#include <cmath>
#include <stdexcept>

namespace l3 {
namespace {
constexpr float kTau = 6.283185307179586476925286766559f;

PlayerPoseMatrix translation(float x, float y, float z) {
    PlayerPoseMatrix r;
    r.m[3] = x; r.m[7] = y; r.m[11] = z;
    return r;
}

PlayerPoseMatrix rotation_x(float a) {
    PlayerPoseMatrix r; const float c=std::cos(a), s=std::sin(a);
    r.m[5]=c; r.m[6]=-s; r.m[9]=s; r.m[10]=c; return r;
}
PlayerPoseMatrix rotation_y(float a) {
    PlayerPoseMatrix r; const float c=std::cos(a), s=std::sin(a);
    r.m[0]=c; r.m[2]=s; r.m[8]=-s; r.m[10]=c; return r;
}
PlayerPoseMatrix rotation_z(float a) {
    PlayerPoseMatrix r; const float c=std::cos(a), s=std::sin(a);
    r.m[0]=c; r.m[1]=-s; r.m[4]=s; r.m[5]=c; return r;
}
}

float pbd_angle12_to_radians(uint16_t angle) {
    return float(angle & 0x0FFFu) * (kTau / 4096.0f);
}

PlayerPoseMatrix player_pose_mul(const PlayerPoseMatrix& a, const PlayerPoseMatrix& b) {
    PlayerPoseMatrix r;
    for (int row=0; row<4; ++row) {
        for (int col=0; col<4; ++col) {
            float v=0.f;
            for (int k=0; k<4; ++k) v += a.m[size_t(row)*4u+size_t(k)] * b.m[size_t(k)*4u+size_t(col)];
            r.m[size_t(row)*4u+size_t(col)] = v;
        }
    }
    return r;
}

PlayerPoseMatrix player_pose_rotation_xyz(uint16_t x, uint16_t y, uint16_t z) {
    const auto rx = rotation_x(pbd_angle12_to_radians(x));
    const auto ry = rotation_y(pbd_angle12_to_radians(y));
    const auto rz = rotation_z(pbd_angle12_to_radians(z));
    return player_pose_mul(player_pose_mul(rx, ry), rz);
}

PlayerPosePoint player_pose_transform_point(const PlayerPoseMatrix& m, PlayerPosePoint p) {
    return {
        m.m[0]*p.x + m.m[1]*p.y + m.m[2]*p.z + m.m[3],
        m.m[4]*p.x + m.m[5]*p.y + m.m[6]*p.z + m.m[7],
        m.m[8]*p.x + m.m[9]*p.y + m.m[10]*p.z + m.m[11]
    };
}

PlayerPoseMatrices build_player_pose_matrices(const PbdMainSkeleton& skeleton,
                                               const PbdMainPoseBank& pose) {
    PlayerPoseMatrices out;
    for (size_t part=0; part<kPbdMainPosePartCount; ++part) {
        const auto& a = pose.part_rotation[part];
        const auto rot = player_pose_rotation_xyz(a[0], a[1], a[2]);
        PlayerPoseMatrix tr;
        if (part == 0) {
            tr = translation(float(pose.root_translation[0]), float(pose.root_translation[1]), float(pose.root_translation[2]));
        } else {
            const auto& t = skeleton.base_translation[part];
            tr = translation(float(t[0]), float(t[1]), float(t[2]));
        }
        out.local[part] = player_pose_mul(tr, rot);
        const int parent = skeleton.parent[part];
        if (parent < 0) out.world[part] = out.local[part];
        else {
            if (parent >= int(part)) throw std::runtime_error("player skeleton parent order is not topological");
            out.world[part] = player_pose_mul(out.world[size_t(parent)], out.local[part]);
        }
    }
    return out;
}

} // namespace l3
