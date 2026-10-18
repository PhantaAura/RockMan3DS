#include "legends3ds/source_event_consumer.h"
#include "legends3ds/source_event_pipeline.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {size_t allocations=0;}
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint8_t pattern(size_t offset,unsigned seed){return static_cast<uint8_t>(offset*37+seed*11);}
template<class Container> void apply_diff(Container& bytes,const std::string& text){
    if(text=="-")return;
    std::istringstream changes(text);std::string item;
    while(std::getline(changes,item,',')){
        const auto colon=item.find(':');require(colon!=std::string::npos,"bad source diff");
        const auto offset=std::stoul(item.substr(0,colon)),value=std::stoul(item.substr(colon+1));
        require(offset<bytes.size()&&value<256,"source diff outside work domain");bytes[offset]=static_cast<uint8_t>(value);
    }
}
l3::SourceEventWork work(unsigned seed=1){
    l3::SourceEventWork out;
    for(size_t i=0;i<out.raw.size();++i)out.raw[i]=pattern(i,seed);
    out.raw[0]=1;out.raw[1]=1;out.raw[4]=1;out.known.fill(1);return out;
}
using Status=l3::SourceEventPlayerConsumerStatus;
void original_differential(const char* path){
    std::ifstream file(path);require(bool(file),"missing event consumer fixture");std::string line;
    std::getline(file,line);size_t cases=0,ready_cases=0,rejected=0;
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::istringstream row(line);unsigned seed,flags,state,action,busy174,busy177,byte93,ready;
        uint32_t lock,afterlock;unsigned current;std::array<unsigned,4> masks{},aftermasks{};
        std::string playerdiff,workdiff,voicediff,extra;
        require(bool(row>>seed>>flags>>state>>action>>busy174>>busy177>>byte93>>lock>>masks[0]>>masks[1]>>masks[2]>>masks[3]>>ready>>afterlock>>current>>aftermasks[0]>>aftermasks[1]>>aftermasks[2]>>aftermasks[3]>>playerdiff>>workdiff>>voicediff)&&!(row>>extra),"bad event consumer fixture row");
        L3PlayerCompatWork player;std::array<uint8_t,L3_PLAYER_COMPAT_WORK_BYTES> expected;
        for(size_t i=0;i<expected.size();++i)player.bytes[i]=expected[i]=pattern(i,seed);
        player.bytes[0]=expected[0]=static_cast<uint8_t>(flags);
        player.bytes[8]=expected[8]=static_cast<uint8_t>(state);
        player.bytes[9]=expected[9]=static_cast<uint8_t>(action);
        player.bytes[10]=expected[10]=2;
        player.bytes[0x174]=expected[0x174]=static_cast<uint8_t>(busy174);
        player.bytes[0x177]=expected[0x177]=static_cast<uint8_t>(busy177);
        auto event=work(seed+1),expected_work=event;apply_diff(expected,playerdiff);apply_diff(expected_work.raw,workdiff);
        l3::SourceEventPlayerKnown known;known.fill(1);
        l3::SourceEventDriverContext driver;driver.lock_800bc050=lock;driver.voice_800c0bfa=0x1234;
        std::array<uint8_t,24*54> expected_voices;
        for(size_t i=0;i<expected_voices.size();++i){
            expected_voices[i]=pattern(i,seed+2);driver.voices[i/54].raw[i%54]=expected_voices[i];
            driver.voices[i/54].known[i%54]=static_cast<uint8_t>(i&1);
        }
        const auto before_driver=driver;apply_diff(expected_voices,voicediff);
        for(unsigned i=0;i<2;++i){driver.pending_800c5680[i]=static_cast<uint16_t>(masks[i]);driver.active_8009ae40[i]=static_cast<uint16_t>(masks[i+2]);}
        l3::SourceEventDriverIntentQueue queue;queue.initialized=true;
        const auto out=l3::consume_source_event1_phase1_80042238(event,player,known,driver,queue,static_cast<uint8_t>(byte93));
        require(out.committed&&out.player_ready==bool(ready)&&out.receipt_count==(ready?2:0),"source player readiness/receipt diverged");
        require(std::equal(expected.begin(),expected.end(),player.bytes)&&event.raw==expected_work.raw,"source player/event retained bytes diverged");
        require(driver.lock_800bc050==afterlock&&driver.voice_800c0bfa==current,"source driver lock/currentvoice diverged");
        for(unsigned i=0;i<2;++i)require(driver.pending_800c5680[i]==aftermasks[i]&&driver.active_8009ae40[i]==aftermasks[i+2],"source pending/active complete-mask operation diverged");
        for(size_t i=0;i<expected_voices.size();++i){
            require(driver.voices[i/54].raw[i%54]==expected_voices[i],"source voice store or retained bytes diverged");
            const bool stored=ready&&lock!=1&&(i/54==22||i/54==23)&&(i%54==0||i%54==1||i%54==4||i%54==5||i%54==0x1d);
            require(driver.voices[i/54].known[i%54]==(stored?1:before_driver.voices[i/54].known[i%54]),"unknown voice retained field became known");
        }
        require(queue.count==(ready?2u:0u),"driver call lacks actual queue receipt");
        if(ready){
            for(unsigned i=0;i<2;++i)require(queue.records[i].source_function==0x80071220&&queue.records[i].voice==22+int(i)&&queue.records[i].source_return==(lock==1?-1:0),"source driver result ignored or relabelled");
            ++ready_cases;if(lock==1)++rejected;
        }
        ++cases;
    }
    require(cases==324&&ready_cases>0&&rejected>0,"source fixture coverage changed");
    std::cout<<cases<<" unchanged original player/event/voice cases PASS; ";
}
void phase3_differential(const char* path){
    const std::string original(path);const auto slash=original.find_last_of("/\\");
    const auto fixture=(slash==std::string::npos?std::string{}:original.substr(0,slash+1))+"source_event_phase3_01422.tsv";
    std::ifstream file(fixture);require(bool(file),"missing phase3 source fixture");std::string line;std::getline(file,line);unsigned count=0;
    while(std::getline(file,line)){
        std::istringstream row(line);unsigned byte;std::string hex,extra;
        require(bool(row>>byte>>hex)&&!(row>>extra)&&hex.size()==168*2,"bad phase3 fixture row");
        std::optional<l3::SourceEventWork> event=work(byte);event->raw[4]=3;
        event->known[12]=event->known[13]=event->known[14]=event->known[15]=0;
        const auto before=*event;l3::SourceEventInputs inputs;inputs.byte_80098ab8=static_cast<uint8_t>(byte);
        const auto out=l3::step_source_event_pipeline(event,nullptr,inputs);
        require(out.committed&&!out.request,"explicit phase3 predicate did not run");
        for(size_t i=0;i<event->raw.size();++i){
            require(event->raw[i]==static_cast<uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16)),"original phase3 work mutation diverged");
            const bool stored=(byte&32)&&(i>=12&&i<16);
            require(event->known[i]==(stored?1:before.known[i]),"phase3 promoted unobserved retained work field");
        }
        ++count;
    }
    require(count==256,"phase3 status domain changed");
    std::cout<<count<<" unchanged original phase3 raw predicates PASS; ";
}
void player_initialization_differential(const char* path){
    const std::string original(path);const auto slash=original.find_last_of("/\\");
    const auto directory=slash==std::string::npos?std::string{}:original.substr(0,slash+1);
    unsigned cases=0;
    for(unsigned kind=0;kind<2;++kind){
        const auto fixture=directory+(kind==0?"source_event_player_initialization_01422.tsv":"source_event_player_header_01422.tsv");
        std::ifstream file(fixture);require(bool(file),"missing player initialization source fixture");std::string line;std::getline(file,line);unsigned count=0;
        while(std::getline(file,line)){
            std::istringstream row(line);unsigned seed;std::string hex,extra;
            require(bool(row>>seed>>hex)&&!(row>>extra)&&hex.size()==L3_PLAYER_COMPAT_WORK_BYTES*2,"bad player initialization source row");
            L3PlayerCompatWork player;l3::SourceEventPlayerKnown known{};
            for(size_t i=0;i<known.size();++i)player.bytes[i]=pattern(i,seed);
            if(kind==0)l3::initialize_source_event_player_fields_8003bee8(player,known);
            else l3::initialize_source_event_fresh_player_header_8003be6c(player,known);
            for(size_t i=0;i<known.size();++i){
                const bool stored=kind==0?(i==0xbd||(i>=0x74&&i<=0x77)||(i>=0x184&&i<=0x187)||i==0x110||i==0x111||i==0x449):
                    (i==0||i==6||(i>=2&&i<6)||(i>=8&&i<16));
                require(player.bytes[i]==static_cast<uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16)),"original player initialization field/retained tail diverged");
                require(known[i]==(stored?1:0),"player initializer manufactured known unwritten field");
            }
            ++count;
        }
        require(count==(kind==0?16u:24u),"player initialization differential coverage changed");cases+=count;
    }
    L3PlayerCompatWork player{};l3::SourceEventPlayerKnown known{};
    l3::initialize_source_event_fresh_player_header_8003be6c(player,known);
    l3::initialize_source_event_player_fields_8003bee8(player,known);
    auto flags=l3::initialize_source_story_flags_8001d8c0();
    require(l3::apply_source_st00_area_event_prefix(0,0,flags,player).committed,"source fresh player/area0 failed");
    std::optional<l3::SourceEventWork> event;event.emplace();l3::reset_source_event_work_8001d928(*event);
    l3::SourceEventPollProfile profile;profile.decoder=1;profile.source_stage=0;profile.records={{1540,1,0}};
    l3::SourceEventInputs inputs;inputs.flags=&flags;inputs.stage_800c1b60=0;inputs.transition_status_800c356c=0;inputs.player_hp_800b521e=1;
    inputs.busy_80098a72=l3::initialize_source_event_busy_from_executable_boot();
    require(l3::step_source_event_pipeline(event,&profile,inputs).committed&&event->raw[4]==1,"source explicit coldboot/area0/poll prefix disconnected");
    l3::SourceEventDriverContext driver;l3::SourceEventDriverIntentQueue queue;l3::initialize_source_event_driver_fields(driver,queue);
    inputs.player_work=&player;inputs.player_known=&known;inputs.driver=&driver;inputs.driver_intents=&queue;
    const auto out=l3::step_source_event_pipeline(event,&profile,inputs);
    require(out.committed&&out.driver_receipt_count==2&&event->raw[4]==2&&queue.count==2,"source minimal known player initialization could not consume phase1");
    require(!known[1]&&!known[7]&&!known[0x174]&&!event->known[0x64],"minimal fresh consumer chain promoted unknown retained fields");
    std::cout<<cases<<" unchanged original player initializer cases/minimal known fresh consumer chain PASS; ";
}
void guards(){
    auto event=work();L3PlayerCompatWork player{};l3::SourceEventPlayerKnown known{};
    player.bytes[0xbd]=8;known[0]=1;known[0xbd]=1;
    l3::SourceEventDriverContext driver;l3::SourceEventDriverIntentQueue queue;
    l3::initialize_source_event_driver_fields(driver,queue);
    require(l3::initialize_source_event_busy_from_executable_boot()==0,"measured executable boot busy initializer changed");
    driver.voices[22].raw.fill(177);driver.voices[22].known.fill(0);
    const auto held=[&](Status status){
        const auto before=event;const auto p=player;const auto d=driver;const auto q=queue;const auto k=known;
        const auto out=l3::consume_source_event1_phase1_80042238(event,player,known,driver,queue);
        require(out.status==status&&!out.committed&&event.raw==before.raw&&event.known==before.known&&
                std::equal(std::begin(p.bytes),std::end(p.bytes),player.bytes)&&known==k&&
                driver.lock_800bc050==d.lock_800bc050&&driver.pending_800c5680==d.pending_800c5680&&
                driver.voices[22].raw==d.voices[22].raw&&queue.count==q.count,"failed preflight published partial source transaction");
    };
    known[0xbd]=0;held(Status::NeedsPlayerField);known[0xbd]=1;
    event.known[4]=0;held(Status::NeedsEventWork);event.known[4]=1;
    event.raw[4]=2;held(Status::WrongEventPhase);event.raw[4]=1;
    queue.initialized=false;held(Status::NeedsIntentQueue);queue.initialized=true;
    queue.count=25;held(Status::MalformedIntentQueue);queue.count=23;held(Status::IntentQueueFull);queue.count=0;
    driver.pending_800c5680[1].reset();held(Status::NeedsDriverField);driver.pending_800c5680[1]=0;
    driver.lock_800bc050.reset();held(Status::NeedsDriverField);driver.lock_800bc050=0;
    player.bytes[0]=1;known[8]=0;held(Status::NeedsPlayerField);known[8]=1;player.bytes[8]=1;
    known[9]=1;player.bytes[9]=14;held(Status::NeedsSourceByte800c1b93);
    player.bytes[9]=0;held(Status::NeedsPlayerField);known[0x174]=1;held(Status::NeedsPlayerField);known[0x177]=1;
    known[0x174]=0;player.bytes[8]=3;
    auto out=l3::consume_source_event1_phase1_80042238(event,player,known,driver,queue);
    require(out.committed&&out.player_ready&&out.receipt_count==2&&event.raw[4]==2&&player.bytes[0]==0,"ready state3 required unconsumed state fields");
    require(known[0x449]&&known[0xb4]&&known[0xb5]&&!known[0xa]&&!known[0x174],"player known mask widened beyond source stores");
    event=work();queue.count=23;player.bytes[8]=1;player.bytes[9]=13;known[0xa]=1;player.bytes[0]=1;player.bytes[0xa]=2;
    out=l3::consume_source_event1_phase1_80042238(event,player,known,driver,queue);
    require(out.status==Status::NotReady&&out.committed&&!out.player_ready&&player.bytes[0xa]==6&&event.raw[4]==1&&queue.count==23,"notready source phase6 mutation or zero-driver-call behavior diverged");
    std::optional<l3::SourceEventWork> pipeline=work();l3::SourceEventInputs inputs;
    inputs.player_work=&player;inputs.player_known=&known;inputs.driver=&driver;inputs.driver_intents=&queue;
    player.bytes[0]=0;queue.count=23;
    auto step=l3::step_source_event_pipeline(pipeline,nullptr,inputs);
    require(!step.committed&&step.player_consumer_status==Status::IntentQueueFull&&pipeline->raw[4]==1,"pipeline bypassed two native receipts");
    queue.count=0;step=l3::step_source_event_pipeline(pipeline,nullptr,inputs);
    require(step.committed&&step.driver_receipt_count==2&&pipeline->raw[4]==2,"pipeline did not publish actual phase1 consumer receipt");
    const auto allocation_start=allocations;bool ok=true;
    for(unsigned i=0;i<4096;++i){
        event=work();queue.count=0;
        const auto r=l3::consume_source_event1_phase1_80042238(event,player,known,driver,queue);
        ok&=r.committed&&r.receipt_count==2&&event.raw[4]==2;
    }
    require(ok&&allocations==allocation_start,"source event consumer allocates in hot path");
    std::cout<<"atomic holds/known masks/two-receipt pipeline/4096 zero-allocation cases PASS\n";
}
}
int main(int argc,char** argv){
    try{require(argc==2,"usage: test_source_event_consumer source_event_consumer_01422.tsv");original_differential(argv[1]);phase3_differential(argv[1]);player_initialization_differential(argv[1]);guards();return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
