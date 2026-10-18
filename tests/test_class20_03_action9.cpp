#include "legends3ds/class20_03_action9.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

static size_t allocation_count=0;
void* operator new(std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using State=l3::Class20_03CallbackState;
using Status=l3::Class20_03Action9Status;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
std::array<int64_t,36> values(const State& s){
    const auto& b=s.behavior;
    std::array<int64_t,36> out{{b.parameter_03,b.action_09,b.phase_0a,s.aux_0b,s.restore_action_0e,
        b.animation_ac,b.flags_328,b.timer_32c,b.work_330,b.heading_56,b.turn_66,b.work_36c,
        s.work_36e,s.work_35c,s.work_35e,s.work_370}};
    size_t cursor=16;
    const auto add=[&](const auto& a){for(auto v:a)out[cursor++]=v;};
    add(s.raw_14);add(s.fixed_1c);add(s.previous_raw_2c);add(s.previous_fixed_34);add(s.angle_54);
    return out;
}
int64_t number(std::istream& row){int64_t n;require(bool(row>>n),"truncated action9 fixture");return n;}
State read_state(std::istream& row){
    State s;auto& b=s.behavior;
    const auto r=[&](auto& f){f=static_cast<std::decay_t<decltype(f)>>(number(row));};
    r(b.parameter_03);r(b.action_09);r(b.phase_0a);r(s.aux_0b);r(s.restore_action_0e);
    r(b.animation_ac);r(b.flags_328);r(b.timer_32c);r(b.work_330);r(b.heading_56);r(b.turn_66);
    r(b.work_36c);r(s.work_36e);r(s.work_35c);r(s.work_35e);r(s.work_370);
    const auto arr=[&](auto& a){for(auto& v:a)r(v);};
    arr(s.raw_14);arr(s.fixed_1c);arr(s.previous_raw_2c);arr(s.previous_fixed_34);arr(s.angle_54);return s;
}
void guards(){
    State s;s.behavior.action_09=9;s.behavior.phase_0a=0;s.behavior.flags_328=16;
    s.behavior.timer_32c=19;s.behavior.animation_ac=22;s.work_36e=123;s.aux_0b=91;
    l3::Class20_03Action9Inputs in;
    const auto before=values(s);
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsPlayerToActorAngle,"invented player-to-actor angle");
    in.target_angle_player_to_actor_80031198=0;
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsPlayerHeading,"invented player heading");
    in.player_heading_800b5206=65535;
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsWork80,"invented distance");
    require(values(s)==before,"missing dependency leaked always-written heading difference or phase0 stores");
    in.work_80=0x480001;
    require(l3::class20_03_step_action9(s,in).status==Status::SourceBackedPartial &&
        s.work_36e==1 && s.behavior.action_09==3 && s.behavior.phase_0a==0 && s.aux_0b==0 &&
        s.behavior.animation_ac==1 && s.behavior.timer_32c==90 && s.behavior.work_330==0,
        "far-detected phase0 route/fallthrough or unsigned difference changed");
    s.behavior.action_09=9;s.behavior.phase_0a=2;in.work_80.reset();
    require(l3::class20_03_step_action9(s,in).status==Status::SourceBackedPartial,
        "inactive phase unnecessarily required distance/motion");
    s.behavior.phase_0a=1;in.work_80=0x480000;
    const auto active=values(s);
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsTurnTarget,"first angle substituted for turn target");
    in.turn_target_actor_to_player_80031198=0;
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsMotion,"invented forward52 motion");
    l3::Class20_03PreparedAction9 m;m.heading_after_turn=1;in.prepared_motion=m;
    require(l3::class20_03_step_action9(s,in).status==Status::StaleMotion,"wrong source turn heading accepted");
    m.heading_after_turn=0;in.prepared_motion=m;
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsAnimationCue,"unknown cue defaulted to zero");
    m.source_a6=1;in.prepared_motion=m;
    require(l3::class20_03_step_action9(s,in).status==Status::NeedsSoundConsumer,"sound consumer omitted");
    require(values(s)==active,"prepared body dependency hold leaked state");
    in.sound_consumer_ready=true;
    m.raw_14={{111,-7,-212,1234}};m.fixed_1c={{1,2,3,4}};m.source_floor_word=26;
    in.prepared_motion=m;s.previous_raw_2c={{81,-7,-182,4321}};s.previous_fixed_34={{5,6,7,8}};
    const auto sound=l3::class20_03_step_action9(s,in);
    require(sound.status==Status::SourceBackedPartial && sound.sound_8001a050==176 && sound.sound_pointed_words &&
        *sound.sound_pointed_words==std::array<uint32_t,2>{{0xfff9006fu,0x04d2ff2cu}},
        "action9 sound did not capture post-wall point before floor/rollback");
    require(s.behavior.action_09==1 && s.behavior.phase_0a==0 && s.aux_0b==0 && s.restore_action_0e==4 &&
        s.raw_14==s.previous_raw_2c && s.fixed_1c==s.previous_fixed_34,
        "source floor33 rollback failed full fourth-lane copies");
    State initial;initial.behavior.action_09=9;initial.behavior.flags_328=16;
    l3::Class20_03Action9Inputs entry;entry.work_80=0x480001;
    entry.target_angle_player_to_actor_80031198=65535;entry.player_heading_800b5206=0;
    const auto allocations=allocation_count;bool ok=true;
    for(int i=0;i<640;++i){auto copy=initial;ok&=l3::class20_03_step_action9(copy,entry).status==Status::SourceBackedPartial;}
    require(ok && allocation_count==allocations,"action9 reducer hot loop allocated");
}
void fixtures(const char* path){
    std::ifstream file(path);require(bool(file),"action9 source fixture unavailable");std::string line;std::getline(file,line);
    size_t count=0,cues=0;
    while(std::getline(file,line)){
        if(line.empty())continue;std::istringstream row(line);
        l3::Class20_03Action9Inputs in;in.work_80=static_cast<uint32_t>(number(row));
        in.target_angle_player_to_actor_80031198=static_cast<uint16_t>(number(row));
        in.player_heading_800b5206=static_cast<uint16_t>(number(row));
        in.turn_target_actor_to_player_80031198=static_cast<uint16_t>(number(row));
        const auto cue=static_cast<int8_t>(number(row));const auto present=number(row);
        l3::Class20_03PreparedAction9 m;
        for(auto& v:m.raw_14)v=static_cast<int16_t>(number(row));
        for(auto& v:m.fixed_1c)v=static_cast<int32_t>(number(row));
        m.source_floor_word=static_cast<int32_t>(number(row));m.source_contact_word=static_cast<uint32_t>(number(row));
        m.heading_after_turn=static_cast<uint16_t>(number(row));m.source_a6=cue;
        if(present)in.prepared_motion=m;in.sound_consumer_ready=true;
        auto state=read_state(row);const auto expected=read_state(row);
        const auto sound=number(row);const std::array<uint32_t,2> words{{static_cast<uint32_t>(number(row)),static_cast<uint32_t>(number(row))}};
        std::string extra;require(!(row>>extra),"extra action9 source fixture fields");
        const auto result=l3::class20_03_step_action9(state,in);
        if(result.status!=Status::SourceBackedPartial || values(state)!=values(expected)){
            const auto got=values(state),want=values(expected);
            std::cerr<<"case "<<count<<" status "<<int(result.status)<<'\n';
            for(size_t i=0;i<got.size();++i)if(got[i]!=want[i])std::cerr<<"field "<<i<<" got "<<got[i]<<" expected "<<want[i]<<'\n';
            throw std::runtime_error("original action9/native reducer mismatch");
        }
        require((result.sound_8001a050?*result.sound_8001a050:-1)==sound,"original action9 cue equality changed");
        if(sound>=0){require(result.sound_pointed_words && *result.sound_pointed_words==words,"original action9 pointed position mismatch");++cues;}
        else require(!result.sound_pointed_words,"no-cue result published sound position");
        ++count;
    }
    require(count>2000 && cues>100,"insufficient original action9 branch/cue coverage");
    std::cout<<"original action9 reducer cases="<<count<<" cues="<<cues<<'\n';
}
}
int main(int argc,char** argv){try{guards();if(argc>1)fixtures(argv[1]);
    std::cout<<"PASS action9 strong dependency holds and640 allocation-free entries\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
