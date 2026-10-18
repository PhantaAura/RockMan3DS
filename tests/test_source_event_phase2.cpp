#include "legends3ds/source_event_phase2.h"
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
void* operator new(std::size_t size){++allocations;if(auto* p=std::malloc(size))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
uint8_t pattern(size_t offset,unsigned seed){return static_cast<uint8_t>(offset*37+seed*11);}
template<size_t N> void fill(std::array<uint8_t,N>& raw,unsigned seed){for(size_t i=0;i<N;++i)raw[i]=pattern(i,seed);}
void put16(uint8_t* p,uint16_t value){p[0]=static_cast<uint8_t>(value);p[1]=static_cast<uint8_t>(value>>8);}
void put32(uint8_t* p,uint32_t value){for(unsigned i=0;i<4;++i)p[i]=static_cast<uint8_t>(value>>(i*8));}
uint16_t read16(const uint8_t* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
template<size_t N> void diff(std::array<uint8_t,N>& raw,const std::string& changes){
    if(changes=="-")return;
    std::istringstream input(changes);std::string item;
    while(std::getline(input,item,',')){
        const auto colon=item.find(':');require(colon!=std::string::npos,"bad source diff");
        const auto offset=std::stoul(item.substr(0,colon)),value=std::stoul(item.substr(colon+1));
        require(offset<raw.size()&&value<256,"diff outside authored source owner");raw[offset]=static_cast<uint8_t>(value);
    }
}
l3::SourceEventWork event(unsigned seed=1){
    l3::SourceEventWork out;fill(out.raw,seed);out.known.fill(1);out.raw[0]=out.raw[1]=1;out.raw[4]=2;return out;
}
using Status=l3::SourceEventPhase2Status;
void differential(const char* path){
    std::ifstream file(path);require(bool(file),"missing source phase2 fixture");std::string line;std::getline(file,line);
    unsigned cases=0,init_cases=0,bounded_holds=0;bool initializer=false;
    while(std::getline(file,line)){
        if(line=="#initializer"){initializer=true;continue;}
        if(line.empty())continue;
        std::istringstream row(line);unsigned seed;
        if(initializer){
            std::string ddiff,qdiff,extra;unsigned write,current;
            require(bool(row>>seed>>ddiff>>qdiff>>write>>current)&&!(row>>extra),"bad initializer fixture");
            l3::SourceEventPhase2Context context;l3::SourceEventCommandQueue queue;
            std::array<uint8_t,32> descriptors;std::array<uint8_t,256> commands;fill(descriptors,seed+2);fill(commands,seed+30);
            for(size_t i=0;i<32;++i){context.descriptors[i/8].raw[i%8]=descriptors[i];context.descriptors[i/8].known[i%8]=static_cast<uint8_t>(i&1);}
            for(size_t i=0;i<256;++i){queue.records[i/16].raw[i%16]=commands[i];queue.records[i/16].known[i%16]=static_cast<uint8_t>(i&1);}
            diff(descriptors,ddiff);diff(commands,qdiff);
            l3::initialize_source_event_phase2_descriptors_80019f0c(context);l3::initialize_source_event_command_queue_8001b44c(queue);
            for(size_t i=0;i<32;++i)require(context.descriptors[i/8].raw[i%8]==descriptors[i]&&context.descriptors[i/8].known[i%8]==(i%8==2?1:i&1),"descriptor reset changed retained bytes/validity");
            for(size_t i=0;i<256;++i)require(queue.records[i/16].raw[i%16]==commands[i]&&queue.records[i/16].known[i%16]==(i%16<4?1:i&1),"command reset changed retained bytes/validity");
            require(queue.initialized&&queue.count==write/16&&queue.current_index==current/16&&queue.counter_80098828==0&&queue.word_80098968==0&&queue.word_800989c8==0&&queue.phase_800989c4==0&&queue.byte_800988d0==0&&queue.byte_80098964==1,"command source initializer globals diverged");
            require(!queue.request_800989b8&&!queue.word_800989bc&&!queue.word_800989c0,"source init manufactured retained request fields");
            ++init_cases;continue;
        }
        unsigned voicecount,count,current,sounds,stops,aftervoice,afterhalf;uint32_t lookupid,pair0,pair1,capture8,capturec;
        std::string descriptors,workdiff,queuediff,voicediff,extra;std::array<std::string,4> seqdiff;
        std::array<unsigned,4> masks,aftermasks;
        require(bool(row>>seed>>voicecount>>count>>current>>lookupid>>pair0>>pair1>>descriptors>>masks[0]>>masks[1]>>masks[2]>>masks[3]>>sounds>>stops>>aftervoice>>aftermasks[0]>>aftermasks[1]>>aftermasks[2]>>aftermasks[3]>>afterhalf>>capture8>>capturec>>workdiff>>queuediff>>voicediff>>seqdiff[0]>>seqdiff[1]>>seqdiff[2]>>seqdiff[3])&&!(row>>extra)&&descriptors.size()==64,"bad phase2 fixture row");
        l3::SourceEventPhase2Context context;context.voice_count_800bc820=static_cast<uint8_t>(voicecount);context.halfword_800b5168=0x4567;
        std::array<int16_t,4> identities{};std::array<uint8_t,4> programs{};std::array<size_t,4> owner_index{};
        std::array<std::array<uint8_t,176>,4> expected_sequences{};
        for(size_t i=0;i<32;++i){context.descriptors[i/8].raw[i%8]=static_cast<uint8_t>(std::stoul(descriptors.substr(i*2,2),nullptr,16));context.descriptors[i/8].known[i%8]=1;}
        for(size_t i=0;i<4;++i){
            const auto& descriptor=context.descriptors[i];identities[i]=static_cast<int16_t>(read16(descriptor.raw.data()));programs[i]=descriptor.raw[2]?descriptor.raw[4]:0;
            size_t owner=i;
            for(size_t prior=0;prior<i;++prior)if(identities[prior]==identities[i]&&programs[prior]==programs[i]){owner=owner_index[prior];break;}
            owner_index[i]=owner;
            if(owner==i){context.sequences[i].id=identities[i];context.sequences[i].program=programs[i];fill(context.sequences[i].raw,seed+10+static_cast<unsigned>(i));context.sequences[i].known.fill(1);}
            expected_sequences[i]=context.sequences[owner].raw;diff(expected_sequences[i],seqdiff[i]);
        }
        l3::SourceEventDriverContext driver;driver.lock_800bc050=1;driver.voice_800c0bfa=0x1234;
        std::array<uint8_t,24*54> expected_voices;fill(expected_voices,seed+20);
        for(size_t voice=0;voice<24;++voice)put16(expected_voices.data()+voice*54+16,static_cast<uint16_t>(uint16_t(identities[voice%4])|(uint16_t(programs[voice%4])<<8)));
        for(size_t i=0;i<expected_voices.size();++i){driver.voices[i/54].raw[i%54]=expected_voices[i];driver.voices[i/54].known[i%54]=1;}
        diff(expected_voices,voicediff);
        for(unsigned half=0;half<2;++half){driver.pending_800c5680[half]=static_cast<uint16_t>(masks[half]);driver.active_8009ae40[half]=static_cast<uint16_t>(masks[half+2]);}
        l3::SourceEventDriverIntentQueue driver_intents;driver_intents.initialized=true;
        // Source allows many duplicate stops; the native bounded queue must
        // hold when that complete transaction exceeds its actual capacity.
        l3::SourceEventCommandQueue commands;commands.initialized=true;commands.count=count;commands.current_index=current;
        std::array<uint8_t,256> expected_queue;fill(expected_queue,seed+30);
        for(size_t i=0;i<16;++i)put32(expected_queue.data()+i*16,i<count?2:0);
        for(size_t i=0;i<256;++i){commands.records[i/16].raw[i%16]=expected_queue[i];commands.records[i/16].known[i%16]=1;}
        diff(expected_queue,queuediff);
        auto work=event(seed+1),expected_work=work;diff(expected_work.raw,workdiff);
        l3::SourceEventPhase2Profile profile{0x20000003,lookupid,{pair0,pair1},true};
        l3::SourceEventPhase2IntentQueue sound_intents;sound_intents.initialized=true;
        const auto before=work;const auto original_context=context;const auto original_commands=commands;const auto original_driver=driver;
        const auto result=l3::consume_source_event1_phase2_8001b33c(work,context,profile,commands,&sound_intents,&driver,&driver_intents);
        if(stops>driver_intents.records.size()){
            require(result.status==Status::IntentQueueFull&&!result.committed&&work.raw==before.raw&&context.sequences[0].raw==original_context.sequences[0].raw&&commands.count==original_commands.count&&driver.voices[0].raw==original_driver.voices[0].raw,"bounded multi-stop transaction partially committed");
            ++cases;++bounded_holds;continue;
        }
        require(result.committed&&result.status==Status::Consumed&&result.sound_receipt_count==sounds&&result.driver_receipt_count==stops&&result.command_receipt_count==1,"phase2 native receipts diverged");
        require(work.raw==expected_work.raw&&commands.count==count+1&&commands.current_index==current,"phase2 event/write/current state diverged");
        require(commands.request_800989b8==0x20000003&&commands.word_800989bc==capture8&&commands.word_800989c0==capturec,"producer current/write alias ordering diverged");
        for(size_t i=0;i<256;++i)require(commands.records[i/16].raw[i%16]==expected_queue[i],"source complete command queue bytes diverged");
        for(size_t i=0;i<4;++i)require(context.sequences[owner_index[i]].raw==expected_sequences[i],"source complete keyed sound sequence bytes diverged");
        for(size_t i=0;i<expected_voices.size();++i)require(driver.voices[i/54].raw[i%54]==expected_voices[i],"source complete voice bytes diverged");
        require(driver.lock_800bc050==1&&driver.voice_800c0bfa==aftervoice&&context.halfword_800b5168==static_cast<int16_t>(afterhalf),"phase2 wrongly applied phase1 lock guard or scalar stores");
        for(unsigned i=0;i<2;++i)require(driver.pending_800c5680[i]==aftermasks[i]&&driver.active_8009ae40[i]==aftermasks[i+2],"phase2 complete pending/active mask operation diverged");
        require(driver_intents.count==stops&&sound_intents.count==sounds,"source stops/sound calls lack native receipts");
        for(size_t i=0;i<driver_intents.count;++i)require(driver_intents.records[i].source_function==0x800742c8&&driver_intents.records[i].source_return==driver_intents.records[i].active[1],"direct phase2 voice stop function/register result diverged");
        ++cases;
    }
    require(cases==128&&init_cases==16&&bounded_holds==8,"source phase2 coverage changed");std::cout<<cases-bounded_holds<<" original phase2 differentials/"<<bounded_holds<<" bounded holds/"<<init_cases<<" source initializer cases PASS; ";
}
void guards(){
    auto work=event();l3::SourceEventPhase2Context context;l3::SourceEventCommandQueue commands;
    std::array<uint8_t,32> descriptor_load{};std::array<uint8_t,256> queue_load{};
    l3::initialize_source_event_phase2_from_executable_boot(context,commands,descriptor_load,queue_load);
    l3::initialize_source_event_phase2_descriptors_80019f0c(context);l3::initialize_source_event_command_queue_8001b44c(commands);
    l3::SourceEventPhase2Profile profile{0x20000003,7,{0x81234567,0x76543210},true};
    l3::SourceEventPhase2IntentQueue sound_intents;sound_intents.initialized=true;
    l3::SourceEventDriverContext driver;l3::SourceEventDriverIntentQueue driver_intents;l3::initialize_source_event_driver_fields(driver,driver_intents);
    const auto held=[&](Status expected){
        const auto before_work=work;const auto before_context=context;const auto before_commands=commands;const auto before_driver=driver;const auto before_sound=sound_intents;const auto before_intents=driver_intents;
        const auto result=l3::consume_source_event1_phase2_8001b33c(work,context,profile,commands,&sound_intents,&driver,&driver_intents);
        require(result.status==expected&&!result.committed&&work.raw==before_work.raw&&work.known==before_work.known&&commands.count==before_commands.count&&commands.request_800989b8==before_commands.request_800989b8&&sound_intents.count==before_sound.count&&driver_intents.count==before_intents.count&&driver.voice_800c0bfa==before_driver.voice_800c0bfa&&driver.pending_800c5680==before_driver.pending_800c5680&&context.halfword_800b5168==before_context.halfword_800b5168,"missing premise published partial source phase2 state");
        for(size_t i=0;i<4;++i)require(context.sequences[i].raw==before_context.sequences[i].raw&&context.sequences[i].known==before_context.sequences[i].known,"held source phase2 altered sound bytes");
        for(size_t i=0;i<16;++i)require(commands.records[i].raw==before_commands.records[i].raw&&commands.records[i].known==before_commands.records[i].known,"held source phase2 altered queue bytes");
    };
    work.known[4]=0;held(Status::NeedsEventWork);work.known[4]=1;
    work.raw[4]=3;held(Status::WrongEventPhase);work.raw[4]=2;
    context.descriptors[1].known[0]=0;held(Status::NeedsDescriptor);context.descriptors[1].known[0]=1;
    profile.initialized=false;held(Status::NeedsProfile);profile.initialized=true;
    profile.request_8001d58c=0;held(Status::UnsupportedRequest);profile.request_8001d58c=0x20000003;
    commands.initialized=false;held(Status::NeedsCommandQueue);commands.initialized=true;
    commands.count=17;held(Status::MalformedCommandQueue);commands.count=15;held(Status::CommandQueueFull);commands.count=0;
    commands.current_index=16;held(Status::MalformedCommandQueue);commands.current_index=1;
    commands.records[1].known[8]=0;held(Status::NeedsCurrentCommandField);commands.current_index=0;
    commands.records[0].known.fill(0); // all producer words become known before alias captures
    auto result=l3::consume_source_event1_phase2_8001b33c(work,context,profile,commands);
    require(result.committed&&work.raw[4]==3&&commands.count==1&&commands.word_800989bc==profile.pair_words[0]&&commands.word_800989c0==profile.pair_words[1],"boot source descriptor skips require fabricated audio owner or pre-write current words");
    const auto request=l3::source_event_command_front_request(commands);
    require(request&&request->source_function==0x8001b89c&&request->words[0]==4&&request->words[1]==profile.id,"source type4 command dispatch missing");
    work=event();commands.count=0;context.descriptors[0].raw[2]=0;
    held(Status::NeedsSequenceOwner);context.sequences[0].id=0;context.sequences[0].program=0;
    held(Status::NeedsSequenceField);
    context.sequences[0].known.fill(1);held(Status::NeedsVoiceCount);context.voice_count_800bc820=25;
    held(Status::UnsupportedVoiceCount);context.voice_count_800bc820=1;held(Status::NeedsDriverField);
    driver.voices[0].known[16]=driver.voices[0].known[17]=1;
    sound_intents.initialized=false;held(Status::NeedsIntentQueue);sound_intents.initialized=true;
    sound_intents.count=5;held(Status::MalformedIntentQueue);sound_intents.count=4;held(Status::IntentQueueFull);sound_intents.count=0;
    driver_intents.count=24;held(Status::IntentQueueFull);driver_intents.count=0;
    driver.pending_800c5680[0].reset();held(Status::NeedsDriverField);driver.pending_800c5680[0]=0;
    // Direct742C8 does not read lock800BC050, so an unknown lock is admissible.
    driver.lock_800bc050.reset();
    context.sequences[0].known.fill(0);
    for(const auto range:{std::array<size_t,2>{0x98,4},{0x84,4},{0x8c,4},{0x56,2},{4,4}})
        for(size_t i=range[0];i<range[0]+range[1];++i)context.sequences[0].known[i]=1;
    const auto prior_sequence_known=context.sequences[0].known;
    std::optional<l3::SourceEventWork> pipeline=work;l3::SourceEventInputs inputs;
    inputs.phase2_context=&context;inputs.phase2_profile=&profile;inputs.command_queue=&commands;
    inputs.phase2_intents=&sound_intents;inputs.driver=&driver;inputs.driver_intents=&driver_intents;
    const auto step=l3::step_source_event_pipeline(pipeline,nullptr,inputs);
    require(step.committed&&step.phase2_consumer_status==Status::Consumed&&step.command_receipt_count==1&&step.sound_receipt_count==1&&step.driver_receipt_count==1&&pipeline->raw[4]==3&&!step.request,"source phase2 pipeline receipts disconnected");
    require(!driver.lock_800bc050,"phase2 manufactured lock800BC050");
    for(size_t i=0;i<context.sequences[0].known.size();++i){
        const bool stored=i<4||(i>=8&&i<12)||(i>=20&&i<=31)||i==33||(i>=39&&i<=70)||(i>=84&&i<86)||(i>=92&&i<128)||(i>=136&&i<140)||(i>=144&&i<156);
        require(context.sequences[0].known[i]==(stored?1:prior_sequence_known[i]),"phase2 promoted a retained sequence field outside source stores");
    }
    for(size_t i=0;i<54;++i){
        const bool stored=i==0||i==1||i==4||i==5||i==0x1d;
        require(driver.voices[0].known[i]==(stored||i==16||i==17?1:0),"direct742C8 promoted a retained voice field");
    }
    context.descriptors[0].raw[2]=255;
    const auto before_allocations=allocations;bool ok=true;
    for(unsigned i=0;i<4096;++i){work=event();commands.count=0;const auto out=l3::consume_source_event1_phase2_8001b33c(work,context,profile,commands);ok&=out.committed&&work.raw[4]==3;}
    require(ok&&allocations==before_allocations,"source phase2 allocates in hot path");
    std::cout<<"atomic holds/producer alias/order/bounded dispatch/pipeline/4096 no-allocation cases PASS\n";
}
}
int main(int argc,char** argv){
    try{require(argc==2,"usage: test_source_event_phase2 source_event_phase2_01423.tsv");differential(argv[1]);guards();return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
