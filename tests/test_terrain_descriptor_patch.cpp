// Native terrain patch for IDX descriptor swaps (door object 0x8003AC24):
// the baked spans of a swapped identity collapse, return byte-exact on the
// restore, and the plan lists the live descriptors to draw as an overlay.
// Synthetic map and vertices only.
#include "legends3ds/render/terrain_descriptor_patch.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace l3;
namespace {
struct V { float x, y, z, u, v; };

RuntimeMap map3() {
    RuntimeMap m;
    m.idx.resize(4);
    m.idx[1].descriptor_index = 0x1005;
    m.idx[2].descriptor_index = 7;
    m.idx[3].descriptor_index = 0xFFFFu;
    return m;
}
} // namespace

int main() {
    auto live = map3();
    TerrainDescriptorPatch patch;
    // Bake: buffer 10 holds idx 1 (6 vertices, appended in two pieces) then
    // idx 2; buffer 11 holds more of idx 1. idx 3 is baked without geometry.
    std::vector<V> b10(12), b11(6);
    for (size_t i = 0; i < b10.size(); ++i) b10[i] = {float(i), float(i) + 1, float(i) + 2, 0.5f, 0.25f};
    for (size_t i = 0; i < b11.size(); ++i) b11[i] = {float(100 + i), 0, 0, 0, 0};
    patch.bake(1, 0x1005); patch.add_span(1, 10, 0, 3); patch.add_span(1, 10, 3, 3);
    patch.bake(2, 7); patch.add_span(2, 10, 6, 6);
    patch.bake(1, 0x9999);   // alias of idx 1: first bake wins
    patch.add_span(1, 11, 0, 6);
    patch.bake(3, 0xFFFFu);
    assert(patch.baked_count() == 3 && patch.span_count() == 3);

    // Nothing swapped: empty plan.
    auto plan = patch.plan(live);
    assert(plan.empty() && plan.overlay.empty());

    auto resolve = [&](uint64_t key) -> uint8_t* {
        if (key == 10) return reinterpret_cast<uint8_t*>(b10.data());
        if (key == 11) return reinterpret_cast<uint8_t*>(b11.data());
        return nullptr;
    };
    size_t written_bytes = 0;
    auto written = [&](uint8_t*, size_t n) { written_bytes += n; };
    const auto before10 = b10; const auto before11 = b11;

    // Door opens: word0 & ~0x1000.
    live.idx[1].descriptor_index = 0x0005;
    plan = patch.plan(live);
    assert(plan.hide == std::vector<uint16_t>{1} && plan.restore.empty());
    assert(plan.overlay == std::vector<uint16_t>{1});
    assert(patch.apply(plan, sizeof(V), resolve, written) == 2);
    assert(patch.hidden(1) && !patch.hidden(2));
    assert(written_bytes == 12 * sizeof(V) && patch.saved_bytes() == 12 * sizeof(V));
    for (size_t i = 0; i < 6; ++i) assert(b10[i].x == 0.f && b10[i].y == 1.f && b10[i].u == 0.5f);
    for (size_t i = 0; i < 6; ++i) assert(b11[i].x == 100.f);
    for (size_t i = 6; i < 12; ++i) assert(b10[i].x == before10[i].x);   // idx 2 untouched

    // Re-planning while open keeps the overlay but does not collapse again.
    plan = patch.plan(live);
    assert(plan.empty() && plan.overlay == std::vector<uint16_t>{1});

    // A second identity swapping from "no descriptor" only needs an overlay.
    live.idx[3].descriptor_index = 2;
    plan = patch.plan(live);
    assert(plan.hide == std::vector<uint16_t>{3} && plan.overlay.size() == 2);
    written_bytes = 0;
    assert(patch.apply(plan, sizeof(V), resolve, written) == 0 && written_bytes == 0);
    assert(patch.hidden(3));

    // Door closes (state 5 writes the saved word back): byte-exact restore.
    live.idx[1].descriptor_index = 0x1005;
    live.idx[3].descriptor_index = 0xFFFFu;
    plan = patch.plan(live);
    assert(plan.hide.empty() && plan.restore.size() == 2 && plan.overlay.empty());
    assert(patch.apply(plan, sizeof(V), resolve, written) == 2);
    assert(!patch.hidden(1) && !patch.hidden(3) && patch.saved_bytes() == 0);
    for (size_t i = 0; i < b10.size(); ++i)
        assert(b10[i].x == before10[i].x && b10[i].y == before10[i].y && b10[i].z == before10[i].z
               && b10[i].u == before10[i].u && b10[i].v == before10[i].v);
    for (size_t i = 0; i < b11.size(); ++i) assert(b11[i].x == before11[i].x);

    // A live map shorter than the bake is treated as unswapped.
    RuntimeMap empty;
    assert(patch.plan(empty).empty());
    patch.reset();
    assert(patch.baked_count() == 0 && patch.span_count() == 0);
    std::puts("terrain_descriptor_patch: PASS");
    return 0;
}
