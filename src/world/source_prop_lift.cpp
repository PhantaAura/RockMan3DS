#include "legends3ds/source_prop_lift.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <limits>
#include <stdexcept>
namespace l3 {
namespace {
uint16_t u16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t u32(const uint8_t* p) noexcept {return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);}
uint8_t hex_digit(char c) noexcept {return uint8_t(c<='9'?c-'0':c-'a'+10);}
bool digest_matches(const uint8_t* bytes,const char* text) noexcept {
    for(size_t i=0;i<32;++i)if(bytes[i]!=uint8_t((hex_digit(text[2*i])<<4)|hex_digit(text[2*i+1])))return false;
    return true;
}
uint32_t rotate(uint32_t word,unsigned bits) noexcept {return (word>>bits)|(word<<(32-bits));}
// Small fixed load-time SHA-256; no frame-loop hashing or allocations.
std::array<uint8_t,32> scalar_sha256(const uint8_t* bytes,size_t size) noexcept {
    constexpr std::array<uint32_t,64> k{{
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2}};
    std::array<uint32_t,8> h{{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}};
    const size_t blocks=(size+9+63)/64;const uint64_t bits=uint64_t(size)*8;
    for(size_t block=0;block<blocks;++block){
        std::array<uint32_t,64> w{};
        for(size_t i=0;i<64;++i){const size_t at=block*64+i;uint8_t b=0;
            if(at<size)b=bytes[at];else if(at==size)b=0x80;
            else if(at>=blocks*64-8)b=uint8_t(bits>>((blocks*64-1-at)*8));
            w[i/4]|=uint32_t(b)<<((3-i%4)*8);}
        for(size_t i=16;i<64;++i){const auto a=w[i-15],b=w[i-2];
            w[i]=w[i-16]+(rotate(a,7)^rotate(a,18)^(a>>3))+w[i-7]+(rotate(b,17)^rotate(b,19)^(b>>10));}
        auto v=h;
        for(size_t i=0;i<64;++i){const uint32_t a=v[7]+(rotate(v[4],6)^rotate(v[4],11)^rotate(v[4],25))+
                ((v[4]&v[5])^(~v[4]&v[6]))+k[i]+w[i];
            const uint32_t b=(rotate(v[0],2)^rotate(v[0],13)^rotate(v[0],22))+
                ((v[0]&v[1])^(v[0]&v[2])^(v[1]&v[2]));
            v={{a+b,v[0],v[1],v[2],v[3]+a,v[4],v[5],v[6]}};}
        for(size_t i=0;i<8;++i)h[i]+=v[i];
    }
    std::array<uint8_t,32> out{};for(size_t i=0;i<32;++i)out[i]=uint8_t(h[i/4]>>((3-i%4)*8));return out;
}
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
}
SourcePropLiftProfile load_source_prop_lift_profile(const std::vector<uint8_t>& bytes) {
    require(bytes.size()==384,"L3PL exact coverage rejected");const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='P'&&p[3]=='L',"L3PL magic rejected");
    require(u16(p+4)==1&&u16(p+6)==160,"L3PL version/header rejected");
    require(u32(p+8)==1&&u32(p+12)==3&&u32(p+16)==64&&u32(p+20)==384&&u32(p+28)==0,"L3PL framing rejected");
    require(u32(p+24)==crc32(p+32,352),"L3PL CRC rejected");
    require(digest_matches(p+32,"10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0")&&
        digest_matches(p+64,"9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c")&&
        digest_matches(p+96,"ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15"),"L3PL owner identity rejected");
    const std::array<uint32_t,8> metadata{{kSourcePropLiftCurveTable,0x8011c958u,0x8011c998u,0x8011c9d8u,kSourcePropLiftCallback,442,2,0}};
    for(size_t i=0;i<8;++i)require(u32(p+128+i*4)==metadata[i],"L3PL source metadata rejected");
    for(size_t i=352;i<384;++i)require(p[i]==0,"L3PL reserved bytes rejected");
    const auto digest=scalar_sha256(p+160,192);
    require(digest_matches(digest.data(),"cb7f79c1f39c21a17b69dd55f71a149dd2f7429acd5e833e64fb15d1c2f96a54"),"L3PL original curve windows rejected");
    SourcePropLiftProfile out;out.curves_.source_table_identity=kSourcePropLiftCurveTable;
    out.curves_.curve_identities={{0x8011c958u,0x8011c998u,0x8011c9d8u}};
    for(size_t i=0;i<3;++i)for(size_t f=0;f<64;++f)out.curves_.windows[i][f]=p[160+i*64+f];
    for(size_t i=0;i<32;++i)out.ebd_digest_[i]=p[96+i];
    out.curves_.admitted=true;return out;
}
namespace {
using Record=SourceEventPhase5Bytes<80>;using Status=SourcePropLiftStatus;
int32_t signed16(uint16_t value) noexcept {return value<32768u?int32_t(value):int32_t(value)-65536;}
int32_t signed32(uint32_t value) noexcept {return value<=0x7fffffffu?int32_t(value):int32_t(int64_t(value)-0x100000000ll);}
template<class R>bool need(const R& r,size_t at,size_t n,uint8_t slot,SourcePropLiftResult& out) noexcept {
    for(size_t i=0;i<n;++i){if(!r.known[at+i]){out.status=Status::NeedsField;out.missing_slot=slot;out.missing_offset=uint16_t(at+i);return false;}}
    return true;
}
template<class R>void put(R& r,size_t at,size_t n,uint32_t value) noexcept {
    for(size_t i=0;i<n;++i){r.raw[at+i]=uint8_t(value>>(i*8));r.known[at+i]=1;}
}
template<size_t Count,size_t Bytes>SourceEventPhase5Bytes<Bytes>* resolve(
    SourceEventPhase5Pool<Count,Bytes>& pool,uint32_t identity,uint32_t base,uint8_t family) noexcept {
    if(identity<base){return nullptr;}
    const uint32_t delta=identity-base;
    if(delta%Bytes||delta/Bytes>=Count){return nullptr;}
    const size_t slot=delta/Bytes;
    if(!pool.used[slot/32]||!(*pool.used[slot/32]&(0x80000000u>>(slot&31u)))){return nullptr;}
    auto& r=pool.slots[slot];
    if(!r.known[1]||r.raw[1]!=uint8_t(family|slot)){return nullptr;}
    return &r;
}
void identity_matrix(Record& r) noexcept {
    for(size_t i=0;i<8;++i)put(r,24+i*4,4,(i==0||i==2||i==4)?4096u:0u);
}
}
SourcePropLiftResult consume_source_prop_lift_80113280(SourceEventPhase5Context& context,
    const SourceEventWork& event,uint32_t identity,const SourcePropLiftInputs& in) noexcept {
    SourcePropLiftResult out;
    auto* current=resolve(context.pool_e0,identity,0x800bf118u,0xe0);
    if(!current){out.status=Status::InvalidIdentity;return out;}
    const uint8_t slot=uint8_t((identity-0x800bf118u)/80u);
    if(!need(*current,0,1,slot,out)||!need(*current,6,1,slot,out))return out;
    if(!(current->raw[0]&current->raw[6]&1u)){out.status=Status::Inactive;return out;}
    if(!need(*current,2,2,slot,out)||!need(*current,8,3,slot,out))return out;
    if(current->raw[2]!=3||current->raw[3]!=2||current->raw[8]==0){out.status=Status::UnsupportedScope;return out;}
    const uint8_t state=current->raw[9],sub=current->raw[10];
    if((state!=1&&state!=2)||(state==1&&sub>4)||(state==2&&sub>2)){out.status=Status::UnsupportedState;return out;}
    Record next=*current;Record* linked=nullptr;uint8_t linked_state=0;bool write_link=false;
    std::optional<size_t> child_slot;SourceEventPhase5Bytes<72> child;uint32_t child_mask=0;
    const bool initial=state==1&&sub==0,lift=state==1&&sub==1;
    const bool drop_initial=(state==1&&sub==2)||(state==2&&sub==0);
    const bool drop=(state==1&&sub==3)||(state==2&&sub==1);
    if(initial||lift){
        if(!need(event,0x24,4,slot,out)){out.status=Status::NeedsEventBinding;return out;}
        const uint32_t actor_id=u32(event.raw.data()+0x24);
        const auto* actor=resolve(context.pool40,actor_id,0x800c3580u,0x40);
        if(!actor){out.status=Status::NeedsActorOwner;return out;}
        if(!need(*actor,442,2,slot,out))return out;
        const uint16_t bone_y=u16(actor->raw.data()+442);
        if(initial){out.source_function=0x80113194u;identity_matrix(next);put(next,58,2,bone_y);put(next,10,1,uint32_t(sub)+1u);}
        else {
            out.source_function=0x801131d8u;
            if(!need(*actor,14,1,slot,out)||!need(*actor,152,4,slot,out)||!need(*current,58,2,slot,out))return out;
            if(!in.frame||!in.frame->admitted){out.status=Status::NeedsFrameReceipt;return out;}
            const uint32_t sequence=u32(actor->raw.data()+152);const uint8_t selector=actor->raw[14];
            if(!in.source_generation||in.frame->source_generation!=in.source_generation||in.frame->source_actor!=actor_id||in.frame->sequence_identity!=sequence||in.frame->frame>=64){out.status=Status::FrameContextMismatch;return out;}
            if(!in.curves||!in.curves->admitted){out.status=Status::NeedsCurves;return out;}
            if(in.curves->source_table_identity!=kSourcePropLiftCurveTable||in.curves->curve_identities!=std::array<uint32_t,3>{{0x8011c958u,0x8011c998u,0x8011c9d8u}}||selector>=3){out.status=Status::CurveContextMismatch;return out;}
            const int32_t desired=signed16(bone_y)-in.curves->windows[selector][in.frame->frame];
            if(signed16(u16(current->raw.data()+58))>=desired){
                // The typed RNG transaction additionally binds all original prop
                // position halfwords, including X/Z read only by a child branch.
                if(!need(*current,56,6,slot,out))return out;
                const std::array<uint16_t,3> xyz{{u16(current->raw.data()+56),u16(current->raw.data()+58),u16(current->raw.data()+60)}};
                const auto receipt=[&](size_t index,uint32_t child_id) noexcept {
                    if(!in.rng||in.rng_count<=index||!in.current_rng_owner||!in.current_rng_owner->owner||!in.current_rng_owner->generation){out.status=Status::NeedsRngReceipts;return false;}
                    const auto& r=in.rng[index];
                    if(r.owner!=in.current_rng_owner->owner||r.owner_generation!=in.current_rng_owner->generation||r.source_generation!=in.source_generation||r.global_ordinal==0||r.callback_identity!=kSourcePropLiftCallback||r.prop_identity!=identity||r.actor_identity!=actor_id||r.sequence_identity!=sequence||r.return_address!=out.rng_return_addresses[index]||r.child_identity!=child_id||r.prop_before_xyz!=xyz||r.actor_part7_y!=bone_y||r.actor_selector14!=selector||r.sequence_frame!=in.frame->frame||r.state09!=state||r.sub10!=sub||
                       (index&&(in.rng[index-1].global_ordinal==std::numeric_limits<uint64_t>::max()||r.global_ordinal!=in.rng[index-1].global_ordinal+1u))){out.status=Status::RngContextMismatch;return false;}
                    return true;
                };
                out.rng_required=1;if(!receipt(0,0))return out;
                if(signed32(in.rng[0].v0)%6==0){
                    for(size_t bank=0;bank<2&&!child_slot;++bank){
                        if(!context.pool80.used[bank]){out.status=Status::NeedsPoolBitmap;return out;}
                        const uint32_t used=*context.pool80.used[bank];
                        for(size_t bit=0;bit<32;++bit){const uint32_t mask=0x80000000u>>bit;
                            if(!(used&mask)){child_slot=bank*32+bit;child_mask=used|mask;break;}}
                    }
                    if(child_slot){
                        const auto& before=context.pool80.slots[*child_slot];
                        if(!before.known[1]||before.raw[1]!=uint8_t(0x80u|*child_slot)){out.status=Status::NeedsPoolHeader;return out;}
                        out.child_identity=0x800a3b40u+uint32_t(*child_slot*72u);out.rng_required=2;
                        if(!receipt(1,out.child_identity))return out;
                        child=before;for(size_t i=2;i<20;++i)put(child,i,1,0);put(child,6,1,3);
                        put(child,0,1,1);put(child,2,1,3);put(child,3,1,0);put(child,12,1,3);put(child,13,1,0);put(child,15,1,1);
                        put(child,28,2,uint32_t(xyz[0])+(in.rng[1].v0&255u)-127u);
                        put(child,30,2,uint16_t(desired));put(child,32,2,xyz[2]);put(child,8,1,0);put(child,9,1,0);
                    }else out.source_null_allocation=true;
                }
                if(in.rng_count!=out.rng_required){out.status=Status::RngContextMismatch;return out;}
                put(next,58,2,uint16_t(desired));out.rng_consumed=out.rng_required;
            }
        }
    }else if(drop_initial||drop){
        if(drop_initial){out.source_function=0x801132bcu;
            if(!need(event,0x4c,4,slot,out)){out.status=Status::NeedsEventBinding;return out;}
            linked=resolve(context.pool_e0,u32(event.raw.data()+0x4c),0x800bf118u,0xe0);
            if(!linked||linked==current){out.status=Status::NeedsEventBinding;return out;}
            put(next,12,4,0);put(next,10,1,uint32_t(sub)+1u);write_link=true;linked_state=1;
        }else {out.source_function=0x801132ecu;
            if(!need(*current,12,4,slot,out))return out;
            const uint32_t value=u32(current->raw.data()+12)+0xfffec000u;
            if(signed32(value)<=-50*65536){
                if(!need(event,0x4c,4,slot,out)){out.status=Status::NeedsEventBinding;return out;}
                linked=resolve(context.pool_e0,u32(event.raw.data()+0x4c),0x800bf118u,0xe0);
                if(!linked||linked==current){out.status=Status::NeedsEventBinding;return out;}
                put(next,10,1,uint32_t(sub)+1u);write_link=true;linked_state=0;
            }
            put(next,12,4,value);put(next,58,2,value>>16);
        }
    }else out.source_function=0x8011334cu;
    // All fallible guards precede publication. External RNG owner is untouched.
    *current=next;
    if(write_link){put(*linked,9,1,linked_state);put(*linked,10,1,0);}
    if(child_slot){context.pool80.slots[*child_slot]=child;context.pool80.used[*child_slot/32]=child_mask;out.child_allocated=true;}
    out.status=Status::SourceBackedPartial;out.committed=true;return out;
}
SourcePropLiftResult consume_source_prop_lift_candidate(SourceEventPhase5Context& context,
    const SourceEventWork& event,uint32_t identity,const SourcePropLiftInputs& inputs,
    const SourcePropLiftCandidateRng& candidate) noexcept {
    auto result=consume_source_prop_lift_80113280(context,event,identity,inputs);
    if(result.committed||result.status!=Status::NeedsRngReceipts||inputs.rng||inputs.rng_count||inputs.current_rng_owner||
       !candidate.service||!candidate.service->admitted()||!inputs.source_generation)return result;
    // The unchanged leaf has already checked these exact linked fields before
    // yielding its first RNG request. No lane is inferred from native time.
    const size_t prop_slot=(identity-0x800bf118u)/80u;const auto& prop=context.pool_e0.slots[prop_slot];
    const uint32_t actor_id=u32(event.raw.data()+0x24);const auto& actor=context.pool40.slots[(actor_id-0x800c3580u)/1444u];
    auto& rng=*candidate.service;SourceRngUpdateTransaction transaction;
    if(begin_source_rng_update(rng,transaction,candidate.update)!=SourceRngStatus::Ready)return result;
    const auto saved_prop=prop;const auto saved_used=context.pool80.used;
    std::optional<size_t> saved_child_slot;SourceEventPhase5Bytes<72> saved_child;
    const SourcePropLiftRngOwner owner{&rng,rng.generation()};std::array<SourcePropLiftRngReceipt,2> receipts{};
    auto staged=inputs;staged.current_rng_owner=&owner;staged.rng=receipts.data();
    for(uint8_t i=0;i<2;++i){
        const auto value=stage_source_rng_prop_lift(transaction,{identity,inputs.source_generation,uint8_t(prop_slot),i});
        if(value.status!=SourceRngStatus::Ready){rollback_source_rng_update(transaction);return result;}
        auto& receipt=receipts[i];receipt.owner=owner.owner;receipt.owner_generation=owner.generation;
        receipt.source_generation=inputs.source_generation;receipt.global_ordinal=value.native_global_ordinal;
        receipt.callback_identity=kSourcePropLiftCallback;receipt.prop_identity=identity;receipt.actor_identity=actor_id;
        receipt.sequence_identity=u32(actor.raw.data()+152);receipt.return_address=result.rng_return_addresses[i];receipt.v0=value.sample32a;
        receipt.child_identity=i?result.child_identity:0;
        receipt.prop_before_xyz={{u16(prop.raw.data()+56),u16(prop.raw.data()+58),u16(prop.raw.data()+60)}};
        receipt.actor_part7_y=u16(actor.raw.data()+442);receipt.actor_selector14=actor.raw[14];
        receipt.sequence_frame=inputs.frame->frame;receipt.state09=prop.raw[9];receipt.sub10=prop.raw[10];
        if(i){saved_child_slot=(result.child_identity-0x800a3b40u)/72u;saved_child=context.pool80.slots[*saved_child_slot];}
        staged.rng_count=size_t(i)+1;result=consume_source_prop_lift_80113280(context,event,identity,staged);
        if(result.committed){
            if(commit_source_rng_update(rng,transaction)!=SourceRngStatus::Ready){
                context.pool_e0.slots[prop_slot]=saved_prop;context.pool80.used=saved_used;
                if(saved_child_slot)context.pool80.slots[*saved_child_slot]=saved_child;
                rollback_source_rng_update(transaction);result={};result.status=Status::NeedsRngReceipts;
            }
            return result;
        }
        if(result.status!=Status::NeedsRngReceipts||result.rng_required!=2||i!=0||!result.child_identity)break;
    }
    rollback_source_rng_update(transaction);return result;
}
} // namespace l3
