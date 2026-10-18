#include "legends3ds/render/area_inspection.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
template <typename Fn> void rejects(Fn&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
l3::RuntimeMap fixture() {
    l3::RuntimeMap map;
    map.areas.push_back({62, 62, 8, 8, 0, 64, -1024, -1024});
    map.cells.resize(64);
    map.cells[0] = map.cells[1] = 1;
    map.idx.resize(2);
    map.descriptors.resize(1);
    map.idx[1] = {0, 0, 0, 0, 0};
    return map;
}
l3::ExpandedRenderPrimitive quad(int32_t x0, int32_t x1, int32_t y,
                                 int32_t z0, int32_t z1) {
    l3::ExpandedRenderPrimitive p;
    p.world_x = {{x0, x1, x0, x1}};
    p.world_y = {{y, y, y, y}};
    p.world_z = {{z0, z0, z1, z1}};
    return p;
}
} // namespace

int main() {
    auto map = fixture();
    const auto original_cells = map.cells;
    l3::AreaInspectionGeometry geometry(2);
    geometry.add(quad(-256, 256, 0, 2048, 2560));
    assert(geometry.primitive_count() == 1 && geometry.bounds().valid);
    assert(geometry.storage_bytes() <= 2u * 48u);
    assert(geometry.bounds().min == (std::array<int32_t, 3>{{-256, 0, 2048}}));
    assert(geometry.bounds().max == (std::array<int32_t, 3>{{256, 0, 2560}}));
    // A boundary/default heading can admit no vertices even with valid submitted
    // geometry. Source coordinates plus a diagnostic inward heading repair that.
    assert(geometry.primitives_with_visible_vertex({0, 0, 0}, 0) == 0);
    assert(geometry.primitives_with_visible_vertex({0, 0, 0}, 2048) == 1);
    const auto inspection = l3::choose_area_inspection_entry(map, 0, geometry);
    assert(map.cells == original_cells);
    assert(inspection.valid && inspection.primitive_count == 1);
    assert(inspection.origin.x == 0 && inspection.origin.y == 0 && inspection.origin.z == 0);
    assert(inspection.candidate_source == l3::AreaInspectionCandidate::PositiveIdx);
    assert(inspection.primitives_with_visible_vertex == 1 && inspection.yaw != 0);
    assert(geometry.primitives_with_visible_vertex(inspection.origin, inspection.yaw) == 1);

    // Packed negative coordinates are renderer-origin candidates, not an inferred
    // retail spawn or physics surface. Positive aliases do not force grid origins.
    l3::AreaInspectionGeometry negative_geometry(1);
    negative_geometry.add(quad(1024, 1536, -128, 1024, 1536));
    map.idx[1] = {-768, 0, -768, 0, 0};
    map.cells[36] = static_cast<int16_t>(0x8100u);
    const auto negative = l3::choose_area_inspection_entry(map, 0, negative_geometry);
    assert(negative.valid && negative.candidate_source == l3::AreaInspectionCandidate::NegativeGrid);
    assert(negative.origin.x == 1280 && negative.origin.y == -128 && negative.origin.z == 1280);
    assert(l3::runtime_map_find_cell(map, 0, negative.origin.x, negative.origin.z));
    assert(negative.primitives_with_visible_vertex != 0);

    // Geometry admission is bounded; empty/unsupported source entries remain
    // explicit. A diagnostic view never fabricates a substitute position.
    geometry.add(quad(-512, 512, -256, 1536, 3072));
    rejects([&] { geometry.add(quad(0, 1, 0, 0, 1)); });
    l3::AreaInspectionGeometry zero_budget(0);
    rejects([&] { zero_budget.add(quad(0, 1, 0, 0, 1)); });
    l3::AreaInspectionGeometry empty(0);
    assert(!l3::choose_area_inspection_entry(map, 0, empty).valid);
    rejects([&] { l3::choose_area_inspection_entry(map, 1, empty); });
    map.cells[0] = 2;
    rejects([&] { l3::choose_area_inspection_entry(map, 0, geometry); });
    map = fixture(); map.idx[1].descriptor_index = 0xFFFFu;
    assert(!l3::choose_area_inspection_entry(map, 0, geometry).valid);
    map = fixture(); map.idx[1].x = 16000;
    assert(!l3::choose_area_inspection_entry(map, 0, geometry).valid);

    l3::AreaInspectionGeometry extremes(1);
    extremes.add(quad(std::numeric_limits<int32_t>::min(),
                      std::numeric_limits<int32_t>::max(), 0, -128, 128));
    assert(l3::choose_area_inspection_entry(fixture(), 0, extremes).valid);
    geometry.release();
    assert(geometry.primitive_count() == 0 && !geometry.bounds().valid);
    assert(geometry.storage_bytes() == 0);
    assert(geometry.primitives_with_visible_vertex({}, 0) == 0);
    assert(!l3::choose_area_inspection_entry(fixture(), 0, geometry).valid);
    std::cout << "area inspection: source candidates/frustum/bounds/budget/release/malformed cases PASS\n";
}
