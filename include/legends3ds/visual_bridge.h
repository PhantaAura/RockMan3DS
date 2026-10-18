#pragma once
#include "pbd_model.h"
#include "player_composition.h"
#include "player_pose.h"
#include "st00_render.h"
#include "st00_source.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

// Backend-neutral player draw primitive. Geometry remains local to its proven transform matrix so a
// renderer can either CPU-transform it or bind a matrix per part. Quads are deliberately preserved.
struct PlayerVisualPrimitive {
    uint8_t component_slot = 0;
    uint8_t part_id = 0;
    uint8_t transform_id = 0;
    uint8_t vertex_count = 0; // 3 or 4
    uint8_t primitive_code = 0;
    uint8_t r = 0, g = 0, b = 0;
    uint16_t clut = 0, tpage = 0;
    std::array<PbdVertex, 4> position{};
    std::array<std::array<uint8_t, 2>, 4> uv{};
};

struct PlayerVisualStats {
    uint32_t parts = 0;
    uint32_t primitives = 0;
    uint32_t triangles = 0;
    uint32_t quads = 0;
    uint32_t vertex_references = 0;
};

std::vector<PlayerVisualPrimitive> build_player_visuals(
    const std::array<PbdModel, kPlayerComponentSlots>& components,
    const PlayerComposition& composition,
    PlayerVisualStats* stats = nullptr);

// CPU-transformed draw primitive for backends that do not bind one matrix per player part.
// This is the bridge from decoded PBD animation/skeleton state to actual posed geometry.
struct PosedPlayerVisualPrimitive {
    uint8_t component_slot = 0;
    uint8_t part_id = 0;
    uint8_t transform_id = 0;
    uint8_t vertex_count = 0;
    uint8_t primitive_code = 0;
    uint8_t r = 0, g = 0, b = 0;
    uint16_t clut = 0, tpage = 0;
    std::array<PlayerPosePoint, 4> position{};
    std::array<std::array<uint8_t, 2>, 4> uv{};
};

std::vector<PosedPlayerVisualPrimitive> pose_player_visuals(
    const std::vector<PlayerVisualPrimitive>& local_visuals,
    const PlayerPoseMatrices& pose);

// Reuses caller-owned storage. Reserve/size this to local_visuals.size() at model load;
// valid subsequent frames of that same model perform no heap allocation. The output
// order, metadata and point math are identical to the value-returning bridge above.
void pose_player_visuals_into(
    const std::vector<PlayerVisualPrimitive>& local_visuals,
    const PlayerPoseMatrices& pose,
    std::vector<PosedPlayerVisualPrimitive>& output);

// Material identity is pose-independent. Compute once for immutable local geometry,
// in the same ascending unique order used by the native material draw loop.
std::vector<uint32_t> collect_player_visual_material_words(
    const std::vector<PlayerVisualPrimitive>& local_visuals);

// Final actor render transform, separate from the unscaled animation hierarchy.
// Supported ROCK_NEO.EXE uses GP+0x498 = 512 with Q12 ScaleMatrix and reduces
// posed translation offsets by 3 bits around the actor. In this float bridge,
// apply their common 1/8 scale to local posed points, preserving actor position.
// The root Y angle is raw work+0x56 minus raw work+0x116; the latter's gameplay
// role remains unknown. PS1 fixed-point coefficient/translation rounding is not
// reproduced here. Use only in the current UV path; preserve the 0143 fallback.
constexpr float kBaselinePlayerRenderScale = 512.0f / 4096.0f;
PlayerPoseMatrix build_player_actor_render_matrix(
    PlayerPosePoint actor_position,
    uint16_t actor_yaw,
    uint16_t source_work_0116 = 0);

struct St00AreaVisualStats {
    uint32_t positive_cells = 0;
    uint32_t near_cells = 0;
    uint32_t mid_cells = 0;
    uint32_t far_cells = 0;
    uint32_t near_replay_cells = 0;
    uint32_t primitives = 0;
};

// `transformed_depths` is parallel to the area's cell array and must contain the GTE/camera-space depth
// for each cell. This keeps camera math out of the format layer while applying the exact game LOD thresholds.
std::vector<ExpandedRenderPrimitive> build_st00_area_primary_visuals(
    const St00Source& stage,
    const St00RenderSource& render,
    size_t area_index,
    const std::vector<uint32_t>& transformed_depths,
    uint8_t global_scale_shift = 0,
    St00AreaVisualStats* stats = nullptr);

} // namespace l3
