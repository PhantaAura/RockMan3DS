#include "legends3ds/actor_runtime.h"
#include "legends3ds/child_runtime.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <tuple>

namespace {
size_t allocations = 0;
bool measure_allocations = false;
}
void* operator new(std::size_t size) {
    if (measure_allocations) ++allocations;
    if (void* pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// Same authored scalar bank shape as test_actor_behavior_runtime. No retail
// meshes, textures, animation payloads, statistics or reference tables.
l3::ActorAnimationBank authored_bank() {
    l3::ActorAnimationBank bank; bank.source_row = 0;
    for (uint8_t id : {uint8_t(0), uint8_t(1), uint8_t(2), uint8_t(3)}) {
        l3::ActorAnimationRequestCache cache; cache.request = id;
        for (uint8_t index = 0; index < 2; ++index) {
            l3::ActorSequenceRecord record{{index, 2, 1, uint8_t(index ? 0x80 : 0), 0, 0, 0, 0}};
            l3::ActorPoseResource pose;
            pose.request = id; pose.record_index = index; pose.frame_index = index;
            pose.countdown = 2; pose.next_control = static_cast<int8_t>(record[3]);
            pose.sequencing_record = record;
            pose.base_translation.push_back({{int16_t(id), int16_t(index), 0}});
            cache.sequence.records.push_back(record); cache.sequence.poses.push_back(pose);
        }
        bank.requests.push_back(cache);
    }
    return bank;
}
l3::ImportedActorInstance authored_actor(uint8_t request = 0) {
    l3::ImportedActorInstance actor;
    actor.source_class = 0x20; actor.subtype = 3; actor.initializer = 0x80102410;
    actor.source_dispatch_flag0 = 1; actor.source_dispatch_flag6 = 3;
    actor.source_behavior.emplace(); actor.source_behavior->behavior.parameter_03 = 0x20;
    l3::bind_imported_actor_floor(actor, -1);
    actor.animation_bank = std::make_shared<const l3::ActorAnimationBank>(authored_bank());
    actor.pose = actor.animation_bank->requests[request].sequence.poses[0];
    actor.animation_cursor = l3::actor_sequence_begin(0, actor.pose->sequencing_record);
    actor.source_behavior->behavior.animation_ac = request;
    actor.pose_selected_by_retail_state = true;
    actor.floor_bounds = l3::RetailPlayerBounds{-60, 60, -60, 60, -128, 0};
    // Distinct fourth raw/fixed lanes expose truncation of the source eight-byte
    // sound payload and four-lane outer snapshots/rollback.
    auto& state = *actor.source_behavior;
    state.raw_14 = {{19, -1, 37, -21846}};
    state.fixed_1c = {{19 * 65536, -65536, 37 * 65536, 0x13572468}};
    state.previous_raw_2c = {{101, 102, 103, 104}};
    state.previous_fixed_34 = {{105, 106, 107, 108}};
    state.angle_54 = {{4, 5, 6, 7}}; state.previous_angle_5c = {{8, 9, 10, 11}};
    state.work_15e = 12;
    actor.source_origin = {{state.raw_14[0], state.raw_14[1], state.raw_14[2]}};
    return actor;
}
l3::RuntimeMap authored_flat_map() {
    l3::RuntimeMap map; l3::RuntimeMapArea area;
    area.start_x = 63; area.start_z = 63; area.width = 2; area.height = 2; area.cell_count = 4;
    map.areas.push_back(area); map.cells.assign(4, int16_t(-32768));
    return map;
}
l3::ReferenceTables authored_reference() {
    l3::ReferenceTables reference;
    reference.trig[0] = {0, 4096}; reference.trig[2048] = {0, -4096};
    return reference;
}
l3::SourceAudioQueue authored_queue(size_t count = 0) {
    l3::SourceAudioQueue queue;
    for (auto& record : queue.records) record.raw.fill(0xa5);
    l3::initialize_source_audio_queue_80019e14(queue); queue.count = count;
    return queue;
}
l3::Class20_03CallbackInputs authored_inputs() {
    l3::Class20_03CallbackInputs inputs; inputs.action.packet_33c = 0;
    inputs.action.work_80 = 0; inputs.action.target_heading_80031198 = 0;
    return inputs;
}

bool same_state(const l3::Class20_03CallbackState& a, const l3::Class20_03CallbackState& b) {
    const auto behavior = [](const l3::Class20_03BehaviorState& s) {
        return std::tie(s.parameter_03, s.action_09, s.phase_0a, s.animation_ac, s.flags_328,
                        s.timer_32c, s.work_330, s.heading_56, s.turn_66, s.work_36c);
    };
    const auto fields = [](const l3::Class20_03CallbackState& s) {
        return std::tie(s.work_00, s.state_08, s.aux_0b, s.flags_0c, s.source_delay_0d,
            s.restore_action_0e, s.previous_animation_ad, s.raw_14, s.fixed_1c, s.angle_54,
            s.previous_raw_2c, s.previous_fixed_34, s.previous_angle_5c, s.work_64,
            s.proxy_position_180, s.previous_proxy_position_334, s.health_6e,
            s.initial_health_b0, s.previous_health_b2, s.work_15e, s.random_32a, s.variant_32e,
            s.work_332, s.packet_33c, s.detection_profile_352, s.detection_origin_34c,
            s.detection_phase_353, s.detection_vector_354, s.detection_countdown_35a,
            s.detection_offset_35b, s.work_35c, s.work_35e, s.defeat_timer_360, s.work_362,
            s.work_36e, s.work_370, s.child_374);
    };
    return behavior(a.behavior) == behavior(b.behavior) && fields(a) == fields(b);
}
bool same_pose(const l3::ActorPoseResource& a, const l3::ActorPoseResource& b) {
    const auto fields = [](const l3::ActorPoseResource& s) {
        return std::tie(s.request, s.record_index, s.frame_index, s.countdown, s.next_control,
            s.sequencing_record, s.root_translation, s.rotations, s.base_translation,
            s.sequence_source_offset, s.pose_source_offset, s.translations_source_offset);
    };
    return fields(a) == fields(b);
}
bool same_cursor(const l3::ActorSequenceCursor& a, const l3::ActorSequenceCursor& b) {
    return std::tie(a.record_index, a.frame_a4, a.countdown_a5, a.opaque_a6, a.opaque_a7) ==
           std::tie(b.record_index, b.frame_a4, b.countdown_a5, b.opaque_a6, b.opaque_a7);
}
bool same_queue(const l3::SourceAudioQueue& a, const l3::SourceAudioQueue& b) {
    if (a.count != b.count || a.initialized != b.initialized) return false;
    for (size_t i = 0; i != a.records.size(); ++i)
        if (a.records[i].raw != b.records[i].raw) return false;
    return true;
}
struct Snapshot {
    l3::Class20_03CallbackState state;
    l3::ActorPoseResource pose;
    l3::ActorSequenceCursor cursor;
    std::array<int16_t, 3> origin, velocity;
    uint16_t heading;
    uint8_t state74;
    bool pose_selected;
    l3::SourceAudioQueue queue;
};
Snapshot snapshot(const l3::ImportedActorInstance& actor, const l3::SourceAudioQueue& queue) {
    return {*actor.source_behavior, *actor.pose, *actor.animation_cursor, actor.source_origin,
            actor.source_contact_velocity, actor.heading, actor.source_contact_state74,
            actor.pose_selected_by_retail_state, queue};
}
void require_held(const l3::ImportedActorInstance& actor, const l3::SourceAudioQueue& queue,
                  const Snapshot& before, const char* message) {
    // Diagnostic last-status/counter fields may change on a hold; source-used
    // work, contact, presentation, queue bytes and producer pointer must not.
    require(same_state(*actor.source_behavior, before.state) &&
            same_pose(*actor.pose, before.pose) && same_cursor(*actor.animation_cursor, before.cursor) &&
            actor.source_origin == before.origin && actor.source_contact_velocity == before.velocity &&
            actor.heading == before.heading && actor.source_contact_state74 == before.state74 &&
            actor.pose_selected_by_retail_state == before.pose_selected && same_queue(queue, before.queue), message);
}
uint32_t word(const l3::SourceAudioQueueRecord& record, size_t offset, unsigned width) {
    uint32_t out = 0;
    for (unsigned i = 0; i != width; ++i) out |= uint32_t(record.raw[offset + i]) << (8 * i);
    return out;
}
void require_sound(const l3::SourceAudioQueueRecord& record, uint8_t type, uint16_t id,
                   std::array<uint32_t, 2> words) {
    require(record.raw[0] == type && word(record, 2, 2) == id &&
            word(record, 4, 4) == words[0] && word(record, 8, 4) == words[1], "producer payload differs");
    require(record.raw[1] == 0xa5, "producer overwrote reserved byte1");
    for (size_t i = 12; i != 20; ++i) require(record.raw[i] == 0xa5, "producer overwrote reserved tail");
}

// Phase1's helper outputs are explicit authored external premises here. Their
// original-instruction proofs and actual-map preparation are separate tests.
l3::ImportedActorDetectionPreparation prepare_detection(l3::ImportedActorInstance& actor,
                                                         const l3::ReferenceTables& reference) {
    auto& state = *actor.source_behavior;
    state.behavior.parameter_03 = 0;
    state.detection_profile_352 = 9; state.detection_phase_353 = 1;
    state.detection_origin_34c = {{state.raw_14[0], state.raw_14[1], state.raw_14[2]}};
    state.detection_vector_354 = {{0, 0, 0}};
    state.detection_countdown_35a = 2; state.detection_offset_35b = -6;
    l3::ActorDetectionInputs inputs;
    inputs.actor_raw = state.detection_origin_34c; inputs.player_raw = inputs.actor_raw;
    inputs.heading56 = state.behavior.heading_56;
    inputs.profile = l3::ActorDetectionProfile{7, 8, 9}; inputs.reference = &reference;
    inputs.phase1 = l3::ActorDetectionPhase1{};
    inputs.global_800bbd96 = 0; inputs.scratch_1f800075 = 2;
    auto preparation = l3::prepare_imported_actor_detection(actor, inputs);
    require(preparation.result.status == l3::ActorDetectionStatus::SourceBackedPartial &&
            preparation.result.sound156_intent && preparation.result.detected &&
            preparation.after.phase_353 == 2, "authored original detector preparation failed");
    return preparation;
}

void pointed_sound_and_queue_tests(const l3::RuntimeMap& map, const l3::ReferenceTables& reference) {
    auto inputs = authored_inputs();
    inputs.sound_consumer_ready = true; // Caller readiness must not bypass queue ownership.
    auto actor = authored_actor(); actor.source_behavior->behavior.action_09 = 2;
    actor.animation_cursor->opaque_a6 = 1;
    auto queue = authored_queue(); const auto pre_raw = actor.source_behavior->raw_14;
    const auto pre_fixed = actor.source_behavior->fixed_1c;
    const auto movement = l3::prepare_imported_actor_action2_contact(actor, map, 0, reference);
    require(movement.inputs.has_value(), "action2 actual contact not prepared");
    auto result = l3::tick_imported_actor_state1(actor, inputs, &movement, nullptr, &queue);
    require(result.committed && result.source_audio_queued == 1 && queue.count == 1,
            "action2 producer did not commit");
    require(actor.source_behavior->raw_14[2] != pre_raw[2], "authored action2 did not move");
    require_sound(queue.records[0], 1, 176, {{0xffff0013u, 0xaaaa0025u}});
    require(actor.source_behavior->previous_raw_2c == pre_raw &&
            actor.source_behavior->previous_fixed_34 == pre_fixed &&
            actor.source_behavior->work_15e == 16912, "outer source snapshot stores omitted");

    for (unsigned mode = 0; mode != 4; ++mode) {
        actor = authored_actor(); actor.source_behavior->behavior.action_09 = 2;
        actor.animation_cursor->opaque_a6 = 1;
        queue = authored_queue(mode == 0 ? l3::kSourceAudioQueueCapacity : 0);
        if (mode == 1) queue.initialized = false;
        if (mode == 2) queue.count = l3::kSourceAudioQueueCapacity + 1;
        const auto prepared = l3::prepare_imported_actor_action2_contact(actor, map, 0, reference);
        if (mode == 3) actor.animation_bank.reset();
        const auto before = snapshot(actor, queue);
        result = l3::tick_imported_actor_state1(actor, inputs, &prepared, nullptr, &queue);
        if (mode == 0) {
            require(result.committed && result.source_audio_queued == 0 && result.source_audio_dropped == 1,
                    "full original queue stopped the body");
            require(actor.source_behavior->raw_14 != before.state.raw_14 && same_queue(queue, before.queue),
                    "full queue modified bytes or suppressed movement");
        } else {
            require(!result.committed && result.status == (mode == 3 ?
                    l3::Class20_03CallbackStatus::NeedsAnimationConsumer :
                    l3::Class20_03CallbackStatus::NeedsSoundConsumer), "missing queue/bank admitted");
            require_held(actor, queue, before, "failed sound/body/animation transaction partly committed");
        }
    }

    actor = authored_actor(2); actor.source_behavior->behavior.action_09 = 3;
    actor.source_behavior->behavior.phase_0a = 2; actor.source_behavior->behavior.timer_32c = 1;
    queue = authored_queue();
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue);
    require(result.committed && actor.source_behavior->behavior.phase_0a == 3 &&
            actor.source_behavior->behavior.timer_32c == 45 && queue.count == 1,
            "source180 phase2 transition did not commit");
    require_sound(queue.records[0], 1, 180, {{0xffff0013u, 0xaaaa0025u}});

    actor = authored_actor(); actor.source_behavior->behavior.action_09 = 2;
    actor.animation_cursor->opaque_a6 = 1; queue = authored_queue();
    const auto stale_cue = l3::prepare_imported_actor_action2_contact(actor, map, 0, reference);
    actor.animation_cursor->opaque_a6 = 0;
    const auto before_cue = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, &stale_cue, nullptr, &queue);
    require(!result.committed && result.status == l3::Class20_03CallbackStatus::NeedsAction2Motion,
            "stale action2 sound cue reused");
    require_held(actor, queue, before_cue, "stale action2 cue published sound/body");

    for (uint8_t cue : {uint8_t(0x80), uint8_t(0xff)}) {
        actor = authored_actor(); actor.source_behavior->behavior.action_09 = 2;
        actor.animation_cursor->opaque_a6 = cue; queue = authored_queue();
        const auto signed_cue = l3::prepare_imported_actor_action2_contact(actor, map, 0, reference);
        result = l3::tick_imported_actor_state1(actor, inputs, &signed_cue, nullptr, &queue);
        require(result.committed && result.source_audio_queued == 0 && queue.count == 0,
                "same signed/raw source cue rejected or emitted sound");
    }
}

void detector_transaction_tests(const l3::ReferenceTables& reference) {
    auto inputs = authored_inputs();
    auto actor = authored_actor(2); actor.source_behavior->behavior.action_09 = 3;
    actor.source_behavior->behavior.phase_0a = 2; actor.source_behavior->behavior.timer_32c = 1;
    auto detection = prepare_detection(actor, reference);
    auto queue = authored_queue();
    auto result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
    require(result.committed && result.source_audio_queued == 2 && queue.count == 2 &&
            actor.source_behavior->detection_phase_353 == 2 &&
            actor.source_behavior->detection_countdown_35a == 8,
            "detector/body producer order failed");
    require_sound(queue.records[0], 0, 156, {{0, 0}});
    require_sound(queue.records[1], 1, 180, {{0xffff0013u, 0xaaaa0025u}});

    for (unsigned mode = 0; mode != 5; ++mode) {
        actor = authored_actor(2); actor.source_behavior->behavior.action_09 = 3;
        actor.source_behavior->behavior.phase_0a = 2; actor.source_behavior->behavior.timer_32c = 1;
        detection = prepare_detection(actor, reference); queue = authored_queue();
        if (mode == 0) actor.animation_bank.reset();
        if (mode == 1) ++actor.source_behavior->detection_countdown_35a;
        if (mode == 2) queue.initialized = false;
        if (mode == 3) queue.count = l3::kSourceAudioQueueCapacity + 1;
        if (mode == 4) queue.count = l3::kSourceAudioQueueCapacity - 1;
        const auto before = snapshot(actor, queue);
        result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
        if (mode == 4) {
            require(result.committed && result.source_audio_queued == 1 && result.source_audio_dropped == 1 &&
                    queue.count == l3::kSourceAudioQueueCapacity, "last queue slot failed ordered drop");
            require_sound(queue.records[l3::kSourceAudioQueueCapacity - 1], 0, 156, {{0, 0}});
        } else {
            require(!result.committed, "invalid detector/consumer transaction admitted");
            require_held(actor, queue, before, "detector or early queued sound published on later hold");
        }
    }

    // Phase2's global notification publishes only with the body/pose commit.
    actor = authored_actor(); actor.source_behavior->behavior.phase_0a = 1;
    actor.source_behavior->behavior.timer_32c = 2;
    (void)prepare_detection(actor, reference); actor.source_behavior->detection_phase_353 = 2;
    l3::ActorDetectionInputs d;
    d.actor_raw = actor.source_behavior->detection_origin_34c; d.player_raw = d.actor_raw;
    d.profile = l3::ActorDetectionProfile{7, 8, 9}; d.reference = &reference; d.global_800bbd96 = 0;
    detection = l3::prepare_imported_actor_detection(actor, d); queue = authored_queue();
    require(detection.result.notify_global_800bbd84, "authored notify path not reached");
    auto bank = actor.animation_bank; actor.animation_bank.reset();
    const auto before = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
    require(!result.committed && !result.notify_detection_global, "global notification escaped failed transaction");
    require_held(actor, queue, before, "notification hold mutated body");
    actor.animation_bank = bank;
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
    require(result.committed && result.notify_detection_global, "committed detector omitted source notification");

    actor = authored_actor(); actor.source_behavior->behavior.parameter_03 = 0;
    queue = authored_queue();
    inputs.detection_8003648c = false; inputs.detection_range_byte2 = 0;
    const auto spoofed_before = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue);
    require(!result.committed && result.status == l3::Class20_03CallbackStatus::NeedsDetection,
            "caller detection premise bypassed trusted preparation");
    require_held(actor, queue, spoofed_before, "caller detection premise partly committed");

    inputs = authored_inputs(); inputs.action.packet_33c = 1;
    actor = authored_actor(2); actor.source_behavior->behavior.action_09 = 3;
    actor.source_behavior->behavior.phase_0a = 2; actor.source_behavior->behavior.timer_32c = 1;
    detection = prepare_detection(actor, reference); queue = authored_queue();
    const auto packet_before = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
    require(!result.committed && result.status == l3::Class20_03CallbackStatus::NeedsPacket,
            "nonzero packet escaped excluded finalizer gate");
    require_held(actor, queue, packet_before, "packet hold published early detector156");
}

l3::ImportedActorInstance moving_action3() {
    auto actor = authored_actor(2);
    auto& state = *actor.source_behavior;
    state.behavior.action_09 = 3; state.behavior.phase_0a = 3;
    state.behavior.timer_32c = 4; state.work_64 = 0;
    return actor;
}
void action3_transaction_tests(const l3::RuntimeMap& map, const l3::ReferenceTables& reference) {
    auto inputs = authored_inputs(); auto actor = moving_action3(); auto queue = authored_queue();
    auto prepared = l3::prepare_imported_actor_action3_contact(actor, map, 0, reference, 0);
    require(prepared.inputs.has_value(), "native action3 contact bridge not prepared");
    const auto before_raw = actor.source_behavior->raw_14;
    const auto before_fixed = actor.source_behavior->fixed_1c;
    auto result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue, &prepared);
    require(result.committed && actor.source_behavior->raw_14[2] == -11 &&
            actor.source_behavior->fixed_1c[2] == 37 * 65536 - 768 * 4096 &&
            actor.source_behavior->previous_raw_2c == before_raw &&
            actor.source_behavior->previous_fixed_34 == before_fixed, "action3 source motion/snapshot mismatch");

    // Every mutable premise admitted by the bridge must still match at commit.
    for (unsigned mode = 0; mode != 13; ++mode) {
        actor = moving_action3(); queue = authored_queue();
        prepared = l3::prepare_imported_actor_action3_contact(actor, map, 0, reference, 0);
        auto& state = *actor.source_behavior;
        if (mode == 0) ++state.raw_14[3];
        if (mode == 1) ++state.fixed_1c[3];
        if (mode == 2) ++state.behavior.timer_32c;
        if (mode == 3) state.behavior.phase_0a = 4;
        if (mode == 4) state.work_64 = 16;
        if (mode == 5) ++state.behavior.heading_56;
        if (mode == 6) ++actor.source_contact_velocity[0];
        if (mode == 7) ++actor.source_contact_state74;
        if (mode == 8) prepared.target_heading = 1;
        if (mode == 9) prepared.initial_packet = 1;
        if (mode == 10) ++prepared.before.previous_raw[3];
        if (mode == 11) ++prepared.before.previous_fixed[3];
        if (mode == 12) prepared.before.object_class = 0x10;
        const auto before = snapshot(actor, queue);
        result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue, &prepared);
        require(!result.committed && result.status == l3::Class20_03CallbackStatus::NeedsAction3Motion,
                "stale action3 signature reused");
        require_held(actor, queue, before, "stale action3 changed body/pose/queue");
    }
    actor = moving_action3(); queue = authored_queue();
    prepared = l3::prepare_imported_actor_action3_contact(actor, map, 0, reference, 0);
    actor.animation_bank.reset();
    auto before = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue, &prepared);
    require(!result.committed && result.status == l3::Class20_03CallbackStatus::NeedsAnimationConsumer,
            "action3 moved without own animation consumer");
    require_held(actor, queue, before, "missing action3 animation partly published contact");

    actor = moving_action3(); queue = authored_queue(); before = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue);
    require(!result.committed && result.status == l3::Class20_03CallbackStatus::NeedsAction3Motion,
            "missing action3 motion was fabricated");
    require_held(actor, queue, before, "missing action3 motion mutated body");

    // Expiration reaches the original request3 pointer bank, without assigning
    // animation semantics or pretending to execute the excluded pool release.
    actor = moving_action3(); actor.source_behavior->behavior.timer_32c = 0;
    auto detection = prepare_detection(actor, reference);
    queue = authored_queue();
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
    require(result.committed && result.request_changed && actor.pose->request == 3 &&
            actor.animation_cursor->record_index == 0 && actor.source_behavior->behavior.phase_0a == 5 &&
            actor.source_behavior->behavior.timer_32c == 9 &&
            actor.pose->base_translation[0][0] == 3, "action3 request3 handoff was not wired");
    actor = moving_action3(); actor.source_behavior->behavior.timer_32c = 0;
    detection = prepare_detection(actor, reference);
    auto missing = authored_bank(); missing.requests.pop_back();
    actor.animation_bank = std::make_shared<const l3::ActorAnimationBank>(std::move(missing));
    queue = authored_queue(); before = snapshot(actor, queue);
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, &detection, &queue);
    require(!result.committed && result.animation_status == l3::ActorAnimationStatus::MissingRequest,
            "request3 was silently substituted");
    require_held(actor, queue, before, "missing request3 partly advanced action/pose");

    for (bool caller_ready : {false, true}) {
        actor = moving_action3(); actor.source_behavior->behavior.phase_0a = 5;
        actor.source_behavior->behavior.timer_32c = 2;
        inputs.release_consumer_ready = caller_ready; queue = authored_queue();
        before = snapshot(actor, queue);
        result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue);
        // The state1 transaction owns the 0x80031824 release (source_released),
        // whatever the caller passes.
        require(result.committed && result.released_80031824 && actor.source_released &&
                actor.source_behavior->work_00 == 0 && !actor.source_proxy_bound,
                "action3 phase5 pool release was not committed");
        (void)before;
    }
    inputs.release_consumer_ready = false;

    actor = moving_action3(); inputs.detection_8003648c.reset();
    actor.source_behavior->raw_14[1] = -100; actor.source_behavior->fixed_1c[1] = -100 * 65536;
    actor.source_origin[1] = -100; queue = authored_queue(); before = snapshot(actor, queue);
    prepared = l3::prepare_imported_actor_action3_contact(actor, map, 0, reference, 0);
    require(prepared.inputs.has_value() && prepared.floor.result == -1,
            "authored rollback actual floor query unsupported");
    result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue, &prepared);
    require(result.committed && actor.source_behavior->behavior.action_09 == 8 &&
            actor.source_behavior->behavior.timer_32c == -1 &&
            (actor.source_behavior->behavior.parameter_03 & 0x40) &&
            actor.source_behavior->raw_14 == before.state.raw_14 &&
            actor.source_behavior->fixed_1c == before.state.fixed_1c &&
            actor.source_behavior->previous_raw_2c == before.state.raw_14 &&
            actor.source_behavior->previous_fixed_34 == before.state.fixed_1c,
            "actual contact rollback did not restore all four current snapshot lanes");
}

void original_shadow_tail_fixture(){
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/actor_shadow_tail_01442.tsv";
    std::ifstream in(path);require(bool(in),"original shadow-tail fixture missing");
    unsigned work00,count,heading,after,x,y,z,heading_scale,radius_type,height_delta;
    int work76,raw_x,raw_y,raw_z,floor;size_t cases=0;
    while(in>>work00>>count>>work76>>heading>>raw_x>>raw_y>>raw_z>>floor>>after>>x>>y>>z>>heading_scale>>radius_type>>height_delta){
        l3::SourceShadowWork work;work.raw={{int16_t(raw_x),int16_t(raw_y),int16_t(raw_z)}};
        work.heading56=uint16_t(heading);work.work76=int8_t(work76);
        const auto before=allocations;measure_allocations=true;
        const auto plan=work00&2u?l3::plan_source_shadow_8002c8cc(count,work,48,512,0,floor):l3::SourceShadowPlan{};
        measure_allocations=false;
        require(plan.status==l3::SourceConsumerStatus::SourceBackedPartial&&allocations==before,
            "original shadow tail planning held or allocated");
        require(count+unsigned(plan.append.has_value())==after,"original shadow queue count mismatch");
        if(plan.append){const auto&r=*plan.append;require(uint16_t(r.point[0])==x&&uint16_t(r.point[1])==y&&
            uint16_t(r.point[2])==z&&r.heading_scale==heading_scale&&r.radius_type==radius_type&&
            r.height_delta==height_delta,"original class03 shadow row differs");}
        ++cases;
    }
    require(in.eof()&&cases==128,"original shadow-tail fixture count");
    std::cout<<cases<<" unchanged-original class03 shadow call/queue cases PASS; authored floor-return premise\n";
}

void proxy_packet_transaction_tests(const l3::RuntimeMap& map,const l3::ReferenceTables& reference){
    // Execute the existing actual parent producer and child callback so the
    // incoming 08000000 packet comes from the real shared native hit bank.
    // Coordinates/profile fields remain explicit authored premises.
    l3::ChildRuntime children;auto queue=authored_queue();
    l3::Class20_03CallbackState parent;parent.behavior.action_09=4;parent.behavior.phase_0a=2;
    parent.behavior.timer_32c=1;parent.behavior.animation_ac=21;
    parent.raw_14={{0,-1,110,0}};parent.fixed_1c={{0,-65536,110*65536,0}};
    l3::ParentChildDelivery delivery;
    require(children.prepare_parent_delivery(parent,reference,queue,delivery)==l3::ChildRuntimeStatus::SourceBackedPartial,
        "actual child producer did not prepare");
    const auto identity=delivery.source_identity();queue=delivery.staged_audio();
    require(children.commit_parent_delivery(delivery),"actual child producer did not commit");
    l3::Class60_06Profile child_profile;
    l3::ChildFrameContext child;child.map=&map;child.references=&reference;child.profile=&child_profile;
    child.proxy_mask_80098798=0xff;child.player_heading_800b5206=0;child.audio=&queue;
    require(children.tick(identity,child).committed&&children.hit_proxy_bank().count==1,
        "actual child callback did not register proxy");
    const auto child_point=children.hit_proxy_bank().records[0].point;
    auto actor=authored_actor();actor.raw_scale={{512,512,512}};
    auto bank_with_root=authored_bank();bank_with_root.requests[0].sequence.poses[1].root_translation={{32,-8,16}};
    actor.animation_bank=std::make_shared<const l3::ActorAnimationBank>(std::move(bank_with_root));
    actor.source_behavior->work_00=1;actor.source_behavior->behavior.flags_328=6;
    actor.source_behavior->health_6e=64;
    for(size_t axis=0;axis<3;++axis){actor.source_behavior->raw_14[axis]=child_point[axis];
        actor.source_behavior->fixed_1c[axis]=int32_t(child_point[axis])*65536;actor.source_origin[axis]=child_point[axis];}
    require(l3::initialize_imported_actor_fresh_pose_proxy_subset(actor),"fresh proxy owner refused");
    l3::ImportedActorProxyTailContext tail;tail.references=&reference;tail.bank=&children.hit_proxy_bank();tail.mask_80098798=0xff;
    auto input=authored_inputs();
    const auto run=[&](){return l3::tick_imported_actor_state1(actor,input,nullptr,nullptr,&queue,
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&tail);};
    auto tick=run();require(tick.committed&&tick.proxy_finalized&&tick.proxy_registered&&tail.bank->count==2,
        "source body/pose did not publish actual actor proxy");
    l3::SourceHitProxyRules rules;rules.unsigned_80097d00.fill(255);
    const auto pairs=children.consume_proxy_pairs(rules);
    require(pairs.overlapping_pairs==1&&actor.source_proxy_target.packet_00==0x08000000u,
        "actual child packet did not reach imported actor owner");
    const auto actor_before=*actor.source_behavior;const auto pose_before=*actor.pose;
    const auto cursor_before=*actor.animation_cursor;const auto queue_before=queue;
    // A missing later mask must preserve the incoming packet, HP, source timer,
    // pose, audio and empty bank. Retrying cannot apply any earlier hit twice.
    tail.mask_80098798.reset();const auto held=run();
    require(!held.committed&&held.status==l3::Class20_03CallbackStatus::NeedsProxyConsumer&&
        held.proxy_status==l3::Class20_03ProxyStatus::NeedsMask,"missing exact tail mask admitted");
    require(same_state(*actor.source_behavior,actor_before)&&same_pose(*actor.pose,pose_before)&&
        same_cursor(*actor.animation_cursor,cursor_before)&&same_queue(queue,queue_before)&&
        tail.bank->count==0&&actor.source_proxy_target.packet_00==0x08000000u,
        "held atomic packet transaction partially published");
    tail.mask_80098798=0xff;const auto alloc_before=allocations;measure_allocations=true;tick=run();measure_allocations=false;
    require(tick.committed&&tick.proxy_finalized&&tick.proxy_registered&&allocations==alloc_before&&
        actor.source_proxy_target.packet_00==0&&actor.source_behavior->packet_33c==0&&tail.bank->count==1,
        "actual child packet froze actor or cleared before full commit");
    require(actor.source_behavior->health_6e==64&&actor.source_behavior->behavior.timer_32c==actor_before.behavior.timer_32c-1,
        "zero-low24 child packet invented damage or skipped ordinary body");
    l3::ActorPoseCollisionInputs selected_pose;selected_pose.reference=&reference;
    selected_pose.heading_56=actor.source_behavior->behavior.heading_56;
    selected_pose.raw_14=child_point;selected_pose.scale_158=actor.raw_scale;
    selected_pose.root_translation=actor.pose->root_translation;selected_pose.flags_06=actor.source_dispatch_flag6;
    selected_pose.scratch_shift_1f800074=actor.source_pose_scratch_shift;
    selected_pose.preserved_186=actor.source_pose_preserved_186;
    const auto selected_point=l3::produce_actor_pose_collision_point_8003346c(selected_pose);
    require(actor.pose->record_index==1&&actor.pose->root_translation==std::array<int16_t,3>{{32,-8,16}}&&
        selected_point.point_180&&actor.source_behavior->proxy_position_180==*selected_point.point_180,
        "proxy used stale pre-animation point instead of selected resident pose");
    // Explicit authored damage packet exercises the already-proved hit reducer
    // through this owning transaction. This is not a genuine player firing.
    tail.bank->count=0;actor.source_proxy_target.packet_00=0x08000001u;
    actor.source_combat_byte09=1;actor.source_behavior->health_6e=64;
    const auto damage_before=snapshot(actor,queue);tail.mask_80098798.reset();tick=run();
    require(!tick.committed&&actor.source_proxy_target.packet_00==0x08000001u,
        "damage packet was dropped on tail hold");
    require_held(actor,queue,damage_before,"tail hold published HP/audio/animation before packet clear");
    tail.mask_80098798=0xff;tick=run();
    require(tick.committed&&tick.proxy_finalized&&actor.source_behavior->health_6e==63&&
        actor.source_behavior->previous_health_b2==64&&actor.source_proxy_target.packet_00==0&&
        queue.count==damage_before.queue.count+1,"admitted damage/audio/finalizer did not commit together");
    tail.bank->count=0;tick=run();require(tick.committed&&actor.source_behavior->health_6e==63,
        "cleared packet reapplied damage on next callback");
    // A real mask lease requires its actual BC/C0 point owner. Missing that
    // owner is checked before any hit/body/pose/audio publication.
    tail.bank->count=0;actor.source_proxy_target.packet_00=0x08000000u;
    actor.source_proxy_target.flags_04=0x80000000u;tail.mask_80098798=0x800000ffu;
    const auto lease_before=snapshot(actor,queue);tick=run();
    require(!tick.committed&&tick.proxy_status==l3::Class20_03ProxyStatus::NeedsOwnerPoint,
        "missing leased proxy point owner accepted");
    require_held(actor,queue,lease_before,"missing lease owner partly published transaction");
    l3::SourceHitProxyOwnerPoint leased_point;tail.owners_800be478[0]=&leased_point;tick=run();
    require(tick.committed&&tick.proxy_finalized&&leased_point.raw_bc_words!=std::array<uint32_t,2>{},
        "admitted leased point did not publish with body");
    tail.mask_80098798=0xff;tail.owners_800be478[0]=nullptr;
    // The old separate API remains unable to bypass a held packet, and an
    // excluded shadow branch remains explicit instead of claiming completion.
    actor.source_proxy_target.packet_00=0x08000000u;actor.source_behavior->work_00=3;
    const auto shadow_before=snapshot(actor,queue);tick=run();
    require(!tick.committed&&tick.status==l3::Class20_03CallbackStatus::NeedsEffectConsumer,
        "unowned shadow/effect branch bypassed");
    require_held(actor,queue,shadow_before,"held shadow branch published body");
    actor.source_behavior->work_00=1;actor.source_proxy_target.packet_00=0x08000001u;
    actor.source_behavior->health_6e.reset();const auto hp_before=snapshot(actor,queue);tick=run();
    require(!tick.committed&&tick.status==l3::Class20_03CallbackStatus::NeedsHealth,
        "unowned HP invented");require_held(actor,queue,hp_before,"held HP branch published");
    // The initializer's real work00=3 shadow branch now joins the same
    // source-owned transaction. Floor comes from the actual bounded map query.
    l3::SourceShadowQueue shadows;tail.shadows=&shadows;tail.map=&map;tail.area_index=0;
    actor.source_behavior->work_00=3;actor.source_behavior->health_6e=64;
    actor.source_proxy_target.packet_00=0x08000001u;tail.bank->count=0;
    actor.source_shadow_work76.reset();const auto shadow_unknown=snapshot(actor,queue);tick=run();
    require(!tick.committed&&tick.status==l3::Class20_03CallbackStatus::NeedsEffectConsumer&&shadows.count==0,
        "unowned source shadow work76 invented");require_held(actor,queue,shadow_unknown,"work76 hold partly published");
    actor.source_shadow_work76=0;tail.mask_80098798.reset();const auto shadow_mask=snapshot(actor,queue);tick=run();
    require(!tick.committed&&shadows.count==0&&actor.source_proxy_target.packet_00==0x08000001u,
        "shadow appended before later proxy check");require_held(actor,queue,shadow_mask,"tail failure partly published shadow body");
    tail.mask_80098798=0xff;tick=run();
    require(tick.committed&&tick.shadow_appended&&tick.proxy_finalized&&shadows.count==1&&
        actor.source_behavior->health_6e==63&&actor.source_proxy_target.packet_00==0,
        "real shadow-enabled body/HP/pose/proxy transaction did not commit");
    require(shadows.records[0].radius_type==48&&
        (shadows.records[0].heading_scale>>12)==8&&shadows.records[0].point[1]==-1,
        "class03 exact48/512/type0 or actual floor packing changed");
    // Original full queue drops before reading work76/floor. It must not turn
    // an otherwise owned callback into an invented missing-floor failure.
    shadows.count=64;actor.source_shadow_work76.reset();tail.map=nullptr;
    actor.source_proxy_target.packet_00=0x08000000u;tail.bank->count=0;tick=run();
    require(tick.committed&&tick.proxy_finalized&&!tick.shadow_appended&&shadows.count==64,
        "full source shadow queue queried missing work76/floor");
    actor.source_behavior->work_00=1;tail.shadows=nullptr;
    // Full-bank source return still completes the outer packet clear.
    actor.source_proxy_target.packet_00=0x08000000u;actor.source_behavior->health_6e=64;
    tail.bank->count=tail.bank->records.size();tick=run();
    require(tick.committed&&tick.proxy_finalized&&tick.proxy_bank_full&&!tick.proxy_registered&&
        actor.source_proxy_target.packet_00==0&&tail.bank->count==tail.bank->records.size(),
        "source full-bank continuation changed");
    std::cout<<"Actual parent/child callback -> shared proxy pair -> imported actor body/pose/tail PASS; "
        "packet holds atomic, no invented HP or shadow ownership, no hot-loop allocation\n";
}

void allocation_test(const l3::RuntimeMap& map, const l3::ReferenceTables& reference) {
    auto actor = moving_action3(); auto queue = authored_queue(); const auto inputs = authored_inputs();
    const auto initial_state = *actor.source_behavior;
    const auto initial_cursor = *actor.animation_cursor;
    const auto origin = actor.source_origin;
    const auto* pose_storage = actor.pose->base_translation.data();
    for (unsigned i = 0; i != 256; ++i) {
        *actor.source_behavior = initial_state; actor.source_origin = origin;
        *actor.animation_cursor = initial_cursor; l3::initialize_source_audio_queue_80019e14(queue);
        measure_allocations = true;
        const auto prepared = l3::prepare_imported_actor_action3_contact(actor, map, 0, reference, 0);
        const auto result = l3::tick_imported_actor_state1(actor, inputs, nullptr, nullptr, &queue, &prepared);
        measure_allocations = false;
        require(result.committed, "allocation loop held supported action3");
    }
    require(allocations == 0 && actor.pose->base_translation.data() == pose_storage,
            "contact/body/audio/pose transaction allocated or replaced pose storage");
}
} // namespace

int main() {
    try {
        const auto map = authored_flat_map(); const auto reference = authored_reference();
        pointed_sound_and_queue_tests(map, reference);
        detector_transaction_tests(reference);
        action3_transaction_tests(map, reference);
        original_shadow_tail_fixture();
        proxy_packet_transaction_tests(map,reference);
        allocation_test(map, reference);
        std::cout << "Actor source transaction: pre-motion four-lane sounds, staged detector/queue, full drops, "
                     "atomic consumer/stale holds, request3 and actual contact rollback PASS; "
                     "256 allocation-free prepare/commit ticks; authored premises, not retail parity\n";
    } catch (const std::exception& error) {
        measure_allocations = false; std::cerr << error.what() << '\n'; return 1;
    }
}
