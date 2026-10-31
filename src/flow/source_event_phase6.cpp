#include "legends3ds/source_event_phase6.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
uint32_t get32(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
bool known(const SourceEventWork& work,size_t offset,size_t count) noexcept {
    for(size_t i=0;i<count;++i)if(!work.known[offset+i])return false;
    return true;
}
template<size_t N>void put(SourceEventPhase5Bytes<N>& owner,size_t offset,size_t count,uint32_t value) noexcept {
    for(size_t i=0;i<count;++i){owner.raw[offset+i]=uint8_t(value>>(8*i));owner.known[offset+i]=1;}
}
template<size_t Count,size_t Bytes>SourceEventPhase5Bytes<Bytes>* resolve(
    SourceEventPhase5Pool<Count,Bytes>& pool,uint32_t token,uint32_t base,uint8_t family) noexcept {
    if(token<base)return nullptr;
    const uint32_t delta=token-base;
    if(delta%Bytes||delta/Bytes>=Count)return nullptr;
    const size_t index=delta/Bytes;const auto& occupied=pool.used[index/32];
    if(!occupied||!(*occupied&(0x80000000u>>(index&31u))))return nullptr;
    auto& slot=pool.slots[index];
    if(!slot.known[1]||slot.raw[1]!=uint8_t(family|index))return nullptr;
    return &slot;
}
bool profile_valid(const SourceEventPhase6Profile& profile) noexcept {
    return profile.initialized&&profile.camera_program_token==0x8011af4cu&&profile.callback_table_token==0x8011b2e8u&&
        profile.camera_words.back()==0xffffffffu&&profile.timeline[0].callback==0x80100738u&&
        profile.timeline.back().header==255u&&profile.timeline.back().callback==0;
}
void require(bool condition,const char* why){if(!condition)throw std::runtime_error(why);}
uint16_t get16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
}
SourceEventPhase6Profile load_source_event_phase6_profile(const std::vector<uint8_t>& bytes){
    require(bytes.size()==1180,"phase6 coverage rejected");const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='P'&&p[3]=='6',"phase6 magic rejected");
    require(get16(p+4)==1&&get16(p+6)==96,"phase6 version/header rejected");
    require(get32(p+8)==1&&get32(p+12)==0&&get32(p+16)==231&&get32(p+20)==18&&get32(p+24)==bytes.size(),"phase6 decoder/budget rejected");
    constexpr char exe[]="10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0";
    constexpr char overlay[]="9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c";
    const auto digit=[](char c)noexcept{return uint8_t(c<='9'?c-'0':c-'a'+10);};
    for(size_t i=0;i<32;++i){require(p[32+i]==uint8_t(digit(exe[2*i])*16+digit(exe[2*i+1])),"phase6 executable identity rejected");
        require(p[64+i]==uint8_t(digit(overlay[2*i])*16+digit(overlay[2*i+1])),"phase6 overlay identity rejected");}
    require(get32(p+28)==crc32(p+96,bytes.size()-96),"phase6 payload CRC rejected");
    SourceEventPhase6Profile out;out.initialized=true;out.camera_program_token=get32(p+96);out.callback_table_token=get32(p+100);
    const uint32_t distance=get32(p+104);require(distance<=65535&&get32(p+108)==0,"phase6 projection/reserved domain rejected");
    out.field_800bc81c=uint16_t(distance);
    for(size_t i=0;i<231;++i)out.camera_words[i]=get32(p+112+i*4);
    for(size_t i=0;i<18;++i){out.timeline[i].header=get32(p+1036+i*8);out.timeline[i].callback=get32(p+1040+i*8);}
    require(profile_valid(out),"phase6 source data identity rejected");return out;
}
void initialize_source_event_scene_from_executable_boot_80068000(SourceEventPhase5Context& scene) noexcept {
    scene.camera_800bc738.raw.fill(0);scene.camera_800bc738.known.fill(1);
}
bool initialize_source_event_phase6_overlay_load(SourceEventPhase6Context& context,
    const SourceEventPhase5Profile& profile) noexcept {
    if(!profile.initialized||profile.callback_table_token!=0x8011b2e8u||profile.directory_token!=0x8011af4cu)return false;
    context={};context.byte_80122424=0;return true;
}
void initialize_source_event_phase6_executable_boot_80068000(SourceEventPhase6Context& context) noexcept {
    context.byte_800c1b6a=0;context.input_800c0c2a.reset();
}
bool consume_source_main_loop_tail_80011ecc(std::optional<uint16_t>& half) noexcept {
    if(!half)return false;
    half=uint16_t(uint32_t(*half)+1u);return true;
}
std::optional<uint32_t> source_event_phase6_selected_callback_8010102c(
    const SourceEventWork& work,const SourceEventPhase6Profile& profile) noexcept {
    if(!profile_valid(profile)||!known(work,8,1)||work.raw[8]==255||!known(work,0xa4,4))return std::nullopt;
    const uint32_t token=get32(work.raw.data()+0xa4);
    if(token<profile.callback_table_token||(token-profile.callback_table_token)%8)return std::nullopt;
    const size_t index=(token-profile.callback_table_token)/8;
    if(index>=profile.timeline.size())return std::nullopt;
    return profile.timeline[index].callback;
}
SourceEventPhase6Result consume_source_event1_phase6_prefix_8010102c(
    const SourceEventWork& work,SourceEventPhase5Context& scene,
    SourceEventPhase6Context& context,const SourceEventPhase5ResourceBinding* resources,
    const SourceEventPhase6Profile* profile,
    const SourceEventRandomEffectReceipts* random_receipts,
    const SourceEventRandomEffectReceiptOwner* random_owner,
    const SourceEventCandidateRng* candidate_rng) noexcept {
    SourceEventPhase6Result out;out.callback=context.consumed_callback;
    if(!known(work,0,2)||!known(work,4,1))return out;
    if(work.active00()==0||work.event01()!=1||(work.phase04()!=6&&work.phase04()!=7)){out.status=SourceEventPhase6Status::WrongEventPhase;return out;}
    if(context.awaiting_caller_801011e8){out.status=SourceEventPhase6Status::AwaitingCallerConsumer;out.next_consumer=0x801011e8u;return out;}
    if(context.awaiting_timeline_8001f070){out.status=SourceEventPhase6Status::AwaitingTimelineConsumer;out.next_consumer=0x8001f070u;return out;}
    if(context.awaiting_camera_8001ec0c){out.status=SourceEventPhase6Status::AwaitingCameraConsumer;out.next_consumer=0x8001ec0cu;return out;}
    out.callback=0;
    if(!context.byte_80122424){out.status=SourceEventPhase6Status::NeedsOverlayControl;return out;}
    std::optional<uint32_t> camera_adjusted;
    if(*context.byte_80122424==1){
        if(!context.scratch_half_1f800002){out.status=SourceEventPhase6Status::NeedsInputWord;return out;}
        for(size_t i=12;i<16;++i)if(!scene.camera_800bc738.known[i]){
            out.status=SourceEventPhase6Status::NeedsCameraField;out.missing_offset=12;return out;}
        camera_adjusted=get32(scene.camera_800bc738.raw.data()+12)+
            ((*context.scratch_half_1f800002&2)?0x20000u:0xfffe0000u);
    }
    if(!known(work,8,1))return out;
    const bool enabled=work.raw[8]!=255;
    uint32_t frame=0;bool random_callback_pending=false;
    struct Bytes {uint8_t* raw=nullptr;uint8_t* known=nullptr;size_t size=0;};
    struct Store {Bytes owner;uint16_t offset=0;uint8_t count=0;uint32_t value=0;};
    std::array<Store,64> stores{};size_t count=0;
    const auto append=[&](Bytes owner,size_t offset,size_t bytes,uint32_t value)noexcept{
        if(!owner.raw||offset+bytes>owner.size||count==stores.size())return false;
        stores[count++]={owner,uint16_t(offset),uint8_t(bytes),value};return true;
    };
    const auto actor=[&](size_t work_offset,bool family40)noexcept -> Bytes {
        if(!known(work,work_offset,4)){out.status=SourceEventPhase6Status::NeedsEventWork;out.missing_offset=uint16_t(work_offset);return {};}
        const uint32_t token=get32(work.raw.data()+work_offset);
        if(family40){auto* slot=resolve(scene.pool40,token,0x800c3580u,0x40);
            if(slot)return {slot->raw.data(),slot->known.data(),slot->raw.size()};
        }else{auto* slot=resolve(scene.pool_e0,token,0x800bf118u,0xe0);
            if(slot)return {slot->raw.data(),slot->known.data(),slot->raw.size()};}
        out.status=SourceEventPhase6Status::NeedsActorBinding;out.missing_offset=uint16_t(work_offset);return {};
    };
    const auto read_actor=[&](Bytes owner,size_t offset,size_t bytes,uint32_t& value)noexcept{
        if(!owner.raw)return false;
        for(size_t i=0;i<bytes;++i)if(!owner.known[offset+i]){
            out.status=SourceEventPhase6Status::NeedsActorField;out.missing_offset=uint16_t(offset);return false;}
        value=0;for(size_t i=0;i<bytes;++i)value|=uint32_t(owner.raw[offset+i])<<(i*8);return true;
    };
    const auto state=[&](size_t offset,bool family40,uint8_t value)noexcept{
        const auto target=actor(offset,family40);
        return append(target,9,1,value)&&append(target,10,1,0);
    };
    std::array<uint32_t,2> used80{};uint32_t used_c0=0;bool allocated80=false,allocated_c0=false;
    const auto allocate_header=[&](Bytes target)noexcept{
        return append(target,2,2,0)&&append(target,4,4,0)&&append(target,8,4,0)&&
            append(target,12,4,0)&&append(target,16,4,0)&&append(target,6,1,3);
    };
    const auto allocate=[&](bool family80)noexcept -> Bytes {
        if(family80){
            if(!allocated80){if(!scene.pool80.used[0]||!scene.pool80.used[1]){out.status=SourceEventPhase6Status::NeedsPoolBitmap;return {};}
                used80={{*scene.pool80.used[0],*scene.pool80.used[1]}};allocated80=true;}
            for(size_t i=0;i<64;++i){const uint32_t bit=0x80000000u>>(i&31u);if(used80[i/32]&bit)continue;
                auto& slot=scene.pool80.slots[i];if(!slot.known[1]||slot.raw[1]!=uint8_t(0x80|i)){
                    out.status=SourceEventPhase6Status::NeedsPoolHeader;out.missing_offset=1;return {};}
                used80[i/32]|=bit;Bytes result{slot.raw.data(),slot.known.data(),slot.raw.size()};
                if(allocate_header(result)){return result;}
                return {};
            }
        }else{
            if(!allocated_c0){if(!scene.pool_c0.used[0]){out.status=SourceEventPhase6Status::NeedsPoolBitmap;return {};}
                used_c0=*scene.pool_c0.used[0];allocated_c0=true;}
            for(size_t i=0;i<16;++i){const uint32_t bit=0x80000000u>>i;if(used_c0&bit)continue;
                auto& slot=scene.pool_c0.slots[i];if(!slot.known[1]||slot.raw[1]!=uint8_t(0xc0|i)){
                    out.status=SourceEventPhase6Status::NeedsPoolHeader;out.missing_offset=1;return {};}
                used_c0|=bit;Bytes result{slot.raw.data(),slot.known.data(),slot.raw.size()};
                if(allocate_header(result)){return result;}
                return {};
            }
        }
        out.status=SourceEventPhase6Status::PoolCapacityExceeded;return {};
    };
    if(enabled){
        if(!known(work,0xa4,4)||!known(work,0x10,4))return out;
        const uint32_t token=get32(work.raw.data()+0xa4);
        if(!profile){
            // Preserve the inherited independently proved first-callback API.
            // Advancing table identities require the admitted data owner.
            if(token!=0x8011b2e8u){out.status=SourceEventPhase6Status::UnsupportedCallbackTable;return out;}
            out.callback=0x80100738u;
        }else{
            if(!profile_valid(*profile)){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
            if(token<profile->callback_table_token||(token-profile->callback_table_token)%8||
                (token-profile->callback_table_token)/8>=profile->timeline.size()){
                out.status=SourceEventPhase6Status::UnsupportedCallbackTable;return out;}
            out.callback=*source_event_phase6_selected_callback_8010102c(work,*profile);
        }
        frame=get32(work.raw.data()+0x10);
        if(!resources||!resources->admitted()){out.status=SourceEventPhase6Status::NeedsResourceOwnership;return out;}
        switch(out.callback){
        case 0x80100738u:
            if(frame==0){const auto a=actor(0x24,true),b=actor(0x30,false);
                if(!append(a,0x15e,2,16912)||!append(b,0x40,4,0x00808080u))return out;
            }else if(frame==425){if(!state(0x24,true,6))return out;
            }else if(frame==465){if(!state(0x30,false,2))return out;}break;
        case 0x801007f0u:
            if(frame==0){const auto a=actor(0x24,true),b=actor(0x30,false);
                if(!append(a,0x1c,4,0)||!append(a,0x56,2,2048)||!append(a,9,1,1)||!append(a,10,1,0)||
                    !append(b,9,1,0)||!append(b,10,1,0)||!append(b,0x38,2,0)||!append(b,0x40,4,0x00808080u))return out;}break;
        case 0x8010086cu:if(frame==30&&!state(0x24,true,2))return out;break;
        case 0x8010089cu:break; // Original JR RA/NOP, no invented operation.
        case 0x801008a4u:case 0x80100e9cu:
            if(frame==0&&!state(0x24,true,0)){return out;}
            break;
        case 0x801008d4u:
            if(frame==0){const auto a=actor(0x24,true);
                if(!append(a,9,1,2)||!append(a,10,1,0)||!append(a,0x1c,4,0)||
                    !append(a,0x20,4,0xfef00000u)||!append(a,0x24,4,0x0d590000u))return out;
            }else if(frame==20&&!state(0x24,true,0))return out;break;
        case 0x80100920u:
            if(frame==30&&!state(0x24,true,3))return out;
            if(frame==96){const auto a=allocate(true);
                if(!append(a,0,1,1)||!append(a,2,1,2)||!append(a,12,1,2)||!append(a,3,1,1)||
                    !append(a,13,1,0)||!append(a,14,1,0)||!append(a,28,2,16)||
                    !append(a,30,2,uint16_t(-404))||!append(a,32,2,3568))return out;}break;
        case 0x801009b0u:
            if(frame==0&&!state(0x24,true,4))return out;
            if(frame==120){auto& slot=scene.pool_c0.slots[0];if(!append({slot.raw.data(),slot.known.data(),slot.raw.size()},3,1,1))return out;}
            if(frame==210&&!state(0x24,true,5))return out;
            if(frame==310&&!state(0x24,true,7))return out;
            if(frame==370){const auto a=actor(0x2c,false);if(!append(a,9,1,1))return out;}break;
        case 0x80100a3cu:
            if(frame==50){const auto a=actor(0x24,true);uint32_t value=0;
                if(!read_actor(a,0x20,4,value)||!append(a,9,1,8)||!append(a,10,1,0)||!append(a,0x20,4,value+0x00500000u))return out;}break;
        case 0x80100a70u:if(frame==60&&!state(0x2c,false,2))return out;break;
        case 0x80100a94u:
            if(frame==0){for(size_t i=0;i<3;++i){const auto a=allocate(false);
                if(!append(a,0,1,1)||!append(a,2,1,i<2?3:4)||!append(a,3,1,i==0?2:(i==1?3:0))||
                    !append(a,4,1,0)||!append(a,5,1,0))return out;}}
            if(frame==26&&!state(0x44,false,1))return out;
            if(frame==33&&!state(0x48,false,1))return out;
            if(frame==40&&!state(0x40,false,1))return out;
            if(frame==49&&!state(0x24,true,9)){return out;}
            break;
        case 0x80100be8u:
            if(frame==0){const auto a=actor(0x2c,false);uint32_t value=0;
                if(!read_actor(a,0,1,value)||!append(a,0,1,value&253u)||!state(0x24,true,10))return out;}break;
        case 0x80100c30u:
            if(frame==0){const auto a=actor(0x24,true);
                if(!append(a,9,1,11)||!append(a,10,1,0)||!append(a,0x1c,4,0)||!append(a,0x20,4,0)||!append(a,0x56,2,0))return out;}
            if(frame==1&&!state(0x34,false,1))return out;
            if(frame==13&&!state(0x38,false,1))return out;
            if(frame==25&&!state(0x3c,false,1))return out;
            if(frame==68&&!state(0x30,false,3)){return out;}
            break;
        case 0x80100d34u:
            if(frame==0){const auto a=actor(0x24,true);uint32_t value=0;
                if(!read_actor(a,0x24,4,value)||!append(a,9,1,12)||!append(a,10,1,0)||!append(a,0x24,4,value+0x00c80000u))return out;}break;
        case 0x80100d6cu:
            // Defer the complete bounded random-result transaction until the
            // common journal commit, after every enclosing prefix guard.
            random_callback_pending=true;break;
        case 0x80100eccu:if(frame==0&&!state(0x24,true,11))return out;break;
        default:out.status=SourceEventPhase6Status::UnsupportedCallback;return out;
        }
    }
    // No callback is repeated after a downstream hold. Only the bounded,
    // preflighted stores/bitmaps below are committed; resource draw, actor AI,
    // audio meaning and retail scene completion are separate consumers.
    std::optional<uint8_t> random_control;
    if(random_callback_pending){
        const auto consumed=candidate_rng&&!random_receipts&&!random_owner
            ?consume_source_event_random_effect_candidate(work,scene,*candidate_rng)
            :consume_source_event_random_effect_80100d6c(work,scene,random_receipts,random_owner);
        out.random_callback_status=consumed.status;out.missing_offset=consumed.missing_offset;
        if(!consumed.committed){
            using RandomStatus=SourceEventRandomEffectStatus;
            switch(consumed.status){
            case RandomStatus::NeedsEventWork:out.status=SourceEventPhase6Status::NeedsEventWork;break;
            case RandomStatus::WrongEventPhase:out.status=SourceEventPhase6Status::WrongEventPhase;break;
            case RandomStatus::NeedsActorBinding:out.status=SourceEventPhase6Status::NeedsActorBinding;break;
            case RandomStatus::NeedsActorField:out.status=SourceEventPhase6Status::NeedsActorField;break;
            case RandomStatus::NeedsPoolBitmap:out.status=SourceEventPhase6Status::NeedsPoolBitmap;break;
            case RandomStatus::NeedsPoolHeader:out.status=SourceEventPhase6Status::NeedsPoolHeader;break;
            default:out.status=SourceEventPhase6Status::NeedsRandomOwner;out.next_consumer=0x8007fe50u;break;
            }
            return out;
        }
        random_control=consumed.control_80122424;out.random_callback_consumed_count=consumed.committed_call_count;
    }
    if(camera_adjusted)put(scene.camera_800bc738,12,4,*camera_adjusted);
    for(size_t i=0;i<count;++i){const auto& store=stores[i];for(size_t j=0;j<store.count;++j){
        store.owner.raw[store.offset+j]=uint8_t(store.value>>(j*8));store.owner.known[store.offset+j]=1;}}
    if(allocated80){scene.pool80.used[0]=used80[0];scene.pool80.used[1]=used80[1];}
    if(allocated_c0)scene.pool_c0.used[0]=used_c0;
    if(enabled){
        if(out.callback==0x80100738u&&frame==0){scene.byte_80122428=1;context.byte_8012242c=64;context.byte_80122430=64;
            context.byte_80122434=64;context.byte_80122438=2;}
        if(out.callback==0x801007f0u&&frame==0)scene.byte_80122428=0;
        if(out.callback==0x80100a94u&&frame==49)context.byte_80122424=1;
        if(random_control)context.byte_80122424=*random_control;
        out.callback_executed=true;
    }
    context.consumed_callback=out.callback;context.awaiting_camera_8001ec0c=true;
    out.status=SourceEventPhase6Status::CallbackConsumed;out.committed=true;out.next_consumer=0x8001ec0cu;return out;
}
SourceEventPhase6Result consume_source_event1_camera_8001ec0c(const SourceEventWork& work,
    SourceEventPhase5Context& scene,SourceEventPhase6Context& context,const SourceEventPhase6Profile* profile) noexcept {
    SourceEventPhase6Result out;out.next_consumer=0x8001ec0cu;
    if(!context.awaiting_camera_8001ec0c){out.status=SourceEventPhase6Status::AwaitingCameraConsumer;return out;}
    if(!profile){out.status=SourceEventPhase6Status::NeedsProfile;return out;}
    if(!profile_valid(*profile)){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
    auto next=scene.camera_800bc738;auto state=context;
    const auto read=[&](size_t offset,uint32_t& value)noexcept{
        for(size_t i=0;i<4;++i)if(!next.known[offset+i]){out.status=SourceEventPhase6Status::NeedsCameraWord;out.missing_offset=uint16_t(offset);return false;}
        value=get32(next.raw.data()+offset);return true;};
    uint32_t token=0;if(!read(184,token))return out;
    const auto counter=[&](size_t offset,uint32_t& value)noexcept{
        if(!known(work,offset,4)){out.status=SourceEventPhase6Status::NeedsEventCounter;out.missing_offset=uint16_t(offset);return false;}
        value=get32(work.raw.data()+offset);return true;};
    bool stopped=false;
    for(size_t budget=0;budget<profile->camera_words.size();++budget){
        if(token<profile->camera_program_token||(token-profile->camera_program_token)%4){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
        const size_t index=(token-profile->camera_program_token)/4;
        if(index>=profile->camera_words.size()){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
        const uint32_t command=profile->camera_words[index];const uint8_t operation=uint8_t(command>>24);
        if(command==0xffffffffu){stopped=true;break;}
        uint32_t value=0;
        if(operation<=2){
            const size_t offset=operation==0?0x14:(operation==1?0x18:0x1c);
            if(!counter(offset,value))return out;
            if(operation==0){if(value<(command&0x00ffffffu)){stopped=true;break;}}
            else{const size_t selector=operation==1?8:9;
                if(!known(work,selector,1)){out.status=SourceEventPhase6Status::NeedsEventWork;return out;}
                if(work.raw[selector]!=uint8_t(command>>16)||value!=(command&65535u)){stopped=true;break;}}
            token+=4;
        }else if(operation==3||operation==4){put(next,0,1,operation-3);token+=4;
        }else if(operation==5||operation==6){state.byte_800bc733=uint8_t(operation-5);token+=4;
        }else if(operation>=16&&operation<=22){
            if(index+3>=profile->camera_words.size()){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
            constexpr size_t offsets[]={8,56,24,72,168,104,120};
            if(operation==16)state.byte_800bc733=0;
            if(operation==20){state.byte_800bc733=1;put(next,1,1,command>>16);}
            for(size_t i=0;i<3;++i)put(next,offsets[operation-16]+i*4,4,profile->camera_words[index+1+i]);
            token+=16;
        }else if(operation>=32&&operation<=34){
            const uint8_t shift=uint8_t(operation-32);const uint32_t distance=profile->field_800bc81c;
            const uint32_t expanded=distance<<shift;const uint32_t selected=expanded<13107u?expanded:distance;
            if(!source_projection_fog_8007a1cc(state.projection,selected,384)){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
            constexpr uint16_t scratch[]={3584,1664,768};
            state.byte_1f800074=shift;state.half_1f800006=scratch[shift];state.half_1f800076=scratch[shift];token+=4;
        }else if(operation==255){stopped=true;break;
        }else{out.status=SourceEventPhase6Status::UnsupportedCameraCommand;return out;}
        put(next,184,4,token);
    }
    if(!stopped){out.status=SourceEventPhase6Status::UnsupportedCameraCommand;return out;}
    for(const size_t base:{size_t(8),size_t(56),size_t(104)})for(size_t i=0;i<3;++i){
        uint32_t position=0,velocity=0;
        if(!read(base+i*4,position)||!read(base+16+i*4,velocity))return out;
        put(next,base+i*4,4,position+velocity);
    }
    scene.camera_800bc738=next;state.awaiting_camera_8001ec0c=false;state.awaiting_timeline_8001f070=true;context=state;
    out.committed=true;out.status=SourceEventPhase6Status::CameraConsumed;out.next_consumer=0x8001f070u;return out;
}
bool apply_source_scene_tracked_target_8001e5bc(const SourceEventWork& work,
    SourceEventPhase5Context& scene,const SourceEventPhase6Context& context) noexcept {
    if(!context.byte_800bc733||*context.byte_800bc733==0)return false;
    auto& camera=scene.camera_800bc738;
    if(!camera.known[1])return false;
    const size_t slot_offset=0x24+size_t(camera.raw[1])*4;
    if(slot_offset+4>work.raw.size()||!known(work,slot_offset,4))return false;
    const uint32_t token=get32(work.raw.data()+slot_offset);
    const uint8_t* raw=nullptr;const uint8_t* valid=nullptr;
    if(auto* a=resolve(scene.pool40,token,0x800c3580u,0x40)){raw=a->raw.data();valid=a->known.data();}
    else if(auto* b=resolve(scene.pool_e0,token,0x800bf118u,0xe0)){raw=b->raw.data();valid=b->known.data();}
    if(!raw)return false;
    for(size_t i=0;i<12;++i)if(!valid[0x1c+i]||!camera.known[0xa8+i])return false;
    for(size_t axis=0;axis<3;++axis)
        put(camera,8+axis*4,4,get32(raw+0x1c+axis*4)+get32(camera.raw.data()+0xa8+axis*4));
    return true;
}
SourceEventPhase6Result consume_source_event1_timeline_8001f070(SourceEventWork& work,
    SourceEventPhase6Context& context,const SourceEventPhase6Profile* profile) noexcept {
    SourceEventPhase6Result out;out.next_consumer=0x8001f070u;
    if(!context.awaiting_timeline_8001f070){out.status=SourceEventPhase6Status::AwaitingTimelineConsumer;return out;}
    if(!profile){out.status=SourceEventPhase6Status::NeedsProfile;return out;}
    if(!profile_valid(*profile)){out.status=SourceEventPhase6Status::UnsupportedProfile;return out;}
    if(!known(work,0x10,16)||!known(work,0xa4,4)){out.status=SourceEventPhase6Status::NeedsEventCounter;return out;}
    auto next=work;const uint32_t token=get32(next.raw.data()+0xa4);
    if(token<profile->callback_table_token||(token-profile->callback_table_token)%8){out.status=SourceEventPhase6Status::NeedsTimelineWord;return out;}
    const size_t index=(token-profile->callback_table_token)/8;
    if(index>=profile->timeline.size()){out.status=SourceEventPhase6Status::NeedsTimelineWord;return out;}
    const auto store=[&](size_t offset,uint32_t value)noexcept{for(size_t i=0;i<4;++i){next.raw[offset+i]=uint8_t(value>>(8*i));next.known[offset+i]=1;}};
    for(size_t offset=0x10;offset<0x20;offset+=4)store(offset,get32(next.raw.data()+offset)+1u);
    const uint16_t raw_deadline=uint16_t(profile->timeline[index].header>>16);
    const uint32_t deadline=raw_deadline<=32767u?uint32_t(raw_deadline):(uint32_t(raw_deadline)|0xffff0000u);
    if(raw_deadline!=65535u&&deadline==get32(next.raw.data()+0x10)){
        if(index+1>=profile->timeline.size()){out.status=SourceEventPhase6Status::NeedsTimelineWord;return out;}
        if(!known(next,8,2)){out.status=SourceEventPhase6Status::NeedsEventWork;return out;}
        const uint32_t header=profile->timeline[index+1].header;
        const uint8_t byte08=uint8_t(header),byte09=uint8_t(header>>8);
        if(next.raw[8]!=byte08){store(0x18,0);store(0x1c,0);}
        if(next.raw[9]!=byte09)store(0x1c,0);
        next.raw[8]=byte08;next.raw[9]=byte09;next.known[8]=next.known[9]=1;
        store(0x10,0);store(0xa4,token+8);
    }
    work=next;context.awaiting_timeline_8001f070=false;context.awaiting_caller_801011e8=true;
    out.committed=true;out.status=SourceEventPhase6Status::TimelineConsumed;out.next_consumer=0x801011e8u;return out;
}
SourceEventCompletionPredicateResult evaluate_source_event_completion_8001de84(
    const SourceEventWork& work,const SourceEventFlagSnapshot* flags,
    std::optional<int8_t> stage,std::optional<uint16_t> input) noexcept {
    SourceEventCompletionPredicateResult out;
    if(!stage)return out;
    if(*stage==0){
        if(!known(work,1,1)){out.status=SourceEventPhase6Status::NeedsEventWork;return out;}
        const uint16_t id=uint16_t(896u+work.event01());const uint8_t mask=uint8_t(1u<<(id&7u));
        if(!flags||(flags->known[id>>3u]&mask)==0){out.status=SourceEventPhase6Status::NeedsStoryFlag;return out;}
        if((flags->values[id>>3u]&mask)==0){out.value=false;out.status=SourceEventPhase6Status::CallerConsumed;return out;}
    }
    if(!input){out.status=SourceEventPhase6Status::NeedsPredicateInput;return out;}
    out.value=(*input&8u)!=0;out.status=SourceEventPhase6Status::CallerConsumed;return out;
}
SourceEventPhase6Result consume_source_event1_caller_801011e8(
    const SourceEventWork& work,const SourceEventPhase5Context& scene,
    SourceEventPhase6Context& context,const SourceEventFlagSnapshot* flags,
    std::optional<int16_t> busy,uint64_t serial) noexcept {
    SourceEventPhase6Result out;out.next_consumer=0x801011e8u;
    if(!known(work,0,2)||!known(work,4,1))return out;
    if(work.active00()==0||work.event01()!=1||(work.phase04()!=6&&work.phase04()!=7)){out.status=SourceEventPhase6Status::WrongEventPhase;return out;}
    if(!context.awaiting_caller_801011e8){out.status=SourceEventPhase6Status::AwaitingCallerConsumer;return out;}
    if(context.scene_overlay_pending){out.status=SourceEventPhase6Status::AwaitingSceneOverlayConsumer;out.next_consumer=0x80100614u;return out;}
    if(!busy){out.status=SourceEventPhase6Status::NeedsCallerBusy;return out;}
    if(work.phase04()==7&&*busy==0){out.status=SourceEventPhase6Status::NeedsHandoffConsumer;out.next_consumer=0x80101260u;return out;}
    if(work.phase04()==6&&*busy==0){
        if(!known(work,9,1)){out.status=SourceEventPhase6Status::NeedsEventWork;return out;}
        bool type10=false;
        if(work.raw[9]==19){
            if(!known(work,0x10,4)){out.status=SourceEventPhase6Status::NeedsEventCounter;return out;}
            type10=get32(work.raw.data()+0x10)==44;
        }
        if(!type10){
            out.next_consumer=0x8001de84u;
            const auto predicate=evaluate_source_event_completion_8001de84(work,flags,context.byte_800c1b6a,context.input_800c0c2a);
            if(!predicate.value){out.status=predicate.status;return out;}
            type10=*predicate.value;
        }
        if(type10){out.status=SourceEventPhase6Status::NeedsType10FadeConsumer;out.next_consumer=0x80017c30u;return out;}
    }
    return consume_source_event1_color_tail_80101310(scene,context,serial);
}
SourceEventPhase6Result consume_source_event1_color_tail_80101310(
    const SourceEventPhase5Context& scene,SourceEventPhase6Context& context,uint64_t serial) noexcept {
    SourceEventPhase6Result out;out.next_consumer=0x80100614u;
    if(!context.awaiting_caller_801011e8){out.status=SourceEventPhase6Status::AwaitingCallerConsumer;return out;}
    if(context.scene_overlay_pending){out.status=SourceEventPhase6Status::AwaitingSceneOverlayConsumer;return out;}
    if(!scene.byte_80122428){out.status=SourceEventPhase6Status::NeedsSceneOverlayControl;return out;}
    if(*scene.byte_80122428==0){
        context.awaiting_caller_801011e8=false;out.committed=true;out.status=SourceEventPhase6Status::CallerConsumed;out.next_consumer=0;return out;
    }
    if(!context.byte_8012242c||!context.byte_80122430||!context.byte_80122434||!context.byte_80122438){
        out.status=SourceEventPhase6Status::NeedsSceneOverlayControl;return out;
    }
    if(*context.byte_80122438!=2){out.status=SourceEventPhase6Status::UnsupportedSceneOverlayMode;return out;}
    if(serial==0){out.status=SourceEventPhase6Status::NeedsSceneOverlayLease;return out;}
    SourceEventSceneOverlayPresentation presentation;presentation.lease.serial=serial;
    presentation.r=*context.byte_8012242c;presentation.g=*context.byte_80122430;presentation.b=*context.byte_80122434;
    context.scene_overlay_pending=presentation;
    out.committed=true;out.status=SourceEventPhase6Status::AwaitingSceneOverlayConsumer;return out;
}
SourceEventPhase6Result consume_source_event1_type10_fade_8010122c(
    SourceEventWork& work,SourceEventPhase6Context& context,SourceFadeContext& fade,
    const SourceFadeStartupProfile* profile,SourceFadeSchedulerIntentQueue* intents,
    const SourceEventFlagSnapshot* flags) noexcept {
    SourceEventPhase6Result out;out.next_consumer=0x80017c30u;
    if(!known(work,0,2)||!known(work,4,1))return out;
    if(!work.active00()||work.event01()!=1||work.phase04()!=6){out.status=SourceEventPhase6Status::WrongEventPhase;return out;}
    if(!context.awaiting_caller_801011e8||context.scene_overlay_pending){out.status=SourceEventPhase6Status::AwaitingCallerConsumer;return out;}
    const auto& raw=fade.work_80098a68;
    if(!raw.known[10]||!raw.known[11]){out.status=SourceEventPhase6Status::NeedsCallerBusy;return out;}
    if(get16(raw.raw.data()+10)!=0){out.status=SourceEventPhase6Status::HandoffConditionFalse;return out;}
    if(!known(work,9,1))return out;
    bool requested=false;
    if(work.raw[9]==19){if(!known(work,16,4)){out.status=SourceEventPhase6Status::NeedsEventCounter;return out;}requested=get32(work.raw.data()+16)==44;}
    if(!requested){const auto predicate=evaluate_source_event_completion_8001de84(work,flags,context.byte_800c1b6a,context.input_800c0c2a);
        if(!predicate.value){out.status=predicate.status;out.next_consumer=0x8001de84u;return out;}requested=*predicate.value;}
    if(!requested){out.status=SourceEventPhase6Status::HandoffConditionFalse;out.next_consumer=0x80100614u;return out;}
    const auto consumed=consume_source_fade_startup_80017c30_type10(fade,profile,intents);
    out.fade_startup_status=consumed.status;
    if(!consumed.committed){out.status=SourceEventPhase6Status::NeedsType10FadeConsumer;return out;}
    work.raw[4]=7;work.known[4]=1;
    out.committed=true;out.status=SourceEventPhase6Status::Type10FadeConsumed;out.next_consumer=0x80100614u;return out;
}
std::optional<SourceEventHeadUvBaseline> capture_source_event_head_uv_baseline(
    const PbdModel& head,const std::vector<PlayerVisualPrimitive>& live) {
    constexpr std::array<uint8_t,4> ids{{1,15,16,17}};
    if(head.parts.size()!=ids.size())return std::nullopt;
    for(size_t i=0;i<ids.size();++i)if(head.parts[i].part_id!=ids[i])return std::nullopt;
    size_t expected=0;
    for(size_t i=2;i<4;++i){
        const auto& part=head.parts[i];
        if(part.triangles.size()>255||part.quads.size()>255)return std::nullopt;
        expected+=part.triangles.size()+part.quads.size();
    }
    if(!expected)return std::nullopt;
    SourceEventHeadUvBaseline out;out.visual_count_=live.size();out.records_.reserve(expected);
    std::array<size_t,2> cursors{};
    for(size_t i=0;i<live.size();++i){
        const auto& visual=live[i];
        if(visual.component_slot!=uint8_t(PlayerComponentSlot::Head)||visual.part_id<16||visual.part_id>17)continue;
        const auto slot=size_t(visual.part_id-16);const auto& part=head.parts[slot+2];
        const auto index=cursors[slot]++;
        if(index>=part.triangles.size()+part.quads.size()||visual.transform_id!=1)return std::nullopt;
        SourceEventHeadUvBaseline::Record record;record.visual_index=i;record.part_id=visual.part_id;
        if(index<part.triangles.size()){
            const auto& face=part.triangles[index];record.vertex_count=3;record.primitive_code=face.code;
            record.clut=face.clut;record.tpage=face.tpage;
            for(size_t v=0;v<3;++v)record.uv[v]={{face.uv[v*2],face.uv[v*2+1]}};
        }else{
            const auto& face=part.quads[index-part.triangles.size()];record.vertex_count=4;record.primitive_code=face.code;
            record.clut=face.clut;record.tpage=face.tpage;
            for(size_t v=0;v<4;++v)record.uv[v]={{face.uv[v*2],face.uv[v*2+1]}};
        }
        if(visual.vertex_count!=record.vertex_count||visual.primitive_code!=record.primitive_code||
           visual.clut!=record.clut||visual.tpage!=record.tpage)return std::nullopt;
        for(size_t v=0;v<record.vertex_count;++v)if(visual.uv[v]!=record.uv[v])return std::nullopt;
        out.records_.push_back(record);
    }
    if(out.records_.size()!=expected)return std::nullopt;
    out.admitted_=true;return out;
}
std::optional<SourceEventHandoffDisplayReceipt> restore_source_event_head_uv_8002d004(
    const SourceEventHeadUvBaseline& baseline,std::vector<PlayerVisualPrimitive>& live,
    const void* native_owner,uint64_t source_generation) noexcept {
    if(!native_owner||!source_generation||!baseline.admitted_||baseline.records_.empty()||
       live.size()!=baseline.visual_count_)return std::nullopt;
    for(const auto& record:baseline.records_){
        if(record.visual_index>=live.size())return std::nullopt;
        const auto& visual=live[record.visual_index];
        if(visual.component_slot!=uint8_t(PlayerComponentSlot::Head)||visual.part_id!=record.part_id||
           visual.transform_id!=1||visual.vertex_count!=record.vertex_count||
           visual.primitive_code!=record.primitive_code)return std::nullopt;
    }
    // 8002D0D0 copies three/four UV words into both PS1 packet buffers. The
    // native pose bridge copies these fields afresh for every submitted frame.
    for(const auto& record:baseline.records_){
        auto& visual=live[record.visual_index];visual.clut=record.clut;visual.tpage=record.tpage;
        for(size_t v=0;v<record.vertex_count;++v)visual.uv[v]=record.uv[v];
    }
    SourceEventHandoffDisplayReceipt receipt;receipt.native_owner=native_owner;
    receipt.source_generation=source_generation;receipt.completed_function=0x8002d004u;
    receipt.first_header=0x800f7820u;receipt.second_header=0x800f7830u;receipt.admitted=true;
    return receipt;
}
bool apply_source_handoff_spawn_80015ee8(const SourceEventHandoffTransitionIntent& intent,
    uint64_t generation,L3PlayerCompatWork& player,SourceEventPlayerKnown& known_player,
    bool source_argument6) noexcept {
    if(!generation||intent.source_generation!=generation||!known_player[8])return false;
    for(size_t i=0;i<6;++i)if(!intent.known[i])return false;
    if(!intent.known[10]||!intent.known[11])return false;
    auto next=player;auto next_known=known_player;
    const auto store=[&](size_t offset,size_t bytes,uint32_t value)noexcept{
        for(size_t i=0;i<bytes;++i){next.bytes[offset+i]=uint8_t(value>>(i*8));next_known[offset+i]=1;}
    };
    for(size_t axis=0;axis<3;++axis){
        const auto bits=get16(intent.raw.data()+axis*2);
        store(0x14+axis*2,2,bits);store(0x2c+axis*2,2,bits);
        store(0x1c+axis*4,4,uint32_t(bits)<<16);store(0x34+axis*4,4,uint32_t(bits)<<16);
    }
    const auto heading=get16(intent.raw.data()+10);
    store(0x5e,2,heading);store(0x56,2,heading);
    if(source_argument6){store(0x75,1,0);store(0x74,1,0);}
    // New-area callers 800157FC/800158A8 pass1; same-area 80015ACC passes0.
    // State3
    // mirrors include retained unknown lanes; copy validity rather than
    // inventing their values or rejecting a branch which does not read them.
    if(next.bytes[8]==3){
        const auto copy=[&](size_t destination,size_t source,size_t bytes)noexcept{
            for(size_t i=0;i<bytes;++i){next.bytes[destination+i]=next.bytes[source+i];next_known[destination+i]=next_known[source+i];}
        };
        copy(0x438,0x14,8);copy(0x428,0x1c,16);copy(0x440,0x54,8);
    }
    player=next;known_player=next_known;return true;
}
bool acknowledge_source_area_load_80015840(
    const SourceEventHandoffTransitionIntent& intent,
    const SourceAreaLoadCompletionReceipt& receipt,
    const void* current_native_owner,
    std::optional<int8_t>& transition_status_800c356c) noexcept {
    if(!current_native_owner||receipt.native_owner!=current_native_owner||
       !receipt.source_generation||receipt.source_generation!=intent.source_generation||
       receipt.destination_area!=intent.destination_area||
       receipt.source_callback!=0x80015840u||receipt.camera_initializer!=0x80038648u||
       !receipt.area_replaced||!receipt.spawn_applied||!receipt.gameplay_camera_initialized||
       intent.source_function!=0x80101274u||!intent.known[12]||intent.raw[12]!=2||
       !transition_status_800c356c||*transition_status_800c356c!=2)return false;
    transition_status_800c356c=0;
    return true;
}
SourceEventPhase6Result consume_source_event1_handoff_80101260(
    SourceEventWork& work,SourceEventPhase6Context& context,const SourceFadeContext& fade,
    const SourceEventHandoffInputs& in) noexcept {
    SourceEventPhase6Result out;out.next_consumer=0x80101260u;
    if(!known(work,0,2)||!known(work,4,1))return out;
    if(!work.active00()||work.event01()!=1||work.phase04()!=7){out.status=SourceEventPhase6Status::WrongEventPhase;return out;}
    if(!context.awaiting_caller_801011e8||context.scene_overlay_pending){out.status=SourceEventPhase6Status::AwaitingCallerConsumer;return out;}
    const auto& raw=fade.work_80098a68;
    if(!raw.known[10]||!raw.known[11]){out.status=SourceEventPhase6Status::NeedsCallerBusy;return out;}
    if(get16(raw.raw.data()+10)!=0){out.status=SourceEventPhase6Status::HandoffConditionFalse;out.next_consumer=0x80100614u;return out;}
    if(context.transition_pending){out.status=SourceEventPhase6Status::AwaitingTransitionLoader;return out;}
    if(!in.player||!in.player_known){out.status=SourceEventPhase6Status::NeedsHandoffPlayer;return out;}
    for(const size_t offset:{size_t(0),size_t(8)})if(!(*in.player_known)[offset]){out.status=SourceEventPhase6Status::NeedsActorField;out.missing_offset=uint16_t(offset);return out;}
    out.next_consumer=0x8002d004u;
    if(!in.display||!in.display->admitted||!in.display->native_owner){out.status=SourceEventPhase6Status::NeedsHandoffDisplayOwner;return out;}
    const auto& display=*in.display;
    if(!in.source_generation||display.source_generation!=in.source_generation||display.completed_function!=0x8002d004u||
       display.first_header!=0x800f7820u||display.second_header!=0x800f7830u||display.argument5||display.argument6){out.status=SourceEventPhase6Status::HandoffDisplayMismatch;return out;}
    if(!in.flags){out.status=SourceEventPhase6Status::NeedsHandoffFlags;return out;}
    out.next_consumer=0x8001d494u;
    if(!in.commands||!in.commands->initialized){out.status=SourceEventPhase6Status::NeedsHandoffCommands;return out;}
    const auto& commands=*in.commands;
    if(commands.count>=commands.records.size()||commands.current_index>=commands.records.size()||
       (commands.count&&commands.current_index>=commands.count)){out.status=SourceEventPhase6Status::HandoffCommandMalformed;return out;}
    auto queued=commands;
    // Exact source scan advances its row even after removal. Adjacent matching
    // records are consequently not coalesced by an invented modern filter.
    for(size_t i=0;i<queued.count;++i){
        const auto& row=queued.records[i];
        for(size_t k=0;k<4;++k)if(!row.known[k]){out.status=SourceEventPhase6Status::HandoffCommandMalformed;out.missing_offset=uint16_t(k);return out;}
        if(get32(row.raw.data())!=2)continue;
        for(size_t k=8;k<12;++k)if(!row.known[k]){out.status=SourceEventPhase6Status::HandoffCommandMalformed;out.missing_offset=uint16_t(k);return out;}
        if(get32(row.raw.data()+8)!=1)continue;
        for(size_t j=i+1;j<queued.count;++j){
            for(size_t k=0;k<16;++k)if(!queued.records[j].known[k]){out.status=SourceEventPhase6Status::HandoffCommandMalformed;out.missing_offset=uint16_t(k);return out;}
            queued.records[j-1]=queued.records[j];
        }
        --queued.count;
    }
    // The source has no bound. Native storage retains its type0 sentinel.
    if(queued.count+1>=queued.records.size()){out.status=SourceEventPhase6Status::HandoffCommandFull;return out;}
    auto& request=queued.records[queued.count++];
    for(size_t i=0;i<16;++i){request.raw[i]=0;request.known[i]=1;}
    request.raw[0]=2;request.raw[8]=1;
    // Source producer only appends; it does not rewrite its following sentinel.
    auto player=*in.player;auto player_known=*in.player_known;
    const auto player_store=[&](size_t offset,size_t count,uint32_t value)noexcept{for(size_t i=0;i<count;++i){player.bytes[offset+i]=uint8_t(value>>(8*i));player_known[offset+i]=1;}};
    player_store(0,1,3);player_store(6,1,3);
    for(const size_t offset:{size_t(0x449),size_t(0x111),size_t(0x71),size_t(0x73),size_t(0x168),size_t(0x169)})player_store(offset,1,0);
    for(size_t offset=0x188;offset<=0x194;offset+=4)player_store(offset,4,0);
    if(player.bytes[8]==3)player_store(9,1,3);else player_store(0xbd,1,8);
    SourceEventHandoffTransitionIntent transition;transition.source_generation=in.source_generation;
    constexpr auto& handoff=kSourceSt00OpeningHandoff;
    transition.spawn={{handoff.x,handoff.y,handoff.z}};
    const auto write=[&](size_t offset,size_t count,uint32_t value)noexcept{for(size_t i=0;i<count;++i){transition.raw[offset+i]=uint8_t(value>>(8*i));transition.known[offset+i]=1;}};
    write(0,2,uint16_t(handoff.x));write(2,2,uint16_t(handoff.y));write(4,2,uint16_t(handoff.z));write(10,2,handoff.heading);
    write(12,1,2);write(13,1,0);write(14,1,0);write(15,1,1);write(16,1,2);write(17,1,255);
    in.flags->values[1540>>3u]&=uint8_t(~(1u<<(1540&7u)));
    in.flags->known[1540>>3u]|=uint8_t(1u<<(1540&7u));
    *in.player=player;*in.player_known=player_known;*in.commands=queued;
    work.raw[0]=0;work.known[0]=1;context.transition_pending=transition;
    out.status=SourceEventPhase6Status::HandoffConsumed;out.committed=true;out.transition_produced=true;out.player_resumed=true;
    out.next_consumer=0x80100614u;return out;
}
bool acknowledge_source_event_scene_overlay_80100614(SourceEventPhase6Context& context,uint64_t serial) noexcept {
    if(!context.awaiting_caller_801011e8||!context.scene_overlay_pending||serial==0||
        context.scene_overlay_pending->lease.serial!=serial)return false;
    context.scene_overlay_pending.reset();context.awaiting_caller_801011e8=false;return true;
}
} // namespace l3
