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
    for (uint8_t id : {uint8_t(0), uint8_t(1), uint8_t(2), uint8_t(3), uint8_t(21), uint8_t(22)}) {
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


l3::ImportedActorInstance actor4(uint8_t phase=0,int16_t timer=0) {
    auto actor=authored_actor();auto& state=*actor.source_behavior;
    state.behavior.action_09=4;state.behavior.phase_0a=phase;state.behavior.timer_32c=timer;
    state.behavior.flags_328=0;state.behavior.heading_56=0;
    state.behavior.animation_ac=phase?21:0;
    state.work_370=0xdeadbeef;state.child_374=0x76543210;
    actor.source_combat_byte_c=200;
    return actor;
}
l3::ImportedActorBehaviorTick tick4(l3::ImportedActorInstance& actor,
    const l3::Class20_03CallbackInputs& input,l3::SourceAudioQueue* queue,
    const l3::ActorAction4ContactPreparation* motion=nullptr) {
    return l3::tick_imported_actor_state1(actor,input,nullptr,nullptr,queue,nullptr,nullptr,motion);
}
void phases(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    auto actor=actor4();auto q=authored_queue();const auto input=authored_inputs();
    const auto before=snapshot(actor,q);
    auto m=l3::prepare_imported_actor_action4_contact(actor,map,0,refs);
    require(m.inputs.has_value(),"entry motion preparation held");
    auto result=tick4(actor,input,&q,&m);
    require(result.committed && actor.source_behavior->behavior.phase_0a==1 &&
        actor.source_behavior->behavior.timer_32c==12 && actor.pose->request==21,"entry body/pose failed");
    require(actor.source_behavior->work_370==before.state.work_370,"attack incorrectly stored wall result");
    require(q.count==1,"entry cue lost");
    require_sound(q.records[0],1,177,l3::actor_source_sound_position_words(before.state.raw_14));
    for(unsigned i=0;i<12;++i){
        m=l3::prepare_imported_actor_action4_contact(actor,map,0,refs);
        require(tick4(actor,input,&q,&m).committed,"negative112 movement failed");
    }
    require(actor.source_behavior->behavior.timer_32c==0,"phase1 timeline differs");
    require(tick4(actor,input,&q).committed && actor.source_behavior->behavior.phase_0a==2 &&
        actor.source_behavior->behavior.timer_32c==15,"phase2 fallthrough differs");
    for(unsigned i=0;i<14;++i)require(tick4(actor,input,&q).committed,"phase2 wait failed");
    const auto held=snapshot(actor,q);auto forged=input;forged.produced_child_374=0;
    result=tick4(actor,forged,&q);
    require(!result.committed && result.status==l3::Class20_03CallbackStatus::NeedsChildConsumer,
        "native accepted a fabricated allocator exhaustion");
    require_held(actor,q,held,"child boundary mutated native state");
    require(q.count==1,"wait repeated entry sound");
}
void stale(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    for(unsigned field=0;field<26;++field){
        auto actor=actor4();auto q=authored_queue();const auto before=snapshot(actor,q);
        auto m=l3::prepare_imported_actor_action4_contact(actor,map,0,refs);
        require(m.inputs.has_value(),"authored contact rejected");
        switch(field){
        case 0: ++m.before.raw[3];break;case 1:++m.before.fixed[3];break;
        case 2:++m.before.previous_raw[3];break;case 3:++m.before.previous_fixed[3];break;
        case 4:++m.before.object_class;break;case 5:++m.before.velocity[2];break;
        case 6:++m.before.state74;break;case 7:++m.initial_action;break;
        case 8:++m.initial_phase;break;case 9:++m.initial_request;break;
        case 10:++m.initial_heading;break;case 11:++m.initial_timer;break;
        case 12:++m.initial_variant;break;case 13:++m.inputs->raw_14[3];break;
        case 14:++m.inputs->fixed_1c[3];break;case 15:++m.inputs->source_floor_word;break;
        case 16:++m.inputs->source_forward;break;case 17:++m.inputs->heading;break;
        case 18:++m.after_wall.raw[3];break;case 19:++m.after_wall.fixed[3];break;
        case 20:m.status=l3::ActorAction4ContactStatus::UnsupportedWall;break;
        case 21:m.wall.status=l3::RuntimeContactStatus::NoData;break;
        case 22:m.floor.status=l3::RuntimeContactStatus::NoData;break;
        case 23:++m.after_wall.previous_raw[3];break;
        case 24:++m.after_wall.previous_fixed[3];break;
        default:++m.after_wall.object_class;break;
        }
        const auto result=tick4(actor,authored_inputs(),&q,&m);
        require(!result.committed && result.status==l3::Class20_03CallbackStatus::NeedsAction4Motion,"stale attack admitted");
        require_held(actor,q,before,"stale attack lost transaction atomicity");
    }
}
void holds(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    for(unsigned mode=0;mode<5;++mode){
        auto actor=actor4();auto q=authored_queue();auto input=authored_inputs();
        if(mode==0)actor.animation_bank.reset();
        if(mode==1){auto bank=authored_bank();bank.requests.pop_back();bank.requests.pop_back();
            actor.animation_bank=std::make_shared<const l3::ActorAnimationBank>(bank);}
        if(mode==3)input.action.packet_33c=1;
        const auto before=snapshot(actor,q);const auto m=l3::prepare_imported_actor_action4_contact(actor,map,0,refs);
        if(mode==4){input.prepared_action4=l3::Class20_03CallbackInputs::Action4Motion{};}
        const auto result=tick4(actor,input,mode==2?nullptr:&q,mode==4?nullptr:&m);
        require(!result.committed,"missing consumer admitted");
        require_held(actor,q,before,"missing consumer did not roll back");
    }
    auto actor=actor4();auto q=authored_queue(l3::kSourceAudioQueueCapacity);
    const auto m=l3::prepare_imported_actor_action4_contact(actor,map,0,refs);
    require(tick4(actor,authored_inputs(),&q,&m).committed && actor.last_source_audio_dropped==1 &&
        q.count==l3::kSourceAudioQueueCapacity,"full source queue behavior differs");
    actor=actor4(4,0);q=authored_queue();actor.source_combat_byte_c.reset();
    auto input=authored_inputs();input.combat_record_byte_c=200;input.source_random_sample=3;
    auto before=snapshot(actor,q);
    require(tick4(actor,input,&q).status==l3::Class20_03CallbackStatus::NeedsCombatProfile,"forged stats admitted");
    require_held(actor,q,before,"unknown byteC mutated state");
    actor.source_combat_byte_c=200;actor.source_behavior->variant_32e=1;before=snapshot(actor,q);
    require(tick4(actor,input,&q).status==l3::Class20_03CallbackStatus::NeedsCombatProfile,"base byteC used for other variant");
    require_held(actor,q,before,"variant mismatch mutated state");
    actor.source_behavior->variant_32e=0;input.source_random_sample.reset();before=snapshot(actor,q);
    require(tick4(actor,input,&q).status==l3::Class20_03CallbackStatus::NeedsRandomSample,"expiry guessed RNG");
    require_held(actor,q,before,"RNG hold mutated body");
    input.source_random_sample=3;
    require(tick4(actor,input,&q).committed && actor.source_behavior->work_35c==199,"byteC unsigned/tail ordering differs");
}
void zero_allocations(const l3::RuntimeMap& map,const l3::ReferenceTables& refs) {
    auto actor=actor4(1,32767);auto q=authored_queue();const auto input=authored_inputs();
    allocations=0;measure_allocations=true;
    for(unsigned i=0;i<512;++i){
        actor.source_behavior->raw_14={{19,-1,37,-21846}};
        actor.source_behavior->fixed_1c={{19*65536,-65536,37*65536,0x13572468}};
        const auto m=l3::prepare_imported_actor_action4_contact(actor,map,0,refs);
        require(tick4(actor,input,&q,&m).committed,"allocation probe failed");
    }
    measure_allocations=false;require(allocations==0,"attack hot loop allocated");
}
} // namespace
int main(){try{
    const auto map=authored_flat_map();const auto refs=authored_reference();
    phases(map,refs);stale(map,refs);holds(map,refs);zero_allocations(map,refs);
    std::cout<<"Action4 native bridge phase timeline,26 stale holds,consumer/stat/RNG/child rollback,512 zero-allocation ticks PASS; authored subset,not retail parity\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}return 0;}
