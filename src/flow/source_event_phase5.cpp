#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
constexpr std::array<uint8_t,32> kOverlayDigest{{
    0x94,0x90,0xc2,0x00,0xed,0xc3,0xa8,0x04,0x5a,0xf4,0x62,0x9e,0x98,0x3b,0xed,0x83,
    0xca,0xb5,0x6a,0x9a,0x53,0xb1,0x78,0xac,0x06,0x7e,0xa7,0x6a,0xf3,0xce,0x41,0x3c}};
constexpr std::array<uint8_t,32> kEbdDigest{{
    0xea,0x72,0x39,0xd2,0x1d,0xb9,0xd0,0x6c,0x26,0xa7,0x08,0x25,0x58,0x8a,0xd7,0x98,
    0x1a,0x98,0x0c,0xe7,0x98,0xf7,0x35,0x25,0x88,0xc5,0x86,0x5a,0x0e,0x00,0x1d,0x15}};
uint16_t get16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t get32(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
// Portable SHA-256 over a bounded immutable owner payload. Admission is load
// time only; its 64-word schedule and state are fixed stack storage.
uint32_t rotate(uint32_t word,unsigned bits) noexcept {return (word>>bits)|(word<<(32-bits));}
std::array<uint8_t,32> payload_sha256(const uint8_t* bytes,size_t size) noexcept {
    constexpr std::array<uint32_t,64> constants{{
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2}};
    std::array<uint32_t,8> hash{{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}};
    const size_t blocks=(size+9+63)/64;const uint64_t bit_size=uint64_t(size)*8;
    for(size_t block=0;block<blocks;++block){
        std::array<uint32_t,64> words{};
        for(size_t i=0;i<64;++i){const size_t at=block*64+i;uint8_t value=0;
            if(at<size)value=bytes[at];else if(at==size)value=0x80;
            else if(at>=blocks*64-8)value=uint8_t(bit_size>>((blocks*64-1-at)*8));
            words[i/4]|=uint32_t(value)<<((3-(i%4))*8);
        }
        for(size_t i=16;i<64;++i){const uint32_t a=words[i-15],b=words[i-2];
            words[i]=words[i-16]+(rotate(a,7)^rotate(a,18)^(a>>3))+words[i-7]+
                (rotate(b,17)^rotate(b,19)^(b>>10));}
        auto v=hash;
        for(size_t i=0;i<64;++i){
            const uint32_t t1=v[7]+(rotate(v[4],6)^rotate(v[4],11)^rotate(v[4],25))+
                ((v[4]&v[5])^(~v[4]&v[6]))+constants[i]+words[i];
            const uint32_t t2=(rotate(v[0],2)^rotate(v[0],13)^rotate(v[0],22))+
                ((v[0]&v[1])^(v[0]&v[2])^(v[1]&v[2]));
            v[7]=v[6];v[6]=v[5];v[5]=v[4];v[4]=v[3]+t1;
            v[3]=v[2];v[2]=v[1];v[1]=v[0];v[0]=t1+t2;
        }
        for(size_t i=0;i<8;++i)hash[i]+=v[i];
    }
    std::array<uint8_t,32> result{};
    for(size_t i=0;i<32;++i)result[i]=uint8_t(hash[i/4]>>((3-(i%4))*8));
    return result;
}
uint32_t signed16(uint16_t value) noexcept {
    return value<32768u ? value : uint32_t(value)|0xffff0000u;
}
template<size_t N> void put(SourceEventPhase5Bytes<N>& value,size_t offset,size_t bytes,uint32_t word) noexcept {
    for(size_t i=0;i<bytes;++i){value.raw[offset+i]=uint8_t(word>>(i*8));value.known[offset+i]=1;}
}
void put_work(SourceEventWork& work,size_t offset,uint32_t word) noexcept {
    for(size_t i=0;i<4;++i){work.raw[offset+i]=uint8_t(word>>(i*8));work.known[offset+i]=1;}
}
template<size_t Count,size_t Bytes> void reset_pool(SourceEventPhase5Pool<Count,Bytes>& pool,uint8_t family) noexcept {
    for(size_t i=0;i<Count;++i){auto& slot=pool.slots[i];slot.raw.fill(0);slot.known.fill(1);slot.raw[1]=uint8_t(family|i);}
    for(auto& used:pool.used)used=0;
}
template<size_t N> void allocate_header(SourceEventPhase5Bytes<N>& value) noexcept {
    put(value,2,2,0);put(value,4,4,0);put(value,8,4,0);put(value,12,4,0);
    put(value,16,4,0);put(value,6,1,3);
}
bool valid_profile(const SourceEventPhase5Profile& profile) noexcept {
    if(!profile.initialized||profile.directory_token!=0x8011af4cu||profile.callback_table_token!=0x8011b2e8u)return false;
    for(size_t i=0;i<profile.records.size();++i){const auto& record=profile.records[i];
        if(record.raw[0]!=i || record.raw[3]!=(i==0?0x40:0xe0))return false;
        if(i==0){if(record.resource_token||record.resource_directory_index!=0xffffffffu)return false;}
        else if(record.resource_token<0x8016c000u || record.resource_token>=0x80187c90u ||
                record.resource_directory_index>=14u)return false;
    }
    return true;
}
template<size_t Count,size_t Bytes> std::optional<size_t> reserve(
    const SourceEventPhase5Pool<Count,Bytes>& pool,std::array<uint32_t,(Count+31)/32>& used,
    uint8_t family,SourceEventPhase5Result& out) noexcept {
    for(size_t i=0;i<Count;++i){const uint32_t bit=0x80000000u>>(i&31u);
        if(used[i/32]&bit)continue;
        const auto& slot=pool.slots[i];
        if(!slot.known[1]||slot.raw[1]!=uint8_t(family|i)){
            out.status=SourceEventPhase5Status::NeedsPoolHeader;out.missing_offset=1;return {};
        }
        used[i/32]|=bit;return i;
    }
    out.status=SourceEventPhase5Status::PoolCapacityExceeded;return {};
}
void require(bool condition,const char* why){if(!condition)throw std::runtime_error(why);}
}
SourceEventPhase5Profile load_source_event_phase5_profile(const std::vector<uint8_t>& bytes) {
    require(bytes.size()>=96,"truncated phase5 header");const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='P'&&p[3]=='5',"phase5 magic rejected");
    require(get16(p+4)==1&&get16(p+6)==96,"phase5 version/header rejected");
    require(get32(p+8)==1&&get32(p+12)==16&&get32(p+24)==32&&get32(p+28)==12,"phase5 decoder/record budget rejected");
    require(bytes.size()==620&&get32(p+16)==bytes.size(),"phase5 coverage rejected");
    for(size_t i=0;i<32;++i){require(p[32+i]==kOverlayDigest[i],"phase5 overlay identity rejected");
        require(p[64+i]==kEbdDigest[i],"phase5 EBD identity rejected");}
    require(get32(p+20)==crc32(p+96,bytes.size()-96),"phase5 CRC rejected");
    SourceEventPhase5Profile profile;profile.initialized=true;
    for(size_t i=0;i<32;++i)profile.ebd_source_digest[i]=p[64+i];
    profile.directory_token=get32(p+96);profile.callback_table_token=get32(p+100);
    require(get32(p+104)==0,"phase5 reserved metadata rejected");
    for(size_t i=0;i<16;++i){const auto* r=p+108+i*32;auto& record=profile.records[i];
        for(size_t j=0;j<20;++j)record.raw[j]=r[j];
        record.resource_token=get32(r+20);record.resource_directory_index=get32(r+24);
        require(r[29]==0&&r[30]==0&&r[31]==0,"phase5 reserved record rejected");record.resource_variant=r[28];
    }
    require(valid_profile(profile),"phase5 spawn/resource domain rejected");return profile;
}
bool SourceEventPhase5ResourceBinding::admitted() const noexcept {
    return status_==SourceEventPhase5ResourceStatus::Admitted&&!payload_.expired();
}
bool SourceEventPhase5ResourceBinding::resident_in(const EbdDirectory& directory) const noexcept {
    if(!admitted()||directory.source_payload.storage_identity()!=payload_identity_||
        directory.load_base!=load_base_||directory.entries.size()!=directory_entries_.size())return false;
    for(size_t i=0;i<directory_entries_.size();++i){const auto& a=directory_entries_[i];const auto& b=directory.entries[i];
        if(a.object_class!=b.object_class||a.subtype!=b.subtype||a.variant!=b.variant||a.key_flags!=b.key_flags)return false;
        for(size_t slot=0;slot<3;++slot)if(a.resource_offset[slot]!=b.resource_offset[slot])return false;
    }
    return true;
}
bool SourceEventPhase5ResourceBinding::matches(const SourceEventPhase5Profile& profile) const noexcept {
    if(!admitted()||profile.initialized!=profile_.initialized||
        profile.directory_token!=profile_.directory_token||profile.callback_table_token!=profile_.callback_table_token||
        profile.ebd_source_digest!=profile_.ebd_source_digest)return false;
    for(size_t i=0;i<profile.records.size();++i){const auto& a=profile.records[i];const auto& b=profile_.records[i];
        if(a.raw!=b.raw||a.resource_token!=b.resource_token||a.resource_directory_index!=b.resource_directory_index||
            a.resource_variant!=b.resource_variant)return false;
    }
    return true;
}
SourceEventPhase5ResourceBinding bind_source_event_phase5_resources(
    const SourceEventPhase5Profile& profile,const EbdDirectory& directory) noexcept {
    using Status=SourceEventPhase5ResourceStatus;SourceEventPhase5ResourceBinding out;
    const auto reject=[&](Status status) noexcept {out.status_=status;return out;};
    if(!profile.initialized)return reject(Status::NeedsProfile);
    if(!valid_profile(profile))return reject(Status::UnsupportedProfile);
    const auto& payload=directory.source_payload;
    if(payload.empty())return reject(Status::NeedsPayload);
    if(payload.size()>256*1024)return reject(Status::PayloadBudgetExceeded);
    // The supported constructor profile is exact. Additional areas need a
    // separately proved decoder profile; no room-number substitution occurs.
    if(directory.load_base!=0x8016c000u||directory.entries.size()!=14u||payload.size()<228u||
        get32(payload.data())!=directory.entries.size())return reject(Status::SourceDirectoryMismatch);
    if(payload_sha256(payload.data(),payload.size())!=profile.ebd_source_digest)
        return reject(Status::PayloadDigestMismatch);
    for(size_t i=0;i<directory.entries.size();++i){const auto* raw=payload.data()+4+i*16;const auto& row=directory.entries[i];
        if(raw[0]!=row.object_class||raw[1]!=row.subtype||raw[2]!=row.variant||raw[3]!=row.key_flags)
            return reject(Status::SourceDirectoryMismatch);
        for(size_t slot=0;slot<3;++slot){const uint32_t token=get32(raw+4+slot*4);
            const uint32_t offset=token?token-directory.load_base:0xffffffffu;
            if((token&&(token<directory.load_base||uint64_t(token)-directory.load_base>=payload.size()))||
                offset!=row.resource_offset[slot])return reject(Status::SourceDirectoryMismatch);
        }
    }
    for(size_t i=1;i<profile.records.size();++i){const auto& record=profile.records[i];
        const size_t index=record.resource_directory_index;const auto& row=directory.entries[index];
        const uint8_t family=normalize_ebd_object_class(record.raw[3]);
        if(row.object_class!=family||row.subtype!=record.raw[1]||row.variant!=record.resource_variant)
            return reject(Status::RowKeyMismatch);
        for(size_t prior=0;prior<index;++prior){const auto& earlier=directory.entries[prior];
            if(earlier.object_class==family&&earlier.subtype==record.raw[1]&&earlier.variant==record.resource_variant)
                return reject(Status::NotFirstMatch);
        }
        const uint64_t offset=uint64_t(record.resource_token)-directory.load_base;
        if(offset>=payload.size()||row.resource_offset[0]==0xffffffffu)
            return reject(Status::ResourceOutsidePayload);
        if(offset!=row.resource_offset[0])return reject(Status::ResourceOffsetMismatch);
    }
    out.profile_=profile;out.payload_=payload.lifetime();out.payload_identity_=payload.storage_identity();
    out.load_base_=directory.load_base;
    for(size_t i=0;i<out.directory_entries_.size();++i) {
        out.directory_entries_[i]=directory.entries[i];
    }
    out.status_=Status::Admitted;
    return out;
}
void initialize_source_event_phase5_pools_8002e88c(SourceEventPhase5Context& context) noexcept {
    reset_pool(context.pool40,0x40);reset_pool(context.pool80,0x80);reset_pool(context.pool_e0,0xe0);reset_pool(context.pool_c0,0xc0);context.word_80098818=0;
    context.pool80_pending_release_800988c8={{0,0}};
}
std::array<uint8_t,188> source_event_phase5_camera_snapshot(const SourceEventPhase5Context& context) noexcept {
    return context.camera_800bc738.raw;
}
bool initialize_source_event_phase5_camera_80038674(SourceEventPhase5Context& context,
    const L3PlayerCompatWork& player,const SourceEventPlayerKnown& known) noexcept {
    for(size_t i=0x14;i<0x1a;++i)if(!known[i])return false;
    std::array<uint32_t,3> eye{};
    for(size_t i=0;i<3;++i)eye[i]=signed16(get16(player.bytes+0x14+i*2));
    context.initial_eye_800a39a0=eye;context.initial_rotation_800a3964=std::array<uint32_t,3>{{512,0,48}};
    context.mode_800a395c=1;return true;
}
SourceEventPhase5Result consume_source_event1_phase5_80100efc(SourceEventWork& work,
    SourceEventPhase5Context& context,const SourceEventPhase5Profile& profile,
    const SourceEventPhase5ResourceBinding* resources) noexcept {
    SourceEventPhase5Result out;
    if(!work.known[0]||!work.known[1]||!work.known[4])return out;
    if(work.raw[0]==0||work.raw[1]!=1||work.raw[4]!=5){out.status=SourceEventPhase5Status::WrongEventPhase;return out;}
    if(!profile.initialized){out.status=SourceEventPhase5Status::NeedsProfile;return out;}
    if(!valid_profile(profile)){out.status=SourceEventPhase5Status::UnsupportedProfile;return out;}
    if(!resources||!resources->matches(profile)){out.status=SourceEventPhase5Status::NeedsResourceOwnership;return out;}
    if(!context.initial_eye_800a39a0||!context.initial_rotation_800a3964){out.status=SourceEventPhase5Status::NeedsCameraFields;return out;}
    if(!context.pool40.used[0]||!context.pool80.used[0]||!context.pool80.used[1]||!context.pool_e0.used[0]){
        out.status=SourceEventPhase5Status::NeedsPoolBitmap;return out;}
    std::array<uint32_t,1> used40{{*context.pool40.used[0]}},used_e0{{*context.pool_e0.used[0]}};
    std::array<uint32_t,2> used80{{*context.pool80.used[0],*context.pool80.used[1]}};
    std::array<size_t,16> selected{};
    for(size_t i=0;i<16;++i){const auto slot=i==0?reserve(context.pool40,used40,0x40,out):reserve(context.pool_e0,used_e0,0xe0,out);
        if(!slot)return out;
        selected[i]=*slot;
    }
    const auto extra=reserve(context.pool80,used80,0x80,out);if(!extra)return out;
    // Every later instruction is a proved store to a preflighted fixed owner.
    auto& camera=context.camera_800bc738;context.mode_800a395c=3;
    const auto camera_put=[&](size_t offset,size_t bytes,uint32_t word) noexcept {put(camera,offset,bytes,word);};
    camera_put(0,1,0);
    for(size_t i=0;i<3;++i){
        camera_put(8+i*4,4,(*context.initial_eye_800a39a0)[i]<<16u);
        camera_put(0x38+i*4,4,((*context.initial_rotation_800a3964)[(i+1)%3]&4095u)<<16u);
        camera_put(0x18+i*4,4,0);camera_put(0x48+i*4,4,0);camera_put(0x78+i*4,4,0);
        camera_put(0x98+i*4,4,0);
    }
    camera_put(0xb8,4,profile.directory_token);
    for(size_t i=0;i<16;++i){const auto& record=profile.records[i];const auto slot_index=selected[i];
        const uint32_t token=(i==0?0x800c3580u+uint32_t(slot_index)*1444u:0x800bf118u+uint32_t(slot_index)*80u);
        put_work(work,0x24+i*4,token);
        if(i==0){auto& slot=context.pool40.slots[slot_index];allocate_header(slot);put(slot,116,4,0);put(slot,120,4,0);
            put(slot,0,1,1);put(slot,2,1,record.raw[1]);put(slot,6,1,0x23);
            put(slot,20,4,get32(record.raw.data()+12));put(slot,24,2,get16(record.raw.data()+16));
            put(slot,84,2,0);put(slot,88,2,0);put(slot,86,2,get16(record.raw.data()+18));
            for(size_t j=0;j<3;++j)put(slot,28+j*4,4,signed16(get16(record.raw.data()+12+j*2))<<16u);
            put(slot,3,1,record.raw[4]);put(slot,4,1,record.raw[2]);put(slot,12,4,get32(record.raw.data()+8));
        }else{auto& slot=context.pool_e0.slots[slot_index];allocate_header(slot);
            put(slot,0,1,1);put(slot,2,1,record.raw[1]);put(slot,6,1,0x23);
            put(slot,56,4,get32(record.raw.data()+12));put(slot,60,4,get32(record.raw.data()+16));
            put(slot,3,1,record.raw[4]);put(slot,4,1,record.raw[2]);put(slot,12,4,get32(record.raw.data()+8));
            put(slot,20,4,record.resource_token);put(slot,64,4,0x00808080u);
        }
    }
    put(context.pool_e0.slots[selected[1]],64,4,0x00808080u);
    put(context.pool_e0.slots[selected[15]],64,4,0x02808080u);
    put(context.pool_e0.slots[selected[3]],64,4,0);
    auto& extra_slot=context.pool80.slots[*extra];allocate_header(extra_slot);
    put(extra_slot,0,1,1);put(extra_slot,2,1,2);put(extra_slot,12,1,2);
    put(extra_slot,3,1,0);put(extra_slot,13,1,0);put(extra_slot,14,1,0);
    put(extra_slot,28,2,0);put(extra_slot,30,2,uint16_t(-404));put(extra_slot,32,2,3584);
    put_work(work,0x64,0x800a3b40u+uint32_t(*extra)*72u);
    put_work(work,0xa4,profile.callback_table_token);context.byte_80122428=0;
    context.pool40.used[0]=used40[0];context.pool_e0.used[0]=used_e0[0];
    context.pool80.used[0]=used80[0];context.pool80.used[1]=used80[1];
    work.raw[4]=6;work.known[4]=1;
    out.status=SourceEventPhase5Status::Consumed;out.committed=true;out.constructed_actors=17;out.resource_bindings=15;return out;
}
} // namespace l3
