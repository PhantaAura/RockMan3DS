#include "legends3ds/actor_runtime.h"
#include "legends3ds/child_runtime.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace {size_t allocations=0;}
void* operator new(std::size_t size){++allocations;if(void* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

namespace {
using namespace l3;
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
ActorAnimationBank authored_bank(bool request22=true){
    ActorAnimationBank bank;bank.source_row=0;
    for(uint8_t request:{uint8_t(21),uint8_t(22)}){
        if(request==22 && !request22)continue;
        ActorAnimationRequestCache cache;cache.request=request;
        for(uint8_t index=0;index<2;++index){
            ActorSequenceRecord record{{index,2,1,uint8_t(index?0x80:0),0,0,0,0}};
            ActorPoseResource pose;pose.request=request;pose.record_index=index;pose.frame_index=index;
            pose.countdown=2;pose.next_control=static_cast<int8_t>(record[3]);pose.sequencing_record=record;
            pose.base_translation.push_back({{int16_t(request),int16_t(index),0}});
            cache.sequence.records.push_back(record);cache.sequence.poses.push_back(pose);
        }
        bank.requests.push_back(cache);
    }
    return bank;
}
ImportedActorInstance authored_actor(bool request22=true){
    ImportedActorInstance actor;actor.source_class=0x20;actor.subtype=3;actor.initializer=0x80102410;
    actor.source_dispatch_flag0=1;actor.source_dispatch_flag6=3;actor.source_behavior.emplace();
    bind_imported_actor_floor(actor,-1);auto& s=*actor.source_behavior;
    s.behavior.parameter_03=0x20;s.behavior.action_09=4;s.behavior.phase_0a=2;
    s.behavior.timer_32c=1;s.behavior.animation_ac=21;s.behavior.heading_56=0;
    s.raw_14={{19,-1,37,-21846}};s.fixed_1c={{19*65536,-65536,37*65536,0x13572468}};
    s.previous_raw_2c={{101,102,103,104}};s.previous_fixed_34={{105,106,107,108}};
    s.angle_54={{4,5,6,7}};s.previous_angle_5c={{8,9,10,11}};s.work_15e=12;
    actor.source_origin={{19,-1,37}};actor.heading=0;actor.pose_selected_by_retail_state=true;
    actor.animation_bank=std::make_shared<const ActorAnimationBank>(authored_bank(request22));
    actor.pose=actor.animation_bank->requests[0].sequence.poses[0];
    actor.animation_cursor=actor_sequence_begin(0,actor.pose->sequencing_record);return actor;
}
SourceAudioQueue audio(size_t count=0){
    SourceAudioQueue q;for(auto& record:q.records)record.raw.fill(0xa5);
    initialize_source_audio_queue_80019e14(q);q.count=count;return q;
}
bool same_audio(const SourceAudioQueue& a,const SourceAudioQueue& b){
    if(a.count!=b.count || a.initialized!=b.initialized)return false;
    for(size_t i=0;i<a.records.size();++i)if(a.records[i].raw!=b.records[i].raw)return false;
    return true;
}
bool same_pool(const Class60SourcePool& a,const Class60SourcePool& b){
    return a.slots==b.slots && a.allocated_bitmap==b.allocated_bitmap && a.pending_release_bitmap==b.pending_release_bitmap;
}
Class20_03CallbackInputs inputs(){
    Class20_03CallbackInputs in;in.action.packet_33c=0;in.action.work_80=0;
    in.action.target_heading_80031198=0;return in;
}
ImportedActorBehaviorTick tick(ImportedActorInstance& a,SourceAudioQueue& q,
    ChildRuntime* children,const ReferenceTables& refs,Class20_03CallbackInputs in=inputs()){
    return tick_imported_actor_state1(a,in,nullptr,nullptr,&q,nullptr,nullptr,nullptr,children,&refs);
}
void same_actor_source(const ImportedActorInstance& a,const ImportedActorInstance& before){
    const auto& x=*a.source_behavior;const auto& y=*before.source_behavior;
    require(x.behavior.action_09==y.behavior.action_09 && x.behavior.phase_0a==y.behavior.phase_0a &&
        x.behavior.timer_32c==y.behavior.timer_32c && x.behavior.animation_ac==y.behavior.animation_ac &&
        x.raw_14==y.raw_14 && x.fixed_1c==y.fixed_1c && x.child_374==y.child_374 &&
        x.previous_raw_2c==y.previous_raw_2c && x.previous_fixed_34==y.previous_fixed_34 &&
        x.angle_54==y.angle_54 && x.previous_angle_5c==y.previous_angle_5c &&
        x.work_15e==y.work_15e && x.previous_animation_ad==y.previous_animation_ad,
        "failed parent transaction changed source-used state");
    require(a.source_origin==before.source_origin && a.heading==before.heading &&
        a.pose->request==before.pose->request && a.pose->sequencing_record==before.pose->sequencing_record &&
        a.animation_cursor->record_index==before.animation_cursor->record_index &&
        a.animation_cursor->countdown_a5==before.animation_cursor->countdown_a5,
        "failed parent transaction changed pose/origin");
}
void actor_delivery(){
    ReferenceTables refs;refs.trig.fill({0,4096});ChildRuntime children;auto q=audio();auto a=authored_actor();
    const auto original=a;const auto initial_pool=children.pool();const auto initial_audio=q;
    auto forged=inputs();forged.produced_child_374=class60_source_pool_base;
    require(tick(a,q,nullptr,refs,forged).status==Class20_03CallbackStatus::NeedsChildConsumer,"arbitrary external374 authorized native child");
    same_actor_source(a,original);require(same_pool(children.pool(),initial_pool) && same_audio(q,initial_audio),"forged scalar changed live owners");
    auto missing_pose=authored_actor(false);const auto missing_before=missing_pose;
    const auto failed=tick(missing_pose,q,&children,refs);
    require(!failed.committed && failed.status==Class20_03CallbackStatus::NeedsAnimationConsumer,"missing request22 did not hold parent");
    same_actor_source(missing_pose,missing_before);
    require(same_pool(children.pool(),initial_pool) && same_audio(q,initial_audio),"pose failure leaked child allocation or cue178");
    const auto committed=tick(a,q,&children,refs);
    require(committed.committed && a.source_behavior->child_374==class60_source_pool_base &&
        a.source_behavior->behavior.phase_0a==3 && a.source_behavior->behavior.timer_32c==11 &&
        a.source_behavior->behavior.animation_ac==22 && a.pose->request==22,
        "checked parent child delivery did not reach state/pose");
    require(children.pool().allocated_bitmap==0x80000000u && children.pool().slots[0][2]==6 &&
        children.pool().slots[0][1]==0x60 && children.pool().slots[0][0]==1 &&
        q.count==1 && committed.source_audio_queued==1 && q.records[0].raw[0]==1 &&
        q.records[0].raw[2]==178 && q.records[0].raw[3]==0 && q.records[0].raw[19]==0xa5,
        "actual source producer/retained positional sound record missing");
    children.initialize();q=audio(24);a=authored_actor();const auto full_audio=q;
    const auto dropped=tick(a,q,&children,refs);require(dropped.committed && dropped.source_audio_dropped==1 &&
        children.pool().allocated_bitmap==0x80000000u && same_audio(q,full_audio),"source full sound queue did not drop cue while admitting child");
    children.initialize();q=audio();a=authored_actor();a.source_behavior->variant_32e=1;const auto unsupported_before=a;
    require(tick(a,q,&children,refs).status==Class20_03CallbackStatus::NeedsChildConsumer,"unowned subtype9 callback admitted");
    same_actor_source(a,unsupported_before);require(children.pool().allocated_bitmap==0 && q.count==0,"unsupported subtype allocated pool/audio");
    a=authored_actor();ParentChildDelivery delivery;
    for(int i=0;i<32;++i){require(children.prepare_parent_delivery(*a.source_behavior,refs,q,delivery)==ChildRuntimeStatus::SourceBackedPartial,"exhaustion setup failed");q=delivery.staged_audio();require(children.commit_parent_delivery(delivery),"exhaustion setup commit failed");}
    const auto exhausted=children.pool();const auto exhausted_audio=q;
    const auto no_slot=tick(a,q,&children,refs);
    require(no_slot.committed && a.source_behavior->child_374==0 && same_pool(children.pool(),exhausted) && same_audio(q,exhausted_audio),"real32-slot exhaustion receipt not delivered atomically");

    // Repeat the actual parent body/animation/producer path using retained
    // pose vector capacities and source-owned queue/pool initialization.
    a=authored_actor();const auto saved=*a.source_behavior;const auto start=allocations;
    for(int i=0;i<512;++i){
        children.initialize();initialize_source_audio_queue_80019e14(q);*a.source_behavior=saved;
        *a.pose=a.animation_bank->requests[0].sequence.poses[0];
        *a.animation_cursor=actor_sequence_begin(0,a.pose->sequencing_record);
        require(tick(a,q,&children,refs).committed,"repeated parent producer transaction held");
    }
    require(allocations==start,"parent child delivery allocated in hot loop");
    std::cout<<"parent/action4 child delivery; pose rollback; real exhaustion;512 allocation-free transactions PASS\n";
}
} // namespace
int main(){try{actor_delivery();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
