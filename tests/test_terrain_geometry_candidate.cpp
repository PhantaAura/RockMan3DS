#include "legends3ds/render/terrain_geometry_candidate.h"
#include <cassert>
#include <limits>
#include <stdexcept>

namespace {
l3::SourceRenderPrimitive retained(const std::array<int8_t, 12>& raw) {
    l3::SourceRenderPrimitive p;
    p.local_x = {raw[0], raw[3], raw[5], raw[10]};
    p.local_z = {raw[1], raw[6], raw[8], raw[11]};
    p.ignored_raw = {raw[2], raw[4], raw[7], raw[9]};
    p.prefix = 0x12808080u;
    p.uv = {3, 5, 7, 11, 13, 17, 19, 23};
    return p;
}
template <typename Fn> void rejects(Fn&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    // Every byte has a distinct role, so a cross-gather or silent UV reorder is
    // detected. Signed extremes also catch unsigned coordinate reconstruction.
    const auto p = retained({-128, -7, 127, -3, 11, -17, 19, -23, 29, -31, 37, -41});
    const auto xyz = l3::terrain_candidate_consecutive_xyz(p);
    assert((xyz.x == std::array<int8_t, 4>{-128, -3, 19, -31}));
    assert((xyz.y == std::array<int8_t, 4>{-7, 11, -23, 37}));
    assert((xyz.z == std::array<int8_t, 4>{127, -17, 29, -41}));
    const l3::St00CellOrigin origin{100, -200, 300};
    for (uint16_t rotation = 0; rotation < 4; ++rotation) {
        const auto expanded = l3::expand_terrain_candidate_primitive(p, 0x12345678u,
            origin, rotation, 2);
        assert(expanded.prefix == p.prefix && expanded.material_word == 0x12345678u);
        assert(expanded.uv == p.uv);
        for (size_t v = 0; v < 4; ++v) {
            const int32_t x = int32_t(xyz.x[v]) * 4, z = int32_t(xyz.z[v]) * 4;
            const int32_t rx = rotation == 0 ? x : rotation == 1 ? z : rotation == 2 ? -x : -z;
            const int32_t rz = rotation == 0 ? z : rotation == 1 ? -x : rotation == 2 ? -z : x;
            assert(expanded.world_x[v] == origin.x + rx);
            assert(expanded.world_y[v] == origin.y + int32_t(xyz.y[v]) * 4);
            assert(expanded.world_z[v] == origin.z + rz);
        }
    }
    assert((l3::kTerrainCandidateStripIndices == std::array<uint8_t, 6>{0, 1, 2, 1, 3, 2}));
    // A vertical synthetic wall must keep all four world points. The former
    // flattening creates an unrelated plane and cannot represent this fixture.
    const auto wall = retained({-4, -2, 0, 4, -2, 0, -4, 3, 0, 4, 3, 0});
    const auto expanded_wall = l3::expand_terrain_candidate_primitive(wall, 0, origin, 0, 3);
    assert((expanded_wall.world_x == std::array<int32_t, 4>{68, 132, 68, 132}));
    assert((expanded_wall.world_y == std::array<int32_t, 4>{-216, -216, -176, -176}));
    assert((expanded_wall.world_z == std::array<int32_t, 4>{300, 300, 300, 300}));
    rejects([&] { l3::expand_terrain_candidate_primitive(p, 0, origin, 0, 16); });
    rejects([&] { l3::expand_terrain_candidate_primitive(p, 0,
        {0, std::numeric_limits<int32_t>::max(), 0}, 0, 0); });
    rejects([&] { l3::expand_terrain_candidate_primitive(p, 0,
        {0, std::numeric_limits<int32_t>::min(), 0}, 0, 0); });

    l3::St00RenderSource source;
    source.descriptors.resize(1);
    source.descriptors[0].record_indices[1] = 0;
    l3::SourceRenderRecord record;
    record.materials = {0x12345678u}; record.stream_indices = {0};
    source.records.push_back(record);
    l3::SourceRenderStream stream;
    stream.base_shift = 2; stream.primitives = {p};
    source.streams.push_back(stream);
    const auto recovered = l3::expand_terrain_candidate_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0, 0);
    const auto baseline_before = l3::expand_st00_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0, 0);
    assert(recovered.size() == 1 && baseline_before.size() == 1);
    assert(recovered[0].uv == baseline_before[0].uv);
    assert(recovered[0].world_y[0] != baseline_before[0].world_y[0]);
    const auto baseline_after = l3::expand_st00_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0, 0);
    assert(baseline_before[0].world_x == baseline_after[0].world_x);
    assert(baseline_before[0].world_y == baseline_after[0].world_y);
    assert(baseline_before[0].world_z == baseline_after[0].world_z);
    assert(l3::expand_terrain_candidate_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Near, origin, 0).empty());
    rejects([&] { l3::expand_terrain_candidate_descriptor_lod(source, 1,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0); });
    rejects([&] { l3::expand_terrain_candidate_descriptor_lod(source, 0,
        static_cast<l3::St00RenderBank>(255), static_cast<l3::St00Lod>(3), origin, 0); });
    source.streams[0].base_shift = 15;
    rejects([&] { l3::expand_terrain_candidate_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0, 1); });
    source.streams[0].base_shift = 0; source.streams[0].material_slot = 1;
    rejects([&] { l3::expand_terrain_candidate_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0); });
    source.streams[0].material_slot = 0; source.records[0].stream_indices = {1};
    rejects([&] { l3::expand_terrain_candidate_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0); });
    source.descriptors[0].record_indices[1] = 1;
    rejects([&] { l3::expand_terrain_candidate_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, 0); });
}
