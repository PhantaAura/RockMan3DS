#include "legends3ds/actor_runtime.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
static size_t allocations=0;
void* operator new(std::size_t n) { ++allocations; if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void* operator new[](std::size_t n) { ++allocations; if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void operator delete(void*p)noexcept{std::free(p);}
void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}
void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
int main(int argc,char**argv){
 assert(argc==2);std::ifstream f(argv[1],std::ios::binary);assert(f);
 std::vector<uint8_t>b{std::istreambuf_iterator<char>(f),{}};
 const auto ebd=l3::load_l3ebd_directory(b);
 l3::StageObjectList list;
 for(unsigned i=0;i<3;++i){l3::StageObjectRecord r;r.id=1;r.object_class=0x20;
  r.raw[0]=1;r.raw[1]=0x20;r.raw[2]=3;r.raw[3]=0xa0;r.raw[7]=64;
  r.x=367;r.y=-1;r.z=-6946;
  const std::array<int16_t,3>xyz{{r.x,r.y,r.z}};
  for(size_t j=0;j<3;++j){const uint16_t v=static_cast<uint16_t>(xyz[j]);r.raw[12+2*j]=static_cast<uint8_t>(v);r.raw[13+2*j]=static_cast<uint8_t>(v>>8);}
  list.records.push_back(r);
 }
 auto area=l3::spawn_imported_actors(l3::build_fresh_stage_runtime_area(list,ebd),ebd);
 assert(area.actors.size()==3);
 const std::array<uint8_t,8>ids{{0,1,2,3,4,6,21,22}};
 l3::bind_imported_actor_animation_banks(area,ebd,ids.data(),ids.size());
 for(auto&a:area.actors)l3::bind_imported_actor_floor(a,-1);
 assert(area.unique_pose_cache_count==1&&area.actors[0].animation_bank==area.actors[1].animation_bank);
 l3::Class20_03CallbackInputs input;input.action.packet_33c=0;input.action.predicate_8001da58=false;
 const auto before_alloc=allocations;
 for(unsigned t=0;t<480;++t)for(auto&a:area.actors){const auto tick=l3::tick_imported_actor_state1(a,input);assert(tick.committed);assert(a.source_origin[0]==367&&a.source_origin[2]==-6946);}
 auto&a=area.actors[0];a.source_behavior->behavior.action_09=1;a.source_behavior->behavior.phase_0a=0;
 input.action.target_heading_80031198=2048;
 const auto reset=l3::tick_imported_actor_state1(a,input);
 assert(reset.committed&&reset.request_changed&&a.pose->request==1&&a.pose->record_index==0);
 assert(a.animation_cursor->record_index==0&&a.pose->countdown==a.animation_cursor->countdown_a5);
 const auto advance=l3::tick_imported_actor_state1(a,input);
 assert(advance.committed&&!advance.request_changed&&a.pose->record_index==1);
 const auto saved_heading=a.heading;
 const auto saved_record=a.pose->record_index;
 const auto saved_countdown=a.animation_cursor->countdown_a5;
 input.action.packet_33c=1;
 const auto blocked=l3::tick_imported_actor_state1(a,input);
 assert(!blocked.committed&&blocked.status==l3::Class20_03CallbackStatus::NeedsPacket);
 assert(a.heading==saved_heading&&a.pose->record_index==saved_record&&a.animation_cursor->countdown_a5==saved_countdown);
 assert(allocations==before_alloc);
 std::weak_ptr<const l3::ActorAnimationBank>life=a.animation_bank;
 l3::unload_imported_actors(area);assert(life.expired());
 std::cout<<"owner integration: shared bank, 1440 state1 steps, request1 reset/advance, packet hold, zero frame allocations, unload lifetime passed\n";
}
