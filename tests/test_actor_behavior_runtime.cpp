#include "legends3ds/actor_runtime.h"
#include <cstdlib>
#include <new>
#include <iostream>
#include <stdexcept>
static size_t allocation_count=0;
void* operator new(std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
static void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
static l3::ActorAnimationBank authored_bank(){
    l3::ActorAnimationBank bank;bank.source_row=0;
    for(uint8_t id:{uint8_t(0),uint8_t(1),uint8_t(2),uint8_t(3)}){
        l3::ActorAnimationRequestCache cache;cache.request=id;
        for(uint8_t index=0;index<2;++index){
            l3::ActorSequenceRecord record{{index,2,1,uint8_t(index?0x80:0),0,0,0,0}};
            l3::ActorPoseResource pose;pose.request=id;pose.record_index=index;pose.frame_index=index;
            pose.countdown=2;pose.next_control=record[3];pose.sequencing_record=record;
            pose.base_translation.push_back({{int16_t(id),int16_t(index),0}});
            cache.sequence.records.push_back(record);cache.sequence.poses.push_back(pose);
        }
        bank.requests.push_back(cache);
    }
    return bank;
}
static l3::ImportedActorInstance authored_actor(){
    l3::ImportedActorInstance actor;actor.source_class=0x20;actor.subtype=3;actor.initializer=0x80102410;
    actor.source_dispatch_flag0=1;actor.source_dispatch_flag6=3;
    actor.source_behavior.emplace();actor.source_behavior->behavior.parameter_03=0x20;
    l3::bind_imported_actor_floor(actor,-1);
    auto bank=std::make_shared<const l3::ActorAnimationBank>(authored_bank());
    actor.pose=bank->requests[0].sequence.poses[0];actor.animation_cursor=l3::actor_sequence_begin(0,actor.pose->sequencing_record);
    actor.animation_bank=bank;actor.pose_selected_by_retail_state=true;return actor;
}
int main(){try{
    auto actor=authored_actor();
    require(actor.source_behavior_initialized&&!actor.source_behavior->health_6e,"unknown HP invented or fields not initialized");
    l3::Class20_03CallbackInputs input;input.action.packet_33c=0;input.action.work_80=123;
    auto shared_bank=actor.animation_bank;actor.animation_bank.reset();
    auto result=l3::tick_imported_actor_state1(actor,input);
    require(!result.committed&&result.status==l3::Class20_03CallbackStatus::NeedsAnimationConsumer,"missing bank committed");
    require(actor.source_behavior->behavior.phase_0a==0&&actor.source_behavior->behavior.timer_32c==0,"failed body changed live state");
    actor.animation_bank=shared_bank;
    const auto* storage=actor.pose->base_translation.data();const auto before=allocation_count;
    for(int i=0;i<30;++i)require(l3::tick_imported_actor_state1(actor,input).committed,"known waiting action held");
    require(actor.source_behavior->behavior.timer_32c==0&&actor.source_behavior->behavior.phase_0a==1,"source waiting timer mismatch");
    const auto cursor=*actor.animation_cursor;
    result=l3::tick_imported_actor_state1(actor,input);
    require(!result.committed&&result.status==l3::Class20_03CallbackStatus::NeedsRandomSample,"missing RNG guessed");
    require(actor.source_behavior->behavior.timer_32c==0&&actor.animation_cursor->countdown_a5==cursor.countdown_a5,"missing RNG changed state/pose");
    input.action.packet_33c=0x20000001u;
    require(l3::tick_imported_actor_state1(actor,input).status==l3::Class20_03CallbackStatus::NeedsPacket,"packet consumed without finalizer");
    input.action.packet_33c=0;
    actor.source_behavior->behavior.action_09=1;actor.source_behavior->behavior.phase_0a=0;
    actor.source_behavior->restore_action_0e=0;input.action.target_heading_80031198=512;
    result=l3::tick_imported_actor_state1(actor,input);
    require(result.committed&&result.request_changed&&actor.pose->request==1,"behavior request did not bind own pose bank");
    require(actor.source_behavior->behavior.animation_ac==1&&actor.animation_cursor->record_index==0,"request changed without source reset");
    for(int i=0;i<600;++i)(void)l3::tick_imported_actor_state1(actor,input);
    require(allocation_count==before&&actor.pose->base_translation.data()==storage,"tick allocated/replaced pose storage");
    actor.source_behavior->behavior.action_09=1;actor.source_behavior->behavior.phase_0a=0;
    actor.source_behavior->behavior.animation_ac=0;actor.pose->request=0;
    actor.animation_cursor=l3::actor_sequence_begin(0,shared_bank->requests[0].sequence.records[0]);
    auto missing=authored_bank();missing.requests.resize(1);
    actor.animation_bank=std::make_shared<const l3::ActorAnimationBank>(std::move(missing));
    const auto heading=actor.heading;
    result=l3::tick_imported_actor_state1(actor,input);
    require(!result.committed&&result.animation_status==l3::ActorAnimationStatus::MissingRequest,"absent request substituted");
    require(actor.source_behavior->behavior.phase_0a==0&&actor.heading==heading&&actor.pose->request==0,"missing request partly committed behavior");
    actor.source_behavior->behavior.parameter_03=0;input.detection_8003648c.reset();input.detection_range_byte2.reset();
    require(l3::tick_imported_actor_state1(actor,input).status==l3::Class20_03CallbackStatus::NeedsDetection,"unknown detection guessed");
    auto walking=authored_actor();
    walking.source_behavior->behavior.action_09=2;walking.animation_cursor->opaque_a6=0;
    walking.floor_bounds=l3::RetailPlayerBounds{-60,60,-60,60,-128,0};
    l3::RuntimeMap flat;l3::RuntimeMapArea region;
    region.start_x=63;region.start_z=63;region.width=2;region.height=2;region.cell_count=4;
    flat.areas.push_back(region);flat.cells.assign(4,int16_t(-32768));
    l3::ReferenceTables refs;refs.trig[0]={0,4096};
    const auto movement=l3::prepare_imported_actor_action2_contact(walking,flat,0,refs);
    require(movement.inputs.has_value()&&movement.wall.result==0&&movement.floor.result==-1,"real flat walk query unsupported");
    l3::Class20_03CallbackInputs walk_input;walk_input.action.packet_33c=0;
    const auto walk_allocations=allocation_count;
    auto walk_result=l3::tick_imported_actor_state1(walking,walk_input,&movement);
    require(walk_result.committed&&walking.pose->request==1&&walking.source_origin[2]==-3,"prepared source movement did not publish with own pose");
    require(walking.source_behavior->fixed_1c[2]==-52*4096,"source fixed movement changed");
    const auto saved_raw=walking.source_behavior->raw_14;
    walk_result=l3::tick_imported_actor_state1(walking,walk_input,&movement);
    require(!walk_result.committed&&walk_result.status==l3::Class20_03CallbackStatus::NeedsAction2Motion&&walking.source_behavior->raw_14==saved_raw,"stale motion transaction reused");
    const auto next_movement=l3::prepare_imported_actor_action2_contact(walking,flat,0,refs);
    walk_input.source_random_sample.reset();
    walking.animation_bank.reset();
    walk_result=l3::tick_imported_actor_state1(walking,walk_input,&next_movement);
    require(!walk_result.committed&&walking.source_behavior->raw_14==saved_raw,"missing pose bank partly moved actor");
    require(allocation_count==walk_allocations,"motion/contact commit allocated");
    actor.source_class=0x40;
    require(!l3::tick_imported_actor_state1(actor,input).committed,"wrong source dispatch admitted");
    std::cout<<"Imported actor state1/request transaction guards and 631 allocation-free ticks PASS; authored inputs, not retail parity\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
