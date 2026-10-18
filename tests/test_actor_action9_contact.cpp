#include "legends3ds/actor_action9_contact.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/map_initialization.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t allocation_count=0;
void* operator new(std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::ActorAction9ContactStatus;
constexpr l3::RetailPlayerBounds bounds{-60,60,-60,60,-128,0};
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
l3::RuntimeMap rectangle(uint16_t code=3,uint16_t flags=0,int16_t raw6=0){
    l3::RuntimeMap m;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
    m.areas.push_back(a);m.cells.assign(16,1);m.idx.resize(2);m.idx[1].descriptor_index=0;m.idx[1].raw6=raw6;
    l3::RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,1});m.contact_groups.push_back({-100,100,-100,100,0,1});
    m.contact_primitives.push_back({-128,128,code,flags});return m;
}
l3::Class20_03CallbackState callback(const l3::ActorContactWork& w,uint8_t phase,uint16_t flags,uint16_t heading){
    l3::Class20_03CallbackState s;s.behavior.action_09=9;s.behavior.phase_0a=phase;s.behavior.flags_328=flags;
    s.behavior.heading_56=heading;s.angle_54[1]=heading;s.raw_14=w.raw;s.fixed_1c=w.fixed;
    s.previous_raw_2c=w.previous_raw;s.previous_fixed_34=w.previous_fixed;return s;
}
void guards(){
    auto m=rectangle();l3::ActorContactWork w;w.raw={{0,-128,0,1234}};
    for(size_t i=0;i<4;++i)w.fixed[i]=int32_t(w.raw[i])*65536+(i==1?0:12345);
    w.previous_raw=w.raw;w.previous_fixed=w.fixed;w.velocity={{321,-654,987}};w.state74=1;
    auto s=callback(w,0,16,0);l3::ReferenceTables refs;refs.trig.fill({0,4096});
    auto p=l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x12000,0,0);
    require(p.status==Status::SourceBackedPartial && p.inputs && p.inputs->raw_14[2]==-3 &&
        p.inputs->fixed_1c[2]==w.fixed[2]-4096*52 && p.inputs->source_floor_word==-128 &&
        p.source_turn==0 && p.inputs->source_a6==0,"distinct source forward52 accumulator/rounding failed");
    require(s.raw_14==w.raw && s.fixed_1c==w.fixed && s.behavior.phase_0a==0,"preparation mutated live state");
    require(p.before.velocity==w.velocity && p.before.state74==1 && p.initial_phase==0 && p.initial_flags==16 &&
        p.work80==0x12000 && p.turn_target==0,"stale preparation snapshot omitted actor/contact fields");
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,std::nullopt,0,0).status==Status::NeedsWork80,"distance defaulted");
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x12000,std::nullopt,0).status==Status::NeedsTurnTarget,"turn angle defaulted");
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x12000,0,std::nullopt).status==Status::NeedsAnimationCue,"cue defaulted");
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x480001,0,0).status==Status::NotMovingBranch,"far detector route moved actor");
    auto stale=w;++stale.previous_raw[3];
    require(l3::prepare_actor_action9_contact(s,stale,m,0,refs,bounds,0x12000,0,0).status==Status::StaleWork,"stale fourth snapshot lane accepted");
    s.behavior.phase_0a=2;
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,std::nullopt,std::nullopt,std::nullopt).status==Status::NotMovingBranch,"inactive phase required unused dependencies");
    s.behavior.phase_0a=1;m=rectangle(8);
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x12000,0,0).status==Status::UnsupportedWall,"unsupported motion contact admitted");
    m=rectangle();m.cells.clear();
    require(l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x12000,0,0).status==Status::UnsupportedBoundary,"malformed map admitted");
    m=rectangle();const auto allocations=allocation_count;bool ready=true;
    for(int i=0;i<640;++i)ready&=l3::prepare_actor_action9_contact(s,w,m,0,refs,bounds,0x12000,0,0).status==Status::SourceBackedPartial;
    require(ready && allocations==allocation_count,"action9 preparation hot loop allocated");
}
int64_t number(std::istream& row){int64_t n;require(bool(row>>n),"truncated action9 contact fixture");return n;}
struct Case {
    uint8_t phase=0;uint16_t flags=0,heading=0,target=0;uint32_t work80=0;int8_t cue=0;
    l3::RetailPlayerBounds bounds;l3::ActorContactWork before,after;
    uint32_t wall=0;int32_t floor=0;uint16_t moved_heading=0;int16_t turn=0;
    int64_t sound=-1;std::array<uint32_t,2> soundwords{};
};
void prefix(std::istream& row,Case& c){
    c.phase=static_cast<uint8_t>(number(row));c.flags=static_cast<uint16_t>(number(row));c.work80=static_cast<uint32_t>(number(row));
    c.heading=static_cast<uint16_t>(number(row));c.target=static_cast<uint16_t>(number(row));c.cue=static_cast<int8_t>(number(row));
}
void tail(std::istream& row,Case& c){
    c.bounds.x0=static_cast<int16_t>(number(row));c.bounds.x1=static_cast<int16_t>(number(row));
    c.bounds.z0=static_cast<int16_t>(number(row));c.bounds.z1=static_cast<int16_t>(number(row));
    c.bounds.y0=static_cast<int16_t>(number(row));c.bounds.y1=static_cast<int16_t>(number(row));
    for(auto& v:c.before.raw)v=static_cast<int16_t>(number(row));
    for(auto& v:c.before.fixed)v=static_cast<int32_t>(number(row));
    for(auto& v:c.before.velocity)v=static_cast<int16_t>(number(row));
    c.before.state74=static_cast<uint8_t>(number(row));c.before.previous_raw=c.before.raw;c.before.previous_fixed=c.before.fixed;
    c.wall=static_cast<uint32_t>(number(row));c.floor=static_cast<int32_t>(number(row));c.after=c.before;
    for(auto& v:c.after.raw)v=static_cast<int16_t>(number(row));
    for(auto& v:c.after.fixed)v=static_cast<int32_t>(number(row));
    for(auto& v:c.after.velocity)v=static_cast<int16_t>(number(row));
    c.after.state74=static_cast<uint8_t>(number(row));c.moved_heading=static_cast<uint16_t>(number(row));c.turn=static_cast<int16_t>(number(row));
    c.sound=number(row);for(auto& v:c.soundwords)v=static_cast<uint32_t>(number(row));
    std::string extra;require(!(row>>extra),"extra source contact fixture field");
}
void compare(const l3::ActorAction9ContactPreparation& p,const Case& c){
    require(p.inputs.has_value(),"supported action9 preparation omitted inputs");
    require(uint32_t(p.wall.result)==c.wall && p.floor.result==c.floor && p.after_wall.raw==c.after.raw &&
        p.after_wall.fixed==c.after.fixed && p.after_wall.velocity==c.after.velocity && p.after_wall.state74==c.after.state74 &&
        p.inputs->heading_after_turn==c.moved_heading && p.source_turn==c.turn && p.inputs->source_a6==c.cue,
        "original source turn/motion52/wall/floor preparation mismatch");
    if(c.sound>=0){
        const auto& raw=p.after_wall.raw;
        const std::array<uint32_t,2> words{{uint32_t(uint16_t(raw[0]))|(uint32_t(uint16_t(raw[1]))<<16),
            uint32_t(uint16_t(raw[2]))|(uint32_t(uint16_t(raw[3]))<<16)}};
        require(c.sound==176 && words==c.soundwords,"original176 producer did not copy post-wall point");
    }
}
void authored(const char* path){
    std::ifstream f(path);require(bool(f),"action9 contact authored fixture unavailable");std::string line;std::getline(f,line);
    size_t supported=0,held=0,corrected=0,cues=0;
    while(std::getline(f,line)){
        if(line.empty())continue;std::istringstream row(line);Case c;prefix(row,c);
        const auto sin=static_cast<int16_t>(number(row)),cos=static_cast<int16_t>(number(row));
        const auto code=static_cast<uint16_t>(number(row)),flags=static_cast<uint16_t>(number(row));
        const auto raw6=static_cast<int16_t>(number(row));tail(row,c);
        auto map=rectangle(code,flags,raw6);const auto s=callback(c.before,c.phase,c.flags,c.heading);
        l3::ReferenceTables refs;refs.trig.fill({sin,cos});
        const auto p=l3::prepare_actor_action9_contact(s,c.before,map,0,refs,c.bounds,c.work80,c.target,c.cue);
        if(p.status==Status::SourceBackedPartial){
            try{compare(p,c);}catch(...){std::cerr<<"authored case "<<supported+held<<" code "<<code<<'\n';throw;}
            ++supported;corrected+=c.wall!=0;cues+=c.sound>=0;
        }else{require(!p.inputs,"held action9 preparation published motion");++held;}
    }
    require(supported>100 && held>0 && corrected>10 && cues>50,"insufficient source52 contact coverage");
    std::cout<<"original action9 contact supported="<<supported<<" held="<<held<<" corrected="<<corrected<<" cues="<<cues<<'\n';
}
void owner(const char* path,const char* owner_pack){
    const std::string pack(owner_pack);const auto mb=l3::read_file(pack+"/st00.l3map"),rb=l3::read_file(pack+"/st00.l3render");
    auto map=l3::load_l3map(mb);const auto render=l3::load_l3render(rb);
    const auto init=l3::load_map_initialization(l3::read_file(pack+"/st00.l3init"));
    l3::apply_map_initialization(map,render,init,l3::crc32(mb.data(),mb.size()),l3::crc32(rb.data(),rb.size()));
    const auto refs=l3::load_l3reference(l3::read_file(pack+"/reference.l3ref"));
    std::ifstream f(path);require(bool(f),"owner action9 fixture unavailable");std::string line;std::getline(f,line);size_t supported=0,held=0;
    while(std::getline(f,line)){
        if(line.empty())continue;std::istringstream row(line);Case c;prefix(row,c);tail(row,c);
        const auto s=callback(c.before,c.phase,c.flags,c.heading);
        const auto p=l3::prepare_actor_action9_contact(s,c.before,map,1,refs,c.bounds,c.work80,c.target,c.cue);
        if(p.status==Status::SourceBackedPartial){compare(p,c);++supported;}
        else{require(!p.inputs,"held owner motion published inputs");++held;}
    }
    require(supported>0 && supported+held==24,"missing owner contact52 coverage");
    std::cout<<"materialized source action9 contact supported="<<supported<<" held="<<held<<'\n';
}
}
int main(int argc,char** argv){try{guards();if(argc>1)authored(argv[1]);if(argc>3)owner(argv[2],argv[3]);
    std::cout<<"PASS distinct action9 contact and640 allocation-free preparations\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
