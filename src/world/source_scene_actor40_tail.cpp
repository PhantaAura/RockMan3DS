#include "legends3ds/source_scene_actor40_tail.h"
#include <utility>

namespace l3 {
namespace {
using Work=SourceEventPhase5Bytes<1444>;
bool known(const Work& w,size_t at,size_t count) noexcept {
    for(size_t i=0;i<count;++i)if(!w.known[at+i])return false;
    return true;
}
uint16_t read16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t read32(const uint8_t* p) noexcept {
    return uint32_t(read16(p))|(uint32_t(read16(p+2))<<16);
}
bool span(uint32_t token,size_t count,const SourceSceneActor40Resources& r,
    const SharedBytePayload::Storage& payload,size_t& offset) noexcept {
    if(token<r.load_base)return false;
    const uint64_t delta=uint64_t(token)-r.load_base;
    if(delta>payload.size()||count>payload.size()-size_t(delta))return false;
    offset=size_t(delta);return true;
}
bool resident(const SourceSceneActor40Resources& r,const EbdDirectory& e,
    const std::shared_ptr<const SharedBytePayload::Storage>& payload) noexcept {
    if(!payload||payload.get()!=r.payload_identity||e.source_payload.storage_identity()!=payload.get()||
        e.load_base!=r.load_base||e.entries.size()!=r.directory_entries.size())return false;
    for(size_t i=0;i<r.directory_entries.size();++i){const auto& a=r.directory_entries[i];const auto& b=e.entries[i];
        if(a.object_class!=b.object_class||a.subtype!=b.subtype||a.variant!=b.variant||a.key_flags!=b.key_flags)return false;
        for(size_t k=0;k<3;++k)if(a.resource_offset[k]!=b.resource_offset[k])return false;
    }
    return true;
}
bool materials_cannot_alias_actor(const SourceSceneActor40Resources& r,
    const SharedBytePayload::Storage& payload) noexcept {
    // Every8002D320 destination is proved inside the resident EBD virtual
    // region; that region must exclude the original class40 work pool.
    constexpr uint32_t actor_base=0x800c3580u,actor_end=actor_base+4u*1444u;
    const uint64_t end=uint64_t(r.load_base)+payload.size();
    if(end>0x100000000ull||(r.load_base<actor_end&&end>actor_base))return false;
    size_t descriptor=0;
    if((r.descriptor_token&3u)||!span(r.descriptor_token,116,r,payload,descriptor))return false;
    const uint32_t table=read32(payload.data()+descriptor+112);
    size_t table_offset=0;
    if((table&3u)||!span(table,20,r,payload,table_offset))return false;
    for(const size_t part:{size_t(16),size_t(18)}){
        const uint64_t dest_record=uint64_t(table)+part*20u+20u;
        const uint64_t source_record=uint64_t(table)+(part+1u)*20u+20u;
        if(dest_record>UINT32_MAX||source_record>UINT32_MAX)return false;
        size_t dest=0,source=0;
        if(!span(uint32_t(dest_record),12,r,payload,dest)||!span(uint32_t(source_record),12,r,payload,source))return false;
        const uint32_t counts=read32(payload.data()+dest);
        for(size_t kind=0;kind<2;++kind){
            const size_t count=(counts>>(kind*8u))&255u;
            if(!count)continue;
            const uint32_t destination=read32(payload.data()+dest+4u+kind*4u);
            const uint32_t origin=read32(payload.data()+source+4u+kind*4u);
            size_t unused=0;
            if((destination&3u)||(origin&3u)||
                !span(destination,count*12u,r,payload,unused)||!span(origin,count*12u,r,payload,unused))return false;
        }
    }
    return true;
}
}
SourceSceneActor40TailMirrors admit_source_scene_actor40_final_mirrors_8010d824(
    const Work& w,const SourceSceneActor40Resources& r,const EbdDirectory& e,
    const SourceSceneActor40PlayerMirrorReceipt& state) noexcept {
    SourceSceneActor40TailMirrors out;using S=SourceSceneActor40TailStatus;
    for(const auto& window:{std::pair<size_t,size_t>{1,2},{4,1},{6,1},{8,1},{20,6},{136,4}})
        if(!known(w,window.first,window.second))return out;
    if((w.raw[1]&0xe0u)!=0x40u||w.raw[2]!=5||w.raw[4]!=1||w.raw[6]!=0x23||w.raw[8]!=1){out.status=S::UnsupportedActor;return out;}
    if(!state.admitted||state.source_tag!=w.raw[1]){out.status=S::StaleStateReceipt;return out;}
    for(size_t axis=0;axis<3;++axis)if(state.primary_14[axis]!=read16(w.raw.data()+20u+axis*2u)){
        out.status=S::StaleStateReceipt;return out;
    }
    const auto payload=r.payload.lock();
    if(!resident(r,e,payload)||read32(w.raw.data()+136)!=r.descriptor_token){out.status=S::NeedsResourceOwnership;return out;}
    if(!materials_cannot_alias_actor(r,*payload)){out.status=S::UnsafeMaterialDestination;return out;}
    out.status=S::SourceBackedPartial;out.admitted=true;out.player_mirrors=state;
    out.presentation_pending=out.shadow_pending=out.material_cues_pending=true;
    return out;
}
} // namespace l3
