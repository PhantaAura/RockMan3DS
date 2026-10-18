#include "legends3ds/source_fade.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>

namespace {bool count_allocations=false;size_t allocations=0;}
void* operator new(size_t n){if(count_allocations)++allocations;auto p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace {
void require(bool value,const char* why){if(!value){std::cerr<<why<<'\n';std::exit(1);}}
template<size_t N> void fill(l3::SourceFadeBytes<N>& bytes,unsigned seed,size_t& offset){
    for(size_t i=0;i<N;++i,++offset){bytes.raw[i]=static_cast<uint8_t>(seed*37+offset*17);bytes.known[i]=(seed+offset)%3==0;}
}
l3::SourceFadeContext authored(unsigned seed,uint16_t busy){
    l3::SourceFadeContext out;size_t offset=0;
    fill(out.work_80098a68,seed,offset);
    for(auto& p:out.tiles_80098a28)fill(p,seed,offset);
    for(auto& p:out.draw_modes_80098a10)fill(p,seed,offset);
    out.work_80098a68.raw[10]=static_cast<uint8_t>(busy);out.work_80098a68.raw[11]=static_cast<uint8_t>(busy>>8);
    out.work_80098a68.known[10]=out.work_80098a68.known[11]=1;return out;
}
template<size_t N> bool same(const l3::SourceFadeBytes<N>& a,const l3::SourceFadeBytes<N>& b){return a.raw==b.raw&&a.known==b.known;}
bool same(const l3::SourceFadeContext& a,const l3::SourceFadeContext& b){
    if(!same(a.work_80098a68,b.work_80098a68))return false;
    for(size_t i=0;i<2;++i)if(!same(a.tiles_80098a28[i],b.tiles_80098a28[i])||!same(a.draw_modes_80098a10[i],b.draw_modes_80098a10[i]))return false;
    return true;
}
void expected_known(l3::SourceFadeContext& out){
    auto& k=out.work_80098a68.known;k[0]=k[10]=k[11]=k[12]=k[13]=1;
    for(auto& tile:out.tiles_80098a28){tile.known[3]=tile.known[7]=1;for(size_t i=8;i<16;++i)tile.known[i]=1;}
    for(auto& draw:out.draw_modes_80098a10)for(size_t i=3;i<12;++i)draw.known[i]=1;
}
template<size_t N> void check_bytes(const l3::SourceFadeBytes<N>& data,const std::string& hex,size_t& offset){
    for(auto value:data.raw){require(offset+2<=hex.size(),"fixture hex size");require(value==std::stoul(hex.substr(offset,2),nullptr,16),"original instruction differential");offset+=2;}
}
std::optional<l3::SourceEventWork> phase4(){
    l3::SourceEventWork work;work.known.fill(1);work.raw[0]=1;work.raw[1]=1;work.raw[4]=4;work.raw[12]=1;return work;
}
void differential(const char* path){
    std::ifstream file(path);require(bool(file),"fixture missing");std::string line;size_t rows=0;
    while(std::getline(file,line)){
        std::istringstream row(line);unsigned seed,duration,step,busy,gpu;std::string hex,extra;
        require(bool(row>>seed>>duration>>step>>busy>>gpu>>hex)&&!(row>>extra),"fixture row");
        require(duration<=65535&&step<=65535&&busy<=65535&&gpu<=255&&hex.size()==144,"fixture domain");
        auto context=authored(seed,static_cast<uint16_t>(busy));auto before=context;
        l3::SourceFadeStartupProfile profile{1,static_cast<uint16_t>(duration),static_cast<uint16_t>(step)};
        l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;
        auto work=phase4();l3::SourceEventInputs input;input.fade_context=&context;input.fade_profile=&profile;input.fade_intents=&queue;
        const auto result=l3::step_source_event_pipeline(work,nullptr,input);
        require(result.committed&&!result.request&&work->phase04()==5&&work->raw[12]==0,"source phase4 transaction");
        require(result.pending_fade_scheduler_intent==(busy==0)&&queue.count==(busy==0?1u:0u),"pending owner intent count");
        require(result.fade_consumer_status==(busy?l3::SourceFadeStartupStatus::SourceBackedBusyNoop:l3::SourceFadeStartupStatus::PendingSchedulerIntent),"fade status");
        if(!busy){expected_known(before);require(queue.entries[0].slot==2&&queue.entries[0].callback==0x80017e94u&&queue.entries[0].source_function==0x80012e10u,"exact scheduler arguments");}
        require(context.work_80098a68.known==before.work_80098a68.known,"work validity preserve");
        for(size_t i=0;i<2;++i)require(context.tiles_80098a28[i].known==before.tiles_80098a28[i].known&&context.draw_modes_80098a10[i].known==before.draw_modes_80098a10[i].known,"packet validity preserve");
        size_t offset=0;check_bytes(context.work_80098a68,hex,offset);for(const auto& p:context.tiles_80098a28)check_bytes(p,hex,offset);for(const auto& p:context.draw_modes_80098a10)check_bytes(p,hex,offset);
        ++rows;
    }
    require(rows==517,"fixture coverage");
}
void holds_and_loop(){
    l3::SourceFadeStartupProfile profile{1,3,7};auto context=authored(9,0);
    l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;
    l3::SourceEventInputs input;input.fade_context=&context;input.fade_profile=&profile;input.fade_intents=&queue;
    const auto hold=[&](l3::SourceFadeStartupStatus expected){auto work=phase4();const auto before=context;const auto qcount=queue.count;
        const auto result=l3::step_source_event_pipeline(work,nullptr,input);
        require(!result.committed&&result.request&&result.fade_consumer_status==expected,"missing owner must hold");
        require(work->phase04()==4&&work->raw[12]==1&&same(context,before)&&queue.count==qcount,"atomic source owner rollback");};
    context.work_80098a68.known[10]=0;hold(l3::SourceFadeStartupStatus::NeedsBusyHalfword);context.work_80098a68.known[10]=1;
    input.fade_profile=nullptr;hold(l3::SourceFadeStartupStatus::NeedsProfile);input.fade_profile=&profile;
    profile.duration_80097a44.reset();hold(l3::SourceFadeStartupStatus::NeedsProfile);profile.duration_80097a44=3;
    profile.step_80097a4c.reset();hold(l3::SourceFadeStartupStatus::NeedsProfile);profile.step_80097a4c=7;
    profile.decoder=0;hold(l3::SourceFadeStartupStatus::UnsupportedProfile);profile.decoder=1;
    queue.initialized=false;hold(l3::SourceFadeStartupStatus::NeedsSchedulerIntentQueue);queue.initialized=true;
    queue.count=queue.entries.size()+1;hold(l3::SourceFadeStartupStatus::NeedsSchedulerIntentQueue);
    queue.count=queue.entries.size();hold(l3::SourceFadeStartupStatus::SchedulerIntentQueueFull);queue.count=0;
    auto busy=authored(9,65535);const auto busy_before=busy;
    const auto no_op=l3::consume_source_fade_startup_80017c30_type2(busy,nullptr,nullptr);
    require(no_op.committed&&!no_op.pending_scheduler_intent&&same(busy,busy_before),"busy requires no profile or scheduler");
    // Authored zero table scalars remain present optional values, not missing.
    l3::SourceFadeStartupProfile zeros{1,0,0};
    require(l3::consume_source_fade_startup_80017c30_type2(context,&zeros,&queue).committed,"zero table fields are owned");
    count_allocations=true;
    for(size_t tick=0;tick<4096;++tick){context=authored(9,0);queue.count=0;auto work=phase4();
        require(l3::step_source_event_pipeline(work,nullptr,input).committed,"hot phase4 commit");}
    count_allocations=false;require(allocations==0,"fade update allocation");
}
void write32(uint8_t* p,uint32_t value){for(size_t i=0;i<4;++i)p[i]=static_cast<uint8_t>(value>>(i*8));}
void profile_validation(){
    const std::array<uint8_t,32> digest{{0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
    std::vector<uint8_t> bytes(68);bytes[0]='L';bytes[1]='3';bytes[2]='F';bytes[3]='D';bytes[4]=1;bytes[6]=64;
    write32(bytes.data()+8,1);write32(bytes.data()+16,1);write32(bytes.data()+20,68);write32(bytes.data()+28,4);
    for(size_t i=0;i<32;++i)bytes[32+i]=digest[i];bytes[64]=3;bytes[66]=7;
    write32(bytes.data()+24,l3::crc32(bytes.data()+64,4));
    const auto loaded=l3::load_source_fade_startup_profile(bytes);
    require(loaded.decoder==1&&loaded.duration_80097a44==3&&loaded.step_80097a4c==7,"authored fade profile");
    const auto rejected=[&](const std::vector<uint8_t>& candidate){bool failed=false;try{(void)l3::load_source_fade_startup_profile(candidate);}catch(const std::exception&){failed=true;}require(failed,"malformed profile accepted");};
    for(size_t offset:std::array<size_t,12>{{0,4,6,8,12,16,20,24,28,32,63,64}}){auto bad=bytes;bad[offset]^=1;rejected(bad);}
    auto bad=bytes;bad.pop_back();rejected(bad);bad=bytes;bad.push_back(0);rejected(bad);
}
}
int main(int argc,char** argv){require(argc==2,"usage: test_source_fade source-fade-startup.tsv");
    differential(argv[1]);holds_and_loop();profile_validation();
    std::cout<<"source fade startup: 517 original-instruction cases, bounded pending intents, profile/rollback guards and4096 allocation-free updates passed\n";
}
