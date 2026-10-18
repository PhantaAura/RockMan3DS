#include "legends3ds/runtime_session.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {
using MapBuffers = std::array<const void*, 7>;

MapBuffers buffers(const l3::RuntimeMap& map) {
    return {map.areas.data(), map.cells.data(), map.idx.data(),
            map.descriptors.data(), map.contact_lists.data(),
            map.contact_groups.data(), map.contact_primitives.data()};
}

// Entirely synthetic: every area has one positive cell and a distinct descriptor.
// Small nonnegative test positions belong to grid64 ([0, 0x200)), whose
// render/contact center is +0x100; grid63 is the preceding negative cell.
l3::RuntimeMap make_map(size_t count, uint32_t tag) {
    l3::RuntimeMap map;
    map.idx.resize(count + 1);
    for (size_t i = 0; i < count; ++i) {
        l3::RuntimeMapArea area{};
        area.start_x = 64;
        area.start_z = 64;
        area.width = 1;
        area.height = 1;
        area.cell_first = static_cast<uint32_t>(i);
        area.cell_count = 1;
        map.areas.push_back(area);
        map.cells.push_back(static_cast<int16_t>(i + 1));
        map.idx[i + 1].descriptor_index = static_cast<uint16_t>(i);
        l3::RuntimeMapDescriptor descriptor{};
        descriptor.word0 = tag + static_cast<uint32_t>(i);
        descriptor.default_contact_index = static_cast<uint16_t>(i);
        map.descriptors.push_back(descriptor);
        map.contact_lists.push_back({static_cast<uint32_t>(i), 1});
        l3::RuntimeContactGroup group{};
        group.primitive_first = static_cast<uint32_t>(i);
        group.primitive_count = 1;
        map.contact_groups.push_back(group);
        map.contact_primitives.push_back({0, 1, 0, 0});
    }
    return map;
}

void assert_probe(const l3::RuntimeSession& session, size_t local_area, uint32_t tag) {
    const auto probe = session.probe_player_map();
    assert(probe.in_area && probe.cell);
    assert(probe.cell->area_index == local_area);
    assert(probe.cell->cell_index == local_area);
    assert(probe.idx == &session.active_map().idx[local_area + 1]);
    assert(probe.descriptor == &session.active_map().descriptors[local_area]);
    assert(probe.descriptor->word0 == tag + local_area);
    assert(probe.contact_list_index == local_area);
    assert(probe.contact_groups.size() == 1);
    assert(probe.contact_groups.front().primitives.size() == 1);
}

void test_location_cleanup(L3QolSettings settings) {
    auto session = std::make_unique<l3::RuntimeSession>(make_map(3, 4000),
        l3::TutorialRuntimeArea{}, std::vector<l3::MmlMessageFile>{}, settings);
    session->register_area(1, {});
    session->register_area(2, {});
    const auto original_lifetime = session->active_map_lifetime();
    const auto original_checkpoint = session->make_checkpoint(1);
    assert(!session->unregister_location(0));
    assert(!session->unregister_location(999));
    assert(session->has_registered_area(0));

    session->register_location(77, make_map(2, 5000), 0, {});
    assert(session->transition_to_location(77, {1, 2, 3}, 0x456));
    session->reset_player({1, 2, 3}, 0x456, 42, 80);
    session->set_story_flag(17, true);
    session->tick({});
    const auto active_lifetime = session->active_map_lifetime();
    const auto active_buffers = buffers(session->active_map());
    const auto save_before = l3::cook_l3save(session->make_save_state(123));
    const auto player_before = session->compat_player();

    // One shared tutorial registration must keep the original map alive until
    // the final inactive owner is removed.
    assert(session->unregister_location(0));
    assert(session->unregister_location(1));
    assert(!original_lifetime.expired());
    assert(session->unregister_location(2));
    assert(original_lifetime.expired());
    for (size_t i = 0; i < 3; ++i) {
        assert(!session->has_registered_location(i));
        assert(!session->has_registered_area(i));
    }
    assert(!session->unregister_location(2));
    assert(!session->transition_to_area(0, {9, 9, 9}, 0));
    bool removed_save_rejected = false;
    try {
        session->restore_checkpoint(original_checkpoint);
    } catch (const std::runtime_error&) {
        removed_save_rejected = true;
    }
    assert(removed_save_rejected);
    assert(l3::cook_l3save(session->make_save_state(123)) == save_before);
    assert(std::equal(std::begin(player_before.bytes), std::end(player_before.bytes),
                      std::begin(session->compat_player().bytes)));
    assert(buffers(session->active_map()) == active_buffers);
    assert(!active_lifetime.expired());

    session->register_location(88, make_map(2, 6000), 0, {});
    assert(session->transition_to_location(88, {1, 2, 3}, 0x456));
    // An inactive tutorial registration may share the current map; dropping it
    // must release its area records while preserving the active allocation.
    session->register_area(1, {});
    session->register_location(99, make_map(1, 7000), 0, {});
    const auto current_lifetime = session->active_map_lifetime();
    const auto current_buffers = buffers(session->active_map());
    const auto current_save = l3::cook_l3save(session->make_save_state(123));
    const auto current_player = session->compat_player();
    assert(session->release_inactive_locations() == 3);
    assert(active_lifetime.expired());
    assert(!current_lifetime.expired());
    assert(buffers(session->active_map()) == current_buffers);
    assert(session->has_registered_location(88));
    assert(!session->has_registered_location(77));
    assert(!session->has_registered_location(99));
    assert(!session->has_registered_location(1));
    assert(!session->has_registered_area(1));
    assert(!session->unregister_location(88));
    assert(session->release_inactive_locations() == 0);
    assert(l3::cook_l3save(session->make_save_state(123)) == current_save);
    assert(std::equal(std::begin(current_player.bytes), std::end(current_player.bytes),
                      std::begin(session->compat_player().bytes)));
    assert_probe(*session, 0, 6000);

    // Copies own shared maps independently. The weak observer cannot keep the
    // final active allocation alive after the last session is destroyed.
    auto retained = std::make_unique<l3::RuntimeSession>(*session);
    session.reset();
    assert(!current_lifetime.expired());
    assert_probe(*retained, 0, 6000);
    retained.reset();
    assert(current_lifetime.expired());
}
}

int main() {
    static_assert(std::is_same_v<decltype(std::declval<l3::RuntimeSession&>().active_map()),
                                 const l3::RuntimeMap&>);
    constexpr size_t area_count = 24;
    constexpr uint32_t original_tag = 1000;
    L3QolSettings settings{};
    l3_qol_settings_defaults(&settings);
    MapBuffers original_buffers{};
    std::unique_ptr<l3::RuntimeSession> session;
    {
        auto map = make_map(area_count, original_tag);
        original_buffers = buffers(map);
        session = std::make_unique<l3::RuntimeSession>(std::move(map), l3::TutorialRuntimeArea{},
                                                     std::vector<l3::MmlMessageFile>{}, settings);
    }
    const auto* original_map = &session->active_map();
    // Construction moves each allocation and keeps it alive after the input dies.
    assert(buffers(*original_map) == original_buffers);
    for (size_t i = 1; i < area_count; ++i) session->register_area(i, {});
    for (size_t i = 0; i < area_count; ++i) {
        assert(session->has_registered_area(i));
        assert(session->has_registered_location(i));
        assert(session->transition_to_area(i, {1, 2, 3}, 0x1234));
        // Every registration must refer to the same map object and all seven
        // buffers. Value equality alone would miss the old per-area copies.
        assert(&session->active_map() == original_map);
        assert(buffers(session->active_map()) == original_buffers);
        assert(session->area_index() == i && session->map_area_index() == i);
        assert(session->player_yaw() == 0x234);
        assert_probe(*session, i, original_tag);
    }
    assert(!session->transition_to_location(999, {100, 100, 100}, 0));
    assert(&session->active_map() == original_map);
    assert(session->area_index() == area_count - 1);
    assert(session->player_position().x == 1);

    session->set_area(7, {});
    session->reset_player({4, 5, 6}, 0x678, 63, 80);
    session->set_story_flag(17, true);
    session->tick({});
    const auto checkpoint = session->make_checkpoint(123);
    {
        auto other_map = make_map(3, 2000);
        // The by-value API still isolates the session from caller mutations.
        session->register_location(77, other_map, 2, {});
        other_map.descriptors[2].word0 = 9999;
    }
    assert(session->transition_to_location(77, {7, 8, 9}, 0xABC));
    const auto* other_map = &session->active_map();
    assert(other_map != original_map);
    assert(session->area_index() == 77 && session->map_area_index() == 2);
    assert_probe(*session, 2, 2000);
    // Transitions retain the same storage for cross-map locations as well.
    session->restore_checkpoint(checkpoint);
    assert(&session->active_map() == original_map);
    assert(buffers(session->active_map()) == original_buffers);
    assert(session->area_index() == 7 && session->map_area_index() == 7);
    assert(session->player_position().x == 4 && session->player_position().y == 5);
    assert(session->player_position().z == 6 && session->player_yaw() == 0x678);
    assert(session->hp() == 63 && session->hp_max() == 80);
    assert(session->frame_counter() == 1 && session->story_flag(17));
    assert_probe(*session, 7, original_tag);
    assert(session->transition_to_location(77, {}, 0));
    assert(&session->active_map() == other_map);

    // Replacing the active registration must not invalidate the current map.
    // It becomes active only on a transition, just as with the by-value storage.
    session->register_location(77, make_map(1, 3000), 0, {});
    assert(&session->active_map() == other_map);
    assert_probe(*session, 2, 2000);
    assert(session->transition_to_location(77, {}, 0));
    assert_probe(*session, 0, 3000);
    // A copied session owns its maps independently of the source session's lifetime.
    auto retained = std::make_unique<l3::RuntimeSession>(*session);
    session.reset();
    assert_probe(*retained, 0, 3000);
    retained->restore_checkpoint(checkpoint);
    assert(&retained->active_map() == original_map);
    assert(buffers(retained->active_map()) == original_buffers);
    assert_probe(*retained, 7, original_tag);
    test_location_cleanup(settings);
}
