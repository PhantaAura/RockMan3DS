#include "legends3ds/source_main_loop.h"

namespace l3 {
namespace {
uint16_t half(const SourceMainLoopPadWork& work,size_t offset) noexcept {
    return uint16_t(work.raw[offset])|uint16_t(uint16_t(work.raw[offset+1])<<8);
}
void put(SourceMainLoopPadWork& work,size_t offset,size_t count,uint32_t value) noexcept {
    for(size_t i=0;i<count;++i){work.raw[offset+i]=uint8_t(value>>(8*i));work.known[offset+i]=1;}
}
bool consume_pad(SourceMainLoopPadWork& work,SourceMainLoopPadResult& out) noexcept {
    const auto need=[&](size_t offset,size_t count) noexcept {
        for(size_t i=0;i<count;++i)if(!work.known[offset+i]){
            out.missing_offset=uint8_t(offset+i);return false;
        }
        return true;
    };
    if(!need(0,1))return false;
    if(work.raw[0]==255){
        put(work,36,1,255);put(work,37,1,0);put(work,38,2,0);
        put(work,48,4,0);put(work,40,4,0);return true;
    }
    if(!need(1,7)||!need(38,2))return false;
    const uint16_t prior=half(work,38),held=uint16_t(~half(work,2));
    uint32_t extra=0;for(size_t i=0;i<4;++i)extra|=uint32_t(work.raw[4+i])<<(8*i);
    put(work,36,1,work.raw[1]>>4);put(work,48,4,extra);
    put(work,40,2,prior);put(work,38,2,held);
    put(work,42,2,uint16_t(held&uint16_t(~prior)));
    put(work,44,2,uint16_t(prior&uint16_t(~held)));
    constexpr uint16_t masks[]={16,64,128,32,1024,2048};
    for(size_t i=0;i<6;++i){
        if(!need(46,2))return false;
        const uint16_t repeat=half(work,46),mask=masks[i];
        if(!(repeat&mask)&&(half(work,42)&mask)==mask){
            if(!need(56+i,1))return false;
            put(work,64+i,1,0);
            if(work.raw[56+i]==0)put(work,56+i,1,1);
            else{put(work,56+i,1,2);put(work,46,2,uint16_t(repeat|mask));}
        }else{
            if(!need(56+i,1))return false;
            const uint8_t state=work.raw[56+i];
            if(state){
                if(state!=2){
                    if(!need(64+i,1))return false;
                    const uint8_t before=work.raw[64+i];put(work,64+i,1,uint8_t(before+1));
                    if(before<6)continue;
                }
                put(work,56+i,1,0);put(work,46,2,uint16_t(repeat&uint16_t(~mask)));
            }
        }
    }
    if(!need(37,1))return false;
    const uint8_t mode=uint8_t(work.raw[37]&3u);
    if(mode==1)put(work,54,1,1);
    if(mode==2)put(work,54,1,0);
    put(work,37,1,uint8_t((work.raw[37]&253u)|((work.raw[37]&1u)<<1)));
    return true;
}
} // namespace
void initialize_source_main_loop_pads_from_executable_boot(SourceMainLoopPads& pads) noexcept {
    for(auto& pad:pads){pad.raw.fill(0);pad.known.fill(1);}
}
void bind_source_main_loop_pad_packet(SourceMainLoopPadWork& work,
    const std::array<uint8_t,8>& packet) noexcept {
    for(size_t i=0;i<packet.size();++i){work.raw[i]=packet[i];work.known[i]=1;}
}
SourceMainLoopPadResult consume_source_main_loop_pads_80012298(SourceMainLoopPads& pads) noexcept {
    SourceMainLoopPadResult out;auto next=pads;
    for(size_t port=0;port<next.size();++port){out.missing_port=uint8_t(port);
        if(!consume_pad(next[port],out))return out;
    }
    pads=next;out.status=SourceMainLoopPadStatus::Consumed;out.committed=true;
    out.missing_port=0;return out;
}
std::optional<uint16_t> source_main_loop_pad_pressed_800c0c2a(const SourceMainLoopPadWork& work) noexcept {
    if(!work.known[42]||!work.known[43])return {};
    return half(work,42);
}
SourceMainLoopPhaseResult evaluate_source_main_loop_phase_800155d4(
    std::optional<int8_t> phase,std::optional<int8_t> subphase,
    std::optional<int8_t> control,std::optional<uint8_t> active) noexcept {
    SourceMainLoopPhaseResult out;
    if(!phase)return out;
    if(*phase!=3){out.status=SourceMainLoopPhaseStatus::NeedsPhaseConsumer;return out;}
    out.source_function=0x800158f0;
    if(!subphase){out.status=SourceMainLoopPhaseStatus::NeedsSubphase;return out;}
    if(*subphase<0||*subphase>=8){out.status=SourceMainLoopPhaseStatus::NotScheduled;return out;}
    if(*subphase==2&&control&&*control==0&&active&&*active!=0){
        out.status=SourceMainLoopPhaseStatus::ReadyForUpdate;
        out.update_pass=0x80015db4;out.e0_sweep=0x80017528;return out;
    }
    switch(*subphase){
    case 1:case 3:case 4:case 5:case 7:
        out.status=SourceMainLoopPhaseStatus::ReadyForUpdate;
        out.update_pass=0x80015db4;out.e0_sweep=0x80017528;return out;
    default:out.status=SourceMainLoopPhaseStatus::NeedsBranchOwners;return out;
    }
}
void initialize_source_main_loop_scratch_prefix_80012004(SourceMainLoopScratchOwners& owner) noexcept {
    owner.byte_1f800000=0;owner.byte_1f800001=2;
}
SourceMainLoopGateStatus evaluate_source_main_loop_gate_80011ce8(
    std::optional<uint8_t> cadence,std::optional<uint32_t> elapsed) noexcept {
    if(!cadence)return SourceMainLoopGateStatus::NeedsCadenceByte;
    if(!elapsed)return SourceMainLoopGateStatus::NeedsVblankWord;
    return *elapsed<*cadence?SourceMainLoopGateStatus::WaitingForVblank:SourceMainLoopGateStatus::Ready;
}
bool consume_source_main_loop_vblank_800123fc(std::optional<uint32_t>& word) noexcept {
    if(!word)return false;
    *word+=1u;return true;
}
} // namespace l3
