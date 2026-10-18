#include "legends3ds/runtime_session.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace {
long allocation_failure_budget = -1;
void* allocate(size_t size) {
    if (allocation_failure_budget == 0) {
        allocation_failure_budget = -1;
        throw std::bad_alloc();
    }
    if (allocation_failure_budget > 0) --allocation_failure_budget;
    if (void* result = std::malloc(size ? size : 1)) return result;
    throw std::bad_alloc();
}
}
void* operator new(size_t size) { return allocate(size); }
void* operator new[](size_t size) { return allocate(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, size_t) noexcept { std::free(pointer); }

namespace {
using namespace l3;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
L3QolSettings settings() {
    L3QolSettings out{};
    l3_qol_settings_defaults(&out);
    return out;
}
RuntimeMap map() {
    RuntimeMap out;
    RuntimeMapArea area;
    area.start_x = 63; area.start_z = 63;
    area.width = 2; area.height = 2; area.cell_count = 4;
    out.areas.push_back(area);
    out.cells.assign(4, static_cast<int16_t>(-32768));
    return out;
}
TutorialRuntimeArea area() {
    TutorialRuntimeArea out;
    StageObjectRecord object;
    object.id = 7; object.object_class = 0x20;
    object.raw[0] = 7; object.raw[1] = 0x20; object.raw[2] = 3;
    object.words_02_to_0a[0] = 3;
    out.flow.objects.records.push_back(object);
    out.stage = build_fresh_stage_runtime_area(out.flow.objects, out.ebd);
    return out;
}
RuntimeSession session() { return RuntimeSession(map(), area(), {}, settings()); }
bool same_queue(const SourceAudioQueue& a, const SourceAudioQueue& b) {
    if (a.count != b.count || a.initialized != b.initialized) return false;
    for (size_t i = 0; i < a.records.size(); ++i)
        if (a.records[i].raw != b.records[i].raw) return false;
    return true;
}
bool same_globals(const ActorDetectionGlobals& a, const ActorDetectionGlobals& b) {
    return a.halfword_800bbd94 == b.halfword_800bbd94 &&
           a.halfword_800bbd96 == b.halfword_800bbd96 &&
           a.byte_800bbd84 == b.byte_800bbd84;
}
void assert_source_context(const RuntimeSession& observed, const RuntimeSession& expected) {
    require(observed.source_stats_selector() == expected.source_stats_selector(), "source selector changed");
    require(same_globals(observed.actor_detection_globals(), expected.actor_detection_globals()), "detector globals changed");
    require(same_queue(observed.source_audio_queue(), expected.source_audio_queue()), "fixed source audio queue changed");
}
void fill_queue(RuntimeSession& value, uint8_t seed) {
    auto& queue = value.source_audio_queue();
    require(queue.initialized && queue.count == 0, "session queue not initialized empty");
    for (size_t i = 0; i < queue.records.size(); ++i)
        for (size_t j = 0; j < queue.records[i].raw.size(); ++j)
            queue.records[i].raw[j] = static_cast<uint8_t>(seed + i * 37 + j * 7);
    for (size_t i = 0; i < kSourceAudioQueueCapacity; ++i)
        require(enqueue_source_audio_8001a008(queue, static_cast<uint32_t>(i + 100),
                    static_cast<uint32_t>(i * 17), static_cast<uint32_t>(i * 31)) == SourceAudioQueueStatus::Queued,
                "bounded producer failed before capacity");
    const auto before = queue;
    require(enqueue_source_audio_8001a008(queue, 999, 1, 2) == SourceAudioQueueStatus::DroppedFull,
            "full source queue did not drop");
    require(same_queue(queue, before), "full drop changed retained bytes");
}
void test_generic_and_fresh_context() {
    auto generic = session();
    require(!generic.source_stats_selector(), "generic session fabricated source selector");
    require(!generic.active_area().source_render_scratch_75, "generic area fabricated scratch mode");
    require(same_globals(generic.actor_detection_globals(), initialize_actor_detection_globals_8005c258()),
            "generic detector subsystem not initialized");
    generic.register_location(77, map(), 0, area());
    fill_queue(generic, 11);
    auto before = generic;
    require(generic.transition_to_location(77, {1, -1, 2}, 31), "generic transition rejected");
    assert_source_context(generic, before);
    generic.initialize_fresh_source_statistics_8001326c();
    require(generic.source_stats_selector() == std::optional<uint8_t>(1), "explicit fresh selector not one");
    generic.notify_actor_detection();
    before = generic;
    generic.reset_player({0, -1, 0}, 0, 10, 10);
    assert_source_context(generic, before);
    require(!generic.transition_to_location(999, {}, 0), "missing location transition accepted");
    assert_source_context(generic, before);
}
void test_copy_and_transition() {
    auto original = session();
    original.initialize_fresh_source_statistics_8001326c(); original.notify_actor_detection();
    fill_queue(original, 42);
    auto destination_area = area(); destination_area.source_render_scratch_75 = 0xA5;
    original.register_location(77, map(), 0, destination_area);
    RuntimeSession constructed(original);
    auto assigned = session(); assigned = original;
    assert_source_context(constructed, original); assert_source_context(assigned, original);
    const RuntimeSession* alias = &assigned;
    assigned = *alias;
    assert_source_context(assigned, original);
    for (auto* copy : {&constructed, &assigned}) {
        require(copy->transition_to_location(77, {100, -1, 10}, 0x345), "copied location lost registration");
        assert_source_context(*copy, original);
        require(copy->active_area().source_render_scratch_75 == std::optional<uint8_t>(0xA5),
                "transition did not preserve destination source mode");
    }
    assigned.source_audio_queue().records.back().raw.back() ^= 0x80;
    require(!same_queue(assigned.source_audio_queue(), original.source_audio_queue()), "copied queue aliases source");
    assert_source_context(constructed, original);
    finish_source_audio_queue_batch_8001a1d0(assigned.source_audio_queue());
    require(original.source_audio_queue().count == kSourceAudioQueueCapacity, "copy drain reset original queue");
}
void test_save_restore_context() {
    size_t failed_allocations = 0;
    bool accepted = false;
    for (long failure = 0; failure < 64; ++failure) {
        auto value = session();
        value.initialize_fresh_source_statistics_8001326c(); value.notify_actor_detection();
        fill_queue(value, 73);
        auto next_area = area(); next_area.source_render_scratch_75 = 0;
        value.register_location(77, map(), 0, next_area);
        auto save = value.make_save_state(5); save.area_id = 77; save.player_x = 15;
        auto before = value;
        auto missing = save; missing.area_id = 999;
        bool rejected = false;
        try { value.restore_checkpoint({missing}); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "missing-location restore accepted");
        assert_source_context(value, before);
        allocation_failure_budget = failure;
        try {
            value.restore_checkpoint({save});
            allocation_failure_budget = -1;
            require(!value.source_stats_selector(), "accepted native save fabricated original selector");
            require(same_globals(value.actor_detection_globals(), before.actor_detection_globals()), "restore replaced detector globals");
            require(same_queue(value.source_audio_queue(), before.source_audio_queue()), "restore replaced fixed audio queue");
            require(value.area_index() == 77 && value.player_position().x == 15, "accepted restore did not change location");
            require(value.active_area().source_render_scratch_75 == std::optional<uint8_t>(0), "known zero scratch mode lost");
            accepted = true; break;
        } catch (const std::bad_alloc&) {
            allocation_failure_budget = -1; ++failed_allocations;
            assert_source_context(value, before);
            require(value.area_index() == before.area_index() && &value.active_map() == &before.active_map(),
                    "allocation-failed restore changed active location");
            require(value.active_area().source_render_scratch_75 == before.active_area().source_render_scratch_75,
                    "allocation-failed restore changed source mode");
        }
    }
    require(accepted && failed_allocations >= 3, "restore allocation rejection coverage incomplete");
    std::cout << failed_allocations << " rejected allocation points; ";
}
void test_cooked_setup_mode() {
    St00FlowSource flow;
    for (size_t i = 0; i < flow.areas.size(); ++i) {
        flow.areas[i].setup_blob[37] = 0xD1;
        flow.areas[i].setup_blob[38] = static_cast<uint8_t>(i * 53);
        flow.areas[i].setup_blob[39] = 0xE2;
        flow.areas[i].setup_seed[0] = 0xA3;
    }
    const auto cooked_flow = cook_l3st00_flow(flow);
    const auto cooked_ebd = cook_l3ebd_directory(EbdDirectory{});
    for (size_t i = 0; i < flow.areas.size(); ++i) {
        auto loaded = load_tutorial_runtime_area(cooked_flow, cooked_ebd, i);
        require(loaded.source_render_scratch_75 == std::optional<uint8_t>(flow.areas[i].setup_blob[38]),
                "cooked source mode not setup low byte38");
        RuntimeSession value(map(), std::move(loaded), {}, settings());
        require(value.active_area().source_render_scratch_75 == std::optional<uint8_t>(flow.areas[i].setup_blob[38]),
                "session lost validated cooked mode");
        require(!value.source_stats_selector(), "cooked area implied fresh-game selector");
    }
}
}
int main() {
    try {
        static_assert(std::is_trivially_copyable<l3::SourceAudioQueue>::value, "source queue must remain fixed scalar storage");
        test_generic_and_fresh_context(); test_copy_and_transition(); test_save_restore_context(); test_cooked_setup_mode();
        std::cout << "RuntimeSession source selector, fixed queue, transactional restore and cooked scratch mode PASS; authored contracts, not retail parity\n";
    } catch (const std::exception& error) {
        allocation_failure_budget = -1;
        std::cerr << error.what() << '\n'; return 1;
    }
}
