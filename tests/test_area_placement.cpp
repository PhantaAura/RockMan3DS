#include "legends3ds/render/area_placement.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
template <typename Fn> void rejects(Fn&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
l3::RuntimeMap fixture() {
    l3::RuntimeMap map;
    map.areas.push_back({63, 62, 4, 2, 0, 8, 0, 0});
    // Identity order differs from grid scan order. The same identity may appear
    // at unrelated cells; distinct identities with equal descriptors are retained.
    map.cells = {7, 2, -32768, 7, 3, 2, 0, 3};
    map.idx.resize(8);
    map.descriptors.resize(2);
    map.idx[2] = {-1024, -640, 256, 0, 0};
    map.idx[3] = {2048, -128, -768, 1, 0};
    map.idx[7] = {-32768, 32767, 32767, -1, 1};
    return map;
}
l3::St00RenderSource render_fixture() {
    l3::St00RenderSource render;
    render.descriptors.resize(2);
    render.descriptors[0].record_indices[1] = 0;
    render.descriptors[0].record_indices[4] = 1;
    render.descriptors[1].record_indices[1] = 2;
    for (size_t i = 0; i < 3; ++i) {
        l3::SourceRenderRecord record;
        record.materials = {static_cast<uint32_t>(i)};
        record.stream_indices = {static_cast<uint32_t>(i)};
        render.records.push_back(record);
        l3::SourceRenderStream stream;
        stream.base_shift = static_cast<uint8_t>(i + 3);
        stream.primitives.resize(i + 1);
        render.streams.push_back(stream);
    }
    return render;
}
} // namespace

int main() {
    auto map = fixture();
    const auto original_cells = map.cells;
    auto placement = l3::collect_area_positive_placement(map, 0, 3);
    assert(map.cells == original_cells);
    assert(placement.instances.size() == 3);
    assert(placement.positive_occurrences == 6 && placement.duplicate_occurrences == 3);
    assert(placement.negative_cells == 1 && placement.zero_cells == 1);
    assert(placement.bitmap_bytes == 1);
    assert(placement.instances[0].idx_index == 2 && placement.instances[1].idx_index == 3
           && placement.instances[2].idx_index == 7);
    const auto& first = placement.instances[0];
    assert(first.origin.x == map.idx[2].x && first.origin.y == map.idx[2].y
           && first.origin.z == map.idx[2].z);
    assert(first.bank == l3::St00RenderBank::Secondary);
    assert(placement.instances[1].bank == l3::St00RenderBank::Primary);
    assert(placement.instances[2].bank == l3::St00RenderBank::Primary);
    // Changing the alias grid's location/order leaves each positive instance
    // invariant. IDX low bits are identity only; no rotation field is inferred.
    map.areas[0].start_x = -500;
    map.areas[0].start_z = 900;
    map.cells = {3, 7, 7, 2, 0, 3, -32768, 2};
    auto shifted = l3::collect_area_positive_placement(map, 0);
    for (size_t i = 0; i < shifted.instances.size(); ++i) {
        assert(shifted.instances[i].idx_index == placement.instances[i].idx_index);
        assert(shifted.instances[i].origin.x == placement.instances[i].origin.x);
        assert(shifted.instances[i].origin.y == placement.instances[i].origin.y);
        assert(shifted.instances[i].origin.z == placement.instances[i].origin.z);
        assert(shifted.instances[i].bank == placement.instances[i].bank);
    }
    rejects([&] { l3::collect_area_positive_placement(map, 1); });
    rejects([&] { l3::collect_area_positive_placement(map, 0, 2); });
    rejects([&] { l3::collect_area_positive_placement(map, 0, 0); });
    map.areas[0].width = 3;
    rejects([&] { l3::collect_area_positive_placement(map, 0); });
    map.areas[0].width = 4; map.areas[0].cell_first = 1;
    rejects([&] { l3::collect_area_positive_placement(map, 0); });
    map.areas[0].cell_first = 0; map.cells[0] = 8;
    rejects([&] { l3::collect_area_positive_placement(map, 0); });
    map = fixture(); map.idx[2].descriptor_index = 2;
    rejects([&] { l3::collect_area_positive_placement(map, 0); });
    map.idx[2].descriptor_index = 0xFFFFu;
    assert(l3::collect_area_positive_placement(map, 0).instances[0].descriptor_index == 0xFFFFu);

    l3::RuntimeMap large;
    large.idx.resize(65536); large.descriptors.resize(1);
    large.idx[0x7FFF].descriptor_index = 0;
    large.cells = {0x7FFF}; large.areas.push_back({0, 0, 1, 1, 0, 1, 0, 0});
    auto last = l3::collect_area_positive_placement(large, 0);
    assert(last.bitmap_bytes == 4096 && last.instances.size() == 1);
    assert(last.instances[0].idx_index == 0x7FFF);
    l3::RuntimeMap empty;
    empty.areas.push_back({});
    assert(l3::collect_area_positive_placement(empty, 0, 0).instances.empty());

    auto render = render_fixture();
    assert(l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 6) == 6);
    assert(l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Near, 0) == 0);
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 5); });
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod(3), 10); });
    placement.instances[0].bank = l3::St00RenderBank(2);
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 10); });
    placement = l3::collect_area_positive_placement(fixture(), 0);
    placement.instances[0].descriptor_index = 2;
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 10); });
    placement.instances[0].descriptor_index = 0xFFFFu;
    assert(l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 4) == 4);
    placement = l3::collect_area_positive_placement(fixture(), 0);
    render.descriptors[0].record_indices[4] = 3;
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 10); });
    render = render_fixture(); render.records[1].stream_indices = {3};
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 10); });
    render = render_fixture(); render.streams[1].material_slot = 1;
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 10); });
    render = render_fixture(); render.streams[1].base_shift = 16;
    rejects([&] { l3::count_area_placed_render_primitives(render, placement, l3::St00Lod::Mid, 10); });
    std::cout << "area placement: identity/order/origin/bank/budget/malformed cases PASS\n";
}
