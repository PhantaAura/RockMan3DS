#include "legends3ds/source_effects.h"
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_rng_service.h"
#include <algorithm>
namespace l3 {
namespace {
uint16_t u16(const SourceEffectWork& w,size_t o) noexcept {return uint16_t(w[o]|(uint16_t(w[o+1])<<8));}
uint32_t u32(const SourceEffectWork& w,size_t o) noexcept {return uint32_t(u16(w,o))|(uint32_t(u16(w,o+2))<<16);}
int16_t s16(uint16_t v) noexcept {return v<32768u?int16_t(v):int16_t(int32_t(v)-65536);}
void put16(SourceEffectWork& w,size_t o,uint16_t v) noexcept {w[o]=uint8_t(v);w[o+1]=uint8_t(v>>8);}
void put32(SourceEffectWork& w,size_t o,uint32_t v) noexcept {for(size_t i=0;i<4;++i)w[o+i]=uint8_t(v>>(i*8));}
uint32_t bit(size_t slot) noexcept {return 0x80000000u>>(slot&31u);}
bool valid(const SourceEffectPool& p) noexcept {
    for(size_t b=0;b<2;++b)if(p.pending_release_800988c8[b]&~p.allocated_80098838[b])return false;
    return true;
}
// Immutable scalar control bytes80097DE4..80097DEF, GP80097864 installed
// at80068080. These are lifecycle counts, not texture or owner asset bytes.
constexpr std::array<std::array<uint8_t,4>,3> fade_durations{{{{3,3,3,3}},{{4,4,4,4}},{{8,8,8,4}}}};
SourceEffectTick callback131(SourceEffectWork& w) noexcept {
    SourceEffectTick result;result.status=SourceEffectStatus::SourceBackedPartial;
    if(w[8]>4){result.status=SourceEffectStatus::UnknownState;return result;}
    if(w[8]>=1 && w[8]<=3 && w[3]>2){result.status=SourceEffectStatus::UnsupportedParameter;return result;}
    switch(w[8]){
    case 0:
        put32(w,0x14,0x3d48002eu);put32(w,0x18,0x2e404040u);
        w[0x22]=32;w[0x23]=0;w[6]|=0x20;w[0]|=2;
        if(w[3]==1)put16(w,0x24,32);
        else if(w[3]==2)put16(w,0x24,64);
        else {put16(w,0x24,w[3]==3?128:13);w[3]=0;}
        w[0xb]=fade_durations[w[3]][0];w[0xa]=0;++w[8];break;
    case 1:case 2:case 3:{
        const auto phase=w[8],parameter=w[3];
        uint16_t extent=u16(w,0x24),y=u16(w,0x1e);
        if(parameter==0){extent=uint16_t(extent+1);y=uint16_t(y-(phase==3?2u:1u));}
        else if(parameter==1)extent=uint16_t(extent-1);
        else {y=uint16_t(y-5);extent=uint16_t(extent+(phase==1?6u:uint16_t(-2)));}
        put16(w,0x1e,y);put16(w,0x24,extent);
        put32(w,0x18,u32(w,0x18)+(phase==1 && parameter==2?0x00040404u:0xfffbfbfcu));
        --w[0xb];if(w[0xb]==0){++w[8];w[0x22]=uint8_t(32u*(phase+1));w[0xb]=fade_durations[parameter][phase];}
        break;
    }
    case 4:--w[0xb];if(w[0xb]==0){w[0]=0;result.released=true;}break;
    }
    result.committed=true;return result;
}
SourceEffectTick callback128(SourceEffectWork& w,const SourceEffect128Inputs& in) noexcept {
    SourceEffectTick result;
    if(w[8]>2){result.status=SourceEffectStatus::UnknownState;return result;}
    if(w[8]==2){w[0]=0;result.status=SourceEffectStatus::SourceBackedPartial;result.released=true;result.committed=true;return result;}
    if(w[8]==1){result.status=SourceEffectStatus::NeedsSource128Consumers;return result;}
    if((w[3]!=2 && w[3]!=66) || w[0xd]!=0 || w[0xe]!=0){result.status=SourceEffectStatus::UnsupportedParameter;return result;}
    if(!in.floor_8004920c){result.status=SourceEffectStatus::NeedsFloorConsumer;return result;}
    const uint8_t selector=int64_t(*in.floor_8004920c)-s16(u16(w,0x3a))<30?0:1;
    if(!in.resource){result.status=SourceEffectStatus::NeedsResourceConsumer;return result;}
    if(in.resource->selector_8002f9c4!=selector || !in.resource->source_descriptor_14){result.status=SourceEffectStatus::StaleResource;return result;}
    if(!in.audio || !in.audio->initialized || in.audio->count>in.audio->records.size()){
        result.status=SourceEffectStatus::NeedsSoundConsumer;return result;
    }
    // Exact child128 subset80043F70..800441C8. A resource receipt is checked
    // before any state/audio commit; no resource or floor value is synthesized.
    w[0xc]=1;w[0]|=2;put16(w,0x44,70);put16(w,0x46,uint16_t(-5));
    put32(w,0x40,0x02808080u);w[0xa]=0;put16(w,0x3e,0);
    for(size_t off=0x18;off<=0x34;off+=4)put32(w,off,0);
    for(size_t off:{size_t(0x18),size_t(0x20),size_t(0x28)})put32(w,off,0x1000u);
    for(size_t off:{size_t(0x18),size_t(0x20),size_t(0x28)})put16(w,off,0);
    put32(w,0x14,in.resource->source_descriptor_14);w[2]=128;w[0xb]=0;++w[8];
    const auto audio=enqueue_source_audio_8001a050(*in.audio,161,{{u32(w,0x38),u32(w,0x3c)}});
    if(audio==SourceAudioQueueStatus::Queued)result.audio_queued=1;
    else if(audio==SourceAudioQueueStatus::DroppedFull)result.audio_dropped=1;
    else {result.status=SourceEffectStatus::NeedsSoundConsumer;return result;}
    result.status=SourceEffectStatus::SourceBackedPartial;result.committed=true;return result;
}
}
std::optional<size_t> source_effect_slot(uint32_t id) noexcept {
    if(id<source_effect_base)return std::nullopt;
    const uint32_t delta=id-source_effect_base;
    if(delta%source_effect_slot_bytes || delta/source_effect_slot_bytes>=source_effect_capacity)return std::nullopt;
    return size_t(delta/source_effect_slot_bytes);
}
void initialize_source_effect_pool_8002e924(SourceEffectPool& p) noexcept {
    p={};for(size_t i=0;i<p.slots.size();++i)p.slots[i][1]=uint8_t(0x80u|i);
}
SourceEffectAllocation allocate_source_effect_80031618(SourceEffectPool& p) noexcept {
    SourceEffectAllocation result;if(!valid(p)){result.status=SourceEffectStatus::InvalidPool;return result;}
    for(size_t slot=0;slot<p.slots.size();++slot){
        if(p.allocated_80098838[slot/32]&bit(slot))continue;
        p.allocated_80098838[slot/32]|=bit(slot);
        auto& w=p.slots[slot];std::fill(w.begin()+2,w.begin()+20,0);w[6]=3;
        result.source_identity=source_effect_base+uint32_t(slot*source_effect_slot_bytes);return result;
    }
    return result;
}
bool mark_source_effect_release_80031988(SourceEffectPool& p,uint32_t id) noexcept {
    const auto slot=source_effect_slot(id);if(!slot || !valid(p))return false;
    p.pending_release_800988c8[*slot/32]=(p.pending_release_800988c8[*slot/32]|bit(*slot))&p.allocated_80098838[*slot/32];
    p.slots[*slot][0]=0;return true;
}
void consume_source_effect_releases_80031b84(SourceEffectPool& p) noexcept {
    for(size_t b=0;b<2;++b){p.allocated_80098838[b]&=~p.pending_release_800988c8[b];p.pending_release_800988c8[b]=0;}
}
SourceEffectStatus initialize_child_effect(SourceEffectPool& p,const Class60_06EffectIntent& e) noexcept {
    if(!valid(p))return SourceEffectStatus::InvalidPool;
    const auto slot=source_effect_slot(e.source_identity);
    if(!slot || !(p.allocated_80098838[*slot/32]&bit(*slot)) || (p.pending_release_800988c8[*slot/32]&bit(*slot)))return SourceEffectStatus::InvalidIdentity;
    if(!((e.subtype02==128 && e.work00==5 && (e.parameter03==2 || e.parameter03==66) && e.copy_raw && !e.copy_primitive) ||
         (e.subtype02==131 && e.work00==1 && e.parameter03==0 && e.copy_primitive && !e.copy_raw)))return SourceEffectStatus::UnsupportedParameter;
    auto& w=p.slots[*slot];w[0]=e.work00;w[2]=e.subtype02;w[3]=e.parameter03;
    if(e.copy_primitive)std::copy(e.primitive_to_14.begin(),e.primitive_to_14.end(),w.begin()+0x14);
    if(e.copy_raw)for(size_t i=0;i<4;++i)put16(w,0x38+i*2,uint16_t(e.raw_to_38[i]));
    return SourceEffectStatus::SourceBackedPartial;
}
namespace {
// Buster impact 0x80044AB0 -> 0x8008A250 states. Presentation-only stores
// (+14..1B, +22..25 sprite fields) are not reproduced.
SourceEffectTick callback82(SourceEffectWork& w,size_t slot,const SourceEffect128Inputs& in) noexcept {
    SourceEffectTick result;
    if(w[8]>1){result.status=SourceEffectStatus::UnknownState;return result;}
    if(!in.impact82_timers_8008a240){result.status=SourceEffectStatus::NeedsSource128Consumers;return result;}
    const auto& timers=*in.impact82_timers_8008a240;
    if(w[8]==0){
        if(w[0x0c]&&(!in.audio||!in.audio->initialized)){result.status=SourceEffectStatus::NeedsSoundConsumer;return result;}
        if(!in.rng||!in.rng->admitted()||!in.source_generation){result.status=SourceEffectStatus::NeedsSource128Consumers;return result;}
        SourceRngUpdateTransaction txn;SourceRngUpdateContext c;c.native_frame=in.rng_frame;c.native_update_ordinal=in.rng_frame;
        c.phase=SourceRngUpdatePhase::RewardPickupCallback;
        if(begin_source_rng_update(*in.rng,txn,c)!=SourceRngStatus::Ready){result.status=SourceEffectStatus::NeedsSource128Consumers;return result;}
        SourceRngRewardContext r;r.caller_ra=0x80044b50u;r.work=source_effect_base+uint32_t(slot*source_effect_slot_bytes);
        r.slot=uint8_t(slot);r.source_generation=in.source_generation;
        const auto sample=stage_source_rng_reward(txn,r);
        if(sample.status!=SourceRngStatus::Ready){rollback_source_rng_update(txn);result.status=SourceEffectStatus::NeedsSource128Consumers;return result;}
        if(w[0x0c]){
            const auto st=enqueue_source_audio_8001a050(*in.audio,0x9b,{{u32(w,0x1c),u32(w,0x20)}});
            if(st==SourceAudioQueueStatus::Queued)++result.audio_queued;
            else if(st==SourceAudioQueueStatus::DroppedFull)++result.audio_dropped;
            else {rollback_source_rng_update(txn);result.status=SourceEffectStatus::NeedsSoundConsumer;return result;}
        }
        if(commit_source_rng_update(*in.rng,txn)!=SourceRngStatus::Ready){result.status=SourceEffectStatus::NeedsSource128Consumers;return result;}
        w[0]|=0x22;put16(w,0x26,uint16_t(sample.sample32a&0x3fu));
        w[0x0a]=0;w[9]=0;w[0x0b]=timers[0];++w[8];
        result.status=SourceEffectStatus::SourceBackedPartial;result.committed=true;return result;
    }
    w[0x0b]=uint8_t(w[0x0b]-1u);++w[9];
    if(w[0x0b]==0){
        ++w[0x0a];
        if(w[0x0a]<6)w[0x0b]=timers[w[0x0a]];
        else {w[0]=0;result.released=true;}
    }
    result.status=SourceEffectStatus::SourceBackedPartial;result.committed=true;return result;
}
}
SourceEffectTick tick_source_effect(SourceEffectPool& p,uint32_t id,const SourceEffect128Inputs& in) noexcept {
    SourceEffectTick result;const auto slot=source_effect_slot(id);
    if(!slot){result.status=SourceEffectStatus::InvalidIdentity;return result;}
    if(!valid(p)){result.status=SourceEffectStatus::InvalidPool;return result;}
    if(!(p.allocated_80098838[*slot/32]&bit(*slot))){result.status=SourceEffectStatus::InvalidIdentity;return result;}
    auto w=p.slots[*slot];if(!(w[0]&w[6]&1u))return result;
    if(w[2]==131)result=callback131(w);
    else if(w[2]==128)result=callback128(w,in);
    else if(w[2]==0x82)result=callback82(w,*slot,in);
    else if(w[2]==0x87)return result; // pickups tick in tick_source_reward_pickups
    else {result.status=SourceEffectStatus::UnknownSubtype;return result;}
    if(result.committed){p.slots[*slot]=w;if(result.released)mark_source_effect_release_80031988(p,id);}
    return result;
}
SourceEffectBatch tick_source_effect_batch(SourceEffectPool& p,const SourceEffect128Inputs& in) noexcept {
    SourceEffectBatch batch;
    for(size_t slot=0;slot<source_effect_capacity;++slot){
        if(!(p.allocated_80098838[slot/32]&bit(slot)))continue;
        const auto result=tick_source_effect(p,source_effect_base+uint32_t(slot*source_effect_slot_bytes),in);
        if(result.committed){++batch.committed;if(result.released)++batch.released;}
        else if(result.status!=SourceEffectStatus::Inactive){if(batch.held==0)batch.first_hold=result.status;++batch.held;}
    }
    return batch;
}
namespace {
using EffectRecord=SourceEventPhase5Bytes<source_effect_slot_bytes>;
bool need(const EffectRecord& record,size_t offset,size_t count,size_t slot,
    SourceEffectPhase5Batch& out) noexcept {
    for(size_t i=0;i<count;++i)if(!record.known[offset+i]){
        out.status=SourceEffectStatus::NeedsField;out.missing_slot=uint8_t(slot);
        out.missing_offset=uint8_t(offset+i);return false;
    }
    return true;
}
bool preflight131(const EffectRecord& record,size_t slot,
    const SourceEventPhase5Context& context,SourceEffectPhase5Batch& out) noexcept {
    if(!need(record,8,1,slot,out))return false;
    const auto& w=record.raw;const uint8_t state=w[8];
    if(state>4){out.status=SourceEffectStatus::UnknownState;out.missing_slot=uint8_t(slot);return false;}
    if(state==0)return need(record,3,1,slot,out);
    if(!need(record,11,1,slot,out))return false;
    if(state<4){
        if(!need(record,3,1,slot,out))return false;
        if(w[3]>2){out.status=SourceEffectStatus::UnsupportedParameter;out.missing_slot=uint8_t(slot);return false;}
        if(!need(record,0x18,4,slot,out)||!need(record,0x24,2,slot,out))return false;
        return w[3]==1||need(record,0x1e,2,slot,out);
    }
    // Source state4 reads no parameter or primitive lanes. Only its terminal
    // timer branch calls80031988 and therefore reads the two bank-local masks.
    if(w[11]!=1)return true;
    if(!context.pool80.used[slot/32]){out.status=SourceEffectStatus::NeedsPoolBitmap;out.missing_slot=uint8_t(slot);return false;}
    if(!context.pool80_pending_release_800988c8[slot/32]){
        out.status=SourceEffectStatus::NeedsReleaseBitmap;out.missing_slot=uint8_t(slot);return false;
    }
    return true;
}
void known_write(EffectRecord& record,size_t offset,size_t count=1) noexcept {
    for(size_t i=0;i<count;++i)record.known[offset+i]=1;
}
void publish131(EffectRecord& record,size_t slot,SourceEventPhase5Context& context,
    SourceEffectPhase5Batch& out) noexcept {
    const uint8_t state=record.raw[8],parameter=state<4?record.raw[3]:0,
        timer=state?record.raw[11]:0;
    auto next=record.raw;const auto result=callback131(next);record.raw=next;
    if(state==0){
        for(size_t offset:{size_t(0),size_t(3),size_t(6),size_t(8),size_t(10),size_t(11)})known_write(record,offset);
        known_write(record,0x14,8);known_write(record,0x22,4);
    }else if(state<4){
        known_write(record,11);known_write(record,0x18,4);known_write(record,0x24,2);
        if(parameter!=1)known_write(record,0x1e,2);
        if(timer==1){known_write(record,8);known_write(record,0x22);}
    }else{
        known_write(record,11);
        if(result.released){
            known_write(record,0);
            auto& pending=context.pool80_pending_release_800988c8[slot/32];
            pending=(*pending|bit(slot))&*context.pool80.used[slot/32];++out.released;
        }
    }
    ++out.updated;
}
}
SourceEffectPhase5Batch tick_source_phase5_effects_800173b4(SourceEventPhase5Context& context) noexcept {
    SourceEffectPhase5Batch out;
    for(size_t slot=0;slot<source_effect_capacity;++slot){
        const auto& record=context.pool80.slots[slot];
        if(!need(record,0,1,slot,out)||!need(record,6,1,slot,out))return out;
        if(!(record.raw[0]&record.raw[6]&1u))continue;
        if(!need(record,2,1,slot,out))return out;
        if(record.raw[2]<128)continue;
        if(record.raw[2]!=131){out.status=SourceEffectStatus::UnknownSubtype;out.missing_slot=uint8_t(slot);return out;}
        if(!preflight131(record,slot,context,out))return out;
    }
    for(size_t slot=0;slot<source_effect_capacity;++slot){auto& record=context.pool80.slots[slot];
        if((record.raw[0]&record.raw[6]&1u)&&record.raw[2]==131)publish131(record,slot,context,out);
    }
    out.committed=true;return out;
}
SourceEffectPhase5Batch consume_source_phase5_effect_releases_80031aa4(SourceEventPhase5Context& context) noexcept {
    SourceEffectPhase5Batch out;
    for(size_t bank=0;bank<2;++bank){
        if(!context.pool80.used[bank]){out.status=SourceEffectStatus::NeedsPoolBitmap;out.missing_slot=uint8_t(bank*32);return out;}
        if(!context.pool80_pending_release_800988c8[bank]){
            out.status=SourceEffectStatus::NeedsReleaseBitmap;out.missing_slot=uint8_t(bank*32);return out;
        }
    }
    for(size_t bank=0;bank<2;++bank){
        context.pool80.used[bank]=*context.pool80.used[bank]&~*context.pool80_pending_release_800988c8[bank];
        context.pool80_pending_release_800988c8[bank]=0;
    }
    out.committed=true;return out;
}
std::optional<Class60_06Primitive> source_effect_primitive(const SourceEffectWork& w) noexcept {
    if(!(w[0]&w[6]&2u) || (w[0]&0x1cu) || (w[0]&0x20u))return std::nullopt;
    Class60_06Primitive p;
    for(size_t i=0;i<3;++i)p.center[i]=s16(u16(w,0x1c+i*2));
    p.half_extent=u16(w,0x24);p.clut=u16(w,0x16);p.tpage=u16(w,0x14);
    p.uv={{w[0x22],w[0x23]}};p.color_command=u32(w,0x18);p.source_fog=(w[0]&8u)==0;
    return p;
}
} // namespace l3
