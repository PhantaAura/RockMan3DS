#include "legends3ds/actor_runtime.h"
#include <array>
#include <cassert>
#include <iostream>

namespace {
l3::ImportedActorInstance actor_at(int16_t x, int16_t z, bool zero_bounds = false) {
    l3::ImportedActorInstance actor;
    actor.source_class = 0x20;
    actor.subtype = zero_bounds ? 0x35 : 3;
    actor.initializer = zero_bounds ? 0x80106974 : 0x80102410;
    actor.source_origin = {{x, -1, z}};
    actor.floor_query_required = true;
    actor.floor_query_uses_zero_bounds = zero_bounds;
    actor.floor_bounds = zero_bounds ? l3::RetailPlayerBounds{0, 0, 0, 0, 0, 0}
                                    : l3::RetailPlayerBounds{-60, 60, -60, 60, -128, 0};
    return actor;
}
l3::RuntimeMap map_at(const l3::ImportedActorInstance& actor, uint16_t code) {
    l3::RuntimeMap map;
    l3::RuntimeMapArea area;
    area.start_x = 60; area.start_z = 60; area.width = 8; area.height = 8;
    area.cell_count = 64;
    map.areas.push_back(area);
    map.cells.assign(64, 1);
    const auto gx = (int32_t(actor.source_origin[0]) + 32768) / 512 - area.start_x;
    const auto gz = (int32_t(actor.source_origin[2]) + 32768) / 512 - area.start_z;
    assert(gx >= 0 && gx < area.width && gz >= 0 && gz < area.height);
    map.cells[static_cast<size_t>(gz * area.width + gx)] = -16384;
    map.idx.resize(2);
    map.idx[1].descriptor_index = 0;
    map.idx[1].raw6 = 1;
    l3::RuntimeMapDescriptor descriptor;
    descriptor.alt_contact_index = 0;
    map.descriptors.push_back(descriptor);
    map.contact_lists.push_back({0, 1});
    map.contact_groups.push_back({-100, 100, -100, 100, 0, 1});
    map.contact_primitives.push_back({-1, 513, code, 0});
    return map;
}
void outside_diagonals() {
    const std::array<std::array<int16_t, 2>, 8> query{{
        {{-161, 0}}, {{-160, 0}}, {{160, 0}}, {{161, 0}},
        {{0, -161}}, {{0, -160}}, {{0, 160}}, {{0, 161}}
    }};
    for (const uint16_t family : {0x20u, 0x30u, 0x40u, 0x50u}) {
        for (const uint16_t low : {1u, 5u}) {
            for (const auto& point : query) {
                const auto actor = actor_at(point[0], point[1]);
                const auto map = map_at(actor, static_cast<uint16_t>(family | low));
                const auto before = actor.source_origin;
                const auto result = l3::sample_imported_actor_floor(actor, map, 0);
                assert(result.candidate_cells == 2);
                assert(result.status == l3::RuntimeContactStatus::SourceBackedPartial);
                assert(result.floor_y == -1 && result.unsupported_primitives == 0);
                assert(actor.source_origin == before && !actor.floor_sample_available);
            }
            const auto zero = actor_at(100, 0, true);
            const auto zero_map = map_at(zero, static_cast<uint16_t>(family | low));
            const auto result = l3::sample_imported_actor_floor(zero, zero_map, 0);
            assert(result.status == l3::RuntimeContactStatus::SourceBackedPartial);
            assert(result.floor_y == -1 && result.unsupported_primitives == 0);
        }
    }
}
void unknowns_remain_explicit() {
    for (const uint16_t code : {0x21u, 0x25u, 0x31u, 0x35u, 0x41u, 0x45u, 0x51u, 0x55u}) {
        const auto actor = actor_at(0, 0);
        const auto result = l3::sample_imported_actor_floor(actor, map_at(actor, code), 0);
        assert(result.status == l3::RuntimeContactStatus::Unsupported);
        assert(!result.floor_y && result.unsupported_primitives != 0);
    }
    const auto outside = actor_at(161, 0);
    auto map = map_at(outside, 0x61);
    assert(l3::sample_imported_actor_floor(outside, map, 0).status == l3::RuntimeContactStatus::Unsupported);
    map.contact_primitives[0].code = 0x21;
    map.idx[1].x = 32760; // Wrapped halfword geometry cannot use this exclusion.
    assert(l3::sample_imported_actor_floor(outside, map, 0).status == l3::RuntimeContactStatus::Unsupported);
    map.idx[1].x = 0;
    map.contact_groups[0].x1 = map.contact_groups[0].x0; // Original division premise unavailable.
    assert(l3::sample_imported_actor_floor(outside, map, 0).status == l3::RuntimeContactStatus::Unsupported);
    map = map_at(outside, 0x21);
    auto asymmetric = outside;
    asymmetric.floor_bounds->x0 = -30;
    assert(l3::sample_imported_actor_floor(asymmetric, map, 0).status == l3::RuntimeContactStatus::Unsupported);
    map.descriptors[0].alt_contact_index = 0xffff;
    assert(l3::sample_imported_actor_floor(outside, map, 0).status == l3::RuntimeContactStatus::Unsupported);
    map = map_at(outside, 0x301); // The original code>=256 group rejection precedes classification.
    const auto skipped = l3::sample_imported_actor_floor(outside, map, 0);
    assert(skipped.status == l3::RuntimeContactStatus::SourceBackedPartial && skipped.floor_y == -1);
}
} // namespace
int main() {
    outside_diagonals();
    unknowns_remain_explicit();
    std::cout << "actor floor initializer bounds: PASS\n";
}
