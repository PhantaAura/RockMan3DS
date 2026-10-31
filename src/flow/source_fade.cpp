#include "legends3ds/source_fade.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
constexpr std::array<uint8_t,32> kExeDigest{{
    0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,
    0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
uint16_t read16(const uint8_t* p) noexcept {return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);}
uint32_t read32(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
template<size_t Size> void store(SourceFadeBytes<Size>& out,size_t offset,size_t bytes,uint32_t value) noexcept {
    for(size_t i=0;i<bytes;++i){out.raw[offset+i]=static_cast<uint8_t>(value>>(8*i));out.known[offset+i]=1;}
}
template<size_t Size> bool known(const SourceFadeBytes<Size>& bytes,size_t offset,size_t count) noexcept {
    for(size_t i=0;i<count;++i)if(!bytes.known[offset+i])return false;
    return true;
}
SourceFadeCallbackResult iterate(SourceFadeContext& context,SourceFadeCallbackState& state,
    std::optional<uint8_t> endpoint_hold,bool initialize) noexcept {
    SourceFadeCallbackResult out;
    auto next=context;
    auto& work=next.work_80098a68;
    if(initialize){
        out.status=SourceFadeCallbackStatus::NeedsMode;
        if(!known(work,0,1))return out;
        if(work.raw[0]!=0){out.status=SourceFadeCallbackStatus::UnsupportedMode;return out;}
        out.status=SourceFadeCallbackStatus::NeedsBusyHalfword;
        if(!known(work,10,2))return out;
        const uint16_t busy=read16(work.raw.data()+10);
        const bool positive=busy!=0&&(busy&0x8000u)==0;
        store(work,1,1,positive?0u:1u);
        store(work,14,2,positive?255u:0u);
        store(work,4,1,0);
        store(work,8,2,1);
    }
    out.status=SourceFadeCallbackStatus::NeedsPhase;
    if(!known(work,1,1))return out;
    const uint8_t phase=work.raw[1];
    if(phase==0||phase==1){
        out.status=SourceFadeCallbackStatus::NeedsIntensity;
        if(!known(work,14,2))return out;
        out.status=SourceFadeCallbackStatus::NeedsStep;
        if(!known(work,12,2))return out;
        const uint16_t intensity=read16(work.raw.data()+14),step=read16(work.raw.data()+12);
        const uint16_t changed=phase==0?static_cast<uint16_t>(intensity-step):
            static_cast<uint16_t>(intensity+step);
        // Original downward branch tests the sign after sll16; upward branch
        // sign-extends the wrapped halfword before comparing it against256.
        const bool endpoint=phase==0?(changed==0||(changed&0x8000u)!=0):
            ((changed&0x8000u)==0&&changed>=256);
        if(endpoint){
            out.status=SourceFadeCallbackStatus::NeedsEndpointHoldByte;
            if(!endpoint_hold)return out;
            store(work,14,2,phase==0?0u:255u);
            store(work,1,1,2);
            store(work,12,2,*endpoint_hold);
        }else store(work,14,2,changed);
    }else if(phase==2){
        out.status=SourceFadeCallbackStatus::NeedsStep;
        if(!known(work,12,2))return out;
        const uint16_t hold=read16(work.raw.data()+12);
        if(hold==0){
            out.status=SourceFadeCallbackStatus::NeedsBusyHalfword;
            if(!known(work,10,2))return out;
            if(read16(work.raw.data()+10)==1)store(work,8,2,0);
            store(work,10,2,0);
            context=next;state.running=false;++state.committed_iterations;
            out.status=SourceFadeCallbackStatus::TerminatedSourceCallback;
            out.committed=true;out.source_callback_terminated=true;out.source_boundary=0x80012eccu;
            return out;
        }
        store(work,12,2,static_cast<uint16_t>(hold-1u));
    }
    // Signed phase values outside0/1/2 fall through unchanged to the yield.
    context=next;state.running=true;++state.committed_iterations;
    out.status=SourceFadeCallbackStatus::YieldedSourceIteration;
    out.committed=true;out.yield_one_scheduler_pass=true;out.source_boundary=0x80012e98u;
    return out;
}
void require(bool condition,const char* reason){if(!condition)throw std::runtime_error(reason);}
}
SourceFadeStartupProfile load_source_fade_startup_profile(const std::vector<uint8_t>& bytes) {
    require(bytes.size()==68||bytes.size()==72||bytes.size()==76,"noncanonical source fade profile size");
    const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='F'&&p[3]=='D',"source fade profile magic rejected");
    const uint16_t version=read16(p+4);
    require(version>=1&&version<=3&&read16(p+6)==64,"unsupported source fade profile version/header");
    const uint32_t payload=version==1?4u:version==2?8u:12u;
    require(bytes.size()==64u+payload&&read32(p+8)==version&&read32(p+12)==0&&read32(p+16)==1&&read32(p+20)==bytes.size()&&read32(p+28)==payload,
            "unsupported source fade profile coverage/decoder");
    for(size_t i=0;i<kExeDigest.size();++i)require(p[32+i]==kExeDigest[i],"unsupported source fade owner identity");
    require(crc32(p+64,payload)==read32(p+24),"source fade profile payload CRC rejected");
    require(version!=2||read16(p+70)==0,"source fade reserved field rejected");
    SourceFadeStartupProfile result;result.decoder=version;
    if(version==3){
        // v3: 0x80097A44, 0x80097A46, then 0x80097A48..0x80097A4E.
        result.duration_80097a44=read16(p+64);result.duration_80097a46=read16(p+66);
        std::array<uint16_t,4> steps{};
        for(size_t i=0;i<4;++i)steps[i]=read16(p+68+2*i);
        result.steps_80097a48=steps;result.step_80097a4c=steps[2];
        return result;
    }
    result.duration_80097a44=read16(p+64);result.step_80097a4c=read16(p+66);
    if(version==2)result.duration_80097a46=read16(p+68);
    return result;
}
SourceFadeStartupResult consume_source_fade_startup_80017c30_type2(
    SourceFadeContext& context,const SourceFadeStartupProfile* profile,
    SourceFadeSchedulerIntentQueue* intents) noexcept {
    SourceFadeStartupResult out;
    const auto& work=context.work_80098a68;
    if(!work.known[10]||!work.known[11])return out;
    if(read16(work.raw.data()+10)!=0){out.status=SourceFadeStartupStatus::SourceBackedBusyNoop;out.committed=true;return out;}
    out.status=SourceFadeStartupStatus::NeedsProfile;
    if(!profile||!profile->duration_80097a44||!profile->step_80097a4c)return out;
    if(profile->decoder<1||profile->decoder>3){out.status=SourceFadeStartupStatus::UnsupportedProfile;return out;}
    out.status=SourceFadeStartupStatus::NeedsSchedulerIntentQueue;
    if(!intents||!intents->initialized||intents->count>intents->entries.size())return out;
    if(intents->count==intents->entries.size()){out.status=SourceFadeStartupStatus::SchedulerIntentQueueFull;return out;}
    auto next=context;
    store(next.work_80098a68,0,1,0);
    store(next.work_80098a68,10,2,*profile->duration_80097a44);
    store(next.work_80098a68,12,2,*profile->step_80097a4c);
    for(auto& tile:next.tiles_80098a28){
        store(tile,3,1,3);store(tile,7,1,0x62);
        store(tile,8,2,0);store(tile,10,2,0);store(tile,12,2,320);store(tile,14,2,240);
    }
    // Original GetTPage/SetDrawMode for type2 produces these words regardless
    // of GPUtype80097718. Tag links and tile RGB are not initialized here.
    for(auto& draw:next.draw_modes_80098a10){store(draw,3,1,2);store(draw,4,4,0xe1000140u);store(draw,8,4,0);}
    intents->entries[intents->count++]=SourceFadeSchedulerIntent{};
    context=next;
    out.status=SourceFadeStartupStatus::PendingSchedulerIntent;
    out.committed=true;out.pending_scheduler_intent=true;return out;
}
SourceFadeStartupResult consume_source_fade_startup_80017c30_type10(
    SourceFadeContext& context,const SourceFadeStartupProfile* profile,
    SourceFadeSchedulerIntentQueue* intents) noexcept {
    SourceFadeStartupResult out;
    const auto& work=context.work_80098a68;
    if(!work.known[10]||!work.known[11])return out;
    if(read16(work.raw.data()+10)!=0){out.status=SourceFadeStartupStatus::SourceBackedBusyNoop;out.committed=true;return out;}
    out.status=SourceFadeStartupStatus::NeedsProfile;
    if(!profile||!profile->duration_80097a46||!profile->step_80097a4c)return out;
    if(profile->decoder<2){out.status=SourceFadeStartupStatus::UnsupportedProfile;return out;}
    // Original type2 andtype10 select80017C74 and mode0, with identical
    // packets and callback. Only the duration index differs (A0>>3).
    SourceFadeStartupProfile selected;selected.decoder=1;
    selected.duration_80097a44=profile->duration_80097a46;
    selected.step_80097a4c=profile->step_80097a4c;
    return consume_source_fade_startup_80017c30_type2(context,&selected,intents);
}
SourceFadeStartupResult consume_source_fade_startup_80017c30_mode0(
    SourceFadeContext& context,const SourceFadeStartupProfile* profile,
    SourceFadeSchedulerIntentQueue* intents,uint8_t type) noexcept {
    SourceFadeStartupResult out;
    if(type>11||(type&4)){out.status=SourceFadeStartupStatus::UnsupportedProfile;return out;}
    const auto& work=context.work_80098a68;
    if(!work.known[10]||!work.known[11])return out;
    if(read16(work.raw.data()+10)!=0){out.status=SourceFadeStartupStatus::SourceBackedBusyNoop;out.committed=true;return out;}
    out.status=SourceFadeStartupStatus::NeedsProfile;
    if(!profile)return out;
    const auto duration=(type>>3)?profile->duration_80097a46:profile->duration_80097a44;
    std::optional<uint16_t> step;
    if(profile->steps_80097a48)step=(*profile->steps_80097a48)[type&3];
    else if((type&3)==2)step=profile->step_80097a4c;
    if(!duration||!step)return out;
    SourceFadeStartupProfile selected;selected.decoder=1;
    selected.duration_80097a44=duration;selected.step_80097a4c=step;
    return consume_source_fade_startup_80017c30_type2(context,&selected,intents);
}
SourceFadeCallbackResult acknowledge_source_fade_scheduler_intent(
    SourceFadeContext& context,SourceFadeSchedulerIntentQueue& intents,
    SourceFadeCallbackState& state,std::optional<uint8_t> endpoint_hold) noexcept {
    SourceFadeCallbackResult out;
    if(state.running){out.status=SourceFadeCallbackStatus::AlreadyRunning;return out;}
    out.status=SourceFadeCallbackStatus::NeedsSchedulerIntent;
    if(!intents.initialized||intents.count==0||intents.count>intents.entries.size())return out;
    const auto& intent=intents.entries[0];
    if(intent.source_function!=0x80012e10u||intent.callback!=0x80017e94u||intent.slot!=2){
        out.status=SourceFadeCallbackStatus::UnsupportedSchedulerIntent;return out;
    }
    out=iterate(context,state,endpoint_hold,true);
    if(!out.committed)return out;
    for(size_t i=1;i<intents.count;++i)intents.entries[i-1]=intents.entries[i];
    intents.entries[--intents.count]={};
    out.native_scheduler_acknowledged=true;
    return out;
}
SourceFadeCallbackResult advance_source_fade_callback(SourceFadeContext& context,
    SourceFadeCallbackState& state,std::optional<uint8_t> endpoint_hold) noexcept {
    if(!state.running)return {};
    return iterate(context,state,endpoint_hold,false);
}
SourceFadePresentation describe_source_fade_presentation(const SourceFadeContext& context) noexcept {
    SourceFadePresentation out;
    const auto& work=context.work_80098a68;
    if(!known(work,8,2))return out;
    if(read16(work.raw.data()+8)==0){out.status=SourceFadePresentationStatus::SourceInactive;return out;}
    out.status=SourceFadePresentationStatus::NeedsIntensity;
    if(!known(work,14,2))return out;
    out.status=SourceFadePresentationStatus::NeedsMode;
    if(!known(work,0,1))return out;
    if(work.raw[0]!=0){out.status=SourceFadePresentationStatus::UnsupportedMode;return out;}
    // Both source buffers must own the same semantic type2 packet constructor.
    // Link words/RGB need not be invented to submit a native equivalent plane.
    out.status=SourceFadePresentationStatus::NeedsTileFields;
    for(const auto& tile:context.tiles_80098a28){
        if(!known(tile,3,1)||!known(tile,7,9))return out;
        if(tile.raw[3]!=3||tile.raw[7]!=0x62u||read16(tile.raw.data()+8)!=0||read16(tile.raw.data()+10)!=0||
           read16(tile.raw.data()+12)!=320||read16(tile.raw.data()+14)!=240){
            out.status=SourceFadePresentationStatus::UnsupportedMode;return out;
        }
    }
    out.status=SourceFadePresentationStatus::NeedsDrawModeFields;
    for(const auto& draw:context.draw_modes_80098a10){
        if(!known(draw,3,9))return out;
        if(draw.raw[3]!=2||read32(draw.raw.data()+4)!=0xe1000140u||read32(draw.raw.data()+8)!=0){
            out.status=SourceFadePresentationStatus::UnsupportedMode;return out;
        }
    }
    out.status=SourceFadePresentationStatus::SourceBackedPresentation;
    out.visible=true;out.grayscale=work.raw[14];out.width=320;out.height=240;out.subtract_from_scene=true;
    return out;
}
SourceFadePacketResult bind_source_fade_presentation_packets(
    SourceFadeContext& context,const SourceFadePacketInput& input) noexcept {
    SourceFadePacketResult out;
    const auto& work=context.work_80098a68;
    if(!known(work,8,2))return out;
    if(read16(work.raw.data()+8)==0){
        out.status=SourceFadePresentationStatus::SourceInactive;out.committed=true;
        out.ordering_table_tag_116=input.ordering_table_tag_116;return out;
    }
    out.status=SourceFadePresentationStatus::NeedsIntensity;
    if(!known(work,14,2))return out;
    out.status=SourceFadePresentationStatus::NeedsBufferSelector;
    if(!input.buffer_1f800000)return out;
    if(*input.buffer_1f800000>1){out.status=SourceFadePresentationStatus::UnsupportedBufferSelector;return out;}
    out.status=SourceFadePresentationStatus::NeedsOrderingTableTag;
    if(!input.ordering_table_tag_116)return out;
    const size_t index=*input.buffer_1f800000;
    out.status=SourceFadePresentationStatus::NeedsPacketTag;
    if(!known(context.tiles_80098a28[index],0,4)||!known(context.draw_modes_80098a10[index],0,4))return out;
    auto next=context;
    auto& tile=next.tiles_80098a28[index];auto& draw=next.draw_modes_80098a10[index];
    const uint32_t previous=*input.ordering_table_tag_116;
    const uint32_t tile_address=0x80098a28u+static_cast<uint32_t>(index)*16u;
    const uint32_t draw_address=0x80098a10u+static_cast<uint32_t>(index)*12u;
    for(size_t i=4;i<7;++i)store(tile,i,1,work.raw[14]);
    store(tile,0,4,(read32(tile.raw.data())&0xff000000u)|(previous&0x00ffffffu));
    // Original first inserts TILE, then DR_MODE at the same OT+116 head.
    store(draw,0,4,(read32(draw.raw.data())&0xff000000u)|(tile_address&0x00ffffffu));
    out.ordering_table_tag_116=(previous&0xff000000u)|(draw_address&0x00ffffffu);
    context=next;out.status=SourceFadePresentationStatus::SourceBackedPresentation;out.committed=true;
    return out;
}
} // namespace l3
