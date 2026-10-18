#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
constexpr std::array<uint8_t,32> kOverlayDigest{{
    0x94,0x90,0xc2,0x00,0xed,0xc3,0xa8,0x04,0x5a,0xf4,0x62,0x9e,0x98,0x3b,0xed,0x83,
    0xca,0xb5,0x6a,0x9a,0x53,0xb1,0x78,0xac,0x06,0x7e,0xa7,0x6a,0xf3,0xce,0x41,0x3c}};
uint16_t read16(const uint8_t* p) noexcept {return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);}
uint32_t read32(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
void write32(uint8_t* p,uint32_t value) noexcept {
    for(unsigned i=0;i<4;++i)p[i]=static_cast<uint8_t>(value>>(i*8));
}
void require(bool condition,const char* reason){if(!condition)throw std::runtime_error(reason);}
bool profile_valid(const SourceEventPollProfile& p) noexcept {
    if(p.decoder!=1 || p.source_stage!=0 || p.records.size()>kSourceEventPollMaximumRecords)return false;
    for(const auto& record:p.records)if(record.flag>=kSourceEventFlagBytes*8)return false;
    return true;
}
void initialize(SourceEventWork& work,uint8_t event) noexcept {
    work.raw[0]=1;work.raw[1]=event;
    work.known[0]=1;work.known[1]=1;
    write32(work.raw.data()+4,0);
    for(size_t i=4;i<12;++i)work.known[i]=1;
    for(size_t i=8;i<12;++i)work.raw[i]=0;
    for(size_t i=0x0c;i<=0x1f;++i)work.known[i]=1;
    for(size_t i=0x0c;i<=0x1c;i+=4)write32(work.raw.data()+i,0);
    for(size_t i=0x24;i<=0x63;++i)work.known[i]=1;
    for(size_t i=0x24;i<=0x60;i+=4)write32(work.raw.data()+i,0);
    write32(work.raw.data()+0xa4,0);
    for(size_t i=0xa4;i<0xa8;++i)work.known[i]=1;
}
}

SourceEventPollProfile load_source_event_poll_profile(const std::vector<uint8_t>& bytes) {
    require(bytes.size()>=64,"truncated source event poll header");
    const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='E'&&p[3]=='P',"source event poll magic rejected");
    require(read16(p+4)==1&&read16(p+6)==64,"unsupported source event poll version/header");
    require(read32(p+8)==1&&read32(p+12)==0,"unsupported source event decoder/stage");
    const uint32_t count=read32(p+16);
    require(count<=kSourceEventPollMaximumRecords,"source event poll exceeds record budget");
    require(read32(p+20)==bytes.size()&&bytes.size()==64u+size_t(count)*4u&&read32(p+28)==4,
            "noncanonical source event poll coverage");
    for(size_t i=0;i<32;++i)require(p[32+i]==kOverlayDigest[i],"unsupported source event overlay identity");
    require(crc32(p+64,bytes.size()-64)==read32(p+24),"source event poll payload CRC rejected");
    // Validate every domain before allocating decoded records.
    for(uint32_t i=0;i<count;++i)
        require(read16(p+64+size_t(i)*4)<kSourceEventFlagBytes*8,"source event flag domain rejected");
    SourceEventPollProfile out;out.decoder=1;out.source_stage=0;out.records.reserve(count);
    for(uint32_t i=0;i<count;++i){const auto* r=p+64+size_t(i)*4;out.records.push_back({read16(r),r[2],r[3]});}
    return out;
}

SourceEventFlagSnapshot source_event_flags_from_original(
    const std::array<uint8_t,kSourceEventFlagBytes>& source) noexcept {
    SourceEventFlagSnapshot out;out.known.fill(255);
    for(size_t byte=0;byte<source.size();++byte)
        for(unsigned bit=0;bit<8;++bit)
            if(source[byte]&(0x80u>>bit))out.values[byte]|=static_cast<uint8_t>(1u<<bit);
    return out;
}

SourceEventWork source_event_work_from_original(
    const std::array<uint8_t,kSourceEventWorkBytes>& source) noexcept {
    SourceEventWork out;out.raw=source;out.known.fill(1);return out;
}
SourceEventFlagSnapshot initialize_source_story_flags_8001d8c0() noexcept {
    std::array<uint8_t,kSourceEventFlagBytes> source{};
    // These are original unrenamed words inside the bank, not a presumed
    // all-clear collection of logical flags. Preserve source overlap exactly.
    write32(source.data()+0xb0,0x3f00);
    write32(source.data()+0xb4,24);
    write32(source.data()+0xb8,128);
    write32(source.data()+0xbc,0);
    source[1216>>3]|=static_cast<uint8_t>(0x80u>>(1216&7));
    return source_event_flags_from_original(source);
}
void reset_source_event_work_8001d928(SourceEventWork& work) noexcept {
    for(size_t i=0;i<4;++i){work.raw[i]=0;work.known[i]=1;}
}
SourceEventAreaPrefixResult apply_source_st00_area_event_prefix(
    int8_t source_stage,int8_t source_area,
    SourceEventFlagSnapshot& flags,L3PlayerCompatWork& player) noexcept {
    SourceEventAreaPrefixResult out;
    if(source_stage!=0||source_area<0||source_area>2)return out;
    if(source_area==0){
        player.bytes[0]&=0xfc;
        for(size_t i=8;i<12;++i)player.bytes[i]=0;
        flags.values[1540>>3]|=static_cast<uint8_t>(1u<<(1540&7));
        flags.known[1540>>3]|=static_cast<uint8_t>(1u<<(1540&7));
        out.store_8009888c=0x80000000u;
    }else{
        player.bytes[0x44e]=1;
        if(source_area==1)player.bytes[0]|=3;
    }
    out.committed=true;return out;
}

SourceEventStepResult step_source_event_pipeline(std::optional<SourceEventWork>& work,
    const SourceEventPollProfile* profile,const SourceEventInputs& inputs) noexcept {
    SourceEventStepResult out;
    if(!work || !work->known[0])return out;
    auto next=*work;
    if(next.active00()==0){
        if(!inputs.transition_status_800c356c){out.status=SourceEventStepStatus::NeedsTransitionStatus;return out;}
        if(*inputs.transition_status_800c356c!=0){out.status=SourceEventStepStatus::SourceBackedPartial;out.transition_pending=true;return out;}
        if(!inputs.stage_800c1b60){out.status=SourceEventStepStatus::NeedsStage;return out;}
        if(!profile){out.status=SourceEventStepStatus::NeedsPollProfile;return out;}
        if(!profile_valid(*profile) || *inputs.stage_800c1b60!=profile->source_stage){out.status=SourceEventStepStatus::UnsupportedStageProfile;return out;}
        out.poll_attempted=true;
        for(const auto& record:profile->records){
            const auto mask=static_cast<uint8_t>(1u<<(record.flag&7u));
            if(!inputs.flags || !(inputs.flags->known[record.flag>>3u]&mask)){
                out.status=SourceEventStepStatus::NeedsInitialStoryFlags;return out;
            }
            if(!(inputs.flags->values[record.flag>>3u]&mask))continue;
            out.selected_flag=record.flag;out.selected_event=record.event;out.initializer_attempted=true;
            if(!inputs.player_hp_800b521e){out.status=SourceEventStepStatus::NeedsPlayerHealth;return out;}
            // Original poll stops at the FIRST enabled flag even when the
            // initializer refuses negative HP. It must not try another event.
            if(*inputs.player_hp_800b521e<0){out.status=SourceEventStepStatus::SourceBackedPartial;return out;}
            initialize(next,record.event);out.initialization_prepared=true;break;
        }
        if(next.active00()==0){out.status=SourceEventStepStatus::SourceBackedPartial;return out;}
    }
    out.handler_selected=true;
    if(!next.known[1] || !next.known[4])return out;
    if(next.event01()!=1){out.status=SourceEventStepStatus::UnknownHandler;return out;}
    const auto request=[&](uint32_t function,SourceEventStepStatus status) noexcept {
        out.status=status;out.request=SourceEventConsumerRequest{function,next.event01(),next.phase04()};
    };
    switch(next.phase04()){
        case 0:
            if(!inputs.busy_80098a72){out.status=SourceEventStepStatus::NeedsBusyHalfword;return out;}
            out.source_busy=*inputs.busy_80098a72!=0;
            if(!out.source_busy)next.raw[4]=1;
            break;
        case 1:{
            request(0x80042238u,SourceEventStepStatus::NeedsPlayerConsumer80042238);
            if(!inputs.player_work||!inputs.player_known||!inputs.driver||!inputs.driver_intents)return out;
            const auto consumed=consume_source_event1_phase1_80042238(next,*inputs.player_work,
                *inputs.player_known,*inputs.driver,*inputs.driver_intents,inputs.byte_800c1b93);
            out.player_consumer_status=consumed.status;out.driver_receipt_count=consumed.receipt_count;
            if(!consumed.committed)return out;
            *work=next;out.committed=true;out.status=SourceEventStepStatus::SourceBackedPartial;
            out.request.reset();return out;
        }
        case 2:{
            request(0x8001b33cu,SourceEventStepStatus::NeedsSourceConsumer);
            if(!inputs.phase2_context||!inputs.phase2_profile||!inputs.command_queue)return out;
            const auto consumed=consume_source_event1_phase2_8001b33c(next,*inputs.phase2_context,
                *inputs.phase2_profile,*inputs.command_queue,inputs.phase2_intents,inputs.driver,inputs.driver_intents);
            out.phase2_consumer_status=consumed.status;
            out.driver_receipt_count=consumed.driver_receipt_count;out.sound_receipt_count=consumed.sound_receipt_count;
            out.command_receipt_count=consumed.command_receipt_count;
            if(!consumed.committed)return out;
            *work=next;out.committed=true;out.status=SourceEventStepStatus::SourceBackedPartial;
            out.request.reset();return out;
        }
        case 3:
            if(!inputs.byte_80098ab8){request(0x8001d878u,SourceEventStepStatus::NeedsSourceConsumer);return out;}
            if((*inputs.byte_80098ab8&0x20)!=0){
                next.raw[4]=4;next.known[4]=1;
                write32(next.raw.data()+0x0c,55);
                for(size_t i=0x0c;i<0x10;++i)next.known[i]=1;
            }
            break;
        case 4:{
            for(size_t i=0x0c;i<0x10;++i)if(!next.known[i])return out;
            const uint32_t decremented=read32(next.raw.data()+0x0c)-1u;
            if(decremented==0){
                request(0x80017c30u,SourceEventStepStatus::NeedsSourceConsumer);
                if(!inputs.fade_context)return out;
                const auto consumed=consume_source_fade_startup_80017c30_type2(
                    *inputs.fade_context,inputs.fade_profile,inputs.fade_intents);
                out.fade_consumer_status=consumed.status;
                out.pending_fade_scheduler_intent=consumed.pending_scheduler_intent;
                if(!consumed.committed)return out;
                // Original80101184 stores count0 and increments phase before
                // calling80017C30. Native publication waits for ownership.
                write32(next.raw.data()+0x0c,0);next.raw[4]=5;next.known[4]=1;
                out.request.reset();break;
            }
            write32(next.raw.data()+0x0c,decremented);break;
        }
        case 5:{
            request(0x80100efcu,SourceEventStepStatus::NeedsSourceConsumer);
            if(!inputs.phase5_context||!inputs.phase5_profile)return out;
            const auto consumed=consume_source_event1_phase5_80100efc(next,*inputs.phase5_context,*inputs.phase5_profile,inputs.phase5_resources);
            out.phase5_consumer_status=consumed.status;
            out.script_actor_construction_count=consumed.constructed_actors;
            out.script_resource_binding_count=consumed.resource_bindings;
            if(!consumed.committed)return out;
            *work=next;out.committed=true;out.status=SourceEventStepStatus::SourceBackedPartial;
            out.request.reset();return out;
        }
        case 6:
        case 7:{
            request(0x8010102cu,SourceEventStepStatus::NeedsSourceConsumer);
            if(!inputs.phase5_context||!inputs.phase6_context)return out;
            if(!inputs.phase5_resources||!inputs.phase5_resources->admitted()||
                (inputs.phase5_profile&&!inputs.phase5_resources->matches(*inputs.phase5_profile))){
                out.phase6_consumer_status=SourceEventPhase6Status::NeedsResourceOwnership;return out;
            }
            auto consumed=consume_source_event1_phase6_prefix_8010102c(next,*inputs.phase5_context,
                *inputs.phase6_context,inputs.phase5_resources,inputs.phase6_profile,
                inputs.random_callback_receipts,inputs.random_callback_owner,inputs.candidate_rng);
            out.committed=consumed.committed;
            out.random_callback_consumed_count=consumed.random_callback_consumed_count;
            out.random_callback_status=consumed.random_callback_status;
            // These are successive original callees within one scene update;
            // a failed downstream owner retains the exact continuation.
            if(inputs.phase6_profile&&inputs.phase6_context->awaiting_camera_8001ec0c){
                const uint32_t callback=consumed.callback;
                consumed=consume_source_event1_camera_8001ec0c(next,*inputs.phase5_context,
                    *inputs.phase6_context,inputs.phase6_profile);
                consumed.callback=callback;out.committed|=consumed.committed;
            }
            if(inputs.phase6_profile&&inputs.phase6_context->awaiting_timeline_8001f070){
                const uint32_t callback=consumed.callback;
                consumed=consume_source_event1_timeline_8001f070(next,*inputs.phase6_context,inputs.phase6_profile);
                consumed.callback=callback;out.committed|=consumed.committed;
                if(consumed.committed)*work=next;
            }
            if(inputs.phase6_context->awaiting_caller_801011e8){
                const uint32_t callback=consumed.callback;
                consumed=consume_source_event1_caller_801011e8(next,*inputs.phase5_context,*inputs.phase6_context,
                    inputs.flags,inputs.busy_80098a72,inputs.scene_overlay_serial);
                consumed.callback=callback;out.committed|=consumed.committed;
            }
            if(consumed.status==SourceEventPhase6Status::NeedsType10FadeConsumer && inputs.fade_context){
                const auto callback=consumed.callback;
                consumed=consume_source_event1_type10_fade_8010122c(next,*inputs.phase6_context,
                    *inputs.fade_context,inputs.fade_profile,inputs.fade_intents,inputs.flags);
                consumed.callback=callback;out.committed|=consumed.committed;
                out.fade_consumer_status=consumed.fade_startup_status;
                if(consumed.committed)*work=next;
            } else if(consumed.status==SourceEventPhase6Status::NeedsHandoffConsumer &&
                      inputs.fade_context && inputs.handoff){
                const auto callback=consumed.callback;
                consumed=consume_source_event1_handoff_80101260(next,*inputs.phase6_context,
                    *inputs.fade_context,*inputs.handoff);
                consumed.callback=callback;out.committed|=consumed.committed;
                if(consumed.committed)*work=next;
            }
            if(consumed.status==SourceEventPhase6Status::Type10FadeConsumed ||
               consumed.status==SourceEventPhase6Status::HandoffConsumed ||
               consumed.status==SourceEventPhase6Status::HandoffConditionFalse){
                const auto callback=consumed.callback;
                consumed=consume_source_event1_color_tail_80101310(*inputs.phase5_context,
                    *inputs.phase6_context,inputs.scene_overlay_serial);
                consumed.callback=callback;out.committed|=consumed.committed;
            }
            out.phase6_consumer_status=consumed.status;out.script_callback=consumed.callback;
            if(consumed.next_consumer!=0x8010102cu)out.request->source_function=consumed.next_consumer;
            if(consumed.status==SourceEventPhase6Status::CallerConsumed){out.request.reset();out.status=SourceEventStepStatus::SourceBackedPartial;}
            return out;
        }
        default:out.status=SourceEventStepStatus::UnknownPhase;return out;
    }
    *work=next;out.status=SourceEventStepStatus::SourceBackedPartial;out.committed=true;return out;
}
} // namespace l3
