#include "legends3ds/scene_e0.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/source_event_pipeline.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
struct Source {
    const EbdDirectory& directory;
    const uint8_t* at(size_t offset,size_t bytes) const {
        const auto& payload=directory.source_payload;
        if(payload.empty()||offset>payload.size()||bytes>payload.size()-offset)
            throw std::runtime_error("scene E0 retained resource window unavailable");
        return payload.data()+offset;
    }
    uint32_t pointer(uint32_t token,size_t bytes) const {
        if(!token||token<directory.load_base||(token&3u))
            throw std::runtime_error("scene E0 required pointer invalid");
        const uint32_t offset=token-directory.load_base;
        (void)at(offset,bytes);return offset;
    }
};
uint32_t descriptor(const EbdDirectory& directory,size_t row) {
    if(row>=directory.entries.size()||directory.entries[row].object_class!=0xe0||
        directory.entries[row].subtype!=3)
        throw std::runtime_error("scene E0 resource class/subtype unsupported");
    const auto span=ebd_resource_span(directory,row,0,0x7c);
    if(span.status!=EbdResourceSpanStatus::AVAILABLE||(span.source_offset&3u))
        throw std::runtime_error("scene E0 descriptor unavailable or unaligned");
    return span.source_offset;
}
uint32_t selected_part(const Source& source,uint32_t desc,SceneE0MeshTier tier) {
    const auto index=static_cast<uint8_t>(tier);
    if(index>2)throw std::runtime_error("scene E0 tier unsupported");
    const auto mesh=source.pointer(rd_u32(source.at(size_t(desc)+0x70+index*4,4)),40);
    return mesh+20;
}
uint32_t word(const SourceEventPhase5Bytes<80>& work,size_t offset) noexcept {
    return uint32_t(work.raw[offset])|uint32_t(work.raw[offset+1])<<8|
        uint32_t(work.raw[offset+2])<<16|uint32_t(work.raw[offset+3])<<24;
}
int16_t half(const SourceEventPhase5Bytes<80>& work,size_t offset) noexcept {
    const uint16_t value=uint16_t(work.raw[offset])|uint16_t(work.raw[offset+1])<<8;
    return static_cast<int16_t>(value<32768?int(value):int(value)-65536);
}
void put(SourceEventPhase5Bytes<80>& work,size_t offset,size_t count,uint32_t value) noexcept {
    for(size_t i=0;i<count;++i){work.raw[offset+i]=uint8_t(value>>(i*8));work.known[offset+i]=1;}
}
bool known(const SourceEventPhase5Bytes<80>& work,size_t offset,size_t count,
    std::optional<uint16_t>& missing) noexcept {
    for(size_t i=0;i<count;++i)if(!work.known[offset+i]){missing=uint16_t(offset+i);return false;}
    return true;
}
}
SceneE0MeshTier scene_e0_tier_for_source_depth(int32_t depth) noexcept {
    return depth>=1801?SceneE0MeshTier::Far:depth>=701?SceneE0MeshTier::Middle:SceneE0MeshTier::Near;
}
SceneE0MeshResource decode_scene_e0_mesh(const EbdDirectory& directory,size_t row,
    SceneE0MeshTier tier,size_t cap) {
    const Source source{directory};SceneE0MeshResource out;
    out.descriptor_offset=descriptor(directory,row);
    out.part_offset=selected_part(source,out.descriptor_offset,tier);
    const auto* part=source.at(out.part_offset,20);
    const size_t triangles=part[0],quads=part[1],vertices=part[2];
    out.source_part_byte3=part[3];out.material_word=rd_u32(part+12);
    if(!vertices&&(triangles||quads))throw std::runtime_error("scene E0 faces without vertices");
    out.decoded_bytes=sizeof(SceneE0MeshResource)+vertices*sizeof(PbdVertex)+
        triangles*sizeof(PbdTri)+quads*sizeof(PbdQuad);
    if(out.decoded_bytes>cap)throw std::runtime_error("scene E0 decoded memory budget exceeded");
    const auto vo=vertices?source.pointer(rd_u32(part+16),vertices*8):0;
    const auto to=triangles?source.pointer(rd_u32(part+4),triangles*12):0;
    const auto qo=quads?source.pointer(rd_u32(part+8),quads*12):0;
    // Validate all indices before allocating decoded owner elements.
    for(size_t i=0;i<triangles;++i){const auto* p=source.at(size_t(to)+i*12,12);
        for(size_t k=0;k<3;++k)if(p[8+k]>=vertices)throw std::runtime_error("scene E0 triangle index out of range");}
    for(size_t i=0;i<quads;++i){const auto* p=source.at(size_t(qo)+i*12,12);
        for(size_t k=0;k<4;++k)if(p[8+k]>=vertices)throw std::runtime_error("scene E0 quad index out of range");}
    out.part.vertices.reserve(vertices);out.part.triangles.reserve(triangles);out.part.quads.reserve(quads);
    for(size_t i=0;i<vertices;++i){const auto* p=source.at(size_t(vo)+i*8,8);
        out.part.vertices.push_back({rd_s16(p),rd_s16(p+2),rd_s16(p+4),rd_u16(p+6)});}
    for(size_t i=0;i<triangles;++i){const auto* p=source.at(size_t(to)+i*12,12);PbdTri tri;
        for(size_t k=0;k<6;++k)tri.uv[k]=p[k];
        for(size_t k=0;k<3;++k)tri.index[k]=p[8+k];
        tri.clut=uint16_t(out.material_word>>16);tri.tpage=uint16_t(out.material_word);
        tri.r=tri.g=tri.b=128;tri.code=0x24;out.part.triangles.push_back(tri);}
    for(size_t i=0;i<quads;++i){const auto* p=source.at(size_t(qo)+i*12,12);PbdQuad quad;
        for(size_t k=0;k<8;++k)quad.uv[k]=p[k];
        for(size_t k=0;k<4;++k)quad.index[k]=p[8+k];
        quad.clut=uint16_t(out.material_word>>16);quad.tpage=uint16_t(out.material_word);
        quad.r=quad.g=quad.b=128;quad.code=0x2c;out.part.quads.push_back(quad);}
    out.decoded_bytes=sizeof(SceneE0MeshResource)+out.part.vertices.capacity()*sizeof(PbdVertex)+
        out.part.triangles.capacity()*sizeof(PbdTri)+out.part.quads.capacity()*sizeof(PbdQuad);
    if(out.decoded_bytes>cap)throw std::runtime_error("scene E0 allocator capacity exceeds memory budget");
    return out;
}
bool SceneE0Resources::resident_in(const EbdDirectory& directory) const noexcept {
    return ownership_.resident_in(directory);
}
const SceneE0MeshResource* SceneE0Resources::find(uint32_t token,uint8_t variant,SceneE0MeshTier tier) const noexcept {
    const auto index=static_cast<uint8_t>(tier);if(index>2)return nullptr;
    for(const auto& binding:bindings_)if(binding.token==token&&binding.variant==variant){
        const auto mesh=binding.mesh[index];return mesh<meshes_.size()?&meshes_[mesh]:nullptr;}
    return nullptr;
}
SceneE0Resources load_scene_e0_resources(const EbdDirectory& directory,
    const SourceEventPhase5Profile& profile,const SourceEventPhase5ResourceBinding& ownership,size_t cap) {
    if(!ownership.resident_in(directory)||!ownership.matches(profile))
        throw std::runtime_error("scene E0 current phase5 ownership unavailable");
    SceneE0Resources out;out.ownership_=ownership;
    const size_t minimum=sizeof(SceneE0Resources)+45*sizeof(SceneE0MeshResource);
    if(minimum>cap)throw std::runtime_error("scene E0 cache memory budget exceeded");
    out.meshes_.reserve(45);
    const size_t fixed=sizeof(SceneE0Resources)+out.meshes_.capacity()*sizeof(SceneE0MeshResource);
    if(fixed>cap)throw std::runtime_error("scene E0 cache allocator capacity exceeds memory budget");
    out.decoded_bytes_=fixed;
    const Source source{directory};
    for(size_t i=0;i<15;++i){const auto& record=profile.records[i+1];auto& binding=out.bindings_[i];
        if(record.raw[1]!=3||record.raw[4]>7)throw std::runtime_error("scene E0 current callback variant unsupported");
        binding.token=record.resource_token;binding.variant=record.raw[4];
        const auto desc=descriptor(directory,record.resource_directory_index);
        for(size_t tier=0;tier<3;++tier){const auto selected=static_cast<SceneE0MeshTier>(tier);
            const auto part=selected_part(source,desc,selected);size_t found=0;
            while(found<out.meshes_.size()&&(out.meshes_[found].descriptor_offset!=desc||out.meshes_[found].part_offset!=part))++found;
            if(found==out.meshes_.size()){
                auto decoded=decode_scene_e0_mesh(directory,record.resource_directory_index,selected,cap-out.decoded_bytes_+sizeof(SceneE0MeshResource));
                const auto owned=decoded.decoded_bytes-sizeof(SceneE0MeshResource);
                if(owned>cap-out.decoded_bytes_)throw std::runtime_error("scene E0 cumulative memory budget exceeded");
                out.decoded_bytes_+=owned;out.meshes_.push_back(std::move(decoded));
            }
            binding.mesh[tier]=uint8_t(found);
        }
    }
    return out;
}
SceneE0Result initialize_scene_e0_first_update_80112bdc(SourceEventPhase5Bytes<80>& work,
    const SceneE0Resources& resources,const EbdDirectory& directory) noexcept {
    SceneE0Result out;
    if(!known(work,0,1,out.missing_offset)||!known(work,6,1,out.missing_offset))return out;
    if(!(work.raw[0]&work.raw[6]&1)){out.status=SceneE0Status::NotScheduled;return out;}
    if(!known(work,2,2,out.missing_offset)||!known(work,8,1,out.missing_offset)||!known(work,20,4,out.missing_offset))return out;
    if(work.raw[2]!=3){out.status=SceneE0Status::UnsupportedSubtype;return out;}
    if(work.raw[3]>7){out.status=SceneE0Status::UnsupportedVariant;return out;}
    if(work.raw[8]){out.status=SceneE0Status::NeedsVariantBehavior;return out;}
    if(!resources.resident_in(directory)||!resources.find(word(work,20),work.raw[3],SceneE0MeshTier::Near)){
        out.status=SceneE0Status::NeedsResourceOwnership;return out;}
    put(work,0,1,work.raw[0]|(work.raw[3]==0?14u:6u));
    for(size_t i=0;i<8;++i)put(work,24+i*4,4,(i==0||i==2||i==4)?4096u:0u);
    put(work,8,1,1);out.status=SceneE0Status::Initialized;out.committed=true;return out;
}
SceneE0Result update_scene_e0_80112b9c(SourceEventPhase5Bytes<80>& work,
    const SceneE0Resources& resources,const EbdDirectory& directory,
    const SceneE0UpdateInputs& inputs) noexcept {
    using Status=SceneE0Status;SceneE0Result out;out.source_function=0x80112b9c;
    const auto need=[&](size_t offset,size_t count) noexcept {return known(work,offset,count,out.missing_offset);};
    if(!need(0,1)||!need(6,1))return out;
    if(!(work.raw[0]&work.raw[6]&1)){out.status=Status::NotScheduled;return out;}
    if(!need(2,2)||!need(8,1)||!need(20,4))return out;
    if(work.raw[2]!=3){out.status=Status::UnsupportedSubtype;return out;}
    if(work.raw[3]>7){out.status=Status::UnsupportedVariant;return out;}
    if(!work.raw[8])return initialize_scene_e0_first_update_80112bdc(work,resources,directory);
    if(!resources.resident_in(directory)||!resources.find(word(work,20),work.raw[3],SceneE0MeshTier::Near)){
        out.status=Status::NeedsResourceOwnership;return out;}
    // Only80 bytes are copied. Retained meshes and the large source pools are
    // borrowed. A failed branch discards this local transaction, including all
    // source-prefix stores that would otherwise repeat on a held callback.
    auto next=work;
    const auto done=[&](bool stores=true) noexcept {work=next;out.committed=true;
        out.status=stores?Status::Updated:Status::NoOp;return out;};
    const auto hold=[&](Status status,uint32_t function) noexcept {out.status=status;out.source_function=function;return out;};
    const auto identity=[&]() noexcept {for(size_t i=0;i<8;++i)put(next,24+i*4,4,(i==0||i==2||i==4)?4096u:0u);};
    const auto increment=[&](size_t offset) noexcept {put(next,offset,1,uint8_t(next.raw[offset]+1));};
    const auto bob=[&](uint32_t function,bool expiry) noexcept {
        if(!inputs.scratch_half_1f800002)return hold(Status::NeedsScratchOwner,function);
        if(expiry&&!need(12,1))return out;
        const int delta=(*inputs.scratch_half_1f800002&1u)?-2:2;
        put(next,58,2,uint16_t(delta));
        if(expiry){put(next,12,1,uint8_t(next.raw[12]-1));if(!next.raw[12]){put(next,58,2,0);increment(10);}}
        return done();
    };
    if(work.raw[3]==6){out.source_function=0x80113e9c;return done(false);}
    if(work.raw[3]==7){
        out.source_function=0x80113bf0;if(!need(12,1))return out;
        if(!inputs.event_work||!inputs.pool)return hold(Status::NeedsLinkedOwner,out.source_function);
        const int index=work.raw[12]<128?int(work.raw[12]):int(work.raw[12])-256;
        const int offset=0x40+index*4;
        if(offset<0||size_t(offset)+4>inputs.event_work->raw.size())return hold(Status::NeedsLinkedOwner,out.source_function);
        uint32_t token=0;for(size_t i=0;i<4;++i){const size_t at=size_t(offset)+i;
            if(!inputs.event_work->known[at])return hold(Status::NeedsLinkedOwner,out.source_function);
            token|=uint32_t(inputs.event_work->raw[at])<<(i*8);}
        if(token<0x800bf118u||(token-0x800bf118u)%80u)return hold(Status::NeedsLinkedOwner,out.source_function);
        const size_t slot=(token-0x800bf118u)/80u;
        if(slot>=32||!inputs.pool->used[0]||!(*inputs.pool->used[0]&(0x80000000u>>slot)))return hold(Status::NeedsLinkedOwner,out.source_function);
        const auto& linked=inputs.pool->slots[slot];
        if(!linked.known[1]||linked.raw[1]!=uint8_t(0xe0u|slot))return hold(Status::NeedsLinkedOwner,out.source_function);
        for(size_t i=0;i<6;++i)if(!linked.known[56+i])return hold(Status::NeedsLinkedOwner,out.source_function);
        for(size_t i=0;i<6;++i){next.raw[56+i]=linked.raw[56+i];next.known[56+i]=1;}
        return done();
    }
    if(!need(9,1))return out;
    const uint8_t selector=work.raw[9];
    const auto current_prop=[&]() noexcept {
        if(!inputs.scene_context||!inputs.prop_identity||!inputs.event_work)return false;
        const auto id=*inputs.prop_identity;
        if(id<0x800bf118u||(id-0x800bf118u)%80u)return false;
        const auto slot=size_t((id-0x800bf118u)/80u);
        return slot<32 && &work==&inputs.scene_context->pool_e0.slots[slot]&&
            inputs.pool==&inputs.scene_context->pool_e0;
    };
    switch(work.raw[3]){
    case 0:
        if(selector>3)return hold(Status::UnsupportedBehaviorState,0x80113010);
        if(selector!=0){
            if(!current_prop())return hold(Status::NeedsLinkedOwner,selector==1?0x80112d14:selector==2?0x80112e80:0x80112f70);
            SourcePropAttachInputs attachment;attachment.references=inputs.references;
            attachment.event_work=inputs.event_work;attachment.actors=&inputs.scene_context->pool40;
            attachment.release_owner=inputs.prop_release_owner;
            const auto result=update_source_prop_attach_80112d14(next,*inputs.prop_identity,attachment);
            out.source_function=result.source_function;out.missing_offset=result.missing_prop_offset;
            if(result.committed){out.foreign_release_committed=result.released;return done();}
            switch(result.status){
            case SourcePropAttachStatus::NeedsReferences:out.status=Status::NeedsReferences;break;
            case SourcePropAttachStatus::NeedsForeignReleaseOwner:
            case SourcePropAttachStatus::ForeignReleaseMismatch:out.status=Status::NeedsForeignReleaseOwner;break;
            case SourcePropAttachStatus::NeedsPropFields:out.status=Status::NeedsFields;break;
            case SourcePropAttachStatus::UnknownState:out.status=Status::UnsupportedBehaviorState;break;
            default:out.status=Status::NeedsLinkedOwner;break;
            }
            return out;
        }
        out.source_function=0x80112c9c;
        if(!need(10,1))return out;
        if(work.raw[10]>1)return hold(Status::UnsupportedBehaviorState,out.source_function);
        if(!inputs.references)return hold(Status::NeedsReferences,0x8007bc68);
        if(work.raw[10]==0){put(next,68,4,0);increment(10);}
        else {if(!need(68,4))return out;put(next,68,4,word(next,68)+0x00091a2bu);}
        identity();
        {const int16_t angle=half(next,70);const uint16_t absolute=uint16_t(angle<0?-int(angle):int(angle))&4095u;
            const auto trig=inputs.references->trig[absolute];const int32_t sine=angle<0?-int32_t(trig.sin_q12):int32_t(trig.sin_q12);
            put(next,24,2,uint16_t(trig.cos_q12));put(next,28,2,uint16_t(sine));
            put(next,36,2,uint16_t(-sine));put(next,40,2,uint16_t(trig.cos_q12));}
        return done();
    case 1:
        if(selector==0){out.source_function=0x8011304c;return done(false);}
        if(selector==1){out.source_function=0x80113054;if(!need(58,2))return out;
            if(!inputs.scratch_half_1f800002)return hold(Status::NeedsScratchOwner,out.source_function);
            put(next,58,2,uint16_t(uint16_t(half(next,58))+((*inputs.scratch_half_1f800002&1u)?2:-2)));return done();}
        if(selector!=2)return hold(Status::UnsupportedBehaviorState,0x80113130);
        out.source_function=0x801130c0;if(!need(10,1))return out;
        if(work.raw[10]>1)return hold(Status::UnsupportedBehaviorState,out.source_function);
        if(!inputs.scratch_half_1f800002)return hold(Status::NeedsScratchOwner,out.source_function);
        if(work.raw[10]==0){if(!need(58,2))return out;put(next,68,4,uint32_t(uint16_t(half(next,58)))<<16u);increment(10);}
        else {if(!need(68,4))return out;put(next,68,4,word(next,68)+0x0001999au);}
        put(next,58,2,uint16_t(uint16_t(half(next,70))+((*inputs.scratch_half_1f800002&1u)?2:-2)));return done();
    case 2:
        if(selector==0){out.source_function=0x8011316c;identity();return done();}
        if(selector>3)return hold(Status::UnsupportedBehaviorState,0x801134e4);
        if(selector==1||selector==2){
            if(!current_prop())return hold(Status::NeedsLinkedOwner,selector==1?0x80113280:0x80113354);
            const auto result=inputs.lift_candidate
                ?consume_source_prop_lift_candidate(*inputs.scene_context,*inputs.event_work,*inputs.prop_identity,inputs.lift,*inputs.lift_candidate)
                :consume_source_prop_lift_80113280(*inputs.scene_context,*inputs.event_work,*inputs.prop_identity,inputs.lift);
            out.source_function=result.source_function;out.committed=result.committed;
            out.missing_offset=result.missing_offset;
            if(result.committed){out.status=Status::Updated;return out;}
            switch(result.status){
            case SourcePropLiftStatus::NeedsCurves:
            case SourcePropLiftStatus::CurveContextMismatch:out.status=Status::NeedsPropCurves;break;
            case SourcePropLiftStatus::NeedsFrameReceipt:
            case SourcePropLiftStatus::FrameContextMismatch:out.status=Status::NeedsPropFrame;break;
            case SourcePropLiftStatus::NeedsRngReceipts:
            case SourcePropLiftStatus::RngContextMismatch:out.status=Status::NeedsRngOrChildOwner;break;
            case SourcePropLiftStatus::NeedsField:out.status=Status::NeedsFields;break;
            case SourcePropLiftStatus::UnsupportedState:out.status=Status::UnsupportedBehaviorState;break;
            default:out.status=Status::NeedsLinkedOwner;break;
            }
            return out;
        }
        if(!need(10,1))return out;
        if(work.raw[10]==0){out.source_function=0x80113390;if(!need(58,2))return out;
            put(next,12,1,18);put(next,68,4,uint32_t(uint16_t(half(next,58)))<<16u);increment(10);return done();}
        if(work.raw[10]==1)return hold(Status::NeedsRngOrChildOwner,0x801133b4);
        if(work.raw[10]==2){out.source_function=0x8011344c;return bob(out.source_function,true);}
        return hold(Status::UnsupportedBehaviorState,0x801134a8);
    case 3:
        if(selector==0){out.source_function=0x801135dc;return done(false);}
        if(selector!=1)return hold(Status::UnsupportedBehaviorState,0x80113768);
        if(!need(10,1))return out;
        if(work.raw[10]==0){out.source_function=0x801135e4;put(next,12,1,18);put(next,68,4,0);increment(10);return done();}
        if(work.raw[10]==1)return hold(Status::NeedsRngOrChildOwner,0x80113600);
        if(work.raw[10]==2)return hold(Status::NeedsLinkedOwner,0x801136a4);
        if(work.raw[10]==3){out.source_function=0x801136d0;return bob(out.source_function,true);}
        if(work.raw[10]==4){out.source_function=0x80113724;return done(false);}
        return hold(Status::UnsupportedBehaviorState,0x8011372c);
    case 4:
        if(selector==0){out.source_function=0x801137fc;return done(false);}
        if(selector!=1)return hold(Status::UnsupportedBehaviorState,0x80113bb4);
        if(!need(10,1))return out;
        if(work.raw[10]==0){out.source_function=0x80113804;put(next,14,1,0);put(next,13,1,0);increment(10);return done();}
        if(work.raw[10]==1){out.source_function=0x8011381c;if(!need(13,1))return out;
            if(uint8_t(next.raw[13]+1)==3)return hold(Status::NeedsResourceMutation,out.source_function);
            increment(13);return done();}
        if(work.raw[10]==2)return hold(Status::NeedsRngOrChildOwner,0x80113a6c);
        if(work.raw[10]==3)return hold(Status::NeedsLinkedOwner,0x80113b00);
        if(work.raw[10]==4){out.source_function=0x80113b2c;if(!need(13,1)||!need(60,2))return out;
            increment(13);if(next.raw[13]==2)increment(10);put(next,60,2,uint16_t(uint16_t(half(next,60))+32));return done();}
        if(work.raw[10]==5){out.source_function=0x80113b70;return done(false);}
        return hold(Status::UnsupportedBehaviorState,0x80113b78);
    case 5:
        if(selector==0){out.source_function=0x80113520;return done(false);}
        if(selector!=1)return hold(Status::UnsupportedBehaviorState,0x801135a0);
        if(!need(10,1))return out;
        if(work.raw[10]==0){out.source_function=0x80113528;if(!need(60,2))return out;
            put(next,68,4,uint32_t(uint16_t(half(next,60)))<<16u);increment(10);return done();}
        if(work.raw[10]==1){out.source_function=0x80113544;if(!need(68,4))return out;
            put(next,68,4,word(next,68)+0xfffd8000u);put(next,60,2,uint16_t(half(next,70)));return done();}
        return hold(Status::UnsupportedBehaviorState,0x80113564);
    default:return hold(Status::UnsupportedVariant,0x80113ea4);
    }
}
SceneE0DrawResult scene_e0_draw_view(const SourceEventPhase5Bytes<80>& work,
    const SceneE0Resources& resources,const EbdDirectory& directory,SceneE0MeshTier tier) noexcept {
    SceneE0DrawResult out;
    if(!known(work,0,1,out.missing_offset)||!known(work,6,1,out.missing_offset))return out;
    if(!(work.raw[0]&work.raw[6]&2)){out.status=SceneE0Status::NotScheduled;return out;}
    if(!known(work,2,2,out.missing_offset))return out;
    if(work.raw[2]!=3){out.status=SceneE0Status::UnsupportedSubtype;return out;}
    const auto draw=work.raw[0]&0x1c;
    if((draw!=4&&draw!=12)||(work.raw[0]&0x20)){out.status=SceneE0Status::UnsupportedDrawMode;return out;}
    if(!known(work,20,4,out.missing_offset)||!known(work,24,18,out.missing_offset)||
        !known(work,56,6,out.missing_offset)||!known(work,64,4,out.missing_offset))return out;
    if(!resources.resident_in(directory)){out.status=SceneE0Status::NeedsResourceOwnership;return out;}
    out.view.mesh=resources.find(word(work,20),work.raw[3],tier);
    if(!out.view.mesh){out.status=SceneE0Status::NeedsResourceOwnership;return out;}
    for(size_t i=0;i<9;++i)out.view.rotation[i]=half(work,24+i*2);
    for(size_t i=0;i<3;++i)out.view.position[i]=half(work,56+i*2);
    out.view.modulation_word=word(work,64)&0x02ffffffu;out.view.flags=work.raw[0];out.status=SceneE0Status::Ready;return out;
}
} // namespace l3
