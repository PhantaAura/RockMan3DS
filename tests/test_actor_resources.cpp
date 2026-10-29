#include "legends3ds/actor_runtime.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <new>

static size_t allocation_count = 0;
void* operator new(std::size_t n) { ++allocation_count; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { ++allocation_count; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
constexpr uint32_t base = 0x8016c000;
void put32(std::vector<uint8_t>& b, size_t at, uint32_t v) { l3::patch_u32(b, at, v); }
void put16(std::vector<uint8_t>& b, size_t at, uint16_t v) {
    b.at(at) = static_cast<uint8_t>(v); b.at(at + 1) = static_cast<uint8_t>(v >> 8);
}
template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
void near(float a, float b) { assert(std::fabs(a - b) < 0.001f); }
std::vector<uint8_t> fixture() {
    std::vector<uint8_t> b(0x800, 0);
    put32(b, 0, 1); b[4] = 0x20; b[5] = 0x35;
    put32(b, 8, base + 0x40); put32(b, 12, base + 0x600); put32(b, 16, base + 0x680);
    b[0x50] = 0; b[0x51] = 2;
    b[0x54] = 1; b[0x55] = 0; b[0x56] = 1;
    for (size_t i = 0; i < 3; ++i) put32(b, 0xb0 + i * 4, base + 0x200);
    const size_t p0 = 0x214, p1 = 0x228;
    b[p0 + 1] = 1; b[p0 + 2] = 3;
    put32(b, p0 + 8, base + 0x520); put32(b, p0 + 12, 0x23ab002bu); put32(b, p0 + 16, base + 0x540);
    b[p1] = 1; b[p1 + 2] = 3; b[p1 + 3] = 77;
    put32(b, p1 + 4, base + 0x500); put32(b, p1 + 12, 0x23ab002bu); put32(b, p1 + 16, base + 0x560);
    for (size_t i = 0; i < 8; ++i) { b[0x500 + i] = uint8_t(i + 1); b[0x520 + i] = uint8_t(i + 1); }
    b[0x508] = 0; b[0x509] = 1; b[0x50a] = 2;
    b[0x528] = 0; b[0x529] = 1; b[0x52a] = 2; b[0x52b] = 1;
    put16(b, 0x560, 8); put16(b, 0x568 + 2, uint16_t(-8)); put16(b, 0x570 + 4, 16);
    put32(b, 0x600, base + 0x640); put32(b, 0x604, base + 0x660);
    put16(b, 0x648, 16); put16(b, 0x64a, 32); put16(b, 0x64c, 48);
    put32(b, 0x660, base + 0x700); put32(b, 0x680, base + 0x6a0);
    b[0x6a0] = 0; b[0x6a1] = 6; b[0x6a3] = 0xff;
    // First three channels 1,2,3; rotations zero. Entire 128-byte consumer window remains valid.
    const std::array<uint16_t, 3> root{{1, 2, 3}};
    for (size_t i = 0; i < root.size(); ++i) {
        const size_t bit = i * 12, byte = 0x700 + bit / 8;
        const uint32_t v = uint32_t(root[i]) << (bit % 8);
        b[byte] |= static_cast<uint8_t>(v); b[byte + 1] |= static_cast<uint8_t>(v >> 8);
    }
    return b;
}
l3::StageRuntimeArea stage(size_t count = 2) {
    l3::StageObjectList list;
    for (size_t i = 0; i < count; ++i) {
        l3::StageObjectRecord r;
        r.id = 1; r.object_class = 0x20; r.raw[0] = 1; r.raw[1] = 0x20; r.raw[2] = 0x35;
        r.words_02_to_0a[0] = 0x35;
        r.x = 100; r.y = 200; r.z = 300;
        list.records.push_back(r);
    }
    return l3::build_fresh_stage_runtime_area(list, l3::parse_ebd_directory(fixture(), base));
}
l3::RuntimeMap floor_map() {
    l3::RuntimeMap map;
    l3::RuntimeMapArea area;
    area.start_x = 63; area.start_z = 63; area.width = 2; area.height = 2; area.cell_count = 4;
    map.areas.push_back(area); map.cells = {1,1,1,1};
    map.idx.resize(2); map.idx[1].descriptor_index = 0;
    l3::RuntimeMapDescriptor descriptor; descriptor.default_contact_index = 0;
    map.descriptors.push_back(descriptor); map.contact_lists.push_back({0,1});
    map.contact_groups.push_back({-512,512,-512,512,0,1});
    map.contact_primitives.push_back({-128,1024,3,0});
    return map;
}
void floor_and_hierarchy_checks(l3::ImportedActorInstance actor) {
    actor.source_origin = {{0,0,0}};
    actor.floor_sample_available = false;
    auto map = floor_map();
    const auto sample = l3::sample_imported_actor_floor(actor, map, 0);
    assert(sample.status == l3::RuntimeContactStatus::SourceBackedPartial && sample.floor_y == -128);
    assert(sample.candidate_cells == 1 && sample.primitive_index == 0 && actor.source_origin[1] == 0);
    map.contact_primitives[0].flags = 0x8000;
    assert(!l3::sample_imported_actor_floor(actor, map, 0).floor_y);
    map.contact_primitives[0].flags = 0;
    actor.source_origin[0] = 512; // source strict X boundary; zero bounds do not expand it.
    assert(!l3::sample_imported_actor_floor(actor, map, 0).floor_y);
    actor.source_origin[0] = 0;
    map.contact_primitives[0].code = 0x31;
    const auto unsupported = l3::sample_imported_actor_floor(actor, map, 0);
    assert(unsupported.status == l3::RuntimeContactStatus::Unsupported && !unsupported.floor_y);
    map = floor_map(); map.cells = {-32512,-32512,-32512,-32512}; // packed level1.
    assert(l3::sample_imported_actor_floor(actor, map, 0).floor_y == -129);
    map.cells = {0,0,0,0};
    assert(l3::sample_imported_actor_floor(actor, map, 0).status == l3::RuntimeContactStatus::NoData);
    map = floor_map(); map.contact_groups[0].primitive_first = UINT32_MAX;
    assert(l3::sample_imported_actor_floor(actor, map, 0).status == l3::RuntimeContactStatus::Unsupported);
    map = floor_map(); map.areas[0].cell_first = UINT32_MAX;
    assert(l3::sample_imported_actor_floor(actor, map, 0).status == l3::RuntimeContactStatus::NoData);
    map = floor_map(); map.contact_lists[0].group_first = UINT32_MAX;
    assert(l3::sample_imported_actor_floor(actor, map, 0).status == l3::RuntimeContactStatus::Unsupported);

    auto geometry = std::make_shared<l3::ActorMeshResource>(*actor.resource);
    geometry->bindings.resize(3);
    geometry->bindings[1].parent = 0; geometry->bindings[1].rotation_translation_index = 1;
    geometry->bindings[2].parent = 1; geometry->bindings[2].rotation_translation_index = 2;
    actor.resource = geometry;
    actor.heading = 0;
    actor.pose->root_translation = {{0,0,0}};
    actor.pose->rotations = {};
    actor.pose->rotations[1][2] = 1024;
    actor.pose->base_translation = {{{0,0,0}},{{16,0,0}},{{16,0,0}}};
    auto matrices = l3::build_imported_actor_pose_matrices(actor);
    const auto point = l3::player_pose_transform_point(matrices.world[2], {8,0,0});
    near(point.x, 2); near(point.y, 3); near(point.z, 0); // scale applied once through two parents.
    geometry->bindings[1].draw_control = -1;
    matrices = l3::build_imported_actor_pose_matrices(actor);
    assert(!matrices.draw_enabled[1] && matrices.draw_enabled[2]); // hidden parent still transforms.
    geometry->bindings[1].parent = -1;
    matrices = l3::build_imported_actor_pose_matrices(actor);
    assert(!matrices.draw_enabled[1] && !matrices.draw_enabled[2]);
    geometry->bindings[2].rotation_translation_index = 0;
    assert(l3::build_imported_actor_pose_matrices(actor).draw_enabled[2]); // explicit root copy.
    const size_t before = allocation_count;
    for (unsigned i = 0; i < 240; ++i) {
        (void)l3::build_imported_actor_pose_matrices(actor);
        (void)l3::sample_imported_actor_floor(actor, map, 0);
    }
    assert(before == allocation_count);
}
void initializer_floor_bounds_checks(l3::ImportedActorInstance actor, bool class20_03) {
    actor.floor_sample_available = false;
    assert(actor.floor_bounds);
    const auto& bounds = *actor.floor_bounds;
    assert(bounds.x0 == (class20_03 ? -60 : 0) && bounds.x1 == (class20_03 ? 60 : 0));
    assert(bounds.z0 == bounds.x0 && bounds.z1 == bounds.x1);
    assert(bounds.y0 == (class20_03 ? -128 : 0) && bounds.y1 == 0);
    auto map = floor_map();
    map.contact_primitives[0].y1 = 0;
    // These exact edge/admission cases were replayed through0x80048EA4 with
    // authored rectangle data, explicit source bounds and fresh deltaY=0.
    const size_t before = allocation_count;
    for (const int16_t x : {-572,-571,-512,-1,0,511,512,571,572})
        for (const int16_t z : {-572,0,572})
            for (const int16_t y : {-200,-128,-1,0,127,128})
                for (const uint16_t code : {1,3,15})
                    for (const uint16_t flags : {0,0x8000}) {
                        actor.source_origin = {{x,y,z}};
                        map.contact_primitives[0].code = code;
                        map.contact_primitives[0].flags = flags;
                        const auto sample = l3::sample_imported_actor_floor(actor, map, 0);
                        const int edge = class20_03 ? 572 : 512;
                        const int top = class20_03 ? 128 : 0;
                        const bool admitted = -edge < x && x < edge && -edge < z && z < edge &&
                                              y < top && flags == 0;
                        assert(sample.status == l3::RuntimeContactStatus::SourceBackedPartial);
                        assert(sample.floor_y == (admitted ? std::optional<int16_t>(-128) : std::nullopt));
                        assert(actor.source_origin[1] == y); // query preserves the source position.
                    }
    assert(before == allocation_count);
    actor.source_origin = {{0,-1,0}};
    map.contact_primitives[0].flags = 0;
    for (const uint16_t code : {8,9,10,11}) {
        map.contact_primitives[0].code = code;
        const auto slope_floor = l3::sample_imported_actor_floor(actor, map, 0);
        // Full original wrapper comparisons exposed a reversed scratch-bound
        // premise in the inherited slope fixture. Hold unproved actor slopes.
        assert(slope_floor.status == l3::RuntimeContactStatus::Unsupported);
        assert(!slope_floor.floor_y);
    }
    actor.source_origin = {{0,0,0}};
    map.cells = {-32512,-32512,-32512,-32512};
    assert(l3::sample_imported_actor_floor(actor, map, 0).floor_y == -129);
    actor.floor_bounds.reset();
    assert(l3::sample_imported_actor_floor(actor, map, 0).status == l3::RuntimeContactStatus::Unsupported);
}
void transactional_reload_checks() {
    const auto ebd = l3::parse_ebd_directory(fixture(), base);
    const auto prepare = [&](size_t count) {
        auto candidate = l3::spawn_imported_actors(stage(count), ebd);
        for (auto& instance : candidate.actors) l3::bind_imported_actor_pose(instance, ebd, 0, 0);
        return candidate;
    };
    auto active = prepare(1);
    l3::bind_imported_actor_floor(active.actors[0], 220);
    const auto* old_mesh = active.actors[0].resource.get();
    const auto old_bytes = active.decoded_resource_bytes;
    std::weak_ptr<const l3::ActorMeshResource> old_lifetime = active.actors[0].resource;
    l3::ActorReloadRevisionState revisions{11,11,std::nullopt};
    const auto old_survives = [&] {
        assert(active.actors.size() == 1 && active.actors[0].resource.get() == old_mesh);
        assert(!old_lifetime.expired() && active.decoded_resource_bytes == old_bytes);
        assert(active.actors[0].floor_sample_available && active.actors[0].source_origin[1] == 220);
        assert(active.actors[0].pose && active.actors[0].pose->pose_source_offset == 0x700);
        assert(revisions.committed_actor_revision == 11);
    };
    struct ExactLoaderFailure : std::runtime_error { using std::runtime_error::runtime_error; };
    size_t attempts = 0;
    std::weak_ptr<const l3::ActorMeshResource> failed_candidate_lifetime;
    bool exact_failure = false;
    try {
        l3::reload_imported_actors_for_revision(active, revisions, 12, [&]() -> l3::ActorAreaRuntime {
            ++attempts;
            auto candidate = prepare(2);
            failed_candidate_lifetime = candidate.actors[0].resource;
            throw ExactLoaderFailure("injected actor loader failure");
        });
    } catch (const ExactLoaderFailure& e) {
        exact_failure = std::string(e.what()) == "injected actor loader failure";
    }
    assert(exact_failure && failed_candidate_lifetime.expired());
    assert(revisions.observed_source_revision == 12 && revisions.failed_source_revision == 12);
    old_survives();
    const size_t before = allocation_count;
    for (unsigned frame = 0; frame < 240; ++frame)
        assert(!l3::reload_imported_actors_for_revision(active, revisions, 12, [&] {
            ++attempts; return prepare(2);
        }));
    assert(attempts == 1 && before == allocation_count);

    auto malformed_bytes = fixture(); malformed_bytes[0x508] = 3;
    const auto malformed = l3::parse_ebd_directory(malformed_bytes, base);
    bool malformed_rejected = false;
    try {
        l3::reload_imported_actors_for_revision(active, revisions, 13, [&] {
            return l3::spawn_imported_actors(stage(1), malformed);
        });
    } catch (const std::runtime_error& e) {
        malformed_rejected = std::string(e.what()) ==
            "actor reload candidate resource failure: actor triangle index out of range";
    }
    assert(malformed_rejected && revisions.failed_source_revision == 13);
    old_survives();
    auto unavailable = ebd; unavailable.source_payload.clear();
    rejects([&] { l3::reload_imported_actors_for_revision(active, revisions, 14, [&] {
        return l3::spawn_imported_actors(stage(1), unavailable);
    }); });
    assert(revisions.failed_source_revision == 14); old_survives();
    rejects([&] { l3::reload_imported_actors_for_revision(active, revisions, 15, [&] {
        return l3::spawn_imported_actors(stage(1), ebd); // neutral mesh lacks its presentation pose.
    }); });
    assert(revisions.failed_source_revision == 15); old_survives();
    rejects([&] { l3::reload_imported_actors_for_revision(active, revisions, 16, [&] {
        auto candidate = prepare(2); candidate.decoded_resource_bytes = 512 * 1024 + 1;
        return candidate;
    }); });
    assert(revisions.failed_source_revision == 16); old_survives();

    assert(l3::reload_imported_actors_for_revision(active, revisions, 17, [&] { return prepare(2); }));
    assert(old_lifetime.expired() && active.actors.size() == 2 && active.unique_resource_count == 1);
    assert(active.actors[0].resource == active.actors[1].resource);
    assert(revisions.observed_source_revision == 17 && revisions.committed_actor_revision == 17);
    assert(!revisions.failed_source_revision);
    const auto* committed_mesh = active.actors[0].resource.get();
    bool exact_nonstandard_failure = false;
    try {
        l3::reload_imported_actors_for_revision(active, revisions, 18, []() -> l3::ActorAreaRuntime { throw 42; });
    } catch (int value) { exact_nonstandard_failure = value == 42; }
    assert(exact_nonstandard_failure && active.actors[0].resource.get() == committed_mesh);
    assert(revisions.failed_source_revision == 18 && revisions.committed_actor_revision == 17);
    std::weak_ptr<const l3::ActorMeshResource> committed_lifetime = active.actors[0].resource;
    auto unsupported_stage = stage(1); unsupported_stage.instances[0].object.raw[4] = 1;
    // Unproved initializers remain explicit partial coverage, not resource failures.
    assert(l3::reload_imported_actors_for_revision(active, revisions, 19, [&] {
        return l3::spawn_imported_actors(unsupported_stage, ebd);
    }));
    assert(committed_lifetime.expired() && active.actors.empty());
    assert(active.diagnostics[0].state == l3::ActorSpawnState::UNSUPPORTED_INITIALIZER);
    assert(revisions.committed_actor_revision == 19 && !revisions.failed_source_revision);
}
void request0_cache_checks() {
    auto bytes = fixture(); bytes.resize(0xc00); bytes[5] = 3;
    for (size_t i = 0; i < 8; ++i) {
        put32(bytes, 0x660 + i * 4, base + 0x800 + uint32_t(i * 128));
        bytes[0x6a0 + i * 8] = uint8_t(i);
        bytes[0x6a1 + i * 8] = 1;
        bytes[0x6a2 + i * 8] = uint8_t(23 + i); // retained opaque source byte.
        bytes[0x6a3 + i * 8] = i == 7 ? 0x80 : 0;
        bytes[0x800 + i * 128] = uint8_t(i); // root X changes with the source pose.
    }
    const auto ebd = l3::parse_ebd_directory(bytes, base);
    auto actors_stage = stage(3);
    for (auto& input : actors_stage.instances) {
        input.object.subtype = 3; input.object.raw[2] = 3;
    }
    auto area = l3::spawn_imported_actors(actors_stage, ebd);
    const auto mesh_bytes = area.decoded_resource_bytes;
    assert(area.actors.size() == 3 && area.unique_pose_cache_count == 0);
    l3::bind_imported_actor_request0_caches(area, ebd);
    assert(area.unique_pose_cache_count == 1 && area.decoded_pose_cache_bytes > 0);
    assert(area.decoded_resource_bytes == mesh_bytes + area.decoded_pose_cache_bytes);
    assert(area.actors[0].request0_pose_cache == area.actors[1].request0_pose_cache);
    assert(area.actors[0].request0_pose_cache->poses.size() == 8);
    assert(area.actors[0].request0_sequence->record_index == 0);
    const auto* pose_storage = area.actors[0].pose->base_translation.data();
    const size_t before = allocation_count;
    for (unsigned tick = 1; tick <= 480; ++tick) {
        const auto result = l3::tick_imported_actor_request0(area);
        assert(result.admitted == 3 && result.pose_changed == 3 && result.unsupported == 0);
        for (const auto& actor : area.actors) {
            assert(actor.pose->record_index == tick % 8 && actor.pose->frame_index == tick % 8);
            assert(actor.pose->root_translation[0] == int(tick % 8));
            assert(actor.request0_sequence->opaque_a6 == 23 + tick % 8);
            assert(l3::build_imported_actor_pose_matrices(actor).part_count == 2);
        }
        assert(area.actors[0].pose->base_translation.data() == pose_storage);
    }
    assert(before == allocation_count);
    area.actors[0].source_dispatch_flag6 = 0;
    assert(l3::tick_imported_actor_request0(area).admitted == 2);
    assert(area.actors[0].pose->record_index == 0);
    l3::bind_imported_actor_pose(area.actors[1], ebd, 0, 0, false);
    assert(!area.actors[1].request0_pose_cache && !area.actors[1].request0_sequence);
    assert(l3::tick_imported_actor_request0(area).admitted == 1);

    auto unbound = l3::spawn_imported_actors(actors_stage, ebd);
    rejects([&] { l3::bind_imported_actor_request0_caches(unbound, ebd,
        mesh_bytes + area.decoded_pose_cache_bytes - 1); });
    assert(unbound.decoded_resource_bytes == mesh_bytes && unbound.unique_pose_cache_count == 0);
    for (const auto& actor : unbound.actors)
        assert(!actor.request0_pose_cache && actor.pose->record_index == 0);
    auto broken = bytes; put32(broken, 0x660 + 7 * 4, base + 0xc00);
    const auto bad_pose = l3::parse_ebd_directory(broken, base);
    rejects([&] { l3::bind_imported_actor_request0_caches(unbound, bad_pose); });
    assert(unbound.decoded_resource_bytes == mesh_bytes && !unbound.actors[0].request0_pose_cache);
    broken = bytes; broken[0x6a3] = 0x90; // unsupported forward skip, not zero-filled holes.
    const auto bad_edge = l3::parse_ebd_directory(broken, base);
    rejects([&] { l3::bind_imported_actor_request0_caches(unbound, bad_edge); });
    assert(!unbound.actors[0].request0_sequence);
    auto neutral = l3::spawn_imported_actors(stage(1), l3::parse_ebd_directory(fixture(), base));
    l3::bind_imported_actor_pose(neutral.actors[0], l3::parse_ebd_directory(fixture(), base), 0, 0, false);
    l3::bind_imported_actor_request0_caches(neutral, ebd);
    assert(neutral.unique_pose_cache_count == 0 && l3::tick_imported_actor_request0(neutral).admitted == 0);
    std::weak_ptr<const l3::ActorPoseSequenceCache> lifetime = area.actors[2].request0_pose_cache;
    l3::unload_imported_actors(area);
    assert(lifetime.expired() && area.decoded_pose_cache_bytes == 0 && area.unique_pose_cache_count == 0);
}
void area3_boss_owner_checks(){
    l3::StageRuntimeArea stage;
    auto none=l3::source_area3_boss_spawn_spec_801076e8(stage);
    assert(none.status==l3::SourceArea3BossStatus::NoEventOwner&&!none.spec);
    l3::StageRuntimeInstance event;
    event.object.source_index=37;event.object.id=3;event.object.source_class=0;
    event.object.family=l3::StageObjectFamily::Class00;
    stage.instances.push_back(event);
    const auto owned=l3::source_area3_boss_spawn_spec_801076e8(stage);
    assert(owned.status==l3::SourceArea3BossStatus::SourceBackedPresentation&&owned.spec);
    assert(owned.spec->event_source_index==37&&owned.spec->object_class==0x40&&
        owned.spec->subtype==2&&owned.spec->parameter==1);
    assert(owned.spec->callback==0x801076e8u&&owned.spec->initializer==0x801078b8u);
    const std::array<int16_t,3> spawn_position{{-256,-385,1001}};
    const std::array<int16_t,3> observed_scale{{512,512,512}};
    assert(owned.spec->spawn_position==spawn_position);
    assert(owned.spec->heading==0&&owned.spec->scale==observed_scale);
    assert(owned.spec->shade==0x4210&&owned.spec->hp==1024&&owned.spec->action==1&&
        owned.spec->phase==0&&owned.spec->substate==1&&owned.spec->pose_request==0&&
        owned.spec->pose_record==0);
    l3::ActorAreaRuntime wrong_area;
    const auto rejected=l3::append_source_area3_boss_801076e8(
        wrong_area,stage,l3::EbdDirectory{},1);
    assert(rejected.status==l3::SourceArea3BossStatus::NoEventOwner&&
        wrong_area.actors.empty());
    stage.instances.push_back(event);
    const auto duplicate=l3::source_area3_boss_spawn_spec_801076e8(stage);
    assert(duplicate.status==l3::SourceArea3BossStatus::DuplicateEventOwner&&!duplicate.spec);
}
std::shared_ptr<const l3::ActorAnimationBank> area3_intro_bank(){
    l3::ActorAnimationBank bank;bank.source_row=0;
    for(const uint8_t request:{uint8_t(0),uint8_t(1),uint8_t(2),uint8_t(3),uint8_t(11),uint8_t(12),uint8_t(13)}){
        l3::ActorAnimationRequestCache cache;cache.request=request;
        l3::ActorSequenceRecord record{{request,1,1,0x80,0,0,0,0}};
        l3::ActorPoseResource pose;pose.request=request;pose.record_index=0;
        pose.frame_index=request;pose.countdown=1;pose.next_control=-128;
        pose.sequencing_record=record;pose.base_translation.push_back({{0,0,0}});
        cache.sequence.records.push_back(record);cache.sequence.poses.push_back(pose);
        bank.requests.push_back(std::move(cache));
    }
    return std::make_shared<const l3::ActorAnimationBank>(std::move(bank));
}
void area3_boss_intro_checks(){
    using l3::source_area3_boss_action1_select_80107b48;
    auto select=source_area3_boss_action1_select_80107b48(1024,1,5,7,0,false);
    assert(!select.handler_complete&&select.substate==1&&select.phase==5);
    select=source_area3_boss_action1_select_80107b48(1024,0,5,7,1,false);
    assert(select.handler_complete&&select.substate==3&&select.saved_substate==1&&
        select.phase==0&&!(select.flags_4e0&1));
    select=source_area3_boss_action1_select_80107b48(1024,4,5,7,0x21,false);
    assert(select.substate==3&&select.saved_substate==1);
    select=source_area3_boss_action1_select_80107b48(1024,3,5,4,1,false);
    assert(select.substate==4);
    select=source_area3_boss_action1_select_80107b48(1024,2,5,7,1,false);
    assert(select.substate==1);
    select=source_area3_boss_action1_select_80107b48(1024,1,5,7,0x21,false);
    assert(select.substate==1);
    select=source_area3_boss_action1_select_80107b48(1024,1,5,7,1,false);
    assert(select.substate==0);
    select=source_area3_boss_action1_select_80107b48(0,2,5,7,1,false);
    assert(select.substate==0&&select.death_started&&(select.flags_4e0&0x40));
    select=source_area3_boss_action1_select_80107b48(0,2,5,7,0x41,false);
    assert(!select.death_started&&(select.flags_4e0&0x40));
    select=source_area3_boss_action1_select_80107b48(0,2,5,7,1,true);
    assert(!select.death_started&&!(select.flags_4e0&0x40));
    l3::ImportedActorInstance boss;boss.source_class=0x40;boss.subtype=2;
    boss.initializer=0x801078b8u;boss.source_origin={{-256,-385,1001}};
    boss.source_area3_boss.emplace();auto& state=*boss.source_area3_boss;
    state.hp=1024;state.action=1;state.phase=0;state.substate=1;
    boss.animation_bank=area3_intro_bank();boss.pose=boss.animation_bank->requests[0].sequence.poses[0];
    boss.animation_cursor=l3::actor_sequence_begin(0,boss.pose->sequencing_record);
    assert(l3::source_area3_boss_forces_player_entry(boss,-1920));
    assert(l3::source_area3_boss_forces_player_entry(boss,-1632));
    const size_t allocations=allocation_count;
    auto tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1920);
    assert(tick.committed&&!tick.state_changed&&state.intro_leg==0&&boss.source_origin[2]==1001);
    tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(tick.committed&&tick.state_changed&&state.intro_leg==1&&state.phase==3&&
        state.phase_ticks==12&&boss.pose->request==3);
    for(unsigned i=0;i<12;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.intro_leg==2&&state.phase==1&&boss.pose->request==1&&boss.source_origin[2]==1001);
    for(unsigned i=0;i<83;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.intro_leg==3&&state.phase==2&&boss.pose->request==2&&boss.source_origin[2]==-327);
    for(unsigned i=0;i<28;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.intro_leg==4&&state.phase==1&&state.substate==0&&boss.source_origin[2]==-1223);
    tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(tick.status==l3::SourceArea3BossTickStatus::SourceBackedCombatPresentation&&
        state.intro_leg==5&&state.action==6&&state.phase==0&&state.phase_ticks==80&&
        boss.pose->request==2);
    tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.phase==1&&state.phase_ticks==12&&boss.pose->request==11);
    for(unsigned i=0;i<12;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.phase==2&&state.phase_ticks==9&&boss.pose->request==11);
    for(unsigned i=0;i<9;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.phase==3&&state.phase_ticks==19&&boss.pose->request==12);
    for(unsigned i=0;i<19;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.phase==4&&state.phase_ticks==9&&boss.pose->request==12);
    for(unsigned i=0;i<9;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(state.phase==5&&state.phase_ticks==14&&boss.pose->request==13);
    for(unsigned i=0;i<14;++i)tick=l3::tick_source_area3_boss_intro_80107b48(boss,-1632);
    assert(tick.status==l3::SourceArea3BossTickStatus::CombatReady&&state.intro_leg==6&&
        state.action==1&&state.phase==0&&state.phase_ticks==UINT16_MAX&&
        boss.pose->request==13&&allocation_count==allocations);
    assert(!l3::source_area3_boss_forces_player_entry(boss,-1632));
}
}
int main() {
    area3_boss_owner_checks();
    area3_boss_intro_checks();
    request0_cache_checks();
    transactional_reload_checks();
    const auto source = fixture();
    const auto ebd = l3::parse_ebd_directory(source, base);
    const auto mesh = l3::decode_ebd_actor_mesh(ebd, 0);
    assert(mesh.model.parts.size() == 2 && mesh.bindings.size() == 2);
    assert(mesh.model.parts[0].quads.size() == 1 && mesh.model.parts[1].triangles.size() == 1);
    assert(mesh.model.parts[1].triangles[0].uv[5] == 6);
    assert(mesh.model.parts[1].triangles[0].clut == 0x23ab && mesh.model.parts[1].triangles[0].tpage == 0x2b);
    assert(mesh.model.parts[1].vertices[1].y == -8 && mesh.bindings[1].source_part_byte3 == 77);
    assert(mesh.footprint.vertex_bytes == 48 && mesh.footprint.packet_bytes == 24);
    assert(l3::decode_ebd_actor_mesh(ebd, 0, l3::ActorMeshTier::FAR).model.parts.size() == 2);
    rejects([&] { (void)l3::decode_ebd_actor_mesh(ebd, 1); });
    rejects([&] { (void)l3::decode_ebd_actor_mesh(ebd, 0, static_cast<l3::ActorMeshTier>(3)); });
    rejects([&] { (void)l3::decode_ebd_actor_mesh(ebd, 0, l3::ActorMeshTier::NEAR, 1); });
    auto legacy = ebd; legacy.source_payload.clear();
    rejects([&] { (void)l3::decode_ebd_actor_mesh(legacy, 0); });
    auto check_bad = [&](std::vector<uint8_t> b) {
        rejects([&] { (void)l3::decode_ebd_actor_mesh(l3::parse_ebd_directory(b, base), 0); });
    };
    auto bad = source; bad[0x51] = 14; check_bad(bad);
    bad = source; bad[0x51] = 0; check_bad(bad);
    bad = source; bad[0x55] = 1; check_bad(bad);
    bad = source; bad[0x56] = 24; check_bad(bad);
    bad = source; bad[0x508] = 3; check_bad(bad);
    bad = source; put32(bad, 0x228 + 16, base + 0x7f8); check_bad(bad);
    bad = source; put32(bad, 0xb0, UINT32_MAX); check_bad(bad);
    bad = source; put32(bad, 0x228 + 4, base + 0x501); check_bad(bad);
    auto absent = ebd; absent.entries[0].resource_offset[0] = UINT32_MAX;
    rejects([&] { (void)l3::decode_ebd_actor_mesh(absent, 0); });
    auto wrong_class = ebd; wrong_class.entries[0].object_class = 0x40;
    rejects([&] { (void)l3::decode_ebd_actor_mesh(wrong_class, 0); });

    const auto pose = l3::decode_ebd_actor_pose(ebd, 0, mesh, 0, 0);
    assert((pose.root_translation == std::array<int16_t, 3>{{1, 2, 3}}));
    assert((pose.base_translation[1] == std::array<int16_t, 3>{{16, 32, 48}}));
    assert(pose.countdown == 6 && pose.next_control == -1 && pose.pose_source_offset == 0x700);
    rejects([&] { (void)l3::unpack_actor_pose_channels(source.data(), 127); });
    rejects([&] { (void)l3::decode_ebd_actor_pose(ebd, 0, mesh, 1, 0); });
    bad = source; put32(bad, 0x660, base + 0x7c0);
    const auto pose_bad = l3::parse_ebd_directory(bad, base);
    rejects([&] { (void)l3::decode_ebd_actor_pose(pose_bad, 0, mesh, 0, 0); });
    std::array<uint8_t, 128> packed{};
    for (size_t i = 0; i < l3::kActorPoseChannels; ++i) {
        const size_t bit = i * 12, byte = bit / 8;
        const uint32_t v = uint32_t((i * 61 + 2048) & 4095) << (bit % 8);
        packed[byte] |= static_cast<uint8_t>(v); packed[byte + 1] |= static_cast<uint8_t>(v >> 8);
    }
    const auto channels = l3::unpack_actor_pose_channels(packed.data(), packed.size());
    for (size_t i = 0; i < channels.size(); ++i) assert(channels[i] == ((i * 61 + 2048) & 4095));
    assert(channels.size() == 75 && pose.rotations.size() == 24);
    bad = source; bad[0x56] = 23; // last rotation vector is a valid source index.
    const auto last_rotation_ebd = l3::parse_ebd_directory(bad, base);
    const auto last_rotation_mesh = l3::decode_ebd_actor_mesh(last_rotation_ebd, 0);
    assert(l3::decode_ebd_actor_pose(last_rotation_ebd, 0, last_rotation_mesh, 0, 0).rotations.size() == 24);

    auto runtime = l3::spawn_imported_actors(stage(), ebd);
    assert(runtime.actors.size() == 2 && runtime.unique_resource_count == 1);
    assert(runtime.actors[0].resource.get() == runtime.actors[1].resource.get());
    assert(runtime.decoded_resource_bytes == mesh.footprint.decoded_bytes);
    auto& actor = runtime.actors[0];
    assert(actor.raw_scale[0] == 512 && actor.raw_shade == 0x4210);
    assert(actor.raw_shade_verified && actor.floor_query_uses_zero_bounds);
    assert(!actor.floor_sample_available && !actor.pose && !actor.pose_selected_by_retail_state);
    l3::bind_imported_actor_floor(actor, 220);
    l3::bind_imported_actor_pose(actor, ebd, 0, 0);
    assert(actor.floor_sample_available && actor.source_origin[1] == 220 && !actor.pose_selected_by_retail_state);
    const auto matrices = l3::build_imported_actor_pose_matrices(actor);
    const auto root = l3::player_pose_transform_point(matrices.world[0], {});
    const auto child = l3::player_pose_transform_point(matrices.world[1], {8, 0, 0});
    near(root.x, 101); near(root.y, 222); near(root.z, 303);
    near(child.x, 104); near(child.y, 226); near(child.z, 309);
    assert(matrices.draw_enabled[0] && matrices.draw_enabled[1]);
    rejects([&] { l3::bind_imported_actor_pose(actor, pose_bad, 0, 0); });
    assert(actor.pose->pose_source_offset == 0x700); // failure preserves old pose.
    actor.heading = 1024;
    const auto yawed = l3::player_pose_transform_point(l3::build_imported_actor_pose_matrices(actor).world[0], {});
    near(yawed.x, 103); near(yawed.y, 222); near(yawed.z, 299);
    floor_and_hierarchy_checks(actor);
    initializer_floor_bounds_checks(actor, false);
    auto unsupported_stage = stage(1); unsupported_stage.instances[0].object.raw[4] = 1;
    auto unsupported = l3::spawn_imported_actors(unsupported_stage, ebd);
    assert(unsupported.actors.empty() && unsupported.diagnostics[0].state == l3::ActorSpawnState::UNSUPPORTED_INITIALIZER);
    auto enemy_source = fixture(); enemy_source[5] = 3;
    const auto enemy_ebd = l3::parse_ebd_directory(enemy_source, base);
    for (const uint8_t parameter : {uint8_t(0), uint8_t(0x80), uint8_t(0xA0)}) {
        auto enemy_stage = stage(1);
        auto& input = enemy_stage.instances[0].object;
        input.subtype = 3; input.raw[2] = 3;
        input.source_param3 = parameter; input.raw[3] = parameter;
        const auto enemy = l3::spawn_imported_actors(enemy_stage, enemy_ebd);
        assert(enemy.actors.size() == 1);
        const auto& instance = enemy.actors[0];
        assert(instance.initializer == 0x80102410 && instance.pose);
        assert(instance.pose->request == 0 && instance.pose->record_index == 0);
        assert(instance.pose_selected_by_retail_state && !instance.raw_shade_verified);
        assert(!instance.floor_query_uses_zero_bounds);
        const auto enemy_floor = l3::sample_imported_actor_floor(instance, floor_map(), 0);
        assert(enemy_floor.status == l3::RuntimeContactStatus::SourceBackedPartial && enemy_floor.floor_y == -128);
        if (parameter == 0) initializer_floor_bounds_checks(instance, true);
        assert(l3::build_imported_actor_pose_matrices(instance).part_count == 2);
        enemy_stage.instances[0].object.raw[4] = 1;
        assert(l3::spawn_imported_actors(enemy_stage, enemy_ebd).actors.empty());
    }
    assert(l3::class20_03_initial_hp(0, 0) == 64);
    assert(l3::class20_03_initial_hp(2, 0) == 128);
    assert(l3::class20_03_initial_hp(1, 0x80) == 64 && !l3::class20_03_initial_hp(1, 0x01) && !l3::class20_03_initial_hp(1, 0x40));
    assert(l3::class20_03_initial_hp(255, 0x80) == 64);
    assert(!l3::class20_03_initial_hp(0, 0x40));
    auto unknown_parameter = stage(1);
    unknown_parameter.instances[0].object.subtype = 3;
    unknown_parameter.instances[0].object.source_param3 = 1;
    assert(!l3::actor_initializer_profile_for(unknown_parameter.instances[0].object));
    auto unavailable = l3::spawn_imported_actors(stage(1), legacy);
    assert(unavailable.actors.empty() && unavailable.diagnostics[0].state == l3::ActorSpawnState::RESOURCE_UNAVAILABLE);
    auto limited = l3::spawn_imported_actors(stage(1), ebd, 1);
    assert(limited.actors.empty() && limited.diagnostics[0].state == l3::ActorSpawnState::RESOURCE_REJECTED);
    std::weak_ptr<const l3::ActorMeshResource> lifetime = runtime.actors[0].resource;
    l3::unload_imported_actors(runtime);
    assert(runtime.actors.empty() && runtime.diagnostics.empty() && runtime.decoded_resource_bytes == 0 && lifetime.expired());
    for (size_t i = 0; i < 100; ++i) {
        runtime = l3::spawn_imported_actors(stage(), ebd);
        assert(runtime.unique_resource_count == 1 && runtime.decoded_resource_bytes == mesh.footprint.decoded_bytes);
        l3::unload_imported_actors(runtime);
    }
    std::cout << "actor resources: bounds, packets, pose, hierarchy, shared lifecycle and failure isolation passed\n";
}
