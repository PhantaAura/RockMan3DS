#include "legends3ds/actor_runtime.h"
#include "legends3ds/child_runtime.h"
#include "legends3ds/class20_03_hit.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace {size_t allocations=0;}
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);} void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);} void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

namespace {
using namespace l3;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
ActorAnimationBank bank(){
    ActorAnimationBank out;out.source_row=0;ActorAnimationRequestCache request;request.request=0;
    ActorSequenceRecord record{{0,2,1,0x80,0,0,0,0}};ActorPoseResource pose;
    pose.request=0;pose.record_index=0;pose.frame_index=0;pose.countdown=2;pose.next_control=-128;
    pose.sequencing_record=record;pose.base_translation.push_back({{0,0,0}});
    request.sequence.records.push_back(record);request.sequence.poses.push_back(pose);out.requests.push_back(request);return out;
}
ImportedActorInstance actor(){
    ImportedActorInstance out;out.source_class=0x20;out.subtype=3;out.initializer=0x80102410;
    out.source_dispatch_flag0=1;out.source_dispatch_flag6=3;out.source_behavior.emplace();
    bind_imported_actor_floor(out,-1);auto& s=*out.source_behavior;
    s.behavior.parameter_03=0x20;s.behavior.action_09=0;s.behavior.phase_0a=0;
    s.behavior.flags_328=0x40;s.defeat_timer_360=0;s.health_6e=0;s.work_00=3;
    s.raw_14={{11,22,33,44}};s.proxy_position_180={{101,-202,303,-404}};
    out.source_origin={{11,22,33}};out.source_reward_word00=0x01010202u;
    out.animation_bank=std::make_shared<const ActorAnimationBank>(bank());
    out.pose=out.animation_bank->requests[0].sequence.poses[0];
    out.animation_cursor=actor_sequence_begin(0,out.pose->sequencing_record);
    return out;
}
SourceAudioQueue audio(){SourceAudioQueue q;initialize_source_audio_queue_80019e14(q);return q;}
Class20_03CallbackInputs input(){Class20_03CallbackInputs in;in.action.packet_33c=0;return in;}
void repeated_hit_boundary(){
    Class20_03CallbackState s;s.health_6e=16;s.variant_32e=0;s.raw_14={{1,2,3,4}};s.packet_33c=0x28000008u;
    Class20_03HitInputs in;in.packet_33c=0x28000008u;in.selected_record=Class20_03HitRecordByte09{0,1};
    in.hit_word_34a=1;in.angle_80031198=0;in.sound_consumer_ready=true;
    require(class20_03_consume_hit_801041b0(s,in).status==Class20_03HitStatus::SourceBackedPartial&&s.health_6e==8,
        "first source Buster hit did not subtract8");
    s.packet_33c=0x28000008u;
    require(class20_03_consume_hit_801041b0(s,in).status==Class20_03HitStatus::SourceBackedPartial&&s.health_6e==0&&
        (s.behavior.flags_328&0x40u),"second source Buster hit did not latch zero-HP defeat");
}
void lifecycle(){
    ReferenceTables refs;ChildRuntime children;auto q=audio();auto a=actor();auto in=input();
    const auto initial_effects=children.effect_pool();const auto initial_audio=q;
    a.source_reward_word00.reset();auto held=tick_imported_actor_state1(a,in,nullptr,nullptr,&q,
        nullptr,nullptr,nullptr,&children,&refs);
    require(!held.committed&&held.status==Class20_03CallbackStatus::NeedsDefeatLifecycle&&!a.source_released&&
        children.enemy_reward_queue().count==0&&children.effect_pool().allocated_80098838==initial_effects.allocated_80098838&&
        q.count==initial_audio.count,"missing reward profile partly committed defeat");
    a.source_reward_word00=0x01010202u;const auto before=allocations;
    const auto done=tick_imported_actor_state1(a,in,nullptr,nullptr,&q,nullptr,nullptr,nullptr,&children,&refs);
    require(done.committed&&done.defeated&&done.reward_enqueued&&done.defeat_effect_allocated&&
        !done.defeat_effect_exhausted&&a.source_released&&!a.source_proxy_bound&&a.source_behavior->work_00==0&&
        a.source_behavior->defeat_effect_368==source_effect_base,"defeat transaction did not release actor");
    require(children.enemy_reward_queue().count==1&&
        children.enemy_reward_queue().records[0].source_descriptor_word00==0x01010202u&&
        children.enemy_reward_queue().records[0].point_180==std::array<int16_t,4>{{101,-202,303,-404}},
        "reward call operands were not retained exactly once");
    const auto& effect=children.effect_pool().slots[0];
    require(children.effect_pool().allocated_80098838[0]==0x80000000u&&effect[0]==5&&effect[2]==128&&effect[3]==65,
        "source defeat effect128/65 was not initialized");
    require(q.count==1&&q.records[0].raw[0]==1&&q.records[0].raw[2]==161&&q.records[0].raw[3]==0,
        "source defeat sound161 was not queued");
    const auto effects_after=children.effect_pool();const auto audio_after=q;const auto state_after=*a.source_behavior;
    const auto again=tick_imported_actor_state1(a,in,nullptr,nullptr,&q,nullptr,nullptr,nullptr,&children,&refs);
    require(!again.committed&&children.enemy_reward_queue().count==1&&
        children.effect_pool().slots==effects_after.slots&&children.effect_pool().allocated_80098838==effects_after.allocated_80098838&&
        q.count==audio_after.count&&a.source_behavior->defeat_effect_368==state_after.defeat_effect_368,
        "released enemy duplicated reward/effect/sound");
    require(allocations==before,"defeat transaction allocated in gameplay path");
    // 0.0.15.12: the queued 0x80047228 request feeds 0x80046B34. Authored
    // tables (not retail): every roll is size 0, so 2 shards + 1 energy spawn
    // at the defeat point (shards 0x14 higher) and the request is consumed once.
    SourceRewardProfile reward;reward.loaded=true;
    for(auto& t:reward.size_rolls_8008a5a8)t.fill(0);
    reward.spread_80097efa.fill(1024);reward.scale_80097ee0.fill(100);reward.speed_80097f1c.fill(8);reward.lift_80097f24.fill(-20);
    for(auto& k:reward.radius_80097eec)k.fill(10);
    for(auto& k:reward.bounds_8008a618)k.fill(RetailPlayerBounds{-8,8,-8,8,-8,8});
    SourceRngService rng;
    require(children.expand_next_enemy_reward(reward,refs,rng,1).status==SourceRewardStatus::NeedsTrig||
            children.expand_next_enemy_reward(reward,refs,rng,1).status==SourceRewardStatus::NeedsRng,"reward expanded without trig/RNG");
    require(children.enemy_reward_queue().count==1,"held reward request was dropped");
}
}
int main(){try{repeated_hit_boundary();lifecycle();std::cout<<"two source hits; atomic defeat effect/release/reward/sound; duplicate suppression PASS\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
