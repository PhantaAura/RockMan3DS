#include "legends3ds/actor_action4_contact.h"
#include <cstdlib>
#include <new>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::ActorAction4ContactStatus;
constexpr l3::RetailPlayerBounds bounds{-60,60,-60,60,-128,0};
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
l3::RuntimeMap rectangle(uint16_t code=3,uint16_t flags=0,int16_t raw6=0){
    l3::RuntimeMap m;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
    m.areas.push_back(a);m.cells.assign(16,1);m.idx.resize(2);m.idx[1].descriptor_index=0;m.idx[1].raw6=raw6;
    l3::RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,1});m.contact_groups.push_back({-100,100,-100,100,0,1});
    m.contact_primitives.push_back({-128,128,code,flags});return m;
}

l3::Class20_03CallbackState callback(const l3::ActorContactWork& w,uint8_t phase,int16_t timer,uint16_t heading){
 l3::Class20_03CallbackState s;s.behavior.action_09=4;s.behavior.phase_0a=phase;s.behavior.timer_32c=timer;
 s.behavior.heading_56=heading;s.behavior.animation_ac=21;s.raw_14=w.raw;s.fixed_1c=w.fixed;
 s.previous_raw_2c=w.previous_raw;s.previous_fixed_34=w.previous_fixed;return s;
}
int64_t number(std::istream& r){int64_t n;require(bool(r>>n),"truncated action4 contact fixture");return n;}
void guard(){
 auto map=rectangle();l3::ActorContactWork w;w.raw={{0,-128,0,1234}};
 for(size_t i=0;i<4;++i)w.fixed[i]=int32_t(w.raw[i])*65536+(i==1?0:12345);
 w.previous_raw=w.raw;w.previous_fixed=w.fixed;w.velocity={{321,-654,987}};w.state74=1;
 auto s=callback(w,0,19,0);l3::ReferenceTables refs;refs.trig.fill({0,4096});
 auto p=l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds);
 require(p.status==Status::SourceBackedPartial && p.inputs && p.inputs->source_forward==-112 && p.inputs->raw_14[2]==7 && p.inputs->fixed_1c[2]==w.fixed[2]+4096*112,"negative112 source motion wrong");
 require(s.behavior.phase_0a==0 && s.raw_14==w.raw,"contact mutated original");
 auto stale=w;++stale.previous_raw[3];require(l3::prepare_actor_action4_contact(s,stale,map,0,refs,bounds).status==Status::StaleWork,"stale fourth snapshot accepted");
 s.behavior.phase_0a=2;s.behavior.timer_32c=1;require(l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds).status==Status::NotMovingBranch,"child boundary manufactured motion");
 s.behavior.phase_0a=3;s.behavior.timer_32c=0;p=l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds);require(p.inputs && p.inputs->source_forward==224 && p.plan.phase_after==4 && p.plan.timer_after==5,"phase3 fallthrough positive224 omitted");
 s.behavior.phase_0a=3;s.behavior.timer_32c=12;p=l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds);require(p.inputs && p.inputs->source_forward==-352,"timer-dependent reverse motion wrong");
 map=rectangle(8);require(l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds).status==Status::UnsupportedWall,"unsupported contact admitted");
 map=rectangle();map.cells.clear();require(l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds).status==Status::UnsupportedBoundary,"invalid cell admitted");
 map=rectangle();const auto count=allocations;bool ok=true;
 for(int i=0;i<640;++i)ok&=l3::prepare_actor_action4_contact(s,w,map,0,refs,bounds).status==Status::SourceBackedPartial;
 require(ok && allocations==count,"action4 contact hot loop allocated");
}
void fixtures(const char* path){
 std::ifstream f(path);require(bool(f),"action4 contact fixture missing");std::string line;std::getline(f,line);size_t admitted=0,held=0,corrected=0;
 while(std::getline(f,line)){if(line.empty())continue;std::istringstream r(line);
 const auto phase=static_cast<uint8_t>(number(r));const auto timer=static_cast<int16_t>(number(r));const auto heading=static_cast<uint16_t>(number(r));const auto forward=static_cast<int16_t>(number(r));
 const auto sin=static_cast<int16_t>(number(r)),cos=static_cast<int16_t>(number(r));const auto code=static_cast<uint16_t>(number(r)),flags=static_cast<uint16_t>(number(r));const auto raw6=static_cast<int16_t>(number(r));
 l3::RetailPlayerBounds b;b.x0=static_cast<int16_t>(number(r));b.x1=static_cast<int16_t>(number(r));b.z0=static_cast<int16_t>(number(r));b.z1=static_cast<int16_t>(number(r));b.y0=static_cast<int16_t>(number(r));b.y1=static_cast<int16_t>(number(r));
 l3::ActorContactWork before;for(auto& v:before.raw)v=static_cast<int16_t>(number(r));for(auto& v:before.fixed)v=static_cast<int32_t>(number(r));for(auto& v:before.velocity)v=static_cast<int16_t>(number(r));before.state74=static_cast<uint8_t>(number(r));before.previous_raw=before.raw;before.previous_fixed=before.fixed;
 const auto wall=static_cast<uint32_t>(number(r));const auto floor=static_cast<int32_t>(number(r));auto after=before;for(auto& v:after.raw)v=static_cast<int16_t>(number(r));for(auto& v:after.fixed)v=static_cast<int32_t>(number(r));for(auto& v:after.velocity)v=static_cast<int16_t>(number(r));after.state74=static_cast<uint8_t>(number(r));std::string extra;require(!(r>>extra),"extra contact4 fields");
 const auto s=callback(before,phase,timer,heading);auto map=rectangle(code,flags,raw6);l3::ReferenceTables refs;refs.trig.fill({sin,cos});const auto p=l3::prepare_actor_action4_contact(s,before,map,0,refs,b);
 if(p.status==Status::SourceBackedPartial){
  if(!p.inputs || uint32_t(p.wall.result)!=wall || p.floor.result!=floor || p.after_wall.raw!=after.raw || p.after_wall.fixed!=after.fixed || p.after_wall.velocity!=after.velocity || p.after_wall.state74!=after.state74 || p.inputs->source_forward!=forward){std::cerr<<"case "<<admitted+held<<" code "<<code<<" forward "<<forward<<" wall "<<uint32_t(p.wall.result)<<" expected "<<wall<<'\n';throw std::runtime_error("original motion4/wall/floor mismatch");}
  ++admitted;corrected+=wall!=0;
 }else {require(!p.inputs,"held contact4 published inputs");++held;}
 }
 std::cerr<<"coverage admitted="<<admitted<<" held="<<held<<" corrected="<<corrected<<'\n';require(admitted>100 && held>0 && corrected>0,"insufficient contact4 coverage");std::cout<<"source contact4 admitted="<<admitted<<" held="<<held<<" corrected="<<corrected<<'\n';
}
}
int main(int argc,char** argv){try{guard();if(argc>1)fixtures(argv[1]);std::cout<<"PASS distinct action4 contact preparation\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
