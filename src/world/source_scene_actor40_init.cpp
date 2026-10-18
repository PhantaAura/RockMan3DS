#include "legends3ds/source_scene_actor40_init.h"

namespace l3 {
namespace {
using Work=SourceEventPhase5Bytes<1444>;
bool known(const Work& w,size_t at,size_t bytes) noexcept {
    for(size_t i=0;i<bytes;++i)if(!w.known[at+i])return false;
    return true;
}
uint16_t get16(const Work& w,size_t at) noexcept {return uint16_t(w.raw[at])|(uint16_t(w.raw[at+1])<<8);}
SourceSceneActor40InitializerStatus pose_hold(SourceSceneActor40PoseStatus status) noexcept {
    using P=SourceSceneActor40PoseStatus;using S=SourceSceneActor40InitializerStatus;
    if(status==P::NeedsReference)return S::NeedsReference;
    if(status==P::NeedsScratchShift)return S::NeedsScratchShift;
    if(status==P::NeedsResourceOwnership)return S::NeedsResourceOwnership;
    if(status==P::NeedsWorkFields)return S::NeedsWorkFields;
    if(status==P::UnsupportedScratchShift||status==P::NeedsPerPartScale||status==P::InvalidHierarchy||status==P::UnsupportedActor)return S::UnsupportedScope;
    return S::NeedsPose;
}
}
std::optional<std::array<uint16_t,3>> capture_source_scene_actor40_previous_8010d824(const Work& w) noexcept {
    if(!known(w,20,6))return std::nullopt;
    return std::array<uint16_t,3>{{get16(w,20),get16(w,22),get16(w,24)}};
}
std::optional<bool> source_player_update_gate_8003c07c(
    const L3PlayerCompatWork& player,const SourceEventPlayerKnown& known_fields) noexcept {
    if(!known_fields[0]||!known_fields[6])return std::nullopt;
    return (player.bytes[0]&player.bytes[6]&1u)!=0;
}
SourceSceneActor40InitializerResult initialize_source_scene_actor40_8010c930(
    Work& w,const SourceSceneActor40Resources& resources,const EbdDirectory& ebd,
    const ReferenceTables* refs,std::optional<uint8_t> shift,SourceSceneActor40InitializerGlobals& globals) noexcept {
    SourceSceneActor40InitializerResult out;using S=SourceSceneActor40InitializerStatus;
    if(!known(w,8,1)){out.status=S::NeedsWorkFields;return out;}
    if(w.raw[8]!=0){out.status=S::NeedsFreshInitializer;return out;}
    Work next=w;
    const auto prefix=initialize_source_scene_actor40_prefix_8010c930(next,resources,ebd);out.prefix_status=prefix.status;
    if(!prefix.committed){out.status=prefix.status==SourceSceneActor40Status::NeedsResourceOwnership?S::NeedsResourceOwnership:
        (prefix.status==SourceSceneActor40Status::NeedsWorkFields?S::NeedsWorkFields:S::NeedsPrefix);return out;}
    const auto pose=commit_source_scene_actor40_initial_pose_800322a8(next,resources,ebd,refs,shift);out.pose_status=pose.status;
    if(!pose.committed){out.status=pose_hold(pose.status);return out;}
    // Exact remaining8010C984..8010C9B0 stores after first-pose return.
    // No further original helper or predicate follows these writes.
    const auto word70=uint32_t(0x80120000u-15560u),word74=uint32_t(0x80120000u-15452u);
    next.raw[8]=uint8_t(next.raw[8]+1u);next.known[8]=1;
    globals.word_80122470=word70;globals.word_80122474=word74;w=next;
    out.status=S::SourceBackedPartial;out.committed=true;out.pose_parts=pose.part_count;return out;
}
SourceSceneActor40InitializerResult initialize_source_scene_actor40_wrapper_8010d824(
    Work& w,const SourceSceneActor40Resources& resources,const EbdDirectory& ebd,
    const ReferenceTables* refs,std::optional<uint8_t> shift,SourceSceneActor40InitializerGlobals& globals) noexcept {
    SourceSceneActor40InitializerResult out;const auto before=capture_source_scene_actor40_previous_8010d824(w);
    if(!before){out.status=SourceSceneActor40InitializerStatus::NeedsWorkFields;return out;}
    out=initialize_source_scene_actor40_8010c930(w,resources,ebd,refs,shift,globals);
    if(!out.committed)return out;
    // The admitted initializer preserves actor20/22/24, so these lanes remain
    // known after completion; no mutation can fail after publication.
    SourceSceneActor40PlayerMirrorReceipt receipt;receipt.previous_2c=*before;
    receipt.primary_14={{get16(w,20),get16(w,22),get16(w,24)}};receipt.source_tag=w.raw[1];receipt.admitted=true;
    out.player_mirrors=receipt;return out;
}
bool apply_source_scene_actor40_player_mirrors_8010d824(
    const SourceSceneActor40PlayerMirrorReceipt& receipt,L3PlayerCompatWork& player,SourceEventPlayerKnown& known_fields) noexcept {
    if(!receipt.admitted)return false;
    for(size_t i=0;i<3;++i){
        const uint16_t previous=receipt.previous_2c[i],primary=receipt.primary_14[i];
        player.bytes[0x2c+i*2]=uint8_t(previous);player.bytes[0x2d+i*2]=uint8_t(previous>>8);
        player.bytes[0x14+i*2]=uint8_t(primary);player.bytes[0x15+i*2]=uint8_t(primary>>8);
        known_fields[0x2c+i*2]=known_fields[0x2d+i*2]=1;
        known_fields[0x14+i*2]=known_fields[0x15+i*2]=1;
    }
    return true;
}
} // namespace l3
