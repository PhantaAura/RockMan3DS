#include "legends3ds/source_effect_dispatch.h"
#include "legends3ds/l3map.h"
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
void hold(SourceEffectDispatchBatch& out,uint8_t slot,uint8_t subtype,
    SourceEffectDispatchHoldKind kind,uint32_t callback=0,uint8_t status=0,
    std::optional<uint8_t> missing={}) noexcept {
    auto& item=out.holds[out.held++];item.slot=slot;item.subtype=subtype;item.kind=kind;
    item.callback=callback;item.consumer_status=status;item.missing_offset=missing;out.status=SourceEffectDispatchStatus::Partial;
}
}
std::optional<uint32_t> SourceEffectDispatchProfile::callback(uint8_t subtype) const noexcept {
    if(!admitted_||subtype>=callbacks_.size()){
        return std::nullopt;
    }
    return callbacks_[subtype];
}
SourceEffectDispatchProfile load_source_effect_dispatch_profile(const std::vector<uint8_t>& bytes) {
    require(bytes.size()==kSourceEffectDispatchPackageBytes,"L3E2 exact package coverage rejected");const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='E'&&p[3]=='2',"L3E2 magic rejected");
    require(u16(p+4)==1&&u16(p+6)==160,"L3E2 version/header rejected");
    require(u32(p+8)==1&&u32(p+12)==16&&u32(p+16)==8&&u32(p+20)==bytes.size()&&u32(p+28)==0,"L3E2 framing rejected");
    require(u32(p+24)==crc32(p+32,bytes.size()-32),"L3E2 CRC rejected");
    require(digest_matches(p+32,"10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0")&&
        digest_matches(p+64,"9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c")&&
        digest_matches(p+96,"ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15"),"L3E2 source identity rejected");
    const std::array<uint32_t,8> metadata{{kSourceEffectDispatchTable,source_effect2_color_table,
        source_effect3_sequence,source_effect4_rows,kSourceEffectDispatchScratchOwner,0x800172f4u,72u,64u}};
    for(size_t i=0;i<metadata.size();++i)require(u32(p+128+i*4)==metadata[i],"L3E2 source metadata rejected");
    require(p[278]==0&&p[279]==0,"L3E2 alignment bytes rejected");
    for(size_t i=300;i<bytes.size();++i)require(p[i]==0,"L3E2 reserved payload rejected");
    const auto payload_hash=scalar_sha256(p+160,140);
    require(digest_matches(payload_hash.data(),"6015208c30d7c82fa3ce30147d74697b65842be15a220278178565aea2295ee3"),"L3E2 original scalar payload rejected");
    SourceEffectDispatchProfile out;
    for(size_t i=0;i<32;++i)out.ebd_digest_[i]=p[96+i];
    for(size_t i=0;i<16;++i)out.callbacks_[i]=u32(p+160+i*4);
    out.colors_.source_table_identity=source_effect2_color_table;
    for(size_t i=0;i<8;++i)out.colors_.words[i]=u32(p+224+i*4);
    out.colors_.admitted=true;out.controls_.sequence3_identity=source_effect3_sequence;out.controls_.rows4_identity=source_effect4_rows;
    for(size_t i=0;i<11;++i)out.controls_.sequence3[i]=u16(p+256+i*2);
    for(size_t i=0;i<5;++i)for(size_t j=0;j<4;++j)out.controls_.rows4[i][j]=p[280+i*4+j];
    out.controls_.admitted=true;out.admitted_=true;return out;
}
void SourceEffectDispatchLease::invalidate() noexcept {
    profile_=nullptr;phase5_=nullptr;binding_=nullptr;scene_owner_=nullptr;payload_identity_=nullptr;generation_=0;payload_.reset();
}
bool SourceEffectDispatchLease::bind_overlay_load_80100354(const SourceEffectDispatchProfile& profile,
    const SourceEventPhase5Profile& phase5,const SourceEventPhase5ResourceBinding& binding,
    const EbdDirectory& ebd,const void* owner,uint64_t generation) noexcept {
    invalidate();
    if(!profile.admitted()||!owner||!generation||profile.ebd_digest()!=phase5.ebd_source_digest||
       !binding.matches(phase5)||!binding.resident_in(ebd)||ebd.source_payload.empty())return false;
    profile_=&profile;phase5_=&phase5;binding_=&binding;scene_owner_=owner;generation_=generation;
    payload_identity_=ebd.source_payload.storage_identity();payload_=ebd.source_payload.lifetime();return true;
}
bool SourceEffectDispatchLease::current(const SourceEffectDispatchProfile& profile,
    const SourceEventPhase5Profile& phase5,const SourceEventPhase5ResourceBinding& binding,
    const EbdDirectory& ebd,const void* owner,uint64_t generation) const noexcept {
    if(!generation_||profile_!=&profile||phase5_!=&phase5||binding_!=&binding||scene_owner_!=owner||generation_!=generation||
       !profile.admitted()||profile.ebd_digest()!=phase5.ebd_source_digest)return false;
    const auto payload=payload_.lock();
    return payload&&payload.get()==payload_identity_&&ebd.source_payload.storage_identity()==payload_identity_&&
        binding.matches(phase5)&&binding.resident_in(ebd);
}
SourceEffectDispatchBatch tick_source_effect_dispatch_800172f4(SourceEventPhase5Context& context,
    const SourceEffectDispatchProfile& profile,const SourceEffectDispatchLease& lease,
    const SourceEventPhase5Profile& phase5,const SourceEventPhase5ResourceBinding& binding,
    const EbdDirectory& ebd,const void* owner,uint64_t generation,
    const SourceEffectDispatchInputProvider& provider) noexcept {
    SourceEffectDispatchBatch out;
    if(!lease.current(profile,phase5,binding,ebd,owner,generation))return out;
    out.status=SourceEffectDispatchStatus::Ready;
    for(size_t slot=0;slot<context.pool80.slots.size();++slot){
        ++out.visited;const auto& record=context.pool80.slots[slot];
        if(!record.known[0]||!record.known[6]){hold(out,uint8_t(slot),255,SourceEffectDispatchHoldKind::GateField,0,0,
            uint8_t(record.known[0]?6:0));continue;}
        if(!(record.raw[0]&record.raw[6]&1u))continue;
        if(!record.known[2]){hold(out,uint8_t(slot),255,SourceEffectDispatchHoldKind::SubtypeField,0,0,2);continue;}
        const uint8_t subtype=record.raw[2];if(subtype>=128)continue;
        const auto callback=profile.callback(subtype);
        if(!callback||(*callback!=source_effect2_callback&&*callback!=source_effect3_callback&&*callback!=source_effect4_callback)){
            hold(out,uint8_t(slot),subtype,SourceEffectDispatchHoldKind::UnsupportedCallback,callback.value_or(0));continue;}
        ++out.callbacks;const uint32_t identity=source_effect_base+uint32_t(slot*source_effect_slot_bytes);
        if(*callback==source_effect2_callback){
            SourceEffect2Inputs in;in.source_generation=generation;if(provider.prepare2)provider.prepare2(provider.user,context,identity,in);
            in.callback_identity=*callback;in.colors=&profile.colors();
            const auto result=provider.effect2_candidate_rng&&!provider.prepare2
                ?tick_source_effect2_candidate_rng(context,identity,in,*provider.effect2_candidate_rng,provider.candidate_update)
                :tick_source_effect2_80116e68(context,identity,in);
            if(result.committed){++out.committed;out.children=uint16_t(out.children+(result.child_allocated?1u:0u));
                out.released=uint8_t(out.released+(result.released?1u:0u));out.rng_consumed=uint16_t(out.rng_consumed+result.rng_consumed);}
            else hold(out,uint8_t(slot),subtype,SourceEffectDispatchHoldKind::Effect2,*callback,uint8_t(result.status),result.missing_offset);
            if(provider.complete2)provider.complete2(provider.user,context,identity,result);
        }else{
            SourceEffect34Inputs in;in.source_generation=generation;if(provider.prepare34)provider.prepare34(provider.user,context,identity,*callback,in);
            in.controls=&profile.controls();
            const auto result=provider.effect34_candidate_rng&&!provider.prepare34
                ?tick_source_effect34_candidate_rng(context,identity,*callback,in,*provider.effect34_candidate_rng,provider.candidate34_update)
                :tick_source_effect34(context,identity,*callback,in);
            if(result.committed){++out.committed;out.children=uint16_t(out.children+result.allocated_count);
                out.released=uint8_t(out.released+(result.released?1u:0u));out.rng_consumed=uint16_t(out.rng_consumed+result.rng_consumed);}
            else hold(out,uint8_t(slot),subtype,SourceEffectDispatchHoldKind::Effect34,*callback,uint8_t(result.status),result.missing_offset);
            if(provider.complete34)provider.complete34(provider.user,context,identity,*callback,result);
        }
    }
    return out;
}
} // namespace l3
