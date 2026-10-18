#include "legends3ds/source_scene_actor40_update.h"
#include "legends3ds/source_event_pipeline.h"

namespace l3 {
namespace {
using Work=SourceEventPhase5Bytes<1444>;
template<class T>bool known(const T& w,size_t at,size_t n) noexcept {
    for(size_t i=at;i<at+n;++i){if(!w.known[i])return false;}
    return true;
}
uint16_t get16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t get32(const uint8_t* p) noexcept {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
template<class T>void put(T& w,size_t at,size_t n,uint32_t value) noexcept {
    for(size_t i=0;i<n;++i){w.raw[at+i]=uint8_t(value>>(i*8));w.known[at+i]=1;}
}
bool resident(const SourceSceneActor40Resources& r,const EbdDirectory& e) noexcept {
    const auto p=r.payload.lock();if(!p||p.get()!=r.payload_identity||e.source_payload.storage_identity()!=p.get()||e.load_base!=r.load_base||e.entries.size()!=r.directory_entries.size())return false;
    for(size_t i=0;i<r.directory_entries.size();++i){const auto& a=r.directory_entries[i];const auto& b=e.entries[i];
        if(a.object_class!=b.object_class||a.subtype!=b.subtype||a.variant!=b.variant||a.key_flags!=b.key_flags)return false;
        for(size_t k=0;k<3;++k)if(a.resource_offset[k]!=b.resource_offset[k])return false;}return true;
}
SourceSceneActor40LaterStatus pose_hold(SourceSceneActor40PoseStatus status) noexcept {
    using P=SourceSceneActor40PoseStatus;using S=SourceSceneActor40LaterStatus;
    if(status==P::NeedsResourceOwnership)return S::NeedsResourceOwnership;
    if(status==P::NeedsWorkFields)return S::NeedsWorkFields;
    if(status==P::NeedsReference)return S::NeedsReference;
    if(status==P::NeedsScratchShift)return S::NeedsScratchShift;
    if(status==P::NeedsPose)return S::NeedsPose;
    return S::UnsupportedScope;
}
template<size_t Count,size_t Bytes>SourceEventPhase5Bytes<Bytes>* resolve(
    SourceEventPhase5Pool<Count,Bytes>& pool,uint32_t token,uint32_t base,uint8_t family) noexcept {
    if(token<base)return nullptr;
    const uint32_t delta=token-base;
    if(delta%Bytes||delta/Bytes>=Count)return nullptr;
    const size_t index=delta/Bytes;
    const auto& bits=pool.used[index/32];if(!bits||!(*bits&(0x80000000u>>(index&31u))))return nullptr;
    auto& slot=pool.slots[index];if(!known(slot,0,2)||slot.raw[1]!=uint8_t(family|index))return nullptr;return &slot;
}
struct Store {uint8_t* raw=nullptr;uint8_t* known_lane=nullptr;uint8_t value=0;};
struct Journal {
    std::array<Store,6> stores{};size_t count=0;
    std::optional<uint32_t>* release=nullptr;uint32_t release_value=0;
    bool external(const SourceEventWork* e,SourceEventPhase5Context* s,size_t global,
        size_t lane,uint8_t value) noexcept {
        if(!e||!s||!known(*e,global,4)||count>=stores.size())return false;
        auto* slot=resolve(s->pool_e0,get32(e->raw.data()+global),0x800bf118u,0xe0);
        if(!slot||lane>=slot->raw.size())return false;
        stores[count++]={slot->raw.data()+lane,slot->known.data()+lane,value};return true;
    }
    bool destroy80(const SourceEventWork* e,SourceEventPhase5Context* s,size_t global) noexcept {
        if(!e||!s||!known(*e,global,4)||count>=stores.size())return false;
        const uint32_t token=get32(e->raw.data()+global);
        auto* slot=resolve(s->pool80,token,0x800a3b40u,0x80);if(!slot)return false;
        const size_t index=(token-0x800a3b40u)/72u,word=index/32;
        if(!s->pool80_pending_release_800988c8[word]||!s->pool80.used[word])return false;
        release=&s->pool80_pending_release_800988c8[word];
        release_value=(**release|(0x80000000u>>(index&31u)))&*s->pool80.used[word];
        stores[count++]={slot->raw.data(),slot->known.data(),0};return true;
    }
    void apply() noexcept {for(size_t i=0;i<count;++i){*stores[i].raw=stores[i].value;*stores[i].known_lane=1;}if(release)*release=release_value;}
};
}
bool project_source_scene_actor40_fixed_position_80030150(Work& w) noexcept {
    if(!known(w,28,12))return false;
    for(size_t axis=0;axis<3;++axis){const uint32_t fixed=get32(w.raw.data()+28+axis*4);put(w,20+axis*2,2,(fixed>>16)+((fixed>>15)&1u));}return true;
}
SourceSceneActor40LaterResult update_source_scene_actor40_prefix_8010d824(
    Work& work,const SourceSceneActor40Resources& r,const EbdDirectory& e,const ReferenceTables* refs,
    std::optional<uint8_t> shift,const SourceEventWork* event,SourceEventPhase5Context* scene) noexcept {
    SourceSceneActor40LaterResult out;using S=SourceSceneActor40LaterStatus;
    for(const auto window:{std::pair<size_t,size_t>{1,2},{4,1},{8,7},{20,6},{28,12}})if(!known(work,window.first,window.second))return out;
    if((work.raw[1]&0xe0)!=0x40||work.raw[2]!=5||work.raw[4]!=1||work.raw[8]!=1){out.status=S::UnsupportedActor;return out;}
    if(work.raw[9]>12){out.status=S::UnsupportedState;return out;}
    if(!resident(r,e)){out.status=S::NeedsResourceOwnership;return out;}
    Work next=work;Journal journal;const uint8_t state=next.raw[9],sub=next.raw[10];out.state=state;out.substate=sub;
    auto inc=[&]() noexcept {put(next,10,1,uint32_t(next.raw[10])+1);};
    auto fixed_add=[&](size_t at,uint32_t delta) noexcept {put(next,at,4,get32(next.raw.data()+at)+delta);};
    auto frame=[&]() noexcept -> std::optional<uint8_t> {
        const auto* p=select_source_scene_actor40_pose(next,r);if(!p)return std::nullopt;return p->sequencing_record[0];};
    auto reset=[&](uint8_t request) noexcept {
        const auto result=reset_source_scene_actor40_pose_800322a8(next,r,e,refs,shift,request);
        if(!result.committed){out.status=pose_hold(result.status);return false;}
        out.request_reset=true;out.request=request;out.request0_reset=request==0;return true;};
    auto external=[&](size_t global,size_t lane,uint8_t value) noexcept {
        if(journal.external(event,scene,global,lane,value))return true;
        out.status=S::NeedsExternalActor;return false;};
    auto need_frame=[&](std::optional<uint8_t>& f) noexcept {f=frame();if(f)return true;out.status=S::NeedsPose;return false;};
    auto unsupported=[&]() noexcept {out.status=S::UnsupportedState;};
    std::optional<uint8_t> f;
    switch(state){
    case 0:if(sub==0){if(!reset(0))return out;inc();}break;
    case 1:
        if(sub>12){unsupported();return out;}
        if(sub==0){put(next,14,1,0);if(!reset(16))return out;inc();}
        else if(sub==3){put(next,14,1,1);if(!reset(17))return out;inc();}
        else if(sub==5){put(next,12,1,uint32_t(next.raw[12])-1);if(next.raw[12]==0)inc();}
        else if(sub==9){put(next,13,1,uint32_t(next.raw[13])-1);if(next.raw[13]==0){if(!external(0x30,9,0)||!external(0x30,10,0)||!reset(19))return out;inc();}}
        else if(sub!=12){if(!need_frame(f))return out;
            if(sub==1){if(*f==50&&(!external(0x30,9,1)||!external(0x30,10,0)))return out;if(*f==61)inc();}
            else if(sub==2){if(*f==61)inc();}
            else if(sub==4){if(*f==23){put(next,12,1,30);inc();}}
            else if(sub==6){fixed_add(36,0x00015d17u);if(*f==44)inc();}
            else if(sub==7){if(*f==61){put(next,14,1,2);if(!reset(18))return out;inc();}}
            else if(sub==8){if(*f==32){put(next,13,1,30);inc();}}
            else if(sub==10){if(*f==45){if(!reset(20))return out;inc();}}
            else if(sub==11){fixed_add(36,0x00030000u);if(*f==59)inc();}}
        break;
    case 2:
        if(sub>1){unsupported();return out;}if(sub==0){if(!reset(21))return out;inc();}else fixed_add(36,0x00050000u);
        if(!need_frame(f))return out;
        if(*f==2||*f==22)out.audio_cue=SourceSceneActor40AudioCue{144,0,100};
        break;
    case 3:if(sub==0){if(!reset(22))return out;inc();}break;
    case 4:
        if(sub==0){if(!reset(23))return out;put(next,264,1,20);put(next,276,1,21);inc();}
        else {if(!need_frame(f))return out;
            if(*f==120){if(!external(0x28,9,1)||!external(0x60,9,1))return out;
                if(!journal.destroy80(event,scene,0x64)){out.status=S::NeedsReleaseOwner;return out;}}
            if(*f==127&&!reset(24))return out;}
        break;
    case 5:if(sub==0){if(!reset(25)||!external(0x28,9,2))return out;inc();}break;
    case 6:
        if(sub>3){unsupported();return out;}
        if(sub==0){if(!reset(48))return out;inc();}
        else if(sub==1){if(!need_frame(f))return out;
        if(*f==38){if(!reset(34))return out;put(next,13,1,47);inc();}}
        else if(sub==2){put(next,13,1,uint32_t(next.raw[13])-1);if(next.raw[13]==0)inc();fixed_add(28,0xfffe4c42u);}
        break;
    case 7:
        if(sub>2){unsupported();return out;}if(sub==0){if(!reset(26))return out;inc();}
        else if(sub==1){if(!need_frame(f))return out;fixed_add(36,0xfff8db6eu);if(*f==21)inc();}
        break;
    case 8:if(sub==0){if(!reset(27))return out;inc();}break;
    case 9:
        if(sub>3){unsupported();return out;}if(sub==0){if(!reset(28))return out;inc();}
        else if(sub==1){if(!need_frame(f))return out;if(*f==9)inc();}
        else if(sub==2){if(!need_frame(f))return out;const uint32_t old=get32(next.raw.data()+28);fixed_add(28,0xfffd8000u);if(*f==32){put(next,28,4,old);inc();}}
        break;
    case 10:
        if(sub>4){unsupported();return out;}if(sub==0){if(!reset(29))return out;inc();}
        else if(sub!=4){if(!need_frame(f))return out;
            if(sub==1){if(*f==8)inc();}
            else if(sub==2){fixed_add(36,0xfff70000u);if(*f==13)inc();}
            else {fixed_add(36,0xfff70000u);fixed_add(32,0x00073333u);if(*f==38)inc();}}break;
    case 11:
        if(sub>1){unsupported();return out;}if(sub==0){if(!reset(30)||!external(0x28,9,3))return out;inc();}else fixed_add(36,0xffe00000u);break;
    case 12:
        if(sub>5){unsupported();return out;}if(sub==0){if(!reset(31))return out;inc();}
        else if(sub==4){if(!reset(32))return out;inc();}
        else if(sub!=5){if(!need_frame(f))return out;
            if(sub==1){fixed_add(36,0xffdf8000u);if(*f==40)inc();}
            else if(sub==2){fixed_add(36,0xffec0000u);if(*f==60)inc();}
            else if(*f==63)inc();}
        if(!need_frame(f))return out;
        if(*f==3||*f==11)out.audio_cue=SourceSceneActor40AudioCue{145,0,100};
        break;
    default:unsupported();return out;
    }
    SourceSceneActor40PlayerMirrorReceipt mirrors;for(size_t i=0;i<3;++i)mirrors.previous_2c[i]=get16(work.raw.data()+20+i*2);
    mirrors.source_tag=work.raw[1];mirrors.admitted=true;
    (void)project_source_scene_actor40_fixed_position_80030150(next);
    if(next.raw[12]==0){const auto result=advance_source_scene_actor40_pose_80032488(next,r,e,refs,shift);
        if(!result.committed){out.status=pose_hold(result.status);return out;}out.sequence_advanced=true;}
    for(size_t i=0;i<3;++i)mirrors.primary_14[i]=get16(next.raw.data()+20+i*2);
    out.pre_presentation_mirrors=mirrors;out.status=S::SourceBackedPrefix;out.committed=true;
    out.shadow_pending=true;out.timed_cues_pending=true;out.external_stores=uint8_t(journal.count);
    journal.apply();work=next;return out;
}
SourceSceneActor40LaterResult update_source_scene_actor40_state0_prefix_8010d824(
    Work& w,const SourceSceneActor40Resources& r,const EbdDirectory& e,const ReferenceTables* refs,std::optional<uint8_t> shift) noexcept {
    if(!known(w,9,1))return {};
    if(w.raw[9]!=0){SourceSceneActor40LaterResult out;out.status=SourceSceneActor40LaterStatus::UnsupportedState;return out;}
    return update_source_scene_actor40_prefix_8010d824(w,r,e,refs,shift);
}
} // namespace l3
