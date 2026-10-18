#include "legends3ds/runtime_session.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

namespace {
using namespace l3;
using Status=SourceEventPhase6Status;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void put(uint8_t* p,uint32_t value){for(size_t i=0;i<4;++i)p[i]=uint8_t(value>>(i*8));}
std::array<uint8_t,32> digest(const char* text){std::array<uint8_t,32> out{};
    const auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
    for(size_t i=0;i<32;++i)out[i]=uint8_t(digit(text[i*2])*16+digit(text[i*2+1]));return out;}
SourceEventWork work(){SourceEventWork out;out.known.fill(1);out.raw[0]=out.raw[1]=1;out.raw[4]=6;
    put(out.raw.data()+0x10,1);put(out.raw.data()+0xa4,0x8011b2e8);return out;}
SourceEventFlagSnapshot flags(unsigned seed,unsigned event,bool value){std::array<uint8_t,kSourceEventFlagBytes> raw{};
    for(size_t i=0;i<144;++i)raw[i]=uint8_t(seed*29+i*17+11);
    const size_t id=896+event;const uint8_t mask=uint8_t(128u>>(id&7));
    if(value)raw[id/8]|=mask;else raw[id/8]&=uint8_t(~mask);return source_event_flags_from_original(raw);}
SourceEventPhase6Context state(){SourceEventPhase6Context out;out.awaiting_caller_801011e8=true;
    out.byte_800c1b6a=0;out.byte_8012242c=64;out.byte_80122430=128;out.byte_80122434=255;out.byte_80122438=2;return out;}
bool same_state(const SourceEventPhase6Context& a,const SourceEventPhase6Context& b){
    return a.awaiting_caller_801011e8==b.awaiting_caller_801011e8&&a.byte_800c1b6a==b.byte_800c1b6a&&
        a.input_800c0c2a==b.input_800c0c2a&&a.byte_8012242c==b.byte_8012242c&&a.byte_80122430==b.byte_80122430&&
        a.byte_80122434==b.byte_80122434&&a.byte_80122438==b.byte_80122438&&
        a.scene_overlay_pending.has_value()==b.scene_overlay_pending.has_value()&&(!a.scene_overlay_pending||
        (a.scene_overlay_pending->lease.serial==b.scene_overlay_pending->lease.serial&&
         a.scene_overlay_pending->r==b.scene_overlay_pending->r&&a.scene_overlay_pending->g==b.scene_overlay_pending->g&&
         a.scene_overlay_pending->b==b.scene_overlay_pending->b));
}
void differential(const std::string& folder){
    std::ifstream predicate(folder+"source-event-caller-predicate.tsv");require(bool(predicate),"predicate fixture missing");
    std::string line;std::getline(predicate,line);size_t count=0;
    while(std::getline(predicate,line)){std::istringstream row(line);unsigned seed,event,stage,flag,input,result,input_reads,steps;std::string extra;
        require(bool(row>>seed>>event>>stage>>flag>>input>>result>>input_reads>>steps)&&!(row>>extra),"predicate row rejected");
        auto current=work();current.raw[1]=uint8_t(event);const auto bank=flags(seed,event,flag!=0);const auto allocated=allocations;
        const auto observed=evaluate_source_event_completion_8001de84(current,&bank,static_cast<int8_t>(stage),uint16_t(input));
        require(observed.value&&unsigned(*observed.value)==result&&allocated==allocations,"unchanged predicate differential failed");
        const auto missing=evaluate_source_event_completion_8001de84(current,&bank,static_cast<int8_t>(stage),std::nullopt);
        require(input_reads==0?(missing.value&&!*missing.value):(!missing.value&&missing.status==Status::NeedsPredicateInput),
            "original short circuit required/fabricated controller input");++count;
    }
    require(count==192,"predicate fixture coverage changed");
    std::ifstream caller(folder+"source-event-caller-caller.tsv");require(bool(caller),"caller fixture missing");std::getline(caller,line);count=0;
    while(std::getline(caller,line)){std::istringstream row(line);unsigned seed,busy,stage,flag,input,forced,enabled,type10,steps;std::string extra;
        require(bool(row>>seed>>busy>>stage>>flag>>input>>forced>>enabled>>type10>>steps)&&!(row>>extra),"caller row rejected");
        auto current=work();current.raw[9]=forced?19:0;put(current.raw.data()+16,forced?44:seed);
        auto context=state();context.byte_800c1b6a=static_cast<int8_t>(stage);context.input_800c0c2a=uint16_t(input);
        SourceEventPhase5Context scene;scene.byte_80122428=uint8_t(enabled);const auto bank=flags(seed,1,flag!=0);
        const auto before=current;const auto previous=context;const auto allocated=allocations;
        const auto observed=consume_source_event1_caller_801011e8(current,scene,context,&bank,static_cast<int16_t>(busy),seed+1);
        require(current.raw==before.raw&&current.known==before.known&&allocations==allocated,"caller changed event phase/counter or allocated");
        if(type10)require(!observed.committed&&observed.status==Status::NeedsType10FadeConsumer&&observed.next_consumer==0x80017c30&&
            same_state(context,previous),"unproved type10 fade or phase advance acknowledged");
        else if(enabled){require(observed.committed&&observed.status==Status::AwaitingSceneOverlayConsumer&&context.scene_overlay_pending&&
                context.scene_overlay_pending->r==64&&context.scene_overlay_pending->g==128&&context.scene_overlay_pending->b==255&&
                context.awaiting_caller_801011e8,"source color tail not queued accurately");
            require(!acknowledge_source_event_scene_overlay_80100614(context,seed+2)&&context.scene_overlay_pending,"stale overlay serial accepted");
            require(acknowledge_source_event_scene_overlay_80100614(context,seed+1)&&!context.awaiting_caller_801011e8&&
                !context.scene_overlay_pending,"exact overlay receipt did not release caller");
        }else {require(observed.committed&&observed.status==Status::CallerConsumed&&!context.awaiting_caller_801011e8,
                "disabled source overlay required renderer/fabricated completion");}
        ++count;
    }
    require(count==64,"caller fixture coverage changed");
    std::ifstream color(folder+"source-event-caller-color.tsv");require(bool(color),"color fixture missing");std::getline(color,line);count=0;
    while(std::getline(color,line)){std::istringstream row(line);unsigned seed,r,g,b,mode,gpu,steps;uint32_t tag;std::string packet,extra;
        require(bool(row>>seed>>r>>g>>b>>mode>>gpu>>packet>>tag>>steps)&&!(row>>extra),"color row rejected");
        auto current=work();auto context=state();context.byte_8012242c=uint8_t(r);context.byte_80122430=uint8_t(g);
        context.byte_80122434=uint8_t(b);context.byte_80122438=uint8_t(mode);
        SourceEventPhase5Context scene;scene.byte_80122428=1;const auto before=context;
        const auto observed=consume_source_event1_caller_801011e8(current,scene,context,nullptr,1,seed+1);
        if(uint8_t(mode)==2){require(observed.committed&&context.scene_overlay_pending&&
            context.scene_overlay_pending->r==uint8_t(r)&&context.scene_overlay_pending->g==uint8_t(g)&&context.scene_overlay_pending->b==uint8_t(b)&&
            context.scene_overlay_pending->width==320&&context.scene_overlay_pending->height==240&&context.scene_overlay_pending->subtract_from_scene&&
            packet.substr(14,2)=="62"&&packet.substr(88,8)=="4f0000e1","source mode2 descriptor differs from original packet");
        }else {require(!observed.committed&&observed.status==Status::UnsupportedSceneOverlayMode&&same_state(context,before),
            "unported source color mode consumed");}
        ++count;
    }
    require(count==48,"color fixture coverage changed");std::cout<<"304 unchanged source differentials PASS; ";
}
void guards(){
    auto current=work();auto bank=flags(0,1,false);SourceEventPhase5Context scene;scene.byte_80122428=1;
    const Status expected[]={Status::NeedsEventWork,Status::WrongEventPhase,Status::NeedsCallerBusy,Status::NeedsPredicateStage,
        Status::NeedsStoryFlag,Status::NeedsPredicateInput,Status::NeedsSceneOverlayControl,Status::NeedsSceneOverlayControl,
        Status::UnsupportedSceneOverlayMode,Status::NeedsSceneOverlayLease};
    for(unsigned i=0;i<10;++i){auto event=current;auto context=state();auto owners=scene;auto flags_copy=bank;
        const auto* supplied=&flags_copy;std::optional<int16_t> busy=0;uint64_t serial=1;
        switch(i){case 0:event.known[4]=0;break;case 1:event.raw[4]=8;break;case 2:busy.reset();break;
            case 3:context.byte_800c1b6a.reset();break;case 4:supplied=nullptr;break;case 5:context.byte_800c1b6a=1;break;
            case 6:owners.byte_80122428.reset();break;case 7:context.byte_80122430.reset();break;
            case 8:context.byte_80122438=1;break;default:serial=0;break;}
        const auto previous=context;const auto observed=consume_source_event1_caller_801011e8(event,owners,context,supplied,busy,serial);
        require(!observed.committed&&observed.status==expected[i]&&same_state(previous,context),"caller missing owner was not atomic");
    }
    auto context=state();require(consume_source_event1_caller_801011e8(current,scene,context,&bank,0,9).committed,"pending overlay fixture failed");
    const auto allocated=allocations;const auto previous=context;
    for(unsigned i=0;i<4096;++i){const auto observed=consume_source_event1_caller_801011e8(current,scene,context,nullptr,std::nullopt,i+10);
        require(!observed.committed&&observed.status==Status::AwaitingSceneOverlayConsumer&&same_state(previous,context),"queued overlay repeated caller/predicate");}
    require(allocations==allocated,"overlay hold allocated per frame");std::cout<<"10 atomic guards/4096 allocation-free holds PASS; ";
}
EbdDirectory directory(){std::vector<uint8_t> bytes(232);for(size_t i=0;i<bytes.size();++i)bytes[i]=uint8_t(i*7+3);put(bytes.data(),14);
    for(size_t i=0;i<14;++i){auto* row=bytes.data()+4+i*16;row[0]=0xe0;row[1]=3;row[2]=uint8_t(i%4);row[3]=0;
        put(row+4,0x8016c000+228+uint32_t(i%4));put(row+8,0);put(row+12,0);}return parse_ebd_directory(bytes,0x8016c000);}
SourceEventPhase5Profile profile5(){SourceEventPhase5Profile out;out.initialized=true;out.directory_token=0x8011af4c;out.callback_table_token=0x8011b2e8;
    out.ebd_source_digest=digest("a5cc2dc41d7325d8e23aca0856bf6f486a81c19316e55bc37424bb49c3078207");
    for(size_t i=0;i<16;++i){auto& row=out.records[i];row.raw[0]=uint8_t(i);row.raw[1]=i?3:5;row.raw[3]=i?0xe0:0x40;
        if(i){row.resource_variant=uint8_t((i-1)%4);row.resource_directory_index=row.resource_variant;row.resource_token=0x8016c000+228+row.resource_variant;}}
    return out;}
SourceEventPhase6Profile profile6(){SourceEventPhase6Profile out;out.initialized=true;out.camera_program_token=0x8011af4c;out.callback_table_token=0x8011b2e8;
    out.camera_words.fill(0xffffffff);out.timeline[0]={0xffff0000,0x80100738};out.timeline.back()={255,0};return out;}
RuntimeSession session(){RuntimeMap map;RuntimeMapArea area;area.start_x=area.start_z=63;area.width=area.height=2;area.cell_count=4;
    map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));TutorialRuntimeArea active;active.ebd=directory();
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);RuntimeSession out(std::move(map),std::move(active),{},settings);
    SourceEventPollProfile poll;poll.decoder=1;out.bind_source_event_profile(poll,0);out.bind_source_event_phase5_profile(profile5());
    require(out.bind_source_event_phase6_profile(profile6()),"authored phase6 profile rejected");
    require(out.initialize_fresh_source_event_subset_st00(),"authored fresh source lifecycle rejected");out.bind_source_event_phase5_resources();
    require(out.source_event_phase5_resources().admitted(),"authored active EBD witness rejected");
    // This test explicitly authors source-state snapshots on a non-const test
    // instance. It is not an observed retail NewGame/scene chronology.
    const_cast<std::optional<SourceEventWork>&>(out.source_event_work())=work();
    auto& scene=const_cast<SourceEventPhase5Context&>(out.source_event_phase5_context());
    scene.byte_80122428=1;put(scene.camera_800bc738.raw.data()+184,0x8011af4c);
    auto& context=const_cast<SourceEventPhase6Context&>(out.source_event_phase6_context());
    context.byte_8012242c=context.byte_80122430=context.byte_80122434=64;context.byte_80122438=2;
    out.tick({});require(out.source_event_scene_overlay().has_value(),"runtime failed to queue original color tail");return out;
}
void leases(){
    auto original=session();const auto first=*original.source_event_scene_overlay();
    require(first.lease.native_owner==&original&&first.lease.serial!=0,"presentation native lease not bound");
    const auto counter=original.source_event_work()->raw[0x10];original.tick({});
    require(original.source_event_work()->raw[0x10]==counter&&original.source_event_scene_overlay()->lease.serial==first.lease.serial,
        "missing renderer receipt reran original scene update");
    auto copy=original;const auto copied=*copy.source_event_scene_overlay();
    require(!copy.acknowledge_source_event_scene_overlay(first.lease)&&!original.acknowledge_source_event_scene_overlay(copied.lease),
        "copied session accepted another owner's renderer receipt");
    require(copy.acknowledge_source_event_scene_overlay(copied.lease)&&original.source_event_scene_overlay(),"copy receipt altered original");
    const RuntimeSession* alias=&original;original=*alias;
    require(original.acknowledge_source_event_scene_overlay(first.lease)&&!original.source_event_scene_overlay(),"self-copy lost valid receipt");
    require(!original.acknowledge_source_event_scene_overlay(first.lease),"duplicate presentation receipt accepted");
    original.tick({});const auto next=*original.source_event_scene_overlay();
    require(next.lease.serial>first.lease.serial&&original.source_event_work()->raw[0x10]==uint8_t(counter+1),"renderer receipt did not resume next source update");
    for(unsigned i=0;i<40;++i){const auto present=*original.source_event_scene_overlay();require(original.acknowledge_source_event_scene_overlay(present.lease),"repeat scene overlay failed");original.tick({});}
    require(original.source_event_work()->phase04()==6,"repeated source updates fabricated phase completion");
    auto replacement=session();const auto stale=original.source_event_scene_overlay()->lease;original=replacement;
    require(!original.acknowledge_source_event_scene_overlay(stale),"same-address assignment reused presentation serial");
    const auto assigned=original.source_event_scene_overlay()->lease;require(original.acknowledge_source_event_scene_overlay(assigned),"assignment lost current owner lease");
    auto same_request=session();const auto same_request_old=same_request.source_event_scene_overlay()->lease;
    auto snapshot=same_request;same_request=snapshot;
    require(same_request.source_event_scene_overlay()->lease.serial==same_request_old.serial&&
        !same_request.acknowledge_source_event_scene_overlay(same_request_old),"same-request copy assignment caused renderer receipt ABA");
    auto reset=session();const auto reset_lease=reset.source_event_scene_overlay()->lease;reset.bind_source_event_phase5_profile(profile5());
    require(!reset.source_event_scene_overlay()&&!reset.acknowledge_source_event_scene_overlay(reset_lease),"profile reset retained outgoing overlay");
    auto state_reset=session();const auto state_lease=state_reset.source_event_scene_overlay()->lease;
    state_reset.bind_source_event_state(work(),initialize_source_story_flags_8001d8c0(),80,0,0);
    require(!state_reset.source_event_scene_overlay()&&!state_reset.acknowledge_source_event_scene_overlay(state_lease),"captured-state reset retained outgoing overlay");
    auto save=session();const auto save_lease=save.source_event_scene_overlay()->lease;save.apply_save_state(save.make_save_state(1));
    require(!save.source_event_scene_overlay()&&!save.acknowledge_source_event_scene_overlay(save_lease),"accepted native save retained outgoing overlay");
    auto transition=session();const auto transition_lease=transition.source_event_scene_overlay()->lease;
    require(!transition.transition_to_location(999,{},0)&&transition.source_event_scene_overlay(),"failed transition discarded pending overlay");
    require(transition.transition_to_location(0,{},0)&&!transition.source_event_scene_overlay()&&
        !transition.acknowledge_source_event_scene_overlay(transition_lease),"accepted area transition retained outgoing overlay");
    auto changed=session();const auto changed_lease=changed.source_event_scene_overlay()->lease;
    const_cast<TutorialRuntimeArea&>(changed.active_area()).ebd.entries[0].variant^=1;
    require(!changed.source_event_scene_overlay()&&!changed.acknowledge_source_event_scene_overlay(changed_lease),"mutated active EBD admitted overlay receipt");
    alignas(RuntimeSession) unsigned char storage[sizeof(RuntimeSession)];auto* value=new(storage)RuntimeSession(session());
    const auto old=value->source_event_scene_overlay()->lease;value->~RuntimeSession();value=new(storage)RuntimeSession(session());
    require(!value->acknowledge_source_event_scene_overlay(old)&&value->source_event_scene_overlay()->lease.serial>old.serial,
        "same-address reconstruction caused renderer receipt ABA");value->~RuntimeSession();
    std::cout<<"40 repeated scene updates/renderer receipt lifetime and ABA guards PASS\n";
}
}
int main(int argc,char** argv){try{require(argc==2,"usage: test_source_event_caller source-event-caller-predicate.tsv");
    const std::string path(argv[1]);const auto slash=path.find_last_of("/\\");const auto folder=slash==std::string::npos?std::string{}:path.substr(0,slash+1);
    differential(folder);guards();leases();return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
