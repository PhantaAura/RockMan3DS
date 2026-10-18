#include "legends3ds/source_scene_actor40_pose.h"
#include <algorithm>

namespace l3 {
namespace {
using Work=SourceEventPhase5Bytes<1444>;
using Matrix=std::array<int16_t,9>;
int16_t short_bits(uint32_t n) noexcept {n&=65535u;return static_cast<int16_t>(n<32768u?int32_t(n):int32_t(n)-65536);}
int32_t signed_bits(uint32_t n) noexcept {return n<0x80000000u?int32_t(n):int32_t(int64_t(n)-4294967296LL);}
int32_t asr32(uint32_t n,unsigned shift) noexcept {shift&=31u;if(!shift)return signed_bits(n);return signed_bits((n>>shift)|((n&0x80000000u)?0xffffffffu<<(32u-shift):0u));}
int32_t asr12(int64_t n) noexcept {return int32_t(n>=0?n/4096:-((-n+4095)/4096));}
uint32_t product(int32_t a,int32_t b) noexcept {return uint32_t(a)*uint32_t(b);}
uint16_t get16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t get32(const uint8_t* p) noexcept {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
bool known(const Work& w,size_t at,size_t n) noexcept {return std::all_of(w.known.begin()+at,w.known.begin()+at+n,[](uint8_t n){return n!=0;});}
void put16(uint8_t* p,uint16_t n) noexcept {p[0]=uint8_t(n);p[1]=uint8_t(n>>8);}
void put32(uint8_t* p,uint32_t n) noexcept {for(size_t i=0;i<4;++i)p[i]=uint8_t(n>>(i*8));}
void put(Work& w,size_t at,size_t n,uint32_t value) noexcept {for(size_t i=0;i<n;++i){w.raw[at+i]=uint8_t(value>>(i*8));w.known[at+i]=1;}}
struct Trig {int32_t s,c;};
Trig angle(const ReferenceTables& refs,uint16_t bits) noexcept {
    const int32_t n=short_bits(bits);const auto& t=refs.trig[uint32_t(n<0?-n:n)&4095u];
    return {n<0?-int32_t(t.sin_q12):int32_t(t.sin_q12),t.cos_q12};
}
Matrix rotate_xyz(const ReferenceTables& refs,uint16_t ax,uint16_t ay,uint16_t az) noexcept {
    const auto x=angle(refs,ax),y=angle(refs,ay),z=angle(refs,az);Matrix m{};
    m[2]=short_bits(uint32_t(y.s));m[5]=short_bits(uint32_t(asr32(0u-product(y.c,x.s),12)));
    m[8]=short_bits(uint32_t(asr32(product(y.c,x.c),12)));m[0]=short_bits(uint32_t(asr32(product(z.c,y.c),12)));
    m[1]=short_bits(uint32_t(asr32(0u-product(z.s,y.c),12)));
    auto t=asr32(product(z.c,-y.s),12);
    m[3]=short_bits(uint32_t(asr32(product(z.s,x.c),12))-uint32_t(asr32(product(t,x.s),12)));
    m[6]=short_bits(uint32_t(asr32(product(z.s,x.s),12))+uint32_t(asr32(product(t,x.c),12)));
    t=asr32(product(z.s,-y.s),12);
    m[4]=short_bits(uint32_t(asr32(product(z.c,x.c),12))+uint32_t(asr32(product(t,x.s),12)));
    m[7]=short_bits(uint32_t(asr32(product(z.c,x.s),12))-uint32_t(asr32(product(t,x.c),12)));return m;
}
Matrix multiply(const Matrix& a,const Matrix& b) noexcept {
    Matrix out{};for(size_t row=0;row<3;++row)for(size_t col=0;col<3;++col){int64_t sum=0;
        for(size_t k=0;k<3;++k)sum+=int64_t(a[row*3+k])*b[k*3+col];
        out[row*3+col]=int16_t(std::max<int32_t>(-32768,std::min<int32_t>(32767,asr12(sum))));}
    return out;
}
std::array<uint8_t,20> scale_matrix(const Matrix& m,const std::array<int16_t,3>& scale) noexcept {
    std::array<uint8_t,20> out{};for(size_t i=0;i<9;++i){const auto value=uint32_t(asr32(product(m[i],scale[i%3]),12));
        if(i==8)put32(out.data()+16,value);else put16(out.data()+i*2,uint16_t(value));}
    return out;
}
Matrix unpack_matrix(const std::array<uint8_t,20>& m) noexcept {Matrix out{};for(size_t i=0;i<9;++i)out[i]=short_bits(get16(m.data()+i*2));return out;}
std::array<int32_t,3> apply_mac(const Matrix& m,const std::array<int16_t,3>& v) noexcept {
    std::array<int32_t,3> out{};for(size_t row=0;row<3;++row){int64_t sum=0;for(size_t k=0;k<3;++k)sum+=int64_t(m[row*3+k])*v[k];out[row]=asr12(sum);}return out;
}
bool resident(const SourceSceneActor40Resources& r,const EbdDirectory& e) noexcept {
    const auto p=r.payload.lock();if(!p||p.get()!=r.payload_identity||e.source_payload.storage_identity()!=p.get()||e.load_base!=r.load_base||e.entries.size()!=r.directory_entries.size())return false;
    for(size_t i=0;i<r.directory_entries.size();++i){const auto& a=r.directory_entries[i];const auto& b=e.entries[i];
        if(a.object_class!=b.object_class||a.subtype!=b.subtype||a.variant!=b.variant||a.key_flags!=b.key_flags)return false;
        for(size_t k=0;k<3;++k)if(a.resource_offset[k]!=b.resource_offset[k])return false;}return true;
}
SourceSceneActor40IntegerPose hold(SourceSceneActor40PoseStatus status) noexcept {SourceSceneActor40IntegerPose out;out.status=status;return out;}
}
SourceSceneActor40IntegerPose produce_source_scene_actor40_integer_pose_8003346c(
    const Work& w,const SourceSceneActor40Resources& r,const ReferenceTables* refs,std::optional<uint8_t> shift,
    const ActorPoseResource& pose) noexcept {
    using Status=SourceSceneActor40PoseStatus;
    if(!refs)return hold(Status::NeedsReference);
    if(!shift)return hold(Status::NeedsScratchShift);
    if(*shift>2)return hold(Status::UnsupportedScratchShift);
    for(const auto& window:{std::pair<size_t,size_t>{1,2},{6,1},{20,6},{86,2},{172,1},{248,96},{344,6}})
        if(!known(w,window.first,window.second))return hold(Status::NeedsWorkFields);
    if((w.raw[1]&0xe0)!=0x40||w.raw[2]!=5)return hold(Status::UnsupportedActor);
    if(w.raw[6]&0x40)return hold(Status::NeedsPerPartScale);
    const auto& bindings=r.mesh.bindings;
    if(bindings.empty()||bindings.size()>kActorDrawDescriptorSlots||bindings.size()!=pose.base_translation.size()||r.mesh.model.parts.size()!=bindings.size()||w.raw[172]!=pose.request||
       w.raw[249]!=bindings.size())return hold(Status::NeedsPose);
    for(size_t i=0;i<r.descriptor_copy.size();++i)
        if(i%4!=0 && r.descriptor_copy[i]!=w.raw[248+i])return hold(Status::NeedsPose);
    SourceSceneActor40IntegerPose out;out.scratch_shift=*shift;
    std::array<int16_t,3> scale{};for(size_t i=0;i<3;++i)scale[i]=short_bits(uint32_t(int32_t(short_bits(get16(w.raw.data()+344+i*2))))<<*shift);
    const auto yaw=rotate_xyz(*refs,0,get16(w.raw.data()+86),0);out.scaled_heading_160=scale_matrix(yaw,scale);
    std::array<Matrix,kActorDrawDescriptorSlots> rotation{};
    const auto& root=pose.rotations[0];rotation[0]=multiply(yaw,rotate_xyz(*refs,root[0],root[1],root[2]));
    auto& p0=out.parts[0];p0.transform_written=true;p0.draw_enabled=bindings[0].draw_control>=0;
    std::array<int16_t,3> root_point{};for(size_t i=0;i<3;++i)root_point[i]=short_bits(uint32_t(int32_t(pose.root_translation[i]))<<3);
    const auto root_delta=apply_mac(unpack_matrix(out.scaled_heading_160),root_point);
    for(size_t i=0;i<3;++i)p0.position[i]=short_bits(uint32_t(root_delta[i])+(uint32_t(int32_t(short_bits(get16(w.raw.data()+20+i*2))))<<*shift));
    for(size_t i=1;i<bindings.size();++i){const auto& b=bindings[i];if(b.parent<0)return hold(Status::InvalidHierarchy);
        if(size_t(b.parent)>=i||b.rotation_translation_index>=pose.rotations.size())return hold(Status::InvalidHierarchy);
        const auto parent=size_t(b.parent);if(b.rotation_translation_index&&!out.parts[parent].transform_written)return hold(Status::InvalidHierarchy);
        auto& p=out.parts[i];p.transform_written=true;p.draw_enabled=b.draw_control>=0;
        if(!b.rotation_translation_index){rotation[i]=rotation[0];p.position=p0.position;continue;}
        const auto& a=pose.rotations[b.rotation_translation_index];rotation[i]=multiply(rotation[parent],rotate_xyz(*refs,a[0],a[1],a[2]));
    }
    for(size_t i=0;i<bindings.size();++i)if(out.parts[i].transform_written)out.parts[i].matrix=scale_matrix(rotation[i],scale);
    // Source positions consume the already scaled parent matrices; no scale
    // is compounded into child rotation composition before this stage.
    for(size_t i=1;i<bindings.size();++i){const auto& b=bindings[i];if(!out.parts[i].transform_written||!b.rotation_translation_index)continue;
        const auto parent=size_t(b.parent);const auto d=apply_mac(unpack_matrix(out.parts[parent].matrix),pose.base_translation[i]);
        for(size_t k=0;k<3;++k)out.parts[i].position[k]=short_bits(uint32_t(uint16_t(out.parts[parent].position[k]))+uint32_t(d[k]));
    }
    out.part_count=uint8_t(bindings.size());out.status=Status::SourceBackedPartial;return out;
}
SourceSceneActor40IntegerPose produce_source_scene_actor40_integer_pose_8003346c(
    const Work& w,const SourceSceneActor40Resources& r,const ReferenceTables* refs,std::optional<uint8_t> shift) noexcept {
    return produce_source_scene_actor40_integer_pose_8003346c(w,r,refs,shift,r.pose);
}
const ActorPoseResource* select_source_scene_actor40_pose(const Work& w,
    const SourceSceneActor40Resources& r) noexcept {
    if(!known(w,148,8)||!known(w,172,1)||!find_source_scene_actor40_pose_sequence(r,w.raw[172]))return nullptr;
    const auto* cache=find_source_scene_actor40_pose_sequence(r,w.raw[172]);
    if(cache->poses.empty()||cache->poses.size()!=cache->records.size())return nullptr;
    const uint32_t base=r.load_base+cache->poses.front().sequence_source_offset;
    if(get32(w.raw.data()+148)!=base)return nullptr;
    const uint32_t pointer=get32(w.raw.data()+152);
    if(pointer<base||(pointer-base)%8u)return nullptr;
    const size_t index=(pointer-base)/8u;
    return index<cache->poses.size()?&cache->poses[index]:nullptr;
}
SourceSceneActor40PoseCommit reset_source_scene_actor40_pose_800322a8(
    Work& w,const SourceSceneActor40Resources& r,const EbdDirectory& e,const ReferenceTables* refs,
    std::optional<uint8_t> shift,uint8_t request,uint16_t record_index) noexcept {
    SourceSceneActor40PoseCommit out;using Status=SourceSceneActor40PoseStatus;
    if(!resident(r,e)){out.status=Status::NeedsResourceOwnership;return out;}
    const auto* cache=find_source_scene_actor40_pose_sequence(r,request);
    if(!cache||cache->poses.empty()||cache->poses.size()!=cache->records.size()||record_index>=cache->poses.size()){
        out.status=Status::NeedsPose;return out;}
    for(const auto& window:{std::pair<size_t,size_t>{4,1},{8,1},{136,4},{144,4},{156,8}})if(!known(w,window.first,window.second)){out.status=Status::NeedsWorkFields;return out;}
    if(w.raw[4]!=1||w.raw[8]>1||get32(w.raw.data()+136)!=r.descriptor_token||get32(w.raw.data()+144)!=r.sequence_token||get32(w.raw.data()+156)!=r.translations_token||get32(w.raw.data()+160)!=r.frames_token){out.status=Status::NeedsPose;return out;}
    Work next=w;put(next,172,1,request);
    const auto& selected=cache->poses[record_index];
    const auto pose=produce_source_scene_actor40_integer_pose_8003346c(next,r,refs,shift,selected);out.status=pose.status;if(pose.status!=Status::SourceBackedPartial)return out;
    put(next,148,4,e.load_base+cache->poses.front().sequence_source_offset);put(next,152,4,e.load_base+selected.sequence_source_offset);
    put(next,164,4,get32(selected.sequencing_record.data()));
    for(size_t i=0;i<20;++i)put(next,352+i,1,pose.scaled_heading_160[i]);
    for(size_t i=372;i<384;++i)next.known[i]=0;
    for(size_t i=0;i<pose.part_count;++i){const auto& p=pose.parts[i];if(!p.transform_written)continue;
        for(size_t k=0;k<20;++k)put(next,576+i*32+k,1,p.matrix[k]);
        for(size_t k=0;k<3;++k)put(next,384+i*8+k*2,2,uint16_t(p.position[k]));}
    out.committed=true;out.part_count=pose.part_count;w=next;return out;
}
SourceSceneActor40PoseCommit commit_source_scene_actor40_initial_pose_800322a8(
    Work& w,const SourceSceneActor40Resources& r,const EbdDirectory& e,const ReferenceTables* refs,
    std::optional<uint8_t> shift) noexcept {
    return reset_source_scene_actor40_pose_800322a8(w,r,e,refs,shift,0,0);
}
SourceSceneActor40SequenceTick advance_source_scene_actor40_pose_80032488(
    Work& w,const SourceSceneActor40Resources& r,const EbdDirectory& e,
    const ReferenceTables* refs,std::optional<uint8_t> shift) noexcept {
    SourceSceneActor40SequenceTick out;using Status=SourceSceneActor40PoseStatus;
    if(!resident(r,e)){out.status=Status::NeedsResourceOwnership;return out;}
    if(!known(w,4,1)||!known(w,8,1)||!known(w,136,4)||!known(w,144,4)||!known(w,156,8)||!known(w,164,4)){out.status=Status::NeedsWorkFields;return out;}
    if(w.raw[4]!=1||w.raw[8]!=1||get32(w.raw.data()+136)!=r.descriptor_token||
       get32(w.raw.data()+144)!=r.sequence_token||get32(w.raw.data()+156)!=r.translations_token||
       get32(w.raw.data()+160)!=r.frames_token){out.status=Status::NeedsPose;return out;}
    const auto* current=select_source_scene_actor40_pose(w,r);
    if(!current){out.status=Status::NeedsPose;return out;}
    const auto* cache=find_source_scene_actor40_pose_sequence(r,w.raw[172]);
    const size_t index=size_t(current-cache->poses.data());
    const auto& record=cache->records[index];
    if(w.raw[164]!=record[0]||w.raw[166]!=record[2]||w.raw[167]!=record[3]){
        out.status=Status::NeedsPose;return out;
    }
    ActorSequenceCursor cursor{uint16_t(index),w.raw[164],w.raw[165],w.raw[166],w.raw[167]};
    const auto step=actor_sequence_step(cursor,cache->records.data(),cache->records.size());
    if(step.status!=ActorSequenceStatus::SourceBackedPartial||cursor.record_index>=cache->poses.size()){
        out.status=Status::NeedsPose;return out;
    }
    const auto& selected=cache->poses[cursor.record_index];
    const auto pose=produce_source_scene_actor40_integer_pose_8003346c(w,r,refs,shift,selected);
    out.status=pose.status;if(pose.status!=Status::SourceBackedPartial)return out;
    put(w,152,4,e.load_base+selected.sequence_source_offset);
    put(w,164,1,cursor.frame_a4);put(w,165,1,cursor.countdown_a5);
    put(w,166,1,cursor.opaque_a6);put(w,167,1,cursor.opaque_a7);
    for(size_t i=0;i<20;++i)put(w,352+i,1,pose.scaled_heading_160[i]);
    for(size_t i=372;i<384;++i)w.known[i]=0;
    for(size_t i=0;i<pose.part_count;++i){const auto& p=pose.parts[i];if(!p.transform_written)continue;
        for(size_t k=0;k<20;++k)put(w,576+i*32+k,1,p.matrix[k]);
        for(size_t k=0;k<3;++k)put(w,384+i*8+k*2,2,uint16_t(p.position[k]));
    }
    out.committed=true;out.record_changed=step.record_changed;
    out.record_index=cursor.record_index;out.part_count=pose.part_count;return out;
}
SourceSceneActor40Matrices present_source_scene_actor40_integer_pose(const SourceSceneActor40IntegerPose& pose) noexcept {
    SourceSceneActor40Matrices out;if(pose.status!=SourceSceneActor40PoseStatus::SourceBackedPartial||pose.part_count>kActorDrawDescriptorSlots||pose.scratch_shift>2)return out;
    const float divisor=float(1u<<pose.scratch_shift);for(size_t i=0;i<pose.part_count;++i){const auto& p=pose.parts[i];out.draw_enabled[i]=p.transform_written&&p.draw_enabled;
        const auto m=unpack_matrix(p.matrix);for(size_t row=0;row<3;++row){for(size_t col=0;col<3;++col)out.world[i].m[row*4+col]=float(m[row*3+col])/(4096.f*divisor);out.world[i].m[row*4+3]=float(p.position[row])/divisor;}}
    out.status=SourceSceneActor40Status::SourceBackedPrefix;out.part_count=pose.part_count;return out;
}
} // namespace l3
