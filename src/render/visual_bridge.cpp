#include "legends3ds/visual_bridge.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {

std::vector<PlayerVisualPrimitive> build_player_visuals(
    const std::array<PbdModel, kPlayerComponentSlots>& components,
    const PlayerComposition& composition,
    PlayerVisualStats* stats_out) {
    std::vector<PlayerVisualPrimitive> out;
    PlayerVisualStats stats;
    stats.parts = static_cast<uint32_t>(composition.attachments.size());
    for (const auto& a : composition.attachments) {
        if (a.component_slot >= components.size()) throw std::runtime_error("player visual component slot out of range");
        const auto& model = components[a.component_slot];
        if (a.mesh_index >= model.parts.size()) throw std::runtime_error("player visual mesh index out of range");
        const auto& part = model.parts[a.mesh_index];
        if (part.part_id != a.part_id) throw std::runtime_error("player composition part ID no longer matches model");
        for (const auto& tri : part.triangles) {
            PlayerVisualPrimitive p;
            p.component_slot = a.component_slot; p.part_id = a.part_id; p.transform_id = a.transform_id;
            p.vertex_count = 3; p.primitive_code = tri.code; p.r = tri.r; p.g = tri.g; p.b = tri.b; p.clut = tri.clut; p.tpage = tri.tpage;
            for (size_t i = 0; i < 3; ++i) {
                if (tri.index[i] >= part.vertices.size()) throw std::runtime_error("player triangle index out of range during visual bridge");
                p.position[i] = part.vertices[tri.index[i]];
                p.uv[i] = {tri.uv[i * 2], tri.uv[i * 2 + 1]};
            }
            out.push_back(p); ++stats.primitives; ++stats.triangles; stats.vertex_references += 3;
        }
        for (const auto& quad : part.quads) {
            PlayerVisualPrimitive p;
            p.component_slot = a.component_slot; p.part_id = a.part_id; p.transform_id = a.transform_id;
            p.vertex_count = 4; p.primitive_code = quad.code; p.r = quad.r; p.g = quad.g; p.b = quad.b; p.clut = quad.clut; p.tpage = quad.tpage;
            for (size_t i = 0; i < 4; ++i) {
                if (quad.index[i] >= part.vertices.size()) throw std::runtime_error("player quad index out of range during visual bridge");
                p.position[i] = part.vertices[quad.index[i]];
                p.uv[i] = {quad.uv[i * 2], quad.uv[i * 2 + 1]};
            }
            out.push_back(p); ++stats.primitives; ++stats.quads; stats.vertex_references += 4;
        }
    }
    if (stats_out) *stats_out = stats;
    return out;
}

std::vector<PosedPlayerVisualPrimitive> pose_player_visuals(
    const std::vector<PlayerVisualPrimitive>& local_visuals,
    const PlayerPoseMatrices& pose) {
    std::vector<PosedPlayerVisualPrimitive> out;
    pose_player_visuals_into(local_visuals, pose, out);
    return out;
}

void pose_player_visuals_into(
    const std::vector<PlayerVisualPrimitive>& local_visuals,
    const PlayerPoseMatrices& pose,
    std::vector<PosedPlayerVisualPrimitive>& out) {
    // Validate before mutating reusable output so malformed geometry cannot leave
    // a partially posed frame. Immutable models need only the bounded scan here.
    for (const auto& src : local_visuals) {
        if (src.transform_id >= pose.world.size()) throw std::runtime_error("player visual transform ID out of range");
        if (src.vertex_count != 3 && src.vertex_count != 4) throw std::runtime_error("player visual primitive has invalid vertex count");
    }
    out.resize(local_visuals.size());
    for (size_t primitive = 0; primitive < local_visuals.size(); ++primitive) {
        const auto& src = local_visuals[primitive];
        PosedPlayerVisualPrimitive dst;
        dst.component_slot = src.component_slot; dst.part_id = src.part_id; dst.transform_id = src.transform_id;
        dst.vertex_count = src.vertex_count; dst.primitive_code = src.primitive_code;
        dst.r = src.r; dst.g = src.g; dst.b = src.b; dst.clut = src.clut; dst.tpage = src.tpage;
        dst.uv = src.uv;
        for (size_t i = 0; i < src.vertex_count; ++i) {
            const auto& v = src.position[i];
            dst.position[i] = player_pose_transform_point(pose.world[src.transform_id],
                                                          {float(v.x), float(v.y), float(v.z)});
        }
        out[primitive] = dst;
    }
}

std::vector<uint32_t> collect_player_visual_material_words(
    const std::vector<PlayerVisualPrimitive>& local_visuals) {
    std::vector<uint32_t> words;
    words.reserve(local_visuals.size());
    for (const auto& p : local_visuals)
        words.push_back((uint32_t(p.clut) << 16u) | uint32_t(p.tpage));
    std::sort(words.begin(), words.end());
    words.erase(std::unique(words.begin(), words.end()), words.end());
    return words;
}

PlayerPoseMatrix build_player_actor_render_matrix(
    PlayerPosePoint actor_position,
    uint16_t actor_yaw,
    uint16_t source_work_0116) {
    // 0x80032648..0x80032660 passes this subtraction to RotMatrix. The
    // original Y-only matrix has +sin at m02 and -sin at m20, matching the
    // shared pose rotation, rather than the inverse yaw used by the old UV loop.
    const auto yaw = static_cast<uint16_t>((uint32_t(actor_yaw) - source_work_0116) & 0x0FFFu);
    auto matrix = player_pose_rotation_xyz(0, yaw, 0);
    for (size_t row=0; row<3; ++row)
        for (size_t col=0; col<3; ++col)
            matrix.m[row*4+col] *= kBaselinePlayerRenderScale;
    matrix.m[3]=actor_position.x;
    matrix.m[7]=actor_position.y;
    matrix.m[11]=actor_position.z;
    return matrix;
}

std::vector<ExpandedRenderPrimitive> build_st00_area_primary_visuals(
    const St00Source& stage,
    const St00RenderSource& render,
    size_t area_index,
    const std::vector<uint32_t>& depths,
    uint8_t global_scale_shift,
    St00AreaVisualStats* stats_out) {
    if (area_index >= stage.areas.size()) throw std::runtime_error("ST00 area index out of range");
    const auto& area = stage.areas[area_index];
    if (depths.size() != area.cells.size()) throw std::runtime_error("ST00 area depth array size mismatch");
    if (area.width == 0) throw std::runtime_error("ST00 area width is zero");
    St00AreaVisualStats stats;
    std::vector<ExpandedRenderPrimitive> out;
    for (size_t ci = 0; ci < area.cells.size(); ++ci) {
        const int16_t packed = area.cells[ci];
        if (packed <= 0) continue;
        const size_t idx_index = static_cast<uint16_t>(packed);
        if (idx_index >= stage.idx.size()) throw std::runtime_error("ST00 positive cell IDX index out of range in visual bridge");
        const uint32_t dp = stage.idx[idx_index].descriptor_ptr;
        size_t descriptor_index = stage.descriptors.size();
        for (size_t di = 0; di < stage.descriptors.size(); ++di) {
            if (stage.descriptors[di].source_ptr == dp) { descriptor_index = di; break; }
        }
        if (descriptor_index >= stage.descriptors.size() || descriptor_index >= render.descriptors.size()) {
            throw std::runtime_error("ST00 cell descriptor pointer is not present in HED descriptor table");
        }
        const uint8_t gx = static_cast<uint8_t>(area.start_x + (ci % area.width));
        const uint8_t gz = static_cast<uint8_t>(area.start_z + (ci / area.width));
        const auto origin = st00_cell_origin(gx, gz, packed, stage);
        const auto sel = select_st00_primary_lod(depths[ci]);
        if (sel.lod == St00Lod::Near) ++stats.near_cells;
        else if (sel.lod == St00Lod::Mid) ++stats.mid_cells;
        else ++stats.far_cells;
        if (sel.near_replay_path) ++stats.near_replay_cells;
        ++stats.positive_cells;
        auto expanded = expand_st00_descriptor_lod(render, descriptor_index, St00RenderBank::Primary, sel.lod,
                                                   origin, static_cast<uint16_t>(packed), global_scale_shift);
        stats.primitives += static_cast<uint32_t>(expanded.size());
        out.insert(out.end(), expanded.begin(), expanded.end());
    }
    if (stats_out) *stats_out = stats;
    return out;
}

} // namespace l3
