#include "legends3ds/source_scene_actor40.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
using Work=SourceEventPhase5Bytes<1444>;
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
uint16_t get16(const Work& w,size_t at){return uint16_t(w.raw[at])|uint16_t(w.raw[at+1])<<8;}
int16_t signed16(uint16_t n){return static_cast<int16_t>(n<32768?int(n):int(n)-65536);}
void put(Work& w,size_t at,size_t n,uint32_t v){for(size_t i=0;i<n;++i){w.raw[at+i]=uint8_t(v>>(i*8));w.known[at+i]=1;}}
bool known(const Work& w,size_t at,size_t n){return std::all_of(w.known.begin()+at,w.known.begin()+at+n,[](uint8_t n){return n!=0;});}
bool resident(const SourceSceneActor40Resources& r,const EbdDirectory& e){
    const auto p=r.payload.lock();
    if(!p||p.get()!=r.payload_identity||e.source_payload.storage_identity()!=p.get()||e.load_base!=r.load_base||e.entries.size()!=r.directory_entries.size())return false;
    for(size_t i=0;i<r.directory_entries.size();++i){const auto& a=r.directory_entries[i];const auto& b=e.entries[i];
        if(a.object_class!=b.object_class||a.subtype!=b.subtype||a.variant!=b.variant||a.key_flags!=b.key_flags)return false;
        for(size_t k=0;k<3;++k)if(a.resource_offset[k]!=b.resource_offset[k])return false;}
    return true;
}
constexpr std::array<uint8_t,32> exe_sha{{0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
// Patched below from the measured first script prefix hash; identity only.
constexpr std::array<uint8_t,32> script_sha{{0x20,0x94,0x0c,0x69,0xda,0xb1,0xa7,0x88,0x3e,0xdb,0x78,0xca,0x34,0x31,0x1a,0x47,0x70,0x99,0xa2,0x0e,0x0a,0xc2,0x6d,0xf6,0x09,0x00,0xc7,0xe7,0x5b,0xe9,0x19,0x06}};
}
SourceSceneActor40Bootstrap load_source_scene_actor40_bootstrap(const std::vector<uint8_t>& b){
    if(b.size()<96||rd_u32(b.data())!=0x3441334cu||rd_u16(b.data()+4)!=1||rd_u16(b.data()+6)!=96||
       rd_u32(b.data()+8)!=0x800810c8u||rd_u32(b.data()+12)!=b.size()||
       rd_u32(b.data()+16)==0||rd_u32(b.data()+16)>128||rd_u32(b.data()+20)!=0||rd_u32(b.data()+24)!=0||rd_u32(b.data()+28)!=0||
       b.size()!=96+size_t(rd_u32(b.data()+16))*2||!std::equal(exe_sha.begin(),exe_sha.end(),b.begin()+32)||
       !std::equal(script_sha.begin(),script_sha.end(),b.begin()+64)||
       payload_sha256(b.data()+96,b.size()-96)!=script_sha)throw std::runtime_error("unsupported class40 bootstrap profile");
    SourceSceneActor40Bootstrap p;p.admitted=true;p.script_token=rd_u32(b.data()+8);
    for(size_t i=96;i<b.size();i+=2){p.words.push_back(rd_u16(b.data()+i));}
    return p;
}
SourceSceneActor40PrefixResult consume_source_scene_actor40_script_80014bcc(Work& w,const uint16_t* words,size_t count) noexcept {
    SourceSceneActor40PrefixResult out;
    if(!words||count>128||!known(w,844,2)){out.status=SourceSceneActor40Status::NeedsWorkFields;return out;}
    Work next=w;size_t index=get16(next,844);
    for(size_t steps=0;steps<128;++steps){
        if(index>=count){out.status=SourceSceneActor40Status::UnsupportedScript;return out;}
        const uint16_t op=words[index];
        if(op==0){if(index+1>=count){out.status=SourceSceneActor40Status::UnsupportedScript;return out;}
            put(next,844,2,uint16_t(index+2));put(next,846,2,uint16_t(words[index+1]-1));
            out.script_index=uint16_t(index+2);out.wait_remaining=uint16_t(words[index+1]-1);
            out.request=next.raw[172];out.status=SourceSceneActor40Status::SourceBackedPrefix;out.committed=true;w=next;return out;}
        if(op==27){put(next,8,1,2);out.status=SourceSceneActor40Status::SourceBackedPrefix;out.committed=true;w=next;return out;}
        if(op==26){if(index+1>=count){out.status=SourceSceneActor40Status::UnsupportedScript;return out;}index=words[index+1];put(next,844,2,uint16_t(index));continue;}
        if(op<1||op>25){out.status=SourceSceneActor40Status::UnsupportedScript;return out;}
        const size_t operands=(op==1||op==19)?3:((op==15||op==16||op==18)?0:1);
        if(index+operands>=count){out.status=SourceSceneActor40Status::UnsupportedScript;return out;}
        const uint16_t value=operands?words[index+1]:0;
        if(op==1){for(size_t i=0;i<3;++i){put(next,20+2*i,2,words[index+1+i]);put(next,28+4*i,4,uint32_t(words[index+1+i])<<16);}}
        else if(op>=2&&op<=4){put(next,20+(op-2)*2,2,value);put(next,28+(op-2)*4,4,uint32_t(value)<<16);}
        else if(op==5)put(next,172,1,value);
        else if(op>=6&&op<=11){const size_t at=68+((op-6)/2)*2+((op-6)%2)*8;put(next,at,2,value);}
        else if(op==12)put(next,86,2,value);
        else if(op==13)put(next,848,2,value);
        else if(op==14)put(next,859,1,value);
        else if(op==15)put(next,858,1,0);
        else if(op==16)put(next,858,1,1);
        else if(op==17){put(next,13,1,1);put(next,14,1,value);put(next,15,1,0);}
        else if(op==18)put(next,13,1,0);
        else if(op==19){for(size_t i=0;i<3;++i)put(next,344+2*i,2,words[index+1+i]);}
        else if(op>=20&&op<=22)put(next,344+(op-20)*2,2,value);
        else if(op>=23&&op<=25)put(next,850+(op-23)*2,2,value);
        index+=operands+1;put(next,844,2,uint16_t(index));
    }
    out.status=SourceSceneActor40Status::UnsupportedScript;return out;
}
SourceSceneActor40Resources decode_source_scene_actor40_resources(const SourceSceneActor40Bootstrap& b,
    const SourceEventPhase5Profile& p,const SourceEventPhase5ResourceBinding& witness,const EbdDirectory& e,size_t cap){
    if(!b.admitted||b.script_token!=0x800810c8u||!witness.matches(p)||!witness.resident_in(e)||
       p.records[0].raw[1]!=5||p.records[0].raw[4]!=0||rd_u32(p.records[0].raw.data()+8)!=0)
        throw std::runtime_error("class40 resource owner unavailable");
    const auto rows=find_ebd_entries(e,0x40,5);if(rows.empty())throw std::runtime_error("class40 source lookup miss");
    SourceSceneActor40Resources r;r.mesh=decode_ebd_scene_actor40_mesh(e,rows.front(),ActorMeshTier::NEAR,cap);
    Work w;w.known.fill(1);put(w,844,2,0);put(w,172,1,255);
    const auto script=consume_source_scene_actor40_script_80014bcc(w,b.words.data(),b.words.size());
    if(!script.committed)throw std::runtime_error("class40 bootstrap script unsupported");
    const size_t duplicate_pose_bytes=sizeof(ActorPoseResource)+
        r.mesh.bindings.size()*sizeof(std::array<int16_t,3>);
    if(r.mesh.footprint.decoded_bytes+duplicate_pose_bytes>=cap)
        throw std::runtime_error("class40 pose cache exceeds decoded memory cap");
    r.pose_sequence=decode_ebd_scene_actor40_pose_sequence(e,rows.front(),r.mesh,script.request,
        cap-r.mesh.footprint.decoded_bytes-duplicate_pose_bytes);
    r.pose=r.pose_sequence.poses.front();
    const auto& row=e.entries[rows.front()];r.descriptor_token=e.load_base+row.resource_offset[0];
    const auto d=ebd_resource_span(e,rows.front(),0,0x7c);std::copy(d.data+16,d.data+112,r.descriptor_copy.begin());
    const auto m=ebd_resource_span(e,rows.front(),1,4);r.translations_token=rd_u32(m.data);
    r.frames_token=e.load_base+row.resource_offset[1]+4;r.sequence_token=e.load_base+row.resource_offset[2];
    r.payload=e.source_payload.lifetime();r.payload_identity=e.source_payload.storage_identity();r.load_base=e.load_base;
    if(e.entries.size()!=r.directory_entries.size())throw std::runtime_error("class40 directory witness coverage invalid");
    std::copy(e.entries.begin(),e.entries.end(),r.directory_entries.begin());return r;
}
SourceSceneActor40Resources decode_source_scene_actor40_phase5_resources(
    const SourceEventPhase5Profile& p,const SourceEventPhase5ResourceBinding& witness,const EbdDirectory& e,size_t cap){
    if(!witness.matches(p)||!witness.resident_in(e)||p.records[0].raw[1]!=5||p.records[0].raw[2]!=1)
        throw std::runtime_error("actual phase5 class40 resource owner unavailable");
    const auto rows=find_ebd_entries(e,0x40,5);if(rows.empty())throw std::runtime_error("class40 source lookup miss");
    SourceSceneActor40Resources r;r.mesh=decode_ebd_scene_actor40_mesh(e,rows.front(),ActorMeshTier::NEAR,cap);
    const size_t duplicate_pose_bytes=sizeof(ActorPoseResource)+
        r.mesh.bindings.size()*sizeof(std::array<int16_t,3>);
    if(r.mesh.footprint.decoded_bytes+duplicate_pose_bytes>=cap)
        throw std::runtime_error("class40 pose cache exceeds decoded memory cap");
    r.pose_sequence=decode_ebd_scene_actor40_pose_sequence(e,rows.front(),r.mesh,0,
        cap-r.mesh.footprint.decoded_bytes-duplicate_pose_bytes);
    r.pose=r.pose_sequence.poses.front();
    const auto& row=e.entries[rows.front()];r.descriptor_token=e.load_base+row.resource_offset[0];
    const auto d=ebd_resource_span(e,rows.front(),0,0x7c);std::copy(d.data+16,d.data+112,r.descriptor_copy.begin());
    const auto m=ebd_resource_span(e,rows.front(),1,4);r.translations_token=rd_u32(m.data);
    r.frames_token=e.load_base+row.resource_offset[1]+4;r.sequence_token=e.load_base+row.resource_offset[2];
    r.payload=e.source_payload.lifetime();r.payload_identity=e.source_payload.storage_identity();r.load_base=e.load_base;
    if(e.entries.size()!=r.directory_entries.size())throw std::runtime_error("class40 directory witness coverage invalid");
    std::copy(e.entries.begin(),e.entries.end(),r.directory_entries.begin());return r;
}
size_t source_scene_actor40_decoded_bytes(const SourceSceneActor40Resources& r) noexcept {
    return r.mesh.footprint.decoded_bytes+r.pose_sequence.decoded_bytes+
        sizeof(ActorPoseResource)+r.pose.base_translation.capacity()*sizeof(std::array<int16_t,3>)+
        r.additional_requests.decoded_bytes+
        (r.mesh_variant?sizeof(SourceSceneActor40MeshVariant)+r.mesh_variant->mesh.footprint.decoded_bytes:0);
}
void prepare_source_scene_actor40_request_bank(SourceSceneActor40Resources& r,
    const EbdDirectory& e,const uint8_t* requests,size_t count,size_t cap) {
    if(!resident(r,e))throw std::runtime_error("class40 animation bank resource owner unavailable");
    if(!requests||!count||count>kActorAnimationRequestLimit)
        throw std::runtime_error("class40 animation bank request set unavailable");
    for(size_t i=0;i<count;++i)if(requests[i]==r.pose.request)
        throw std::runtime_error("class40 initial request must not be duplicated");
    const size_t existing=source_scene_actor40_decoded_bytes(r)-r.additional_requests.decoded_bytes;
    if(existing>=cap)throw std::runtime_error("class40 animation bank total memory cap exceeded");
    auto bank=decode_ebd_scene_actor40_animation_bank(e,r.mesh.source_row,r.mesh,requests,count,cap-existing);
    r.additional_requests=std::move(bank);
}
const ActorPoseSequenceCache* find_source_scene_actor40_pose_sequence(
    const SourceSceneActor40Resources& r,uint8_t request) noexcept {
    if(request==r.pose.request)return &r.pose_sequence;
    const auto* cache=find_actor_animation_request(r.additional_requests,request);
    return cache?&cache->sequence:nullptr;
}
void prepare_source_scene_actor40_mesh_variant(SourceSceneActor40Resources& r,
    const EbdDirectory& e,const std::array<uint8_t,96>& descriptor,size_t cap) {
    if(!resident(r,e))throw std::runtime_error("class40 mesh variant resource owner unavailable");
    const size_t replaced=r.mesh_variant?sizeof(SourceSceneActor40MeshVariant)+r.mesh_variant->mesh.footprint.decoded_bytes:0;
    const size_t existing=source_scene_actor40_decoded_bytes(r)-replaced;
    if(existing>=cap||sizeof(SourceSceneActor40MeshVariant)>cap-existing)
        throw std::runtime_error("class40 mesh variant total memory cap exceeded");
    SourceSceneActor40MeshVariant variant;
    variant.mesh=decode_ebd_scene_actor40_mesh_variant(e,r.mesh.source_row,descriptor,r.mesh.tier,
        cap-existing-sizeof(SourceSceneActor40MeshVariant));variant.descriptor=descriptor;
    r.mesh_variant=std::move(variant);
}
const ActorMeshResource* select_source_scene_actor40_mesh(const Work& w,
    const SourceSceneActor40Resources& r) noexcept {
    if(!known(w,248,96))return nullptr;
    if(std::equal(r.descriptor_copy.begin(),r.descriptor_copy.end(),w.raw.begin()+248))return &r.mesh;
    if(r.mesh_variant&&std::equal(r.mesh_variant->descriptor.begin(),r.mesh_variant->descriptor.end(),w.raw.begin()+248))
        return &r.mesh_variant->mesh;
    return nullptr;
}
SourceSceneActor40PrefixResult initialize_source_scene_actor40_prefix_8010c930(Work& w,
    const SourceSceneActor40Resources& r,const EbdDirectory& e) noexcept {
    SourceSceneActor40PrefixResult out;
    if(!resident(r,e)){out.status=SourceSceneActor40Status::NeedsResourceOwnership;return out;}
    for(const size_t at:{size_t(0),size_t(1),size_t(2),size_t(4),size_t(8)})if(!known(w,at,1)){out.status=SourceSceneActor40Status::NeedsWorkFields;return out;}
    if((w.raw[1]&0xe0)!=0x40||w.raw[2]!=5||w.raw[4]!=1||w.raw[8]!=0){out.status=SourceSceneActor40Status::UnsupportedActor;return out;}
    if(r.pose.request!=0||r.pose.record_index!=0){out.status=SourceSceneActor40Status::NeedsPose;return out;}
    Work n=w;put(n,0,1,n.raw[0]|2u);put(n,136,4,r.descriptor_token);put(n,144,4,r.sequence_token);
    put(n,156,4,r.translations_token);put(n,160,4,r.frames_token);
    for(size_t i=0;i<r.descriptor_copy.size();++i)put(n,248+i,1,r.descriptor_copy[i]);
    for(size_t at:{size_t(344),size_t(346),size_t(348)})put(n,at,2,512);
    put(n,172,1,0);put(n,120,4,0);put(n,86,2,2213);
    w=n;out.status=SourceSceneActor40Status::SourceBackedPrefix;out.committed=true;out.request=0;return out;
}
SourceSceneActor40PrefixResult initialize_source_scene_actor40_prefix_80014ac0(Work& w,
    const SourceSceneActor40Bootstrap& b,const SourceSceneActor40Resources& r,const EbdDirectory& e) noexcept {
    SourceSceneActor40PrefixResult out;if(!b.admitted){out.status=SourceSceneActor40Status::NeedsBootstrap;return out;}
    if(!resident(r,e)){out.status=SourceSceneActor40Status::NeedsResourceOwnership;return out;}
    for(const size_t at:{size_t(0),size_t(1),size_t(2),size_t(3),size_t(8),size_t(12)})if(!known(w,at,1)){out.status=SourceSceneActor40Status::NeedsWorkFields;return out;}
    if((w.raw[1]&0xe0)!=0x40||w.raw[2]!=5||w.raw[3]!=0||w.raw[8]!=0||w.raw[12]!=0){out.status=SourceSceneActor40Status::UnsupportedActor;return out;}
    if(!known(w,20,8)||!known(w,28,16)||!known(w,84,8)){out.status=SourceSceneActor40Status::NeedsWorkFields;return out;}
    Work n=w;put(n,0,1,n.raw[0]|2u);put(n,136,4,r.descriptor_token);put(n,144,4,r.sequence_token);
    put(n,156,4,r.translations_token);put(n,160,4,r.frames_token);
    for(size_t i=0;i<r.descriptor_copy.size();++i)put(n,248+i,1,r.descriptor_copy[i]);
    put(n,344,4,0x04000400);put(n,348,4,0x42100400);put(n,120,4,0x800979fc);
    for(size_t at:{size_t(100),size_t(68),size_t(72),size_t(76),size_t(80),size_t(84),size_t(88),size_t(16)})put(n,at,4,0);
    put(n,840,4,b.script_token);for(size_t at:{size_t(844),size_t(846),size_t(848),size_t(850),size_t(852),size_t(854)})put(n,at,2,0);
    put(n,858,1,0);put(n,859,1,255);put(n,172,1,255);put(n,8,1,n.raw[8]+1u);
    put(n,173,1,n.raw[172]);put(n,860,1,n.raw[859]);
    // Previous work copies happen before script, as in800150E0.
    for(size_t i=0;i<8;++i)put(n,44+i,1,n.raw[20+i]);
    for(size_t i=0;i<16;++i)put(n,52+i,1,n.raw[28+i]);
    for(size_t i=0;i<8;++i)put(n,92+i,1,n.raw[84+i]);
    out=consume_source_scene_actor40_script_80014bcc(n,b.words.data(),b.words.size());
    if(out.committed&&out.request==r.pose.request)w=n;
    else {out.status=SourceSceneActor40Status::NeedsPose;out.committed=false;}return out;
}
SourceSceneActor40Matrices build_source_scene_actor40_pose_matrices(const Work& w,const SourceSceneActor40Resources& r) noexcept {
    SourceSceneActor40Matrices out;const auto& bindings=r.mesh.bindings;const auto& pose=r.pose;
    if(bindings.empty()||bindings.size()>kActorDrawDescriptorSlots||bindings.size()!=pose.base_translation.size()||r.mesh.model.parts.size()!=bindings.size())return out;
    if(!known(w,6,1)||!known(w,20,6)||!known(w,86,2)||!known(w,344,6)){out.status=SourceSceneActor40Status::NeedsWorkFields;return out;}
    if(w.raw[6]&0x40){out.status=SourceSceneActor40Status::NeedsPerPartScale;return out;}
    std::array<PlayerPoseMatrix,kActorDrawDescriptorSlots> rotation{};std::array<PlayerPosePoint,kActorDrawDescriptorSlots> position{};
    std::array<bool,kActorDrawDescriptorSlots> valid{};
    const std::array<float,3> scale{{float(signed16(get16(w,344)))/4096.f,float(signed16(get16(w,346)))/4096.f,float(signed16(get16(w,348)))/4096.f}};
    const auto yaw=player_pose_rotation_xyz(0,get16(w,86),0);const auto& root=pose.rotations[0];
    rotation[0]=player_pose_mul(yaw,player_pose_rotation_xyz(root[0],root[1],root[2]));
    const auto delta=player_pose_transform_point(yaw,{float(pose.root_translation[0])*8.f*scale[0],float(pose.root_translation[1])*8.f*scale[1],float(pose.root_translation[2])*8.f*scale[2]});
    position[0]={float(signed16(get16(w,20)))+delta.x,float(signed16(get16(w,22)))+delta.y,float(signed16(get16(w,24)))+delta.z};
    valid[0]=true;out.draw_enabled[0]=bindings[0].draw_control>=0;
    for(size_t i=1;i<bindings.size();++i){const auto& b=bindings[i];if(b.parent<0)continue;
        if(size_t(b.parent)>=i||b.rotation_translation_index>=pose.rotations.size())return {};
        if(b.rotation_translation_index&&!valid[size_t(b.parent)])continue;
        valid[i]=true;out.draw_enabled[i]=b.draw_control>=0;
        if(!b.rotation_translation_index){rotation[i]=rotation[0];position[i]=position[0];continue;}
        const auto parent=size_t(b.parent);const auto& a=pose.rotations[b.rotation_translation_index];
        rotation[i]=player_pose_mul(rotation[parent],player_pose_rotation_xyz(a[0],a[1],a[2]));
        const auto& t=pose.base_translation[i];const auto d=player_pose_transform_point(rotation[parent],{float(t[0])*scale[0],float(t[1])*scale[1],float(t[2])*scale[2]});
        position[i]={position[parent].x+d.x,position[parent].y+d.y,position[parent].z+d.z};}
    for(size_t i=0;i<bindings.size();++i){auto& m=out.world[i];m=rotation[i];for(size_t row=0;row<3;++row)for(size_t c=0;c<3;++c)m.m[row*4+c]*=scale[c];m.m[3]=position[i].x;m.m[7]=position[i].y;m.m[11]=position[i].z;}
    out.part_count=bindings.size();out.status=SourceSceneActor40Status::SourceBackedPrefix;return out;
}
} // namespace l3
