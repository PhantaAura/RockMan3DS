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
void* operator new[](std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::SourceEventPhase5Status;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void write32(uint8_t* p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=uint8_t(v>>(i*8));}
unsigned digit(char c){if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);throw std::runtime_error("fixture hex rejected");}
template<size_t N>std::array<uint8_t,N> from_hex(const std::string& value){
    require(value.size()==N*2,"fixture coverage rejected");std::array<uint8_t,N> out{};
    for(size_t i=0;i<N;++i)out[i]=uint8_t(digit(value[2*i])*16+digit(value[2*i+1]));return out;
}
template<size_t N>std::array<uint8_t,N> authored(unsigned seed){std::array<uint8_t,N> value{};
    for(size_t i=0;i<N;++i)value[i]=uint8_t(seed*37+i*17+N*11);return value;}
l3::EbdDirectory authored_ebd(){
    std::vector<uint8_t> bytes(113808);
    for(size_t i=0;i<bytes.size();++i)bytes[i]=uint8_t(i*7+3);
    write32(bytes.data(),14);
    for(size_t i=0;i<14;++i){auto* row=bytes.data()+4+i*16;row[0]=0xe0;row[1]=3;row[2]=uint8_t(i%4);row[3]=0;
        write32(row+4,0x80180000u+uint32_t(i%4)*128u);write32(row+8,0);write32(row+12,0);}
    return l3::parse_ebd_directory(bytes,0x8016c000);
}
l3::SourceEventPhase5Profile profile(unsigned seed){
    l3::SourceEventPhase5Profile out;out.initialized=true;out.directory_token=0x8011af4c;out.callback_table_token=0x8011b2e8;
    out.ebd_source_digest=from_hex<32>("4e385673e312e94f24075bdd66cd943432c9a642c5355151158029c06ebcd2bc");
    for(size_t i=0;i<out.records.size();++i){auto& record=out.records[i];
        for(size_t j=0;j<20;++j)record.raw[j]=uint8_t(seed*13+i*19+j*23);
        record.raw[0]=uint8_t(i);record.raw[1]=i==0?5:3;record.raw[3]=i==0?0x40:0xe0;
        if(i){record.resource_variant=uint8_t((i-1)%4);record.resource_directory_index=record.resource_variant;
            record.resource_token=0x80180000u+uint32_t(record.resource_variant)*128u;}
    }
    return out;
}
template<size_t Count,size_t Bytes> void authored_pool(l3::SourceEventPhase5Pool<Count,Bytes>& pool,unsigned seed,uint8_t family){
    const auto bytes=authored<Count*Bytes>(seed);
    for(size_t i=0;i<Count;++i){auto& slot=pool.slots[i];for(size_t j=0;j<Bytes;++j)slot.raw[j]=bytes[i*Bytes+j];
        slot.raw[1]=uint8_t(family|i);slot.known.fill(1);}
}
l3::SourceEventPhase5Context context(unsigned seed){
    l3::SourceEventPhase5Context out;authored_pool(out.pool40,seed,0x40);authored_pool(out.pool80,seed,0x80);authored_pool(out.pool_e0,seed,0xe0);
    const std::array<uint32_t,4> used40{{0,0x80000000,0xc0000000,0xe0000000}},used_e0{{0,0x80000000,0xf0000000,0xffff0000}};
    const std::array<uint32_t,3> used80{{0,0x80000000,0xffffffff}};
    out.pool40.used[0]=used40[seed%4];out.pool_e0.used[0]=used_e0[seed%4];
    out.pool80.used[0]=used80[seed%3];out.pool80.used[1]=0;
    out.camera_800bc738.raw=authored<188>(seed);out.camera_800bc738.known.fill(1);
    std::array<uint32_t,3> eye{},rotation{};
    for(size_t i=0;i<3;++i){eye[i]=uint32_t(seed*271+i*0x10203456u);rotation[i]=uint32_t(seed*137+i*0x13579u);}
    out.initial_eye_800a39a0=eye;out.initial_rotation_800a3964=rotation;return out;
}
l3::SourceEventWork work(unsigned seed){l3::SourceEventWork out;out.raw=authored<168>(seed);out.known.fill(1);
    out.raw[0]=1;out.raw[1]=1;out.raw[4]=5;return out;}
template<size_t Count,size_t Bytes> bool same_pool(const l3::SourceEventPhase5Pool<Count,Bytes>& a,
    const l3::SourceEventPhase5Pool<Count,Bytes>& b){
    if(a.used!=b.used)return false;
    for(size_t i=0;i<Count;++i)if(a.slots[i].raw!=b.slots[i].raw||a.slots[i].known!=b.slots[i].known)return false;return true;
}
bool same(const l3::SourceEventPhase5Context& a,const l3::SourceEventPhase5Context& b){
    return same_pool(a.pool40,b.pool40)&&same_pool(a.pool80,b.pool80)&&same_pool(a.pool_e0,b.pool_e0)&&
        a.camera_800bc738.raw==b.camera_800bc738.raw&&a.camera_800bc738.known==b.camera_800bc738.known&&
        a.initial_eye_800a39a0==b.initial_eye_800a39a0&&a.initial_rotation_800a3964==b.initial_rotation_800a3964&&
        a.mode_800a395c==b.mode_800a395c&&a.byte_80122428==b.byte_80122428;
}
template<size_t Count,size_t Bytes> void check_pool(const l3::SourceEventPhase5Pool<Count,Bytes>& pool,const std::string& text){
    const auto expected=from_hex<Count*Bytes>(text);
    for(size_t i=0;i<Count;++i)require(std::equal(pool.slots[i].raw.begin(),pool.slots[i].raw.end(),expected.begin()+i*Bytes),"original pool byte differential failed");
}
void differential(const char* path){
    std::ifstream file(path);require(bool(file),"missing phase5 fixture");std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){if(line.empty())continue;std::istringstream row(line);unsigned seed,mode,last;uint32_t used40,used80lo,used80hi,used_e0;
        std::string w,c,p40,p80,pe0,extra;
        require(bool(row>>seed>>w>>c>>p40>>p80>>pe0>>used40>>used80lo>>used80hi>>used_e0>>mode>>last)&&!(row>>extra),"invalid phase5 fixture row");
        auto state=context(seed);auto event=work(seed);const auto p=profile(seed);const auto owner=authored_ebd();const auto resources=l3::bind_source_event_phase5_resources(p,owner);
        const auto allocated=allocations;const auto result=l3::consume_source_event1_phase5_80100efc(event,state,p,&resources);
        require(result.committed&&result.status==Status::Consumed&&result.constructed_actors==17&&result.resource_bindings==15&&allocations==allocated,
            "constructor held or allocated in update");
        const auto expected_work=from_hex<168>(w);const auto expected_camera=from_hex<188>(c);
        const auto camera=l3::source_event_phase5_camera_snapshot(state);
        if(event.raw!=expected_work||camera!=expected_camera){
            for(size_t i=0;i<168;++i)if(event.raw[i]!=expected_work[i])std::cerr<<"seed="<<seed<<" work+"<<i<<" native="<<unsigned(event.raw[i])<<" original="<<unsigned(expected_work[i])<<'\n';
            for(size_t i=0;i<188;++i)if(camera[i]!=expected_camera[i])std::cerr<<"seed="<<seed<<" camera+"<<i<<" native="<<unsigned(camera[i])<<" original="<<unsigned(expected_camera[i])<<'\n';
            require(false,"original event/camera byte differential failed");
        }
        check_pool(state.pool40,p40);check_pool(state.pool80,p80);check_pool(state.pool_e0,pe0);
        require(state.pool40.used[0]==used40&&state.pool80.used[0]==used80lo&&state.pool80.used[1]==used80hi&&state.pool_e0.used[0]==used_e0&&
            state.mode_800a395c==mode&&state.byte_80122428==last,"original pool/camera scalar differential failed");++count;
    }
    require(count==24,"phase5 fixture case count changed");std::cout<<count<<" unchanged constructor differential cases PASS; ";
}
void initializers(const char* path){
    auto state=context(7);l3::initialize_source_event_phase5_pools_8002e88c(state);
    const auto check=[](const auto& pool,uint8_t family){
        for(size_t i=0;i<pool.slots.size();++i){const auto& slot=pool.slots[i];
            for(size_t j=0;j<slot.raw.size();++j)require(slot.raw[j]==(j==1?uint8_t(family|i):0)&&slot.known[j],"source cold-clear/tag initialization diverged");}
        for(const auto& used:pool.used)require(used==0,"source bitmap reset diverged");};
    check(state.pool40,0x40);check(state.pool80,0x80);check(state.pool_e0,0xe0);
    std::string folder(path);const auto slash=folder.find_last_of("/\\");folder=slash==std::string::npos?std::string{}:folder.substr(0,slash+1);
    std::ifstream file(folder+"source-event-phase5-camera.tsv");require(bool(file),"missing camera initializer fixture");std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){std::istringstream row(line);unsigned seed;std::string position,extra;std::array<uint32_t,3> eye{},rotation{};
        require(bool(row>>seed>>position>>eye[0]>>eye[1]>>eye[2]>>rotation[0]>>rotation[1]>>rotation[2])&&!(row>>extra),"invalid camera initializer row");
        const auto bytes=from_hex<6>(position);L3PlayerCompatWork player{};l3::SourceEventPlayerKnown known{};
        std::copy(bytes.begin(),bytes.end(),player.bytes+0x14);std::fill(known.begin()+0x14,known.begin()+0x1a,1);
        require(l3::initialize_source_event_phase5_camera_80038674(state,player,known)&&state.initial_eye_800a39a0==eye&&
            state.initial_rotation_800a3964==rotation&&state.mode_800a395c==1,"source player-position camera initializer differential failed");++count;
        known[0x17]=0;const auto before=state;
        require(!l3::initialize_source_event_phase5_camera_80038674(state,player,known)&&same(state,before),"unknown player position invented camera initialization");
    }
    require(count==24,"camera initializer fixture case count changed");std::cout<<count<<" unchanged camera initializer cases PASS; ";
}
void guards(){
    const std::array<Status,12> statuses{{Status::NeedsEventWork,Status::WrongEventPhase,Status::NeedsProfile,Status::UnsupportedProfile,
        Status::NeedsCameraFields,Status::NeedsCameraFields,Status::NeedsPoolBitmap,Status::NeedsPoolHeader,
        Status::PoolCapacityExceeded,Status::PoolCapacityExceeded,Status::PoolCapacityExceeded,Status::UnsupportedProfile}};
    for(unsigned i=0;i<statuses.size();++i){auto state=context(0);auto event=work(0);auto p=profile(0);
        const auto owner=authored_ebd();const auto resources=l3::bind_source_event_phase5_resources(p,owner);
        switch(i){case 0:event.known[4]=0;break;case 1:event.raw[4]=6;break;case 2:p.initialized=false;break;
            case 3:p.records[1].raw[3]=0x20;break;case 4:state.initial_eye_800a39a0.reset();break;
            case 5:state.initial_rotation_800a3964.reset();break;case 6:state.pool_e0.used[0].reset();break;
            case 7:state.pool_e0.slots[0].known[1]=0;break;case 8:state.pool40.used[0]=0xffffffff;break;
            case 9:state.pool_e0.used[0]=0xffffc000;break;case 10:state.pool80.used[0]=state.pool80.used[1]=0xffffffff;break;
            default:p.records[2].resource_token=0x87654321;break;}
        const auto before=state;const auto before_work=event;const auto result=l3::consume_source_event1_phase5_80100efc(event,state,p,&resources);
        require(!result.committed&&result.status==statuses[i]&&same(state,before)&&event.raw==before_work.raw&&event.known==before_work.known,
            "missing owner/profile/pool capacity violated atomic rollback");
    }
    auto state=context(0);auto p=profile(0);std::optional<l3::SourceEventWork> event=work(0);l3::SourceEventInputs inputs;
    const auto owner=authored_ebd();const auto resources=l3::bind_source_event_phase5_resources(p,owner);
    inputs.phase5_context=&state;inputs.phase5_profile=&p;inputs.phase5_resources=&resources;
    auto step=l3::step_source_event_pipeline(event,nullptr,inputs);
    require(step.committed&&event->phase04()==6&&step.phase5_consumer_status==Status::Consumed&&step.script_actor_construction_count==17&&
        step.script_resource_binding_count==15&&!step.request,"phase5 constructor disconnected from shared pipeline");
    const auto before=state;const auto event_before=event->raw;step=l3::step_source_event_pipeline(event,nullptr,inputs);
    require(!step.committed&&step.request&&step.request->source_function==0x8010102c&&event->raw==event_before&&same(state,before),
        "phase6 callback/interpreters acknowledged without implementation");
    const auto allocated=allocations;bool held=true;
    for(unsigned i=0;i<4096;++i){step=l3::step_source_event_pipeline(event,nullptr,inputs);held&=!step.committed;}
    require(held&&allocations==allocated,"phase6 hold hot loop allocated or advanced");
}
void packages(){
    auto p=profile(0);std::vector<uint8_t> bytes(620);bytes[0]='L';bytes[1]='3';bytes[2]='P';bytes[3]='5';bytes[4]=1;bytes[6]=96;
    write32(bytes.data()+8,1);write32(bytes.data()+12,16);write32(bytes.data()+16,620);write32(bytes.data()+24,32);write32(bytes.data()+28,12);
    const auto overlay=from_hex<32>("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    const auto ebd=from_hex<32>("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");
    std::copy(overlay.begin(),overlay.end(),bytes.begin()+32);std::copy(ebd.begin(),ebd.end(),bytes.begin()+64);
    write32(bytes.data()+96,p.directory_token);write32(bytes.data()+100,p.callback_table_token);
    for(size_t i=0;i<16;++i){auto* r=bytes.data()+108+i*32;const auto& record=p.records[i];std::copy(record.raw.begin(),record.raw.end(),r);
        write32(r+20,record.resource_token);write32(r+24,record.resource_directory_index);r[28]=record.resource_variant;}
    const auto repair=[](auto& value){if(value.size()>=96)write32(value.data()+20,l3::crc32(value.data()+96,value.size()-96));};repair(bytes);
    const auto loaded=l3::load_source_event_phase5_profile(bytes);
    require(loaded.initialized&&loaded.records[15].resource_token==p.records[15].resource_token,"neutral phase5 package roundtrip failed");
    for(unsigned i=0;i<16;++i){auto bad=bytes;
        switch(i){case 0:bad.resize(95);break;case 1:bad[0]^=1;break;case 2:bad[4]=2;break;case 3:bad[6]=64;break;
            case 4:write32(bad.data()+8,2);break;case 5:write32(bad.data()+12,17);break;case 6:write32(bad.data()+24,20);break;
            case 7:bad.push_back(0);break;case 8:bad[32]^=1;break;case 9:bad[64]^=1;break;case 10:bad.back()^=1;break;
            case 11:bad[104]=1;repair(bad);break;case 12:bad[108+32+3]=0x20;repair(bad);break;
            case 13:write32(bad.data()+108+32+20,0x87654321);repair(bad);break;
            case 14:bad[108+32+29]=1;repair(bad);break;default:write32(bad.data()+108+32+24,14);repair(bad);break;}
        bool failed=false;try{(void)l3::load_source_event_phase5_profile(bad);}catch(const std::runtime_error&){failed=true;}
        require(failed,"malformed phase5 package accepted");
    }
}
}
int main(int argc,char** argv){
    try{require(argc==2,"usage: test_source_event_phase5 source-event-phase5.tsv");differential(argv[1]);initializers(argv[1]);guards();packages();
        std::cout<<"16 malformed packages/12 rollback cases/pipeline phase5->6/4096 allocation-free holds PASS; context_bytes="<<sizeof(l3::SourceEventPhase5Context)<<'\n';return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
