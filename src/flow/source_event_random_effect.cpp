#include "legends3ds/source_event_random_effect.h"
#include "legends3ds/source_event_pipeline.h"
#include <limits>

namespace l3 {
namespace {
uint32_t word(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
template<size_t N>void put(SourceEventPhase5Bytes<N>& owner,size_t offset,size_t count,uint32_t value) noexcept {
    for(size_t i=0;i<count;++i){owner.raw[offset+i]=uint8_t(value>>(8*i));owner.known[offset+i]=1;}
}
bool known(const SourceEventWork& work,size_t offset,size_t count) noexcept {
    for(size_t i=0;i<count;++i)if(!work.known[offset+i])return false;
    return true;
}
} // namespace
SourceEventRandomEffectPlan plan_source_event_random_effect_80100d6c(
    const SourceEventWork& work,const SourceEventPhase5Context& scene) noexcept {
    using Status=SourceEventRandomEffectStatus;SourceEventRandomEffectPlan out;
    if(!known(work,0,2)||!known(work,4,1))return out;
    if(work.active00()!=1||work.event01()!=1||work.phase04()!=6){out.status=Status::WrongEventPhase;return out;}
    if(!known(work,0x10,4)){out.missing_offset=0x10;return out;}
    out.frame=word(work.raw.data()+0x10);out.clears_control_80122424=out.frame==6;
    if(out.frame>=31u&&out.frame<0x80000000u){out.status=Status::Ready;return out;}
    if(!known(work,0x24,4)){out.missing_offset=0x24;return out;}
    out.source_actor=word(work.raw.data()+0x24);out.bitmap_before=scene.pool80.used;out.bitmap_after=out.bitmap_before;
    for(size_t attempt=0;attempt<2;++attempt){
        bool allocated=false;
        for(size_t bank=0;bank<2&&!allocated;++bank){
            if(!out.bitmap_after[bank]){out.status=Status::NeedsPoolBitmap;return out;}
            for(size_t bit=0;bit<32;++bit){const uint32_t mask=0x80000000u>>bit;
                if(*out.bitmap_after[bank]&mask)continue;
                const size_t slot=bank*32+bit;const auto& record=scene.pool80.slots[slot];
                if(!record.known[1]||record.raw[1]!=uint8_t(0x80u|slot)){
                    out.status=Status::NeedsPoolHeader;out.missing_offset=1;return out;
                }
                *out.bitmap_after[bank]|=mask;out.slots[out.allocation_count++]=uint8_t(slot);allocated=true;break;
            }
        }
    }
    out.rng_calls=uint8_t(out.allocation_count*3u);
    constexpr uint32_t ras[]={0x80100dd4u,0x80100decu,0x80100e04u,0x80100e40u,0x80100e58u,0x80100e70u};
    for(size_t i=0;i<out.rng_calls;++i)out.caller_ras[i]=ras[i];
    if(out.allocation_count){
        constexpr uint32_t base=0x800c3580u;constexpr size_t bytes=1444;
        if(out.source_actor<base||(out.source_actor-base)%bytes||(out.source_actor-base)/bytes>=4){out.status=Status::NeedsActorBinding;return out;}
        const size_t slot=(out.source_actor-base)/bytes;const auto& used=scene.pool40.used[0];const auto& actor=scene.pool40.slots[slot];
        if(!used||!(*used&(0x80000000u>>slot))||!actor.known[1]||actor.raw[1]!=uint8_t(0x40u|slot)){
            out.status=Status::NeedsActorBinding;return out;
        }
        for(size_t axis=0;axis<3;++axis){const size_t offset=20+axis*2;
            for(size_t i=0;i<2;++i)if(!actor.known[offset+i]){
                out.status=Status::NeedsActorField;out.missing_offset=uint16_t(offset+i);return out;
            }
            out.source_position[axis]=uint16_t(uint16_t(actor.raw[offset])|(uint16_t(actor.raw[offset+1])<<8));
        }
    }
    out.status=Status::Ready;return out;
}
SourceEventRandomEffectResult consume_source_event_random_effect_80100d6c(
    const SourceEventWork& work,SourceEventPhase5Context& scene,
    const SourceEventRandomEffectReceipts* receipts,const SourceEventRandomEffectReceiptOwner* current) noexcept {
    using Status=SourceEventRandomEffectStatus;SourceEventRandomEffectResult out;
    const auto plan=plan_source_event_random_effect_80100d6c(work,scene);out.status=plan.status;out.missing_offset=plan.missing_offset;
    if(plan.status!=Status::Ready)return out;
    if(receipts&&receipts->count!=plan.rng_calls){out.status=Status::WrongReceiptCount;return out;}
    if(plan.rng_calls){
        if(!receipts||!current||!receipts->completed||!receipts->owner||!current->owner||receipts->generation==0||current->generation==0){
            out.status=Status::NeedsRandomOwner;return out;
        }
        if(receipts->owner!=current->owner||receipts->generation!=current->generation){out.status=Status::StaleReceipt;return out;}
        if(receipts->callback!=kSourceEventRandomEffectCallback||receipts->source_work!=0x800c4c48u||
            receipts->event_frame!=plan.frame||receipts->source_actor!=plan.source_actor){out.status=Status::WrongReceiptContext;return out;}
        for(size_t i=0;i<plan.rng_calls;++i){const auto& receipt=receipts->calls[i];
            if(receipt.caller_ra!=plan.caller_ras[i]||receipt.return_v0>32767u||receipt.global_ordinal==0||
                (i&&(receipts->calls[i-1].global_ordinal==std::numeric_limits<uint64_t>::max()||
                    receipt.global_ordinal!=receipts->calls[i-1].global_ordinal+1u))){out.status=Status::InvalidReceipt;return out;}
        }
        for(size_t i=plan.rng_calls;i<receipts->calls.size();++i)if(receipts->calls[i].caller_ra||
            receipts->calls[i].return_v0||receipts->calls[i].global_ordinal){out.status=Status::InvalidReceipt;return out;}
    }
    std::array<SourceEventPhase5Bytes<72>,2> next{};
    for(size_t i=0;i<plan.allocation_count;++i){next[i]=scene.pool80.slots[plan.slots[i]];
        auto& record=next[i];put(record,2,2,0);put(record,4,4,0);put(record,12,4,0);
        put(record,8,4,0);put(record,16,4,0);put(record,6,1,3);
        put(record,0,1,1);put(record,2,1,131);put(record,3,1,0);
        constexpr uint32_t mask[]={63u,31u,63u};constexpr uint32_t bias[]={31u,15u,31u};
        for(size_t axis=0;axis<3;++axis){const uint32_t value=uint32_t(plan.source_position[axis])+
                (receipts->calls[i*3+axis].return_v0&mask[axis])-bias[axis];
            put(record,28+axis*2,2,uint16_t(value));
        }
    }
    // Only the bounded changed records/bitmaps are published after complete
    // receipt/field preflight. The external RNG owner is deliberately untouched.
    for(size_t i=0;i<plan.allocation_count;++i)scene.pool80.slots[plan.slots[i]]=next[i];
    for(size_t i=0;i<plan.bitmap_after.size();++i)if(plan.bitmap_after[i]!=plan.bitmap_before[i])scene.pool80.used[i]=plan.bitmap_after[i];
    out.committed=true;out.status=Status::Consumed;out.committed_call_count=plan.rng_calls;
    out.allocated_count=plan.allocation_count;out.allocated_slots=plan.slots;
    if(plan.clears_control_80122424)out.control_80122424=0;
    return out;
}
SourceEventRandomEffectResult consume_source_event_random_effect_candidate(
    const SourceEventWork& work,SourceEventPhase5Context& scene,const SourceEventCandidateRng& candidate) noexcept {
    // An original zero-call frame or full-pool branch needs no RNG owner.
    auto result=consume_source_event_random_effect_80100d6c(work,scene);
    if(result.committed||result.status!=SourceEventRandomEffectStatus::NeedsRandomOwner||
       !candidate.service||!candidate.service->admitted()||!candidate.source_generation)return result;
    const auto plan=plan_source_event_random_effect_80100d6c(work,scene);
    if(plan.status!=SourceEventRandomEffectStatus::Ready||!plan.rng_calls)return result;
    auto& rng=*candidate.service;SourceRngUpdateTransaction transaction;
    if(begin_source_rng_update(rng,transaction,candidate.update)!=SourceRngStatus::Ready)return result;
    SourceEventRandomEffectReceipts receipts;receipts.owner=&rng;receipts.generation=rng.generation();
    receipts.completed=true;receipts.callback=kSourceEventRandomEffectCallback;receipts.source_work=0x800c4c48u;
    receipts.event_frame=plan.frame;receipts.source_actor=plan.source_actor;receipts.count=plan.rng_calls;
    for(uint8_t i=0;i<plan.rng_calls;++i){
        const auto value=stage_source_rng_scene_event(transaction,{candidate.source_generation,i,plan.rng_calls});
        if(value.status!=SourceRngStatus::Ready){rollback_source_rng_update(transaction);return result;}
        receipts.calls[i]={plan.caller_ras[i],value.sample32a,value.native_global_ordinal};
    }
    std::array<SourceEventPhase5Bytes<72>,2> saved{};
    for(size_t i=0;i<plan.allocation_count;++i)saved[i]=scene.pool80.slots[plan.slots[i]];
    const auto bitmap=scene.pool80.used;
    const SourceEventRandomEffectReceiptOwner owner{&rng,rng.generation()};
    result=consume_source_event_random_effect_80100d6c(work,scene,&receipts,&owner);
    if(!result.committed){rollback_source_rng_update(transaction);return result;}
    if(commit_source_rng_update(rng,transaction)!=SourceRngStatus::Ready){
        for(size_t i=0;i<plan.allocation_count;++i)scene.pool80.slots[plan.slots[i]]=saved[i];
        scene.pool80.used=bitmap;rollback_source_rng_update(transaction);
        result={};result.status=SourceEventRandomEffectStatus::NeedsRandomOwner;
    }
    return result;
}
} // namespace l3
