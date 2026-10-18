#pragma once
#include <string>

namespace l3 {
// Source-backed initial gameplay baseline, corroborated in owner PS1 emulation.
// H=384, radial=512, pitch=48, target offset=150, global shift=0.
// At actor yaw zero: eye relative [0,-187,+511], target [0,-150,0].
// Dynamic terms, scripted ownership/handoff and float vs Q12 rounding are partial.
float frontend_vertical_fov_from_focal_pixels(float focal_pixels) noexcept;
struct FrontendViewSettings {
    float vertical_fov_degrees = frontend_vertical_fov_from_focal_pixels(384.0f);
    float follow_distance = 511.0f / 512.0f;
    float eye_height = 187.0f / 512.0f;
    float target_height = 150.0f / 512.0f;
};
inline constexpr float kFrontendWorldScale = 1.0f / 512.0f;
inline constexpr float kFrontendAspect = 400.0f / 240.0f;
inline constexpr float kFrontendNear = 0.05f;
inline constexpr float kFrontendFar = 180.0f;
struct FrontendPoint { float x=0, y=0, z=0; };
struct FrontendCameraFrame { FrontendPoint eye{}, target{}; };
struct FrontendProjection {
    bool valid=false, visible=false;
    float x_pixels=0, y_pixels=0, depth=0;
};
FrontendViewSettings legacy_frontend_view() noexcept;
bool valid_frontend_view(const FrontendViewSettings& settings) noexcept;
// Strict optional owner-local calibration config. Reject unknown/duplicate keys,
// non-finite values and unreasonable diagnostic ranges; default is unchanged.
FrontendViewSettings parse_frontend_view_config(const std::string& text);
FrontendPoint frontend_world_point(float x,float y,float z) noexcept;
FrontendCameraFrame frontend_follow_camera(FrontendPoint player,unsigned yaw,
    const FrontendViewSettings& settings={}) noexcept;
// Projection in the real 400x240 viewport; visibility is point admission only.
FrontendProjection project_frontend_point(FrontendPoint point,
    const FrontendCameraFrame& camera,const FrontendViewSettings& settings={}) noexcept;
}
