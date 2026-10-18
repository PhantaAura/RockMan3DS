#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::SourceEventStepStatus;
constexpr std::array<uint16_t,3> flags{{23,1540,2047}};
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
void write16(std::vector<uint8_t>& v,size_t offset,uint16_t n){v[offset]=uint8_t(n);v[offset+1]=uint8_t(n>>8);}
void write32(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=uint8_t(n>>(i*8));}
void write32(std::vector<uint8_t>& v,size_t offset,uint32_t n){write32(v.data()+offset,n);}
unsigned digit(char c){if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw std::runtime_error("invalid authored fixture hex");}
template<size_t N>std::array<uint8_t,N> bytes_from_hex(const std::string& text){
    require(text.size()==N*2,"truncated authored initialization fixture");
    std::array<uint8_t,N> out{};
    for(size_t i=0;i<N;++i)out[i]=uint8_t(digit(text[i*2])*16+digit(text[i*2+1]));
    return out;
}
l3::SourceEventWork from_hex(const std::string& s){
    require(s.size()==l3::kSourceEventWorkBytes*2,"truncated authored work fixture");
    l3::SourceEventWork work;
    for(size_t i=0;i<work.raw.size();++i)work.raw[i]=uint8_t(digit(s[i*2])*16+digit(s[i*2+1]));
    return l3::source_event_work_from_original(work.raw);
}
l3::SourceEventPollProfile profile(){
    l3::SourceEventPollProfile p;p.decoder=1;p.source_stage=0;
    for(size_t i=0;i<flags.size();++i)p.records.push_back({flags[i],1,uint8_t(0xa0+i)});
    return p;
}
l3::SourceEventFlagSnapshot snapshot(unsigned mask){
    l3::SourceEventFlagSnapshot s;s.known.fill(255);
    for(size_t i=0;i<flags.size();++i)if(mask&(1u<<i))s.values[flags[i]>>3]|=uint8_t(1u<<(flags[i]&7u));
    return s;
}
void malformed_profiles(){
    const std::string digest="9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c";
    std::vector<uint8_t> bytes(76);bytes[0]='L';bytes[1]='3';bytes[2]='E';bytes[3]='P';
    write16(bytes,4,1);write16(bytes,6,64);write32(bytes,8,1);write32(bytes,12,0);
    write32(bytes,16,3);write32(bytes,20,uint32_t(bytes.size()));write32(bytes,28,4);
    for(size_t i=0;i<32;++i)bytes[32+i]=uint8_t(digit(digest[i*2])*16+digit(digest[i*2+1]));
    for(size_t i=0;i<flags.size();++i){write16(bytes,64+i*4,flags[i]);bytes[66+i*4]=1;bytes[67+i*4]=uint8_t(0xa0+i);}
    write32(bytes,24,l3::crc32(bytes.data()+64,bytes.size()-64));
    const auto p=l3::load_source_event_poll_profile(bytes);
    require(p.records.size()==3&&p.records[1].flag==1540&&p.records[1].source_byte03==0xa1,"neutral authored record/reserved-byte decoding failed");
    for(unsigned mutation=0;mutation<11;++mutation){
        auto bad=bytes;
        switch(mutation){case 0:bad.resize(63);break;case 1:bad[0]^=1;break;case 2:bad[4]=2;break;
            case 3:bad[6]=60;break;case 4:bad[8]=2;break;case 5:bad[12]=1;break;
            case 6:write32(bad,16,257);break;case 7:bad.push_back(0);break;case 8:bad[32]^=1;break;
            case 9:bad[66]^=1;break;default:write16(bad,64,2048);write32(bad,24,l3::crc32(bad.data()+64,bad.size()-64));break;}
        bool failed=false;try{(void)l3::load_source_event_poll_profile(bad);}catch(const std::runtime_error&){failed=true;}
        require(failed,"malformed/budget/profile/CRC/flag source event package accepted");
    }
}
void guards(){
    auto p=profile();auto flagbank=snapshot(3);l3::SourceEventInputs input;
    input.flags=&flagbank;input.stage_800c1b60=0;input.transition_status_800c356c=0;
    input.player_hp_800b521e=1;input.busy_80098a72=0;
    std::optional<l3::SourceEventWork> work;
    require(l3::step_source_event_pipeline(work,&p,input).status==Status::NeedsInitialWork,"zero NewGame work was manufactured");
    work.emplace();
    require(l3::step_source_event_pipeline(work,&p,input).status==Status::NeedsInitialWork,"zero-initialized raw bytes promoted to source evidence");
    l3::reset_source_event_work_8001d928(*work);const auto before=work->raw;
    for(unsigned missing=0;missing<6;++missing){
        auto in=input;
        switch(missing){case 0:in.transition_status_800c356c.reset();break;case 1:in.stage_800c1b60.reset();break;
            case 2:in.flags=nullptr;break;case 3:in.player_hp_800b521e.reset();break;case 4:in.busy_80098a72.reset();break;
            default:in.stage_800c1b60=1;break;}
        const std::array<Status,6> expected{{Status::NeedsTransitionStatus,Status::NeedsStage,Status::NeedsInitialStoryFlags,
            Status::NeedsPlayerHealth,Status::NeedsBusyHalfword,Status::UnsupportedStageProfile}};
        const auto r=l3::step_source_event_pipeline(work,&p,in);
        require(r.status==expected[missing]&&!r.committed&&work->raw==before,"missing source premise changed event work");
    }
    require(l3::step_source_event_pipeline(work,nullptr,input).status==Status::NeedsPollProfile,"missing source table invented");
    auto unknown=flagbank;unknown.known[flags[0]>>3]&=uint8_t(~(1u<<(flags[0]&7u)));
    auto in=input;in.flags=&unknown;
    require(l3::step_source_event_pipeline(work,&p,in).status==Status::NeedsInitialStoryFlags,"unknown first flag skipped to later true flag");
    in=input;in.player_hp_800b521e=-1;in.busy_80098a72.reset();
    auto rejected=l3::step_source_event_pipeline(work,&p,in);
    require(rejected.status==Status::SourceBackedPartial&&rejected.initializer_attempted&&!rejected.initialization_prepared&&
        rejected.selected_flag==flags[0]&&!rejected.committed&&work->raw==before,"negative HP invented event or polled a later flag");
    auto actual=l3::step_source_event_pipeline(work,&p,input);
    require(actual.committed&&actual.initialization_prepared&&actual.handler_selected&&work->active00()==1&&
        work->event01()==1&&work->phase04()==1,"first proved flag did not initialize and run phase0 in same source update");
    const auto phase1=work->raw;
    actual=l3::step_source_event_pipeline(work,nullptr,{});
    require(actual.status==Status::NeedsPlayerConsumer80042238&&actual.request&&
        actual.request->source_function==0x80042238u&&!actual.committed&&work->raw==phase1,"player-state7 consumer acknowledged without implementation");
    for(uint8_t phase=2;phase<8;++phase){
        work->raw=phase1;work->raw[4]=phase;write32(work->raw.data()+0x0c,1);
        const auto current=work->raw;actual=l3::step_source_event_pipeline(work,nullptr,{});
        require(actual.status==Status::NeedsSourceConsumer&&actual.request&&!actual.committed&&work->raw==current,
            "event command/fade/countdown expiry or phase7 transition bypassed consumer");
    }
    work->raw=phase1;work->raw[4]=255;
    require(l3::step_source_event_pipeline(work,nullptr,{}).status==Status::UnknownPhase,"unknown phase invented");
    work->raw[1]=2;require(l3::step_source_event_pipeline(work,nullptr,{}).status==Status::UnknownHandler,"untranslated event handler invented");
    std::array<uint8_t,256> sourceflags{};
    for(unsigned id=0;id<2048;++id){
        sourceflags.fill(0);sourceflags[id>>3]=uint8_t(0x80u>>(id&7u));
        const auto converted=l3::source_event_flags_from_original(sourceflags);
        require(converted.values[id>>3]==uint8_t(1u<<(id&7u))&&converted.known[id>>3]==255,
            "original MSB story flag layout copied into native LSB semantics");
    }
    work->raw.fill(0);work->known.fill(1);work->raw[0]=1;work->raw[1]=1;work->raw[4]=0;
    l3::SourceEventInputs busy;busy.busy_80098a72=1;
    const auto allocation_start=allocations;bool held=true;
    for(unsigned i=0;i<4096;++i){const auto r=l3::step_source_event_pipeline(work,nullptr,busy);held&=r.committed&&r.source_busy&&work->phase04()==0;}
    require(held&&allocations==allocation_start,"source event hot loop allocated or skipped nonzero busy");
}
void original_differential(const char* path){
    std::ifstream file(path);require(bool(file),"missing original event differential fixture");
    std::string line;std::getline(file,line);size_t count=0;auto p=profile();
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::istringstream row(line);int transition,hp,busy;unsigned mask;int selected;
        std::string before,after,extra;
        require(bool(row>>transition>>hp>>busy>>mask>>selected>>before>>after)&&!(row>>extra),"invalid original event fixture row");
        std::optional<l3::SourceEventWork> work=from_hex(before);const auto expected=from_hex(after);
        auto values=snapshot(mask);l3::SourceEventInputs input;
        input.stage_800c1b60=0;input.transition_status_800c356c=static_cast<int8_t>(transition);
        input.player_hp_800b521e=static_cast<int16_t>(hp);input.busy_80098a72=static_cast<int16_t>(busy);input.flags=&values;
        const auto result=l3::step_source_event_pipeline(work,&p,input);
        require(result.status==Status::SourceBackedPartial&&work->raw==expected.raw,
            "original dispatcher/poll/init/phase0/pure phase4 work bytes diverged");
        require((selected<0&&!result.selected_flag)||(selected>=0&&result.selected_flag==unsigned(selected)),
            "original first-enabled flag order diverged");
        ++count;
    }
    require(count==336,"original source event fixture coverage changed");
    std::cout<<count<<" original dispatcher/event differential cases PASS; ";
}
void original_initialization_differential(const char* path){
    const std::string original(path);
    const auto slash=original.find_last_of("/\\");
    const auto fixture=(slash==std::string::npos?std::string{}:original.substr(0,slash+1))+"source_event_initialization_01422.tsv";
    std::ifstream file(fixture);require(bool(file),"missing original event initialization fixture");
    std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::istringstream row(line);std::string kind,before,after,flags_before,flags_after,global,extra;int selector;
        require(bool(row>>kind>>selector>>before>>after>>flags_before>>flags_after>>global)&&!(row>>extra),"invalid original initialization fixture row");
        if(kind=="flags"){
            const auto expected=l3::source_event_flags_from_original(bytes_from_hex<256>(after));
            const auto actual=l3::initialize_source_story_flags_8001d8c0();
            require(actual.values==expected.values&&actual.known==expected.known,"original fresh bank-resident defaults or1216 diverged");
        }else if(kind=="reset"){
            l3::SourceEventWork work;work.raw=bytes_from_hex<168>(before);
            for(size_t i=4;i<work.known.size();++i)work.known[i]=uint8_t(i&1);
            const auto retained_known=work.known;
            l3::reset_source_event_work_8001d928(work);
            require(work.raw==bytes_from_hex<168>(after),"original scene reset clobbered retained work tail");
            for(size_t i=0;i<work.known.size();++i)
                require(work.known[i]==(i<4?1:retained_known[i]),"original scene reset manufactured known retained tail");
        }else if(kind=="area"){
            L3PlayerCompatWork player;
            const auto authored=bytes_from_hex<L3_PLAYER_COMPAT_WORK_BYTES>(before);
            const auto expected=bytes_from_hex<L3_PLAYER_COMPAT_WORK_BYTES>(after);
            std::copy(authored.begin(),authored.end(),player.bytes);
            auto flags=l3::source_event_flags_from_original(bytes_from_hex<256>(flags_before));
            const auto expected_flags=l3::source_event_flags_from_original(bytes_from_hex<256>(flags_after));
            const auto result=l3::apply_source_st00_area_event_prefix(0,static_cast<int8_t>(selector),flags,player);
            require(result.committed&&std::equal(expected.begin(),expected.end(),player.bytes)&&flags.values==expected_flags.values&&flags.known==expected_flags.known,
                "original ST00 area initialization player/event flag bytes diverged");
            require(selector==0?(result.store_8009888c==uint32_t(std::stoul(global))):!result.store_8009888c,
                "original opaque area0 global store diverged");
        }else throw std::runtime_error("unknown initialization fixture kind");
        ++count;
    }
    require(count==58,"original event initialization fixture coverage changed");
    L3PlayerCompatWork player{};auto flags=l3::initialize_source_story_flags_8001d8c0();
    require(!l3::apply_source_st00_area_event_prefix(1,0,flags,player).committed&&
        !l3::apply_source_st00_area_event_prefix(0,-1,flags,player).committed&&
        !l3::apply_source_st00_area_event_prefix(0,3,flags,player).committed,"unknown stage/area lifecycle invented");
    auto p=profile();l3::SourceEventInputs input;input.flags=&flags;input.stage_800c1b60=0;input.transition_status_800c356c=0;
    input.player_hp_800b521e=1;input.busy_80098a72=0;
    std::optional<l3::SourceEventWork> work;work.emplace();l3::reset_source_event_work_8001d928(*work);
    require(l3::apply_source_st00_area_event_prefix(0,0,flags,player).committed,"source area0 lifecycle held");
    const auto result=l3::step_source_event_pipeline(work,&p,input);
    require(result.committed&&result.selected_flag==1540&&work->phase04()==1,"source fresh flags→source area0→original event prefix disconnected");
    require(work->known[4]&&work->known[0x0c]&&!work->known[0x20]&&!work->known[0x64],"event initializer manufactured known source tail");
    work->known[4]=0;
    require(l3::step_source_event_pipeline(work,nullptr,{}).status==Status::NeedsInitialWork,"unobserved retained event phase used");
    std::cout<<count<<" original initialization differential cases PASS; ";
}
}
int main(int argc,char** argv){
    try{require(argc==2,"usage: test_source_event_pipeline source_event_pipeline_01422.tsv");malformed_profiles();guards();original_differential(argv[1]);original_initialization_differential(argv[1]);
        std::cout<<"malformed/missing-premise/consumer/2048 flag mapping/4096 zero-allocation tests PASS; no full cutscene parity asserted\n";return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
