#include "legends3ds/source_effect2.h"
#include <algorithm>
#include <limits>
namespace l3 {
namespace {
using Record=SourceEventPhase5Bytes<72>;
uint16_t get16(const SourceEffectWork& w,size_t o) noexcept {return uint16_t(w[o]|(uint16_t(w[o+1])<<8));}
uint32_t get32(const SourceEffectWork& w,size_t o) noexcept {return uint32_t(get16(w,o))|(uint32_t(get16(w,o+2))<<16);}
void put16(Record& w,size_t o,uint16_t v) noexcept {for(size_t i=0;i<2;++i){w.raw[o+i]=uint8_t(v>>(i*8));w.known[o+i]=1;}}
void put32(Record& w,size_t o,uint32_t v) noexcept {for(size_t i=0;i<4;++i){w.raw[o+i]=uint8_t(v>>(i*8));w.known[o+i]=1;}}
void put8(Record& w,size_t o,uint8_t v) noexcept {w.raw[o]=v;w.known[o]=1;}
uint32_t bit(size_t slot) noexcept {return 0x80000000u>>(slot&31u);}
int32_t signed32(uint32_t v) noexcept {return v<=0x7fffffffu?int32_t(v):int32_t(int64_t(v)-0x100000000ll);}
int32_t shift_signed(int32_t v,unsigned n) noexcept {return v>=0?v/int32_t(1u<<n):int32_t(-((-int64_t(v)+(int64_t(1)<<n)-1)/(int64_t(1)<<n)));}
bool need(const Record& r,size_t off,size_t count,size_t slot,SourceEffect2Result& out) noexcept {
    for(size_t i=0;i<count;++i){
        if(!r.known[off+i]){out.status=SourceEffect2Status::NeedsField;out.missing_slot=uint8_t(slot);out.missing_offset=uint8_t(off+i);return false;}
    }
    return true;
}
bool color(const Record& r,size_t slot,const SourceEffect2Inputs& in,uint32_t& value,SourceEffect2Result& out) noexcept {
    if(!need(r,14,1,slot,out))return false;
    if(!in.colors || !in.colors->admitted){out.status=SourceEffect2Status::NeedsColorReceipt;return false;}
    if(in.colors->source_table_identity!=source_effect2_color_table || r.raw[14]>=8){out.status=SourceEffect2Status::ColorContextMismatch;return false;}
    value=in.colors->words[r.raw[14]];return true;
}
bool masks(const SourceEventPhase5Context& context,bool pending,SourceEffect2Result& out) noexcept {
    for(size_t bank=0;bank<2;++bank){
        if(!context.pool80.used[bank]){out.status=SourceEffect2Status::NeedsPoolBitmap;return false;}
        if(pending && !context.pool80_pending_release_800988c8[bank]){out.status=SourceEffect2Status::NeedsReleaseBitmap;return false;}
        if(context.pool80_pending_release_800988c8[bank] && (*context.pool80_pending_release_800988c8[bank]&~*context.pool80.used[bank])){out.status=SourceEffect2Status::InvalidPool;return false;}
    }return true;
}
bool release_preflight(const SourceEventPhase5Context& context,const Record& r,size_t slot,SourceEffect2Result& out) noexcept {
    if(!need(r,1,1,slot,out))return false;
    if(r.raw[1]!=uint8_t(0x80u|slot)){out.status=SourceEffect2Status::InvalidIdentity;return false;}
    return masks(context,true,out);
}
}
SourceEffect2Result tick_source_effect2_80116e68(SourceEventPhase5Context& context,uint32_t identity,const SourceEffect2Inputs& in) noexcept {
    SourceEffect2Result out;
    if(in.callback_identity!=source_effect2_callback){out.status=SourceEffect2Status::UnknownCallback;return out;}
    const auto slot=source_effect_slot(identity);if(!slot){out.status=SourceEffect2Status::InvalidIdentity;return out;}
    const auto& current=context.pool80.slots[*slot];
    if(!need(current,0,1,*slot,out)||!need(current,6,1,*slot,out))return out;
    if(!(current.raw[0]&current.raw[6]&1u)){out.status=SourceEffect2Status::Inactive;return out;}
    if(!need(current,2,1,*slot,out)||!need(current,13,1,*slot,out))return out;
    if(current.raw[2]!=2){out.status=SourceEffect2Status::UnknownCallback;return out;}
    auto parent=current;std::optional<size_t> child_slot;Record child;bool release=false;
    if(parent.raw[13]==0){
        if(!need(current,3,1,*slot,out))return out;
        if(parent.raw[3]>2){out.status=SourceEffect2Status::UnknownDispatch;return out;}
        if(parent.raw[3]==2){out.committed=true;return out;}
        if(!need(current,11,1,*slot,out))return out;
        const uint8_t period_mask=parent.raw[3]==0?3:7;
        const bool spawn=(parent.raw[11]&period_mask)==0;
        const auto clock_next=uint8_t(parent.raw[11]+1u);
        release=parent.raw[3]==1 && clock_next>=17;
        if(release && !release_preflight(context,current,*slot,out))return out;
        if(spawn){
            if(!need(current,14,1,*slot,out)||!need(current,28,6,*slot,out)||!masks(context,false,out))return out;
            for(size_t candidate=0;candidate<64;++candidate)if(!(*context.pool80.used[candidate/32]&bit(candidate))){child_slot=candidate;break;}
            if(!child_slot){out.status=SourceEffect2Status::SourceAllocationExhausted;return out;}
            if(*child_slot==*slot){out.status=SourceEffect2Status::InvalidPool;return out;}
            out.child_identity=source_effect_base+uint32_t(*child_slot*72);
            const auto& target=context.pool80.slots[*child_slot];
            if(!need(target,1,1,*child_slot,out))return out;
            if(target.raw[1]!=uint8_t(0x80u|*child_slot)){out.status=SourceEffect2Status::InvalidIdentity;return out;}
            child=target;
            for(size_t i=2;i<20;++i)put8(child,i,0);
            put8(child,6,3);put8(child,0,3);put8(child,2,2);put8(child,3,parent.raw[3]);put8(child,12,2);put8(child,13,1);put8(child,14,parent.raw[14]);
            for(size_t o:{size_t(28),size_t(30),size_t(32)})put16(child,o,get16(parent.raw,o));
            if(parent.raw[3]==0){
                out.rng_required=3;
                if(!in.rng || in.rng_count!=3){out.status=SourceEffect2Status::NeedsRngReceipts;return out;}
                // Auditor ordinals are positive and monotone; their three-call
                // reservation may not wrap the shared64-bit receipt sequence.
                if(in.rng[0].global_ordinal==0 || in.rng[0].global_ordinal>std::numeric_limits<uint64_t>::max()-2u){out.status=SourceEffect2Status::RngContextMismatch;return out;}
                for(size_t i=0;i<3;++i){const auto& receipt=in.rng[i];
                    if(receipt.callback_identity!=source_effect2_callback || receipt.parent_identity!=identity || receipt.child_identity!=out.child_identity || receipt.parent_clock11!=parent.raw[11] || receipt.parent_parameter03!=parent.raw[3] || receipt.parent_mode0d!=parent.raw[13] || receipt.parent_tag0e!=parent.raw[14] || receipt.source_generation==0 || receipt.source_generation!=in.source_generation || receipt.global_ordinal!=in.rng[0].global_ordinal+i || receipt.parent_xyz1c!=std::array<uint16_t,3>{{get16(parent.raw,28),get16(parent.raw,30),get16(parent.raw,32)}} || receipt.return_address!=out.rng_return_addresses[i]){out.status=SourceEffect2Status::RngContextMismatch;return out;}
                }
                const auto x=signed32(in.rng[0].v0)%20;
                const auto y=shift_signed(signed32(in.rng[1].v0),8)%38;
                const auto z=shift_signed(signed32(in.rng[2].v0),4)%20;
                put16(child,28,uint16_t(uint32_t(get16(child.raw,28))+uint32_t(x)-10u));
                put16(child,30,uint16_t(uint32_t(get16(child.raw,30))+uint32_t(y)-19u));
                put16(child,32,uint16_t(uint32_t(get16(child.raw,32))+uint32_t(z)-10u));
                put32(child,20,parent.raw[14]==7?0x3d48000eu:0x3d48002eu);put32(child,24,0x2e000000u);
                put8(child,34,160);put8(child,35,96);put16(child,36,3);put8(child,11,24);
                out.rng_consumed=3;
            }else{
                uint32_t word=0;if(!color(current,*slot,in,word,out))return out;
                put32(child,20,parent.raw[14]==7?0x3c880006u:0x3c880026u);put32(child,24,(word<<7)|0x2e000000u);
                put8(child,34,128);put8(child,35,0);put16(child,36,1);put8(child,11,32);put32(child,40,256);put32(child,44,256);
            }
        }
        put8(parent,11,clock_next);
    }else{
        if(!need(current,8,1,*slot,out))return out;
        if(parent.raw[8]>1){out.status=SourceEffect2Status::UnknownDispatch;return out;}
        if(parent.raw[8]==1){if(!release_preflight(context,current,*slot,out))return out;release=true;}
        else{
            if(!need(current,3,1,*slot,out)||!need(current,11,1,*slot,out))return out;
            if(parent.raw[3]==0){
                uint32_t word=0;if(!need(current,24,4,*slot,out)||!color(current,*slot,in,word,out))return out;
                put32(parent,24,parent.raw[11]>=9?get32(parent.raw,24)+(word<<3):get32(parent.raw,24)-(word<<4));
            }else if(parent.raw[3]==1){
                uint32_t word=0;if(!need(current,24,4,*slot,out)||!need(current,40,8,*slot,out)||!color(current,*slot,in,word,out))return out;
                const uint32_t old=get32(parent.raw,40),next=get32(parent.raw,44)+old;
                put32(parent,40,old-8u);put32(parent,44,next);put32(parent,24,get32(parent.raw,24)-(word<<2));put16(parent,36,uint16_t(next>>8));
            }
            const uint8_t timer=uint8_t(parent.raw[11]-1u);put8(parent,11,timer);if(timer==0)put8(parent,8,uint8_t(parent.raw[8]+1u));
        }
    }
    if(release){put8(parent,0,0);out.released=true;}
    // Publication has no fallible operation. All untouched raw/known lanes survive.
    context.pool80.slots[*slot]=parent;
    if(child_slot){context.pool80.slots[*child_slot]=child;*context.pool80.used[*child_slot/32]|=bit(*child_slot);out.child_allocated=true;}
    if(release){const size_t bank=*slot/32;*context.pool80_pending_release_800988c8[bank]=(*context.pool80_pending_release_800988c8[bank]|bit(*slot))&*context.pool80.used[bank];}
    out.committed=true;return out;
}
SourceEffect2Result tick_source_effect2_candidate_rng(SourceEventPhase5Context& context,
    uint32_t identity,const SourceEffect2Inputs& inputs,SourceRngService& rng,
    const SourceRngUpdateContext& update) noexcept {
    // Never replace a caller's explicit receipts with candidate values.
    const auto fallback=[&]() noexcept { return tick_source_effect2_80116e68(context,identity,inputs); };
    const auto slot=source_effect_slot(identity);
    if(!slot||inputs.rng||inputs.rng_count||inputs.callback_identity!=source_effect2_callback||
       !inputs.source_generation||!rng.admitted())return fallback();
    const auto& parent=context.pool80.slots[*slot];
    for(size_t lane:{size_t(0),size_t(2),size_t(3),size_t(6),size_t(11),size_t(13),size_t(14),
                     size_t(28),size_t(29),size_t(30),size_t(31),size_t(32),size_t(33)})
        if(!parent.known[lane])return fallback();
    if(!(parent.raw[0]&parent.raw[6]&1u)||parent.raw[2]!=2||parent.raw[13]!=0||
       parent.raw[3]!=0||(parent.raw[11]&3u)||!context.pool80.used[0]||!context.pool80.used[1])
        return fallback();
    std::optional<size_t> child;
    for(size_t i=0;i<64;++i)if(!(*context.pool80.used[i/32]&bit(i))){child=i;break;}
    if(!child||*child==*slot)return fallback();
    SourceRngUpdateTransaction transaction;
    if(begin_source_rng_update(rng,transaction,update)!=SourceRngStatus::Ready)return fallback();
    auto staged=inputs;std::array<SourceEffect2RngReceipt,3> receipts{};
    for(uint8_t i=0;i<3;++i){
        const auto value=stage_source_rng_effect2(transaction,{identity,inputs.source_generation,uint8_t(*slot),i});
        if(value.status!=SourceRngStatus::Ready){rollback_source_rng_update(transaction);return fallback();}
        auto& receipt=receipts[i];receipt.callback_identity=source_effect2_callback;
        receipt.parent_identity=identity;receipt.child_identity=source_effect_base+uint32_t(*child*72);
        receipt.return_address=SourceEffect2Result{}.rng_return_addresses[i];receipt.v0=value.sample32a;
        receipt.global_ordinal=value.native_global_ordinal;receipt.source_generation=inputs.source_generation;
        receipt.parent_xyz1c={{get16(parent.raw,28),get16(parent.raw,30),get16(parent.raw,32)}};
        receipt.parent_clock11=parent.raw[11];receipt.parent_parameter03=parent.raw[3];
        receipt.parent_mode0d=parent.raw[13];receipt.parent_tag0e=parent.raw[14];
    }
    staged.rng=receipts.data();staged.rng_count=receipts.size();
    // The original leaf can touch only this parent, its one allocated child,
    // and the two allocation/deferred-release banks. No whole scene copy.
    const auto saved_parent=parent,saved_child=context.pool80.slots[*child];
    const auto saved_used=context.pool80.used,saved_pending=context.pool80_pending_release_800988c8;
    auto result=tick_source_effect2_80116e68(context,identity,staged);
    if(!result.committed){rollback_source_rng_update(transaction);return result;}
    if(commit_source_rng_update(rng,transaction)!=SourceRngStatus::Ready){
        context.pool80.slots[*slot]=saved_parent;context.pool80.slots[*child]=saved_child;
        context.pool80.used=saved_used;context.pool80_pending_release_800988c8=saved_pending;
        rollback_source_rng_update(transaction);result={};result.status=SourceEffect2Status::NeedsRngReceipts;
    }
    return result;
}
} // namespace l3
