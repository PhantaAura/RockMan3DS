#include "legends3ds/actor_runtime.h"
#include <cstdlib>
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

// Authored scalar bank shared in shape with the inherited transaction tests. No retail
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

constexpr uint32_t work80=0x12000;
l3::Class20_03CallbackInputs inputs9() {
    auto input=authored_inputs();input.action.work_80=work80;
    input.angle_player_to_actor_80031198=0;input.player_heading_800b5206=0;
    return input;
}
l3::ImportedActorInstance actor9() {
    auto actor=authored_actor();auto& state=*actor.source_behavior;
    state.behavior.parameter_03=0;state.behavior.action_09=9;state.behavior.phase_0a=0;
    state.behavior.flags_328=0;state.behavior.heading_56=0;
    state.detection_profile_352=9;state.detection_phase_353=2;
    state.detection_origin_34c={{state.raw_14[0],state.raw_14[1],state.raw_14[2]}};
    state.detection_countdown_35a=8;state.detection_offset_35b=-6;
    state.work_35c=30;state.work_35e=30;
    actor.animation_cursor->opaque_a6=1;return actor;
}
l3::ImportedActorDetectionPreparation detector9(const l3::ImportedActorInstance& actor,
                                                const l3::ReferenceTables& refs,
                                                uint32_t distance=work80) {
    const auto& state=*actor.source_behavior;l3::ActorDetectionInputs input;
    input.distance_work80=distance;input.actor_raw={{state.raw_14[0],state.raw_14[1],state.raw_14[2]}};
    input.player_raw=input.actor_raw;input.heading56=state.behavior.heading_56;
    input.profile=l3::ActorDetectionProfile{7,8,9};input.reference=&refs;
    input.phase1=l3::ActorDetectionPhase1{};input.global_800bbd96=0;input.scratch_1f800075=2;
    const auto prepared=l3::prepare_imported_actor_detection(actor,input);
    require(prepared.result.status==l3::ActorDetectionStatus::SourceBackedPartial && prepared.result.detected,
        "authored phase2 source detector not admitted");return prepared;
}
l3::ImportedActorBehaviorTick tick9(l3::ImportedActorInstance& actor,const l3::Class20_03CallbackInputs& input,
    const l3::ImportedActorDetectionPreparation* detector,l3::SourceAudioQueue* queue,
    const l3::ActorAction9ContactPreparation* motion) {
    return l3::tick_imported_actor_state1(actor,input,nullptr,detector,queue,nullptr,motion);
}

void commit_and_sound_order(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    auto actor=actor9();actor.source_behavior->raw_14[1]=0;actor.source_behavior->fixed_1c[1]=0;
    actor.source_origin[1]=0;auto queue=authored_queue();auto input=inputs9();
    const auto detection=detector9(actor,refs);
    const auto movement=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,detection.result.detected);
    require(movement.inputs && movement.after_wall.raw[1]==-1 && movement.after_wall.raw[2]!=37,
        "actual source motion52/postwall point not produced");
    const auto old_raw=actor.source_behavior->raw_14;const auto old_fixed=actor.source_behavior->fixed_1c;
    const auto result=tick9(actor,input,&detection,&queue,&movement);
    require(result.committed && result.request_changed && actor.pose->request==1 &&
        actor.animation_cursor->record_index==0 && actor.pose->base_translation[0][0]==1,
        "source action9 request1/pose handoff not committed");
    require(actor.source_behavior->behavior.action_09==9 && actor.source_behavior->behavior.phase_0a==1 &&
        actor.source_behavior->behavior.timer_32c==90 && actor.source_behavior->work_35c==29 &&
        actor.source_behavior->previous_raw_2c==old_raw && actor.source_behavior->previous_fixed_34==old_fixed,
        "source action9 phase0 fallthrough or snapshot fields differ");
    require(actor.source_behavior->raw_14==movement.after_wall.raw &&
        actor.source_contact_state74==movement.after_wall.state74 && actor.source_contact_velocity==movement.after_wall.velocity,
        "source contact/presentation did not publish with pose");
    require(result.source_audio_queued==1 && queue.count==1 && result.notify_detection_global,
        "source176/detector notification did not commit");
    require_sound(queue.records[0],1,176,l3::actor_source_sound_position_words(movement.after_wall.raw));
    require(word(queue.records[0],4,4)!=l3::actor_source_sound_position_words(old_raw)[0],
        "action9 sound incorrectly used pre-wall point");

    // A later floor gap rolls back all lanes, but176 still retains the source
    // post-wall point copied before floor stores/rollback.
    actor=actor9();actor.source_behavior->raw_14[1]=-100;
    actor.source_behavior->fixed_1c[1]=-100*65536;actor.source_origin[1]=-100;
    queue=authored_queue();const auto rollback_detection=detector9(actor,refs);
    const auto rollback=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
    require(rollback.inputs && rollback.floor.result==-1 && rollback.after_wall.raw[1]==-100,
        "source rollback floor gap not prepared");
    const auto before=snapshot(actor,queue);const auto rolled=tick9(actor,input,&rollback_detection,&queue,&rollback);
    require(rolled.committed && actor.source_behavior->behavior.action_09==1 &&
        actor.source_behavior->restore_action_0e==4 && actor.source_behavior->raw_14==before.state.raw_14 &&
        actor.source_behavior->fixed_1c==before.state.fixed_1c,"source rollback lost raw/fixed fourth lanes");
    require_sound(queue.records[0],1,176,l3::actor_source_sound_position_words(rollback.after_wall.raw));
    require(word(queue.records[0],8,4)!=l3::actor_source_sound_position_words(actor.source_behavior->raw_14)[1],
        "source sound copied after rollback instead of before floor stores");
}

void motion_signature_holds(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    for(unsigned mode=0;mode<24;++mode) {
        auto actor=actor9();auto queue=authored_queue();auto input=inputs9();
        const auto detection=detector9(actor,refs);
        auto prepared=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
        require(prepared.inputs.has_value(),"source9 motion not prepared for stale test");
        if(mode==0)++prepared.before.raw[3];
        if(mode==1)++prepared.before.fixed[3];
        if(mode==2)++prepared.before.previous_raw[3];
        if(mode==3)++prepared.before.previous_fixed[3];
        if(mode==4)++prepared.before.velocity[0];
        if(mode==5)++prepared.before.state74;
        if(mode==6)prepared.before.object_class=0x10;
        if(mode==7)prepared.initial_action=3;
        if(mode==8)prepared.initial_phase=1;
        if(mode==9)prepared.initial_flags^=0x100;
        if(mode==10)++prepared.initial_heading;
        if(mode==11)prepared.work80=work80+1;
        if(mode==12)prepared.turn_target=1;
        if(mode==13)prepared.source_a6=0;
        if(mode==14)prepared.inputs->source_a6=0;
        if(mode==15)prepared.inputs->heading_after_turn=1;
        if(mode==16)prepared.status=l3::ActorAction9ContactStatus::UnsupportedWall;
        if(mode==17)actor.animation_cursor->opaque_a6=0;
        if(mode==18)prepared.work80.reset();
        if(mode==19)prepared.turn_target.reset();
        if(mode==20)++prepared.inputs->raw_14[3];
        if(mode==21)++prepared.inputs->fixed_1c[3];
        if(mode==22)++prepared.inputs->source_floor_word;
        if(mode==23)prepared.inputs->source_contact_word^=0x100;
        const auto before=snapshot(actor,queue);const auto result=tick9(actor,input,&detection,&queue,&prepared);
        require(!result.committed && result.status==l3::Class20_03CallbackStatus::NeedsAction9Motion,
            "stale action9 motion/cue signature reused");
        require_held(actor,queue,before,"stale action9 moved work,pose or queue");
    }
    auto actor=actor9();auto input=inputs9();auto queue=authored_queue();
    const auto detection=detector9(actor,refs);const auto before=snapshot(actor,queue);
    // Caller-supplied motion has no producer signature and is cleared.
    input.prepared_action9=l3::Class20_03CallbackInputs::Action9Motion{};
    auto result=tick9(actor,input,&detection,&queue,nullptr);
    require(!result.committed && result.status==l3::Class20_03CallbackStatus::NeedsAction9Motion,
        "caller motion bypassed trusted source9 producer");
    require_held(actor,queue,before,"missing source9 producer partly committed");
}

void detector_and_consumer_holds(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    for(unsigned mode=0;mode<12;++mode) {
        auto actor=actor9();auto input=inputs9();auto queue=authored_queue();auto detection=detector9(actor,refs);
        const auto motion=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
        if(mode==0)++detection.before.countdown_35a;
        if(mode==1)detection.actor_raw[0]++;
        if(mode==2)detection.work80++;
        if(mode==3)detection.heading++;
        if(mode==4)actor.animation_bank.reset();
        if(mode==5)queue.initialized=false;
        if(mode==6)queue.count=l3::kSourceAudioQueueCapacity+1;
        if(mode==7)input.angle_player_to_actor_80031198.reset();
        if(mode==8)input.player_heading_800b5206.reset();
        if(mode==9)input.action.packet_33c=1;
        if(mode==10){auto bank=authored_bank();bank.requests.erase(bank.requests.begin()+1);
            actor.animation_bank=std::make_shared<const l3::ActorAnimationBank>(std::move(bank));}
        if(mode==11){auto bank=authored_bank();bank.requests[1].sequence.poses[0].base_translation.push_back({{1,2,3}});
            actor.animation_bank=std::make_shared<const l3::ActorAnimationBank>(std::move(bank));}
        const auto before=snapshot(actor,queue);const auto result=tick9(actor,input,&detection,&queue,&motion);
        require(!result.committed,"missing action9 detector/input/consumer admitted");
        require_held(actor,queue,before,"action9 failed after staging detector/audio/contact partly published");
        if(mode<=3)require(result.status==l3::Class20_03CallbackStatus::NeedsDetection,"stale detector status lost");
        if(mode==5 || mode==6)require(result.status==l3::Class20_03CallbackStatus::NeedsSoundConsumer,
            "source9 sound bypassed queue ownership");
        if(mode==7)require(result.status==l3::Class20_03CallbackStatus::NeedsTargetHeading,"missing reverse angle defaulted");
        if(mode==8)require(result.status==l3::Class20_03CallbackStatus::NeedsPlayerHeading,"player heading defaulted");
    }
    auto actor=actor9();actor.source_behavior->detection_phase_353=1;
    auto input=inputs9();auto queue=authored_queue();const auto detection=detector9(actor,refs);
    require(detection.result.sound156_intent,"authored phase1 did not request source156");
    const auto motion=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
    actor.animation_bank.reset();const auto before=snapshot(actor,queue);
    const auto result=tick9(actor,input,&detection,&queue,&motion);
    require(!result.committed,"staged detector156/body176 escaped missing pose bank");
    require_held(actor,queue,before,"late pose hold published detector156/body176");
}

void full_queue_and_optional_routes(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    auto actor=actor9();auto input=inputs9();auto queue=authored_queue(l3::kSourceAudioQueueCapacity);
    const auto detection=detector9(actor,refs);const auto motion=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
    const auto before=snapshot(actor,queue);auto result=tick9(actor,input,&detection,&queue,&motion);
    require(result.committed && !result.source_audio_queued && result.source_audio_dropped==1 && same_queue(queue,before.queue),
        "full source queue blocked action9 or changed retained records");
    for(uint8_t cue:{uint8_t(0),uint8_t(0x80),uint8_t(0xff)}) {
        actor=actor9();actor.animation_cursor->opaque_a6=cue;queue=authored_queue();
        const auto d=detector9(actor,refs);const auto p=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
        result=tick9(actor,input,&d,&queue,&p);
        require(result.committed && !result.source_audio_queued && !queue.count,"signed cue emitted176");
    }
    actor=actor9();actor.source_behavior->behavior.phase_0a=2;actor.source_behavior->behavior.parameter_03=0x20;
    input=inputs9();input.action.work_80.reset();input.action.target_heading_80031198.reset();
    input.angle_player_to_actor_80031198=4090;input.player_heading_800b5206=20;
    const auto raw=actor.source_behavior->raw_14;queue=authored_queue();
    result=tick9(actor,input,nullptr,nullptr,nullptr);
    require(result.committed && actor.source_behavior->work_36e==4070 && actor.source_behavior->raw_14==raw,
        "nonmoving phase required unused movement/sound dependencies or lost raw angle difference");
}

void allocations9(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    auto actor=actor9();auto queue=authored_queue();const auto input=inputs9();
    const auto state=*actor.source_behavior;const auto cursor=*actor.animation_cursor;const auto origin=actor.source_origin;
    const auto* storage=actor.pose->base_translation.data();
    for(unsigned i=0;i<384;++i) {
        *actor.source_behavior=state;*actor.animation_cursor=cursor;actor.source_origin=origin;
        // Reset the scalar current request inside the existing same-sized pose.
        actor.pose->request=0;actor.pose->sequencing_record=actor.animation_bank->requests[0].sequence.records[0];
        actor.source_contact_velocity={};actor.source_contact_state74=0;
        l3::initialize_source_audio_queue_80019e14(queue);measure_allocations=true;
        const auto detection=detector9(actor,refs);
        const auto prepared=l3::prepare_imported_actor_action9_contact(actor,map,0,refs,work80,0,true);
        const auto result=tick9(actor,input,&detection,&queue,&prepared);measure_allocations=false;
        require(result.committed,"allocation loop held source9 transaction");
    }
    require(allocations==0 && actor.pose->base_translation.data()==storage,"action9 prepare/body/queue/pose allocated");
}
} // namespace
int main() {
    try {
        const auto map=authored_flat_map();const auto refs=authored_reference();
        commit_and_sound_order(map,refs);motion_signature_holds(map,refs);detector_and_consumer_holds(map,refs);
        full_queue_and_optional_routes(map,refs);allocations9(map,refs);
        std::cout<<"PASS source action9 runtime transaction: pose1, motion52/postwall176, rollback, stale/missing holds, full drops;384 allocation-free ticks; authored premises, not retail parity\n";
        return 0;
    }catch(const std::exception& error){measure_allocations=false;std::cerr<<error.what()<<'\n';return 1;}
}
