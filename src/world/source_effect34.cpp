#include "legends3ds/source_effect34.h"
#include <limits>

namespace l3 {
namespace {
using Record=SourceEventPhase5Bytes<72>;
using Status=SourceEffect34Status;
constexpr uint32_t base=0x800a3b40u;
uint32_t bit(size_t s) noexcept {return 0x80000000u>>(s&31u);}
uint16_t u16(const Record& r,size_t o) noexcept {return uint16_t(r.raw[o]|(uint16_t(r.raw[o+1])<<8));}
int32_t s8(uint8_t v) noexcept {return v<128u?int32_t(v):int32_t(v)-256;}
void put(Record& r,size_t o,size_t n,uint32_t v) noexcept {
    for(size_t i=0;i<n;++i){r.raw[o+i]=uint8_t(v>>(i*8));r.known[o+i]=1;}
}
bool need(const Record& r,size_t o,size_t n,size_t slot,SourceEffect34Result& out) noexcept {
    for(size_t i=0;i<n;++i)if(!r.known[o+i]){out.status=Status::NeedsField;out.missing_slot=uint8_t(slot);out.missing_offset=uint8_t(o+i);return false;}
    return true;
}
bool masks(const SourceEventPhase5Context& c,bool release,SourceEffect34Result& out) noexcept {
    for(size_t b=0;b<2;++b){
        if(!c.pool80.used[b]){out.status=Status::NeedsPoolBitmap;return false;}
        if(release&&!c.pool80_pending_release_800988c8[b]){out.status=Status::NeedsReleaseBitmap;return false;}
        if(c.pool80_pending_release_800988c8[b]&&(*c.pool80_pending_release_800988c8[b]&~*c.pool80.used[b])){out.status=Status::InvalidPool;return false;}
    }return true;
}
bool control(const SourceEffect34Inputs& in,uint8_t subtype,SourceEffect34Result& out) noexcept {
    if(!in.controls||!in.controls->admitted){out.status=Status::NeedsControls;return false;}
    if((subtype==3&&(in.controls->sequence3_identity!=source_effect3_sequence||in.controls->sequence3[10]!=0xffffu))||
        (subtype==4&&(in.controls->rows4_identity!=source_effect4_rows||in.controls->rows4[4]!=std::array<uint8_t,4>{}))) {
        out.status=Status::ControlContextMismatch;return false;
    }return true;
}
bool release_guard(const SourceEventPhase5Context& c,const Record& r,size_t slot,SourceEffect34Result& out) noexcept {
    if(!need(r,1,1,slot,out))return false;
    if(r.raw[1]!=uint8_t(0x80u|slot)){out.status=Status::InvalidIdentity;return false;}
    return masks(c,true,out);
}
bool receipts(const SourceEffect34Inputs& in,const SourceEffect34Result& plan,
    const Record& parent,uint32_t id,uint32_t callback,uint8_t subtype,SourceEffect34Result& out) noexcept {
    if(!in.rng||in.rng_count!=plan.rng_required||in.source_generation==0){out.status=Status::NeedsRngReceipts;return false;}
    const uint64_t count=plan.rng_required,first=in.rng[0].global_ordinal;
    if(count==0||first==0||first>std::numeric_limits<uint64_t>::max()-(count-1u)){
        out.status=Status::RngContextMismatch;return false;
    }
    const auto xyz=std::array<uint16_t,3>{{u16(parent,28),u16(parent,30),u16(parent,32)}};
    for(size_t i=0;i<plan.rng_required;++i){const auto& r=in.rng[i];
        if(r.callback_identity!=callback||r.parent_identity!=id||r.child_identity!=plan.child_identities[i/(subtype==3?2u:5u)]||
            r.return_address!=plan.rng_return_addresses[i]||r.source_generation!=in.source_generation||r.parent_xyz1c!=xyz||
            r.parent_clock09!=parent.raw[9]||r.parent_parameter03!=parent.raw[3]||r.parent_mode0d!=parent.raw[13]||(subtype==3&&r.parent_tag0f!=parent.raw[15])||
            r.global_ordinal!=first+i) {
            out.status=Status::RngContextMismatch;return false;
        }
    }return true;
}
void init_child(Record& r,uint8_t subtype) noexcept {
    for(size_t i=2;i<20;++i)put(r,i,1,0);
    put(r,6,1,3);put(r,0,1,3);put(r,2,1,subtype);put(r,12,1,subtype);put(r,13,1,1);
}
}
SourceEffect34Result tick_source_effect34(SourceEventPhase5Context& context,uint32_t identity,
    uint32_t callback,const SourceEffect34Inputs& in) noexcept {
    SourceEffect34Result out;
    const uint8_t subtype=callback==source_effect3_callback?3:callback==source_effect4_callback?4:0;
    if(!subtype){out.status=Status::UnknownCallback;return out;}
    if(identity<base||(identity-base)%72u||(identity-base)/72u>=64u){out.status=Status::InvalidIdentity;return out;}
    const size_t slot=(identity-base)/72u;const auto& current=context.pool80.slots[slot];
    if(!need(current,0,1,slot,out)||!need(current,6,1,slot,out))return out;
    if(!(current.raw[0]&current.raw[6]&1u)){out.status=Status::Inactive;return out;}
    if(!need(current,2,1,slot,out)||!need(current,13,1,slot,out))return out;
    if(current.raw[2]!=subtype){out.status=Status::UnknownCallback;return out;}
    auto parent=current;
    std::array<Record,24> children;
    std::array<uint8_t,24> slots{};
    std::array<uint32_t,2> allocation{};
    uint8_t child_count=0;bool release=false,allocated=false;
    if(parent.raw[13]==0){
        if(!need(current,3,1,slot,out))return out;
        if(parent.raw[3]>uint8_t(subtype==3?2:0)){out.status=Status::UnknownDispatch;return out;}
        if(parent.raw[3]){out.committed=true;return out;}
        if(!need(current,9,1,slot,out))return out;
        if(!(parent.raw[9]&3u)){
            if(!masks(context,false,out))return out;
            allocation={{*context.pool80.used[0],*context.pool80.used[1]}};
            const size_t maximum=subtype==3?1:24;
            for(size_t attempt=0;attempt<maximum;++attempt){
                size_t selected=64;
                for(size_t candidate=0;candidate<64;++candidate)if(!(allocation[candidate/32]&bit(candidate))){selected=candidate;break;}
                if(selected==64){out.source_null_allocation=true;break;}
                if(selected==slot){out.status=Status::InvalidPool;return out;}
                const auto& target=context.pool80.slots[selected];
                if(!need(target,1,1,selected,out))return out;
                if(target.raw[1]!=uint8_t(0x80u|selected)){out.status=Status::InvalidIdentity;return out;}
                allocation[selected/32]|=bit(selected);slots[child_count]=uint8_t(selected);
                children[child_count]=target;out.child_identities[child_count]=base+uint32_t(selected*72u);++child_count;
            }
            if(subtype==3&&child_count==0){out.committed=true;return out;}
            if(child_count){
                if(!need(current,28,6,slot,out)||(subtype==3&&!need(current,15,1,slot,out)))return out;
                if(!control(in,subtype,out))return out;
                out.rng_required=uint8_t(child_count*(subtype==3?2u:5u));
                for(size_t i=0;i<child_count;++i){
                    if(subtype==3){out.rng_return_addresses[i*2]=0x801173c4u;out.rng_return_addresses[i*2+1]=0x80117408u;}
                    else {out.rng_return_addresses[i*5]=i<12?0x801177b4u:0x801177e8u;out.rng_return_addresses[i*5+1]=i<12?0x801177d0u:0x80117804u;
                        out.rng_return_addresses[i*5+2]=0x80117814u;out.rng_return_addresses[i*5+3]=0x801178a0u;out.rng_return_addresses[i*5+4]=0x801178b4u;}
                }
                if(!receipts(in,out,current,identity,callback,subtype,out))return out;
                for(size_t i=0;i<child_count;++i){auto& child=children[i];init_child(child,subtype);
                    if(subtype==3){const auto* r=in.rng+i*2;
                        put(child,28,2,uint16_t(uint32_t(u16(current,28))+(r[0].v0&7u)-3u));
                        put(child,30,2,uint16_t(uint32_t(u16(current,30))+(current.raw[15]==0?48u:24u)));
                        put(child,32,2,uint16_t(uint32_t(u16(current,32))+(r[1].v0&7u)-3u));
                        put(child,20,4,0x3d080026u);put(child,24,4,0x2c804040u);
                        put(child,34,1,uint8_t(in.controls->sequence3[0]));put(child,35,1,0);
                        put(child,36,2,current.raw[15]==0?48u:24u);put(child,10,1,1);
                    }else{const auto* r=in.rng+i*5;
                        put(child,32,2,uint16_t(uint32_t(u16(current,32))+(r[0].v0&15u)+(i<12?uint32_t(-71):57u)));
                        put(child,14,1,uint8_t(i<12?7u-(r[1].v0&15u):8u+(r[1].v0&7u)));
                        put(child,28,2,uint16_t(uint32_t(u16(current,28))+uint32_t((i%12u)*64u)+(r[2].v0&63u)-351u));
                        put(child,30,2,u16(current,30));put(child,20,4,0x3d48002eu);put(child,24,4,0x2e403028u);
                        put(child,34,1,in.controls->rows4[0][0]);put(child,35,1,0);
                        put(child,36,2,(r[3].v0&7u)+29u);put(child,9,1,uint8_t(in.controls->rows4[0][2]+(r[4].v0&3u)));
                    }
                }
                allocated=true;
            }
        }
        if(current.raw[9]>=4){if(!need(current,8,1,slot,out))return out;put(parent,8,1,uint8_t(current.raw[8]+1u));put(parent,13,1,uint8_t(current.raw[13]+1u));}
        put(parent,9,1,uint8_t(current.raw[9]+1u));
    }else{
        if(!need(current,8,1,slot,out))return out;
        if(current.raw[8]>1){out.status=Status::UnknownDispatch;return out;}
        if(current.raw[8]==1){if(!release_guard(context,current,slot,out))return out;release=true;}
        else{
            if(!need(current,10,1,slot,out)||!need(current,30,2,slot,out)||!control(in,subtype,out))return out;
            const uint8_t phase=current.raw[10];
            if(subtype==3){
                if(phase>=10){out.status=Status::ControlContextMismatch;return out;}
                put(parent,30,2,uint16_t(u16(current,30)+6u));put(parent,10,1,uint8_t(phase+1u));
                put(parent,34,1,uint8_t(in.controls->sequence3[phase]));
                if(in.controls->sequence3[phase+1u]==0xffffu)put(parent,8,1,uint8_t(current.raw[8]+1u));
            }else{
                if(phase>=4){out.status=Status::ControlContextMismatch;return out;}
                if(!need(current,14,1,slot,out)||!need(current,32,2,slot,out)||!need(current,9,1,slot,out)||!need(current,36,2,slot,out))return out;
                const auto& row=in.controls->rows4[phase];
                put(parent,32,2,uint16_t(uint32_t(u16(current,32))+uint32_t(s8(current.raw[14]))));
                put(parent,30,2,uint16_t(uint32_t(u16(current,30))+uint32_t(s8(row[1]))));
                put(parent,34,1,row[0]);put(parent,36,2,uint16_t(uint32_t(u16(current,36))+uint32_t(s8(row[3]))));
                const uint8_t clock=uint8_t(current.raw[9]-1u);put(parent,9,1,clock);
                if(clock==0){put(parent,10,1,uint8_t(phase+1u));put(parent,9,1,in.controls->rows4[phase+1u][2]);
                    if(in.controls->rows4[phase+1u][2]==0)put(parent,8,1,uint8_t(current.raw[8]+1u));}
            }
        }
    }
    if(release){put(parent,0,1,0);out.released=true;}
    // All publication follows complete field/receipt preflight. Provider state
    // stays outside this callback; its ticket commits with these scalar owners.
    context.pool80.slots[slot]=parent;
    if(allocated){for(size_t i=0;i<child_count;++i)context.pool80.slots[slots[i]]=children[i];
        context.pool80.used[0]=allocation[0];context.pool80.used[1]=allocation[1];out.allocated_count=child_count;out.rng_consumed=out.rng_required;}
    if(release){const size_t b=slot/32;*context.pool80_pending_release_800988c8[b]=(*context.pool80_pending_release_800988c8[b]|bit(slot))&*context.pool80.used[b];}
    out.committed=true;return out;
}
SourceEffect34Result tick_source_effect34_candidate_rng(SourceEventPhase5Context& context,
    uint32_t identity,uint32_t callback,const SourceEffect34Inputs& inputs,SourceRngService& rng,
    const SourceRngUpdateContext& update) noexcept {
    auto result=tick_source_effect34(context,identity,callback,inputs);
    if(result.committed||result.status!=Status::NeedsRngReceipts||inputs.rng||inputs.rng_count||
       !rng.admitted()||!inputs.source_generation)return result;
    const size_t slot=(identity-base)/72u;const auto& parent=context.pool80.slots[slot];
    const uint8_t subtype=parent.raw[2],per_child=subtype==3?2:5;
    const size_t child_count=result.rng_required/per_child;
    if(!child_count||child_count>source_effect34_maximum_children||result.rng_required>source_effect34_maximum_rng)return result;
    SourceRngUpdateTransaction transaction;if(begin_source_rng_update(rng,transaction,update)!=SourceRngStatus::Ready)return result;
    std::array<SourceEffect34RngReceipt,source_effect34_maximum_rng> receipts{};
    for(uint8_t i=0;i<result.rng_required;++i){
        const auto value=stage_source_rng_effect34(transaction,{identity,callback,inputs.source_generation,uint8_t(slot),i,result.rng_required});
        if(value.status!=SourceRngStatus::Ready){rollback_source_rng_update(transaction);return result;}
        auto& receipt=receipts[i];receipt.callback_identity=callback;receipt.parent_identity=identity;
        receipt.child_identity=result.child_identities[i/per_child];receipt.return_address=result.rng_return_addresses[i];receipt.v0=value.sample32a;
        receipt.global_ordinal=value.native_global_ordinal;receipt.source_generation=inputs.source_generation;
        receipt.parent_xyz1c={{u16(parent,28),u16(parent,30),u16(parent,32)}};
        receipt.parent_clock09=parent.raw[9];receipt.parent_parameter03=parent.raw[3];receipt.parent_mode0d=parent.raw[13];
        if(subtype==3)receipt.parent_tag0f=parent.raw[15];
    }
    const auto saved_parent=parent;const auto saved_used=context.pool80.used,saved_pending=context.pool80_pending_release_800988c8;
    std::array<uint8_t,source_effect34_maximum_children> slots{};
    std::array<SourceEventPhase5Bytes<72>,source_effect34_maximum_children> saved{};
    for(size_t i=0;i<child_count;++i){slots[i]=uint8_t((result.child_identities[i]-base)/72u);saved[i]=context.pool80.slots[slots[i]];}
    auto staged=inputs;staged.rng=receipts.data();staged.rng_count=result.rng_required;
    result=tick_source_effect34(context,identity,callback,staged);
    if(!result.committed){rollback_source_rng_update(transaction);return result;}
    if(commit_source_rng_update(rng,transaction)!=SourceRngStatus::Ready){
        context.pool80.slots[slot]=saved_parent;context.pool80.used=saved_used;context.pool80_pending_release_800988c8=saved_pending;
        for(size_t i=0;i<child_count;++i)context.pool80.slots[slots[i]]=saved[i];
        rollback_source_rng_update(transaction);result={};result.status=Status::NeedsRngReceipts;
    }
    return result;
}
} // namespace l3
