#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <algorithm>
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
using Status=l3::SourceEventPhase6Status;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void put(uint8_t* p,uint32_t value){for(size_t i=0;i<4;++i)p[i]=uint8_t(value>>(8*i));}
unsigned digit(char c){if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);throw std::runtime_error("fixture hex rejected");}
template<size_t N>std::array<uint8_t,N> hex(const std::string& text){
    require(text.size()==N*2,"fixture coverage rejected");std::array<uint8_t,N> result{};
    for(size_t i=0;i<N;++i)result[i]=uint8_t(digit(text[2*i])*16+digit(text[2*i+1]));return result;
}
template<size_t N>std::array<uint8_t,N> authored(unsigned seed){std::array<uint8_t,N> result{};
    for(size_t i=0;i<N;++i)result[i]=uint8_t(seed*37+i*17+N*11);return result;
}
l3::EbdDirectory owned_directory(){
    std::vector<uint8_t> bytes(232);for(size_t i=0;i<bytes.size();++i)bytes[i]=uint8_t(i*7+3);put(bytes.data(),14);
    for(size_t i=0;i<14;++i){auto* row=bytes.data()+4+i*16;row[0]=0xe0;row[1]=3;row[2]=uint8_t(i%4);row[3]=0;
        put(row+4,0x8016c000u+228u+uint32_t(i%4));put(row+8,0);put(row+12,0);}
    return l3::parse_ebd_directory(bytes,0x8016c000u);
}
l3::SourceEventPhase5Profile profile(){l3::SourceEventPhase5Profile out;out.initialized=true;
    out.directory_token=0x8011af4c;out.callback_table_token=0x8011b2e8;
    out.ebd_source_digest=hex<32>("a5cc2dc41d7325d8e23aca0856bf6f486a81c19316e55bc37424bb49c3078207");
    for(size_t i=0;i<16;++i){auto& record=out.records[i];record.raw[0]=uint8_t(i);record.raw[1]=i?3:5;record.raw[3]=i?0xe0:0x40;
        if(i){record.resource_variant=uint8_t((i-1)%4);record.resource_directory_index=record.resource_variant;
            record.resource_token=0x8016c000u+228u+record.resource_variant;}}
    return out;
}
l3::SourceEventWork work(unsigned seed,uint32_t frame,bool enabled=true){l3::SourceEventWork out;
    out.raw=authored<168>(seed);out.known.fill(1);out.raw[0]=1;out.raw[1]=1;out.raw[4]=6;out.raw[8]=enabled?0:255;
    put(out.raw.data()+0x10,frame);put(out.raw.data()+0xa4,0x8011b2e8);
    put(out.raw.data()+0x24,0x800c3580u+uint32_t(seed%4)*1444u);put(out.raw.data()+0x30,0x800bf118u+uint32_t(seed%32)*80u);return out;
}
l3::SourceEventPhase5Context scene(unsigned seed){l3::SourceEventPhase5Context out;
    l3::initialize_source_event_phase5_pools_8002e88c(out);
    auto& a40=out.pool40.slots[seed%4];a40.raw=authored<1444>(seed);a40.raw[1]=uint8_t(0x40|(seed%4));
    auto& ae0=out.pool_e0.slots[seed%32];ae0.raw=authored<80>(seed);ae0.raw[1]=uint8_t(0xe0|(seed%32));
    out.pool40.used[0]=0x80000000u>>(seed%4);out.pool_e0.used[0]=0x80000000u>>(seed%32);
    out.camera_800bc738.raw=authored<188>(seed);out.camera_800bc738.known.fill(1);
    out.byte_80122428=authored<21>(seed)[4];return out;
}
l3::SourceEventPhase6Context context(unsigned seed,uint8_t control,uint16_t input){const auto raw=authored<21>(seed);
    l3::SourceEventPhase6Context out;out.byte_80122424=control;out.scratch_half_1f800002=input;
    out.byte_8012242c=raw[8];out.byte_80122430=raw[12];out.byte_80122434=raw[16];out.byte_80122438=raw[20];return out;
}
template<size_t Count,size_t Bytes>bool same_pool(const l3::SourceEventPhase5Pool<Count,Bytes>& a,
    const l3::SourceEventPhase5Pool<Count,Bytes>& b){if(a.used!=b.used)return false;
    for(size_t i=0;i<Count;++i)if(a.slots[i].raw!=b.slots[i].raw||a.slots[i].known!=b.slots[i].known)return false;return true;}
bool same_scene(const l3::SourceEventPhase5Context& a,const l3::SourceEventPhase5Context& b){
    return same_pool(a.pool40,b.pool40)&&same_pool(a.pool_e0,b.pool_e0)&&same_pool(a.pool80,b.pool80)&&same_pool(a.pool_c0,b.pool_c0)&&a.word_80098818==b.word_80098818&&
        a.camera_800bc738.raw==b.camera_800bc738.raw&&a.camera_800bc738.known==b.camera_800bc738.known&&a.byte_80122428==b.byte_80122428;
}
bool same_context(const l3::SourceEventPhase6Context& a,const l3::SourceEventPhase6Context& b){
    return a.byte_80122424==b.byte_80122424&&a.byte_8012242c==b.byte_8012242c&&a.byte_80122430==b.byte_80122430&&
        a.byte_80122434==b.byte_80122434&&a.byte_80122438==b.byte_80122438&&a.scratch_half_1f800002==b.scratch_half_1f800002&&
        a.awaiting_camera_8001ec0c==b.awaiting_camera_8001ec0c&&a.awaiting_timeline_8001f070==b.awaiting_timeline_8001f070&&
        a.awaiting_caller_801011e8==b.awaiting_caller_801011e8&&a.byte_800bc733==b.byte_800bc733&&a.byte_1f800074==b.byte_1f800074&&
        a.consumed_callback==b.consumed_callback&&a.half_1f800006==b.half_1f800006&&a.half_1f800076==b.half_1f800076&&a.projection.ofx==b.projection.ofx&&
        a.projection.ofy==b.projection.ofy&&a.projection.h==b.projection.h&&a.projection.dqa==b.projection.dqa&&a.projection.dqb==b.projection.dqb;
}
uint32_t value(unsigned seed,unsigned i){return uint32_t(seed*0x1020304u+i*0x11bb77eeu);}
l3::SourceEventPhase6Profile scene_profile(unsigned seed){l3::SourceEventPhase6Profile out;out.initialized=true;
    out.camera_program_token=0x8011af4c;out.callback_table_token=0x8011b2e8;out.camera_words.fill(0xffffffff);
    size_t index=0;for(uint32_t operation=3;operation<=6;++operation)out.camera_words[index++]=operation<<24;
    for(uint32_t operation=16;operation<=22;++operation){out.camera_words[index++]=(operation<<24)|(3u<<16);
        for(unsigned i=0;i<3;++i)out.camera_words[index++]=value(seed,operation*3+i);}
    out.camera_words[index++]=(32u+seed%3)<<24;
    const unsigned operation=seed%3;
    out.camera_words[index]=operation==0?seed+1:((operation<<24)|((seed%2==0?1u:2u)<<16)|seed);
    for(size_t i=0;i<18;++i){out.timeline[i].header=uint32_t((seed+i)%4)|uint32_t(((seed+i)%3)<<8)|(1u<<16);
        out.timeline[i].callback=i==0?0x80100738u:0;}
    constexpr uint16_t deadline[]={0,1,32767,32768,65534,65535};
    out.timeline[seed%16].header=(out.timeline[seed%16].header&65535u)|(uint32_t(deadline[seed%6])<<16);
    out.timeline.back()={255,0};return out;
}
l3::SourceEventWork camera_work(unsigned seed){l3::SourceEventWork out;out.raw=authored<168>(seed);out.known.fill(1);
    out.raw[0]=1;out.raw[1]=1;out.raw[4]=6;out.raw[8]=out.raw[9]=1;
    for(size_t offset=0x14;offset<0x20;offset+=4)put(out.raw.data()+offset,seed);
    put(out.raw.data()+0xa4,0x8011b2e8u+uint32_t(seed%16)*8);return out;}
void camera_timeline(const char* path,const l3::SourceEventPhase5ResourceBinding& witness){
    const std::string source(path);const size_t separator=source.find_last_of("/\\");
    const std::string folder=separator==std::string::npos?std::string{}:source.substr(0,separator+1);
    std::ifstream file(folder+"source-event-camera.tsv");require(bool(file),"missing source camera fixture");std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){std::istringstream row(line);unsigned seed,distance,byte733,shift,half006,half076;std::string camera,extra;
        std::array<uint32_t,5> controls{};
        require(bool(row>>seed>>distance>>camera>>byte733>>shift>>half006>>half076>>controls[0]>>controls[1]>>controls[2]>>controls[3]>>controls[4])&&!(row>>extra),"camera row rejected");
        auto owners=scene(seed);put(owners.camera_800bc738.raw.data()+184,0x8011af4c);auto state=context(seed,0,0);
        state.awaiting_camera_8001ec0c=true;state.byte_800bc733=uint8_t(seed);state.byte_1f800074=uint8_t(seed);
        state.half_1f800006=uint16_t(seed+100);state.half_1f800076=uint16_t(seed+200);
        state.projection={value(seed,0),value(seed,1),value(seed,2),value(seed,3),value(seed,4)};
        auto p=scene_profile(seed);p.field_800bc81c=uint16_t(distance);const auto event=camera_work(seed);const auto allocated=allocations;
        const auto result=l3::consume_source_event1_camera_8001ec0c(event,owners,state,&p);
        require(result.committed&&result.status==Status::CameraConsumed&&result.next_consumer==0x8001f070&&allocations==allocated&&
            owners.camera_800bc738.raw==hex<188>(camera)&&state.byte_800bc733==byte733&&state.byte_1f800074==shift&&
            state.half_1f800006==half006&&state.half_1f800076==half076&&state.projection.ofx==controls[0]&&state.projection.ofy==controls[1]&&
            state.projection.h==controls[2]&&state.projection.dqa==controls[3]&&state.projection.dqb==controls[4],"unchanged camera interpreter differential failed");++count;
    }
    require(count==64,"camera fixture case count changed");
    std::ifstream timeline_file(folder+"source-event-timeline.tsv");require(bool(timeline_file),"missing source timeline fixture");std::getline(timeline_file,line);count=0;
    while(std::getline(timeline_file,line)){std::istringstream row(line);unsigned seed;uint32_t before;std::string expected,extra;
        require(bool(row>>seed>>before>>expected)&&!(row>>extra),"timeline row rejected");auto event=camera_work(seed);put(event.raw.data()+0x10,before);
        auto state=context(seed,0,0);state.awaiting_timeline_8001f070=true;const auto p=scene_profile(seed);const auto allocated=allocations;
        const auto result=l3::consume_source_event1_timeline_8001f070(event,state,&p);
        require(result.committed&&result.status==Status::TimelineConsumed&&result.next_consumer==0x801011e8&&state.awaiting_caller_801011e8&&
            event.raw==hex<168>(expected)&&allocations==allocated,"unchanged timeline counter/handoff differential failed");++count;
    }
    require(count==64,"timeline fixture case count changed");
    for(unsigned i=0;i<7;++i){auto owners=scene(0);put(owners.camera_800bc738.raw.data()+184,0x8011af4c);
        auto state=context(0,0,0);state.awaiting_camera_8001ec0c=true;auto event=camera_work(0);auto p=scene_profile(0);const auto* supplied=&p;
        switch(i){case 0:supplied=nullptr;break;case 1:p.initialized=false;break;case 2:put(owners.camera_800bc738.raw.data()+184,0x8011af4d);break;
            case 3:owners.camera_800bc738.known[184]=0;break;case 4:event.known[0x14]=0;break;case 5:p.camera_words[0]=0x07000000;break;
            default:owners.camera_800bc738.known[104]=0;p.camera_words.fill(0xffffffff);break;}
        const auto before=owners;const auto before_context=state;const auto result=l3::consume_source_event1_camera_8001ec0c(event,owners,state,supplied);
        require(!result.committed&&same_scene(owners,before)&&same_context(state,before_context),"camera malformed/missing owner failed atomic rollback");
    }
    auto owners=scene(0);put(owners.camera_800bc738.raw.data()+184,0x8011af4c);
    auto state=context(0,0,0);const auto p=scene_profile(0);auto event=work(0,0);
    std::optional<l3::SourceEventWork> current=event;l3::SourceEventInputs inputs;inputs.phase5_context=&owners;inputs.phase6_context=&state;
    inputs.phase5_resources=&witness;inputs.phase6_profile=&p;inputs.busy_80098a72=0;
    auto result=l3::step_source_event_pipeline(current,nullptr,inputs);
    require(result.committed&&current->phase04()==6&&current->raw[0x10]==1&&state.awaiting_caller_801011e8&&
        result.request&&result.request->source_function==0x8001de84,"callback/camera/timeline chain not wired or completion fabricated");
    const auto complete=owners;const auto completed_context=state;const auto completed_event=current->raw;
    // An expired/replaced active owner cannot resume any continuation.
    inputs.phase5_resources=nullptr;result=l3::step_source_event_pipeline(current,nullptr,inputs);
    require(!result.committed&&result.phase6_consumer_status==Status::NeedsResourceOwnership&&same_scene(complete,owners)&&
        same_context(state,completed_context)&&current->raw==completed_event,"resource replacement changed pending scene owners");
    l3::SourceEventPhase5ResourceBinding expired;
    {const auto temporary=owned_directory();expired=l3::bind_source_event_phase5_resources(profile(),temporary);require(expired.admitted(),"temporary fixture failed");}
    inputs.phase5_resources=&expired;result=l3::step_source_event_pipeline(current,nullptr,inputs);
    require(!result.committed&&result.phase6_consumer_status==Status::NeedsResourceOwnership&&same_scene(complete,owners)&&
        same_context(state,completed_context)&&current->raw==completed_event,"expired resource resumed scene continuation");
    l3::initialize_source_event_scene_from_executable_boot_80068000(owners);
    for(size_t i=0;i<188;++i)require(owners.camera_800bc738.raw[i]==0&&owners.camera_800bc738.known[i],"explicit cold scene clear missed retained owner");
    std::cout<<"64 unchanged camera/64 timeline cases; 7 atomic camera guards; bounded pipeline/cold boot/expired owner PASS; ";
}
void packages(){auto p=scene_profile(0);p.field_800bc81c=1024;
    std::vector<uint8_t> bytes(1180);bytes[0]='L';bytes[1]='3';bytes[2]='P';bytes[3]='6';bytes[4]=1;bytes[6]=96;
    put(bytes.data()+8,1);put(bytes.data()+16,231);put(bytes.data()+20,18);put(bytes.data()+24,1180);
    const auto exe=hex<32>("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    const auto overlay=hex<32>("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    std::copy(exe.begin(),exe.end(),bytes.begin()+32);std::copy(overlay.begin(),overlay.end(),bytes.begin()+64);
    put(bytes.data()+96,p.camera_program_token);put(bytes.data()+100,p.callback_table_token);put(bytes.data()+104,p.field_800bc81c);
    for(size_t i=0;i<231;++i)put(bytes.data()+112+i*4,p.camera_words[i]);
    for(size_t i=0;i<18;++i){put(bytes.data()+1036+i*8,p.timeline[i].header);put(bytes.data()+1040+i*8,p.timeline[i].callback);}
    const auto repair=[](std::vector<uint8_t>& b){put(b.data()+28,l3::crc32(b.data()+96,b.size()-96));};repair(bytes);
    const auto loaded=l3::load_source_event_phase6_profile(bytes);
    require(loaded.initialized&&loaded.field_800bc81c==1024&&loaded.camera_words==p.camera_words&&loaded.timeline.back().header==255,
        "neutral scene profile roundtrip rejected");
    for(unsigned i=0;i<14;++i){auto bad=bytes;
        switch(i){case 0:bad.resize(1172);break;case 1:bad[0]^=1;break;case 2:bad[4]=2;break;case 3:bad[6]=64;break;
            case 4:put(bad.data()+16,232);break;case 5:put(bad.data()+20,17);break;case 6:bad[32]^=1;break;case 7:bad[64]^=1;break;
            case 8:bad.back()^=1;break;case 9:put(bad.data()+104,65536);repair(bad);break;case 10:put(bad.data()+108,1);repair(bad);break;
            case 11:put(bad.data()+96,0x8011af50);repair(bad);break;case 12:put(bad.data()+1032,0);repair(bad);break;
            default:put(bad.data()+1172,0);repair(bad);break;}
        bool rejected=false;try{(void)l3::load_source_event_phase6_profile(bad);}catch(const std::runtime_error&){rejected=true;}
        require(rejected,"malformed or incomplete scene profile admitted");
    }
    std::cout<<"14 malformed L3P6 packages rejected; ";
}
void differential(const char* path,const l3::SourceEventPhase5ResourceBinding& witness){
    std::ifstream file(path);require(bool(file),"missing phase6 fixture");std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){std::istringstream row(line);unsigned seed,control,input,selector;uint32_t frame;
        std::string a40,ae0,camera,controls,extra;
        require(bool(row>>seed>>frame>>control>>input>>selector>>a40>>ae0>>camera>>controls)&&!(row>>extra),"fixture row rejected");
        auto owners=scene(seed);auto event=work(seed,frame,selector!=255);event.raw[8]=uint8_t(selector);
        auto state=context(seed,uint8_t(control),uint16_t(input));
        const auto event_before=event;const auto original_owners=owners;const auto allocated=allocations;
        const auto result=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness);
        require(result.committed&&result.status==Status::CallbackConsumed&&result.callback_executed==(selector!=255)&&
            result.next_consumer==0x8001ec0c&&event.raw==event_before.raw&&event.known==event_before.known&&allocations==allocated,
            "prefix held/advanced event/allocated in update");
        require(owners.pool40.slots[seed%4].raw==hex<1444>(a40)&&owners.pool_e0.slots[seed%32].raw==hex<80>(ae0)&&
            owners.camera_800bc738.raw==hex<188>(camera),"unchanged callback/camera byte differential failed");
        const auto raw=hex<21>(controls);
        require(state.byte_80122424==raw[0]&&owners.byte_80122428==raw[4]&&state.byte_8012242c==raw[8]&&
            state.byte_80122430==raw[12]&&state.byte_80122434==raw[16]&&state.byte_80122438==raw[20],"source control byte differential failed");
        for(size_t i=0;i<4;++i)if(i!=seed%4)require(owners.pool40.slots[i].raw==original_owners.pool40.slots[i].raw,"unrelated player slot changed");
        for(size_t i=0;i<32;++i)if(i!=seed%32)require(owners.pool_e0.slots[i].raw==original_owners.pool_e0.slots[i].raw,"unrelated E0 slot changed");
        const auto completed=owners;const auto next=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness);
        require(!next.committed&&next.status==Status::AwaitingCameraConsumer&&same_scene(completed,owners),"pending downstream owner repeated callback");++count;
    }
    require(count==54,"phase6 differential case count changed");std::cout<<count<<" unchanged callback/prefix differential cases PASS; ";
}

void patch(std::vector<uint8_t>& bytes,const std::string& changes){
    if(changes=="-")return;
    std::istringstream data(changes);std::string field;
    while(std::getline(data,field,',')){const auto colon=field.find(':');require(colon!=std::string::npos,"sparse oracle colon absent");
        const size_t index=std::stoul(field.substr(0,colon));const unsigned value=unsigned(std::stoul(field.substr(colon+1)));
        require(index<bytes.size()&&value<=255,"sparse oracle bound rejected");bytes[index]=uint8_t(value);}
}
template<size_t Count,size_t Bytes>void initialize_pool(l3::SourceEventPhase5Pool<Count,Bytes>& pool,unsigned seed,uint8_t family){
    const auto data=authored<Count*Bytes>(seed);
    for(size_t i=0;i<Count;++i){auto& slot=pool.slots[i];std::copy_n(data.data()+i*Bytes,Bytes,slot.raw.data());
        slot.raw[1]=uint8_t(family|i);slot.known.fill(1);}
}
template<size_t Count,size_t Bytes>std::vector<uint8_t> flatten(const l3::SourceEventPhase5Pool<Count,Bytes>& pool){
    std::vector<uint8_t> bytes;bytes.reserve(Count*Bytes);
    for(const auto& slot:pool.slots)bytes.insert(bytes.end(),slot.raw.begin(),slot.raw.end());return bytes;
}
l3::SourceEventPhase5Context callback_scene(unsigned seed,uint32_t used80,uint32_t used_e0,uint32_t used_c0=0){l3::SourceEventPhase5Context out;
    initialize_pool(out.pool40,seed,0x40);initialize_pool(out.pool80,seed,0x80);initialize_pool(out.pool_e0,seed,0xe0);initialize_pool(out.pool_c0,seed,0xc0);out.pool_c0.slots[0].raw[3]=uint8_t(seed+3);
    out.pool40.used[0]=0x80000000u;out.pool80.used[0]=used80;out.pool80.used[1]=0;out.pool_e0.used[0]=used_e0;out.pool_c0.used[0]=used_c0;
    out.camera_800bc738.raw=authored<188>(seed);out.camera_800bc738.known.fill(1);out.byte_80122428=authored<21>(seed)[4];return out;
}
l3::SourceEventWork callback_work(unsigned seed,uint32_t frame){auto event=work(seed,frame);put(event.raw.data()+0xa4,0x8011b2f0);
    put(event.raw.data()+0x24,0x800c3580);
    for(size_t i=1;i<16;++i)put(event.raw.data()+0x24+i*4,0x800bf118u+uint32_t(i-1)*80u);return event;
}
void callback_differential(const char* inherited,const l3::SourceEventPhase5ResourceBinding& witness){
    const std::string source(inherited);const auto separator=source.find_last_of("/\\");
    const auto folder=separator==std::string::npos?std::string{}:source.substr(0,separator+1);
    std::ifstream file(folder+"source-event-callbacks-01431.tsv");require(bool(file),"callback oracle fixture absent");std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){std::istringstream row(line);unsigned seed,control,input,global;uint32_t callback,frame,used80,used_e0,used_c0,out80low,out80high,out_e0,out_c0;
        std::string diff40,diff80,diff_e0,diff_c0,diff_camera,controls,extra;
        require(bool(row>>seed>>callback>>frame>>control>>input>>used80>>used_e0>>used_c0>>diff40>>diff80>>diff_e0>>diff_c0>>diff_camera>>controls>>global>>out80low>>out80high>>out_e0>>out_c0)&&
            !(row>>extra),"callback oracle row rejected");
        auto owners=callback_scene(seed,used80,used_e0,used_c0);auto event=callback_work(seed,frame);event.raw[8]=uint8_t(control);
        auto state=context(seed,uint8_t(control),uint16_t(input));
        auto p=scene_profile(seed);p.timeline[1].callback=callback;
        auto expected40=flatten(owners.pool40),expected80=flatten(owners.pool80),expected_e0=flatten(owners.pool_e0),expected_c0=flatten(owners.pool_c0);
        std::vector<uint8_t> expected_camera(owners.camera_800bc738.raw.begin(),owners.camera_800bc738.raw.end());
        patch(expected40,diff40);patch(expected80,diff80);patch(expected_e0,diff_e0);patch(expected_c0,diff_c0);patch(expected_camera,diff_camera);
        const auto expected_controls=hex<21>(controls);const auto event_before=event;const auto allocated=allocations;
        const auto result=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness,&p);
        require(result.committed&&result.callback_executed&&result.callback==callback&&state.consumed_callback==callback&&
            result.status==Status::CallbackConsumed&&allocations==allocated&&event.raw==event_before.raw&&event.known==event_before.known,
            "data-selected callback did not commit exactly once without allocation");
        require(flatten(owners.pool40)==expected40&&flatten(owners.pool80)==expected80&&flatten(owners.pool_e0)==expected_e0&&flatten(owners.pool_c0)==expected_c0&&
            std::equal(expected_camera.begin(),expected_camera.end(),owners.camera_800bc738.raw.begin())&&
            owners.pool80.used[0]==out80low&&owners.pool80.used[1]==out80high&&owners.pool_e0.used[0]==out_e0&&owners.pool_c0.used[0]==out_c0,
            "unchanged selected callback/pool differential failed");
        require(state.byte_80122424==expected_controls[0]&&owners.byte_80122428==expected_controls[4]&&state.byte_8012242c==expected_controls[8]&&
            state.byte_80122430==expected_controls[12]&&state.byte_80122434==expected_controls[16]&&state.byte_80122438==expected_controls[20]&&
            owners.pool_c0.slots[0].raw[3]==global,"selected callback control differential failed");
        const auto completed=owners;const auto completed_context=state;const auto allocated_again=allocations;
        const auto hold=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness,&p);
        require(!hold.committed&&hold.callback==callback&&hold.status==Status::AwaitingCameraConsumer&&allocations==allocated_again&&
            same_scene(completed,owners)&&same_context(completed_context,state),"callback repeated after downstream continuation hold");++count;
    }
    require(count==156,"selected callback oracle count changed");
    for(unsigned i=0;i<11;++i){auto owners=callback_scene(0,0x80000000u,0xfffe0000u);auto event=callback_work(0,0);
        auto state=context(0,0,0);auto p=scene_profile(0);p.timeline[1].callback=0x801007f0;
        const auto* supplied=&p;Status expected=Status::NeedsActorBinding;
        switch(i){case 0:put(event.raw.data()+0xa4,0x8011b2f1);expected=Status::UnsupportedCallbackTable;break;
            case 1:p.timeline[1].callback=0x801007f4;expected=Status::UnsupportedCallback;break;
            case 2:event.known[0x30]=0;expected=Status::NeedsEventWork;break;
            case 3:owners.pool40.used[0]=0;break;
            case 4:p.timeline[1].callback=0x80100a3c;put(event.raw.data()+0x10,50);owners.pool40.slots[0].known[0x20]=0;expected=Status::NeedsActorField;break;
            case 5:p.timeline[1].callback=0x80100920;put(event.raw.data()+0x10,96);owners.pool80.used[0].reset();expected=Status::NeedsPoolBitmap;break;
            case 6:p.timeline[1].callback=0x80100920;put(event.raw.data()+0x10,96);owners.pool80.slots[1].known[1]=0;expected=Status::NeedsPoolHeader;break;
            case 7:p.timeline[1].callback=0x80100920;put(event.raw.data()+0x10,96);owners.pool80.used[0]=owners.pool80.used[1]=0xffffffffu;expected=Status::PoolCapacityExceeded;break;
            case 8:p.timeline[1].callback=0x80100a94;owners.pool_c0.used[0]=0xfffc0000u;expected=Status::PoolCapacityExceeded;break;
            case 9:p.timeline[1].callback=0x80100d6c;expected=Status::NeedsRandomOwner;break;
            default:supplied=nullptr;expected=Status::UnsupportedCallbackTable;break;}
        const auto before=owners;const auto before_context=state;const auto allocated=allocations;
        const auto result=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness,supplied);
        require(!result.committed&&result.status==expected&&same_scene(before,owners)&&same_context(before_context,state)&&allocations==allocated,
            "selected callback malformed/owner/capacity/RNG hold was not atomic");
    }
    std::cout<<count<<" selected callback oracle cases/11 atomic guards PASS; ";
}

void source_main_loop_owners(const char* inherited){
    const std::string source(inherited);const auto separator=source.find_last_of("/\\");
    const auto folder=separator==std::string::npos?std::string{}:source.substr(0,separator+1);
    std::ifstream file(folder+"source-main-loop-tail-01431.tsv");require(bool(file),"counter tail fixture absent");
    std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){std::istringstream row(line);unsigned before,after;std::string extra;
        require(bool(row>>before>>after)&&!(row>>extra)&&before<=65535&&after<=65535,"counter tail row rejected");
        std::optional<uint16_t> half=uint16_t(before);const auto allocated=allocations;
        require(l3::consume_source_main_loop_tail_80011ecc(half)&&half==after&&allocations==allocated,"unchanged main-loop tail differential failed");++count;}
    require(count==13,"counter tail fixture count changed");
    std::optional<uint16_t> unknown;const auto allocated=allocations;
    for(unsigned i=0;i<4096;++i)require(!l3::consume_source_main_loop_tail_80011ecc(unknown)&&!unknown,"missing scratch owner invented an initial value");
    require(allocations==allocated,"scratch hold allocated");
    auto owners=callback_scene(91,0x12345678u,0x98765432u,0xabcdef01u);owners.word_80098818=0xabcdef02;
    l3::initialize_source_event_phase5_pools_8002e88c(owners);
    require(owners.pool_c0.used[0]==0&&owners.word_80098818==0,"source C0 reset bitmap/global omitted");
    for(size_t slot=0;slot<16;++slot)for(size_t at=0;at<36;++at){const auto& bytes=owners.pool_c0.slots[slot];
        require(bytes.known[at]&&bytes.raw[at]==(at==1?uint8_t(0xc0|slot):0),"source C0 inclusive reset bytes/header differ");}
    std::cout<<"13 counter-tail cases/C0 reset/4096 unknown counter holds PASS; ";
}
void random_callback_integration(const l3::SourceEventPhase5ResourceBinding& witness){
    const int rng_owner=0;l3::SourceEventRandomEffectReceiptOwner current{&rng_owner,19};
    auto event=callback_work(0,6);auto original=callback_scene(0,0x80000000u,0xfffe0000u);
    auto initial=context(0,1,2);auto p=scene_profile(0);p.timeline[1].callback=0x80100d6c;
    const auto plan=l3::plan_source_event_random_effect_80100d6c(event,original);
    require(plan.status==l3::SourceEventRandomEffectStatus::Ready&&plan.rng_calls==6,"random callback integration plan failed");
    l3::SourceEventRandomEffectReceipts batch;batch.owner=&rng_owner;batch.generation=19;batch.completed=true;
    batch.callback=0x80100d6c;batch.source_work=0x800c4c48;batch.event_frame=6;batch.source_actor=plan.source_actor;batch.count=6;
    for(size_t i=0;i<6;++i)batch.calls[i]={plan.caller_ras[i],uint32_t(i*17),1001+i};
    for(unsigned test=0;test<5;++test){auto owners=original;auto state=initial;auto pending=batch;auto owner=current;
        const auto* supplied=&pending;
        switch(test){case 0:supplied=nullptr;break;case 1:owner.generation++;break;case 2:pending.calls[2].caller_ra++;break;
        case 3:owners.pool80.slots[2].known[1]=0;break;default:owners.camera_800bc738.known[12]=0;break;}
        const auto before=owners;const auto before_context=state;const auto result=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness,&p,supplied,&owner);
        require(!result.committed&&result.random_callback_consumed_count==0&&same_scene(owners,before)&&same_context(state,before_context),
            "random callback failure published camera/common journal/pool/control/continuation stores");
    }
    auto owners=original;auto state=initial;const uint32_t camera_before=uint32_t(owners.camera_800bc738.raw[12])|
        (uint32_t(owners.camera_800bc738.raw[13])<<8)|(uint32_t(owners.camera_800bc738.raw[14])<<16)|(uint32_t(owners.camera_800bc738.raw[15])<<24);
    const auto result=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness,&p,&batch,&current);
    const uint32_t camera_after=uint32_t(owners.camera_800bc738.raw[12])|(uint32_t(owners.camera_800bc738.raw[13])<<8)|
        (uint32_t(owners.camera_800bc738.raw[14])<<16)|(uint32_t(owners.camera_800bc738.raw[15])<<24);
    require(result.committed&&result.callback_executed&&result.random_callback_consumed_count==6&&
        result.random_callback_status==l3::SourceEventRandomEffectStatus::Consumed&&state.byte_80122424==0&&camera_after==camera_before+0x20000u&&
        state.awaiting_camera_8001ec0c&&owners.pool80.used[0]==0xe0000000u,"completed random callback did not join common prefix transaction");
    const auto completed=owners;const auto completed_state=state;
    const auto held=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,&witness,&p,&batch,&current);
    require(!held.committed&&held.random_callback_consumed_count==0&&same_scene(owners,completed)&&same_context(state,completed_state),"downstream continuation re-consumed RNG callback");
    owners=original;owners.camera_800bc738.known[184]=0;state=initial;
    l3::SourceEventInputs inputs;inputs.phase5_context=&owners;inputs.phase6_context=&state;inputs.phase5_resources=&witness;
    inputs.phase6_profile=&p;inputs.random_callback_receipts=&batch;inputs.random_callback_owner=&current;
    std::optional<l3::SourceEventWork> current_work=event;
    const auto pipeline=l3::step_source_event_pipeline(current_work,nullptr,inputs);
    require(pipeline.committed&&pipeline.random_callback_consumed_count==6&&pipeline.random_callback_status==l3::SourceEventRandomEffectStatus::Consumed&&
        pipeline.phase6_consumer_status==Status::NeedsCameraWord&&state.awaiting_camera_8001ec0c,"downstream camera hold lost completed RNG count for external atomic commit");
    const auto pipeline_scene=owners;const auto pipeline_state=state;
    const auto retry=l3::step_source_event_pipeline(current_work,nullptr,inputs);
    require(!retry.committed&&retry.random_callback_consumed_count==0&&same_scene(owners,pipeline_scene)&&same_context(state,pipeline_state),"pipeline continuation repeated RNG callback or journal stores");
    std::cout<<"random callback prefix/pipeline atomic integration PASS; ";
}
void guards(const l3::SourceEventPhase5ResourceBinding& witness){
    const Status expected[]={Status::NeedsEventWork,Status::WrongEventPhase,Status::NeedsOverlayControl,Status::NeedsInputWord,
        Status::NeedsCameraField,Status::UnsupportedCallbackTable,Status::NeedsActorBinding,Status::NeedsActorBinding,Status::NeedsResourceOwnership};
    for(unsigned i=0;i<9;++i){auto event=work(0,0);auto owners=scene(0);auto state=context(0,1,2);const auto* resource=&witness;
        switch(i){case 0:event.known[4]=0;break;case 1:event.raw[4]=8;break;case 2:state.byte_80122424.reset();break;
            case 3:state.scratch_half_1f800002.reset();break;case 4:owners.camera_800bc738.known[12]=0;break;
            case 5:put(event.raw.data()+0xa4,0x8011b2f0);break;case 6:owners.pool40.used[0]=0;break;
            case 7:put(event.raw.data()+0x30,0x800bf119);break;default:resource=nullptr;break;}
        const auto before=owners;const auto context_before=state;const auto result=l3::consume_source_event1_phase6_prefix_8010102c(event,owners,state,resource);
        require(!result.committed&&result.status==expected[i]&&same_scene(before,owners)&&same_context(context_before,state),"missing owner did not roll back atomically");
    }
    auto event=work(0,0);auto owners=scene(0);auto state=context(0,0,0);l3::SourceEventInputs inputs;
    inputs.phase5_context=&owners;inputs.phase6_context=&state;inputs.phase5_resources=&witness;
    std::optional<l3::SourceEventWork> work_copy=event;auto result=l3::step_source_event_pipeline(work_copy,nullptr,inputs);
    require(result.committed&&result.request&&result.request->source_function==0x8001ec0c&&result.script_callback==0x80100738&&
        work_copy->phase04()==6&&work_copy->raw==event.raw&&state.awaiting_camera_8001ec0c,"callback not wired to shared pipeline or falsely advanced");
    const auto allocated=allocations;bool held=true;
    for(unsigned i=0;i<4096;++i){result=l3::step_source_event_pipeline(work_copy,nullptr,inputs);held&=!result.committed&&result.request&&result.request->source_function==0x8001ec0c;}
    require(held&&allocations==allocated&&work_copy->raw==event.raw,"continuation hold allocated or advanced");
    require(l3::initialize_source_event_phase6_overlay_load(state,profile())&&state.byte_80122424==0&&
        !state.awaiting_camera_8001ec0c&&!state.scratch_half_1f800002&&!state.byte_80122438,"fresh overlay lifecycle fabricated retained owner fields");
}
}
void callback_selector_guards(){
    auto profile=scene_profile(1);auto work_value=work(1,0);const auto allocated=allocations;
    for(size_t index=0;index<profile.timeline.size();++index){
        put(work_value.raw.data()+0xa4,profile.callback_table_token+uint32_t(index)*8);
        for(unsigned enabled=0;enabled<255;++enabled){work_value.raw[8]=uint8_t(enabled);
            require(l3::source_event_phase6_selected_callback_8010102c(work_value,profile)==profile.timeline[index].callback,"enable byte incorrectly used as table index");}
    }
    work_value.raw[8]=255;require(!l3::source_event_phase6_selected_callback_8010102c(work_value,profile),"disabled callback selected");work_value.raw[8]=0;
    for(uint32_t token:{profile.callback_table_token-1,profile.callback_table_token+1,profile.callback_table_token+uint32_t(profile.timeline.size())*8}){
        put(work_value.raw.data()+0xa4,token);require(!l3::source_event_phase6_selected_callback_8010102c(work_value,profile),"malformed pointer selected");}
    put(work_value.raw.data()+0xa4,profile.callback_table_token);
    for(size_t offset:{size_t(8),size_t(164),size_t(165),size_t(166),size_t(167)}){work_value.known[offset]=0;
        require(!l3::source_event_phase6_selected_callback_8010102c(work_value,profile),"unknown callback owner selected");work_value.known[offset]=1;}
    profile.initialized=false;require(!l3::source_event_phase6_selected_callback_8010102c(work_value,profile)&&allocations==allocated,"invalid selector owner/hot allocation");
}

int main(int argc,char** argv){try{require(argc==2,"usage: test_source_event_phase6 source-event-phase6-prefix.tsv");
    const auto owner=owned_directory();const auto witness=l3::bind_source_event_phase5_resources(profile(),owner);
    require(witness.admitted(),"authored ownership fixture did not admit");differential(argv[1],witness);guards(witness);camera_timeline(argv[1],witness);callback_differential(argv[1],witness);source_main_loop_owners(argv[1]);random_callback_integration(witness);callback_selector_guards();packages();
    std::cout<<"9 atomic guards/shared pipeline/4096 allocation-free continuation holds PASS\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
