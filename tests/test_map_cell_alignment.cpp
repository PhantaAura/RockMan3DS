#include "legends3ds/l3map.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>

namespace {
// Synthetic identities cover every packed quarter-turn value. Contact lists
// remain authored data; this test does not invent a runtime rotation rule.
l3::RuntimeMap make_map(int16_t sx, int16_t sz, uint16_t width, uint16_t height) {
    l3::RuntimeMap map;
    l3::RuntimeMapArea area{};
    area.start_x = sx; area.start_z = sz;
    area.width = width; area.height = height;
    area.cell_count = uint32_t(width) * height;
    area.world_origin_x = int32_t(sx) * 512 - 0x7F00;
    area.world_origin_z = int32_t(sz) * 512 - 0x7F00;
    map.areas.push_back(area);
    map.idx.resize(size_t(area.cell_count) + 1);
    for (uint32_t i = 0; i < area.cell_count; ++i) {
        map.cells.push_back(static_cast<int16_t>(i + 1));
        map.idx[i + 1].descriptor_index = static_cast<uint16_t>(i);
        map.idx[i + 1].raw6 = (i & 1u) ? 1 : 0;
        l3::RuntimeMapDescriptor desc{};
        desc.default_contact_index = static_cast<uint16_t>(i * 2);
        desc.alt_contact_index = static_cast<uint16_t>(i * 2 + 1);
        map.descriptors.push_back(desc);
        // Distinct default/alternate groups make wrong IDX/list selection fail.
        for (uint32_t branch = 0; branch < 2; ++branch) {
            const uint32_t gi = uint32_t(map.contact_groups.size());
            map.contact_lists.push_back({gi, 1});
            l3::RuntimeContactGroup g{};
            g.x0 = -256; g.x1 = 256;
            g.z0 = -256; g.z1 = 256;
            g.primitive_first = uint32_t(map.contact_primitives.size());
            g.primitive_count = 1;
            map.contact_groups.push_back(g);
            map.contact_primitives.push_back({static_cast<int16_t>(i),
                static_cast<int16_t>(branch), static_cast<uint16_t>(0x10u + branch), 0});
        }
    }
    return map;
}

void check_cell(const l3::RuntimeMap& map, int32_t x, int32_t z,
                uint16_t expected_gx, uint16_t expected_gz) {
    const auto cell = l3::runtime_map_find_cell(map, 0, x, z);
    assert(cell && cell->grid_x == expected_gx && cell->grid_z == expected_gz);
    assert(cell->world_origin_x == int32_t(expected_gx) * 512 - 0x7F00);
    assert(cell->world_origin_z == int32_t(expected_gz) * 512 - 0x7F00);
    // A query in signed-halfword range must select the retail gather's first
    // containing-cell sample, regardless of the stored quarter-turn value.
    const auto retail = l3::runtime_map_collect_contact_candidates_8004b1e0(
        map, 0, static_cast<int16_t>(x), static_cast<int16_t>(z), true);
    assert(!retail.empty());
    const auto& containing = retail.front();
    assert(containing.cell_index == cell->cell_index);
    assert(containing.packed_cell == cell->packed_cell);
    const auto actual_groups = l3::runtime_map_cell_contact_groups(map, *cell);
    const auto retail_groups = l3::runtime_map_cell_contact_groups(map, containing);
    assert(actual_groups.size() == 1 && retail_groups.size() == 1);
    const auto& actual = actual_groups.front();
    const auto& reference = retail_groups.front();
    assert(actual.contact_list_index == reference.contact_list_index);
    assert(actual.group_index == reference.group_index);
    assert(actual.world_x0 == reference.world_x0 && actual.world_x1 == reference.world_x1);
    assert(actual.world_z0 == reference.world_z0 && actual.world_z1 == reference.world_z1);
    assert(actual.world_x0 == int32_t(expected_gx) * 512 - 0x8000);
    assert(actual.world_x1 == int32_t(expected_gx + 1) * 512 - 0x8000);
    assert(actual.world_z0 == int32_t(expected_gz) * 512 - 0x8000);
    assert(actual.world_z1 == int32_t(expected_gz + 1) * 512 - 0x8000);
    assert(actual.primitives.front().primitive.y0 == int16_t(cell->cell_index));
    assert(actual.primitives.front().primitive.y1 == int16_t(cell->cell_index & 1u));
}

void centered_boundaries_and_quarter_turn_contacts() {
    const auto map = make_map(62, 62, 4, 4);
    std::array<bool, 4> rotations_seen{};
    for (uint16_t gx = 62; gx < 66; ++gx) for (uint16_t gz = 62; gz < 66; ++gz) {
        const int32_t left = int32_t(gx) * 512 - 0x8000;
        const int32_t near = int32_t(gz) * 512 - 0x8000;
        for (int32_t dx : {0, 255, 256, 511}) for (int32_t dz : {0, 255, 256, 511})
            check_cell(map, left + dx, near + dz, gx, gz);
        const auto cell = l3::runtime_map_find_cell(map, 0, left + 256, near + 256);
        rotations_seen[uint16_t(cell->packed_cell) & 3u] = true;
    }
    for (bool seen : rotations_seen) assert(seen);
    // The zero-world boundary is between grid63 and grid64, not through grid63.
    check_cell(map, -1, -1, 63, 63);
    check_cell(map, 0, 0, 64, 64);
    check_cell(map, 255, 255, 64, 64);
    check_cell(map, 256, 256, 64, 64);
    check_cell(map, 511, 511, 64, 64);
    check_cell(map, 512, 512, 65, 65);
    assert(!l3::runtime_map_find_cell(map, 0, -1025, -256));
    assert(!l3::runtime_map_find_cell(map, 0, -256, -1025));
    assert(!l3::runtime_map_find_cell(map, 0, 1024, 256));
    assert(!l3::runtime_map_find_cell(map, 0, 256, 1024));
    assert(!l3::runtime_map_find_cell(map, 1, 0, 0));
}

void signed_halfword_edges_and_extreme_queries() {
    const auto low = make_map(0, 0, 1, 1);
    check_cell(low, -32768, -32768, 0, 0);
    check_cell(low, -32512, -32512, 0, 0);
    check_cell(low, -32257, -32257, 0, 0);
    assert(!l3::runtime_map_find_cell(low, 0, -32769, -32512));
    assert(!l3::runtime_map_find_cell(low, 0, -32256, -32512));
    const auto high = make_map(127, 127, 1, 1);
    check_cell(high, 32256, 32256, 127, 127);
    check_cell(high, 32512, 32512, 127, 127);
    check_cell(high, 32767, 32767, 127, 127);
    assert(!l3::runtime_map_find_cell(high, 0, 32768, 32512));
    for (int32_t extreme : {std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()}) {
        assert(!l3::runtime_map_find_cell(high, 0, extreme, 32512));
        assert(!l3::runtime_map_find_cell(high, 0, 32512, extreme));
    }
    // Malformed cell storage is rejected even when coordinate bounds match.
    auto malformed = make_map(64, 64, 1, 1);
    malformed.cells.clear();
    assert(!l3::runtime_map_find_cell(malformed, 0, 256, 256));
}

void previous_cell_below_grid_zero() {
    // Negative grid indices can occur in authored synthetic/malformed area
    // bounds. The low signed-halfword query's preceding neighbors are grid -1;
    // their centers must remain defined without left shifting a negative value.
    const auto map = make_map(-1, -1, 2, 2);
    const auto candidates = l3::runtime_map_collect_contact_candidates_8004b1e0(
        map, 0, -32768, -32768, true);
    assert(candidates.size() == 4);
    const std::array<uint32_t, 4> expected_indices{{3, 2, 1, 0}};
    const std::array<int32_t, 4> expected_x{{-32512, -33024, -32512, -33024}};
    const std::array<int32_t, 4> expected_z{{-32512, -32512, -33024, -33024}};
    for (size_t i = 0; i < candidates.size(); ++i) {
        assert(candidates[i].cell_index == expected_indices[i]);
        assert(candidates[i].world_origin_x == expected_x[i]);
        assert(candidates[i].world_origin_z == expected_z[i]);
        assert(candidates[i].packed_cell == int16_t(expected_indices[i] + 1));
    }
    const auto containing = l3::runtime_map_find_cell(map, 0, -32768, -32768);
    assert(containing && containing->cell_index == expected_indices.front());
    const auto outside_halfword = l3::runtime_map_find_cell(map, 0, -32769, -32769);
    assert(outside_halfword && outside_halfword->world_origin_x == -33024);
    assert(outside_halfword->world_origin_z == -33024);
}
}

int main() {
    centered_boundaries_and_quarter_turn_contacts();
    signed_halfword_edges_and_extreme_queries();
    previous_cell_below_grid_zero();
}
