#include "legends3ds/class20_03_action4.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using State=l3::Class20_03CallbackState;using Status=l3::Class20_03Action4Status;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int64_t number(std::istream& r){int64_t n;require(bool(r>>n),"truncated action4 fixture");return n;}
std::array<int64_t,32> values(const State& s){
 const auto& b=s.behavior;std::array<int64_t,32> a{{b.action_09,b.phase_0a,b.animation_ac,b.timer_32c,b.work_330,b.flags_328,b.heading_56,s.aux_0b,s.variant_32e,s.work_35c,s.work_370,s.child_374}};
 size_t c=12;const auto add=[&](const auto& v){for(auto n:v)a[c++]=n;};add(s.raw_14);add(s.fixed_1c);add(s.previous_raw_2c);add(s.previous_fixed_34);add(s.angle_54);return a;
}
State read(std::istream& r){
 State s;auto& b=s.behavior;const auto take=[&](auto& n){n=static_cast<std::decay_t<decltype(n)>>(number(r));};
 take(b.action_09);take(b.phase_0a);take(b.animation_ac);take(b.timer_32c);take(b.work_330);take(b.flags_328);take(b.heading_56);take(s.aux_0b);take(s.variant_32e);take(s.work_35c);take(s.work_370);take(s.child_374);
 const auto arr=[&](auto& a){for(auto& v:a)take(v);};arr(s.raw_14);arr(s.fixed_1c);arr(s.previous_raw_2c);arr(s.previous_fixed_34);arr(s.angle_54);return s;
}
void guards(){
 State s;s.behavior.action_09=4;s.behavior.phase_0a=0;s.behavior.timer_32c=719;s.behavior.work_330=92;s.child_374=0x12345678;
 l3::Class20_03Action4Inputs in;const auto before=values(s);
 require(l3::class20_03_step_action4(s,in).status==Status::NeedsSoundConsumer,"entry cue skipped");in.sound_consumer_ready=true;
 require(l3::class20_03_step_action4(s,in).status==Status::NeedsMotion,"negative motion invented");
 l3::Class20_03PreparedAction4 m;m.source_forward=224;in.prepared_motion=m;
 require(l3::class20_03_step_action4(s,in).status==Status::StaleMotion,"positive motion substituted");require(values(s)==before,"held entry leaked fields");
 m.source_forward=-112;in.prepared_motion=m;
 require(l3::class20_03_step_action4(s,in).status==Status::SourceBackedPartial && s.behavior.phase_0a==1 && s.behavior.timer_32c==12 && s.behavior.animation_ac==21 && s.behavior.work_330==0,"entry phase/timer fallthrough");
 s.behavior.phase_0a=2;s.behavior.timer_32c=1;const auto spawn=values(s);
 require(l3::class20_03_step_action4(s,in).status==Status::NeedsChildConsumer && values(s)==spawn,"missing child consumer admitted");
 in.produced_child_374=0;const auto full=l3::class20_03_step_action4(s,in);
 require(full.status==Status::SourceBackedPartial && full.child_80104078 && s.child_374==0 && s.behavior.phase_0a==3 && s.behavior.timer_32c==11,"explicit pool exhaustion not honored");
 s.behavior.phase_0a=4;s.behavior.timer_32c=0;const auto expired=values(s);
 require(l3::class20_03_step_action4(s,in).status==Status::NeedsCombatProfile && values(s)==expired,"byteC invented");in.combat_record_byte_c=231;
 require(l3::class20_03_step_action4(s,in).status==Status::SourceBackedPartial && s.work_35c==231 && (s.behavior.flags_328&1),"unsigned byteC expiry mismatch");
 s.behavior.phase_0a=3;s.behavior.timer_32c=0;m.source_forward=224;in.prepared_motion=m;
 require(l3::class20_03_step_action4(s,in).status==Status::SourceBackedPartial && s.behavior.phase_0a==4 && s.behavior.timer_32c==5,"phase3->4 immediate decrement");
 s.behavior.phase_0a=2;s.behavior.timer_32c=2;const auto count=allocations;bool ok=true;
 for(int i=0;i<640;++i){auto copy=s;ok &= l3::class20_03_step_action4(copy,in).status==Status::SourceBackedPartial;}
 require(ok && count==allocations,"action4 hot-loop allocation");
}
void fixture(const char* path){
 std::ifstream f(path);require(bool(f),"action4 source fixture missing");std::string line;std::getline(f,line);size_t cases=0,moved=0,childs=0,cues=0;
 while(std::getline(f,line)){if(line.empty())continue;std::istringstream row(line);l3::Class20_03Action4Inputs in;l3::Class20_03PreparedAction4 m;
 const auto present=number(row);m.source_forward=static_cast<int16_t>(number(row));m.source_floor_word=static_cast<int32_t>(number(row));
 in.produced_child_374=static_cast<uint32_t>(number(row));in.combat_record_byte_c=static_cast<uint8_t>(number(row));
 for(auto& v:m.raw_14)v=static_cast<int16_t>(number(row));for(auto& v:m.fixed_1c)v=static_cast<int32_t>(number(row));
 auto s=read(row);const auto expected=read(row);const auto sound=number(row);std::array<uint32_t,2> words{};for(auto& v:words)v=static_cast<uint32_t>(number(row));const auto child=number(row);
 std::string extra;require(!(row>>extra),"extra action4 fields");m.heading=s.behavior.heading_56;if(present)in.prepared_motion=m;in.sound_consumer_ready=true;
 const auto out=l3::class20_03_step_action4(s,in);
 if(out.status!=Status::SourceBackedPartial || values(s)!=values(expected)){
  std::cerr<<"action4 case "<<cases<<" status "<<int(out.status)<<'\n';const auto a=values(s),b=values(expected);for(size_t i=0;i<a.size();++i)if(a[i]!=b[i])std::cerr<<i<<": "<<a[i]<<" expected "<<b[i]<<'\n';throw std::runtime_error("original action4 differential mismatch");}
 require((out.sound_8001a050?*out.sound_8001a050:-1)==sound && out.child_80104078==bool(child),"source action4 consumer ordering mismatch");
 if(sound>=0){require(out.sound_pointed_words && *out.sound_pointed_words==words,"entry cue did not copy pre-motion point");++cues;}else require(!out.sound_pointed_words,"no cue published position");
 ++cases;moved+=present;childs+=child;
 }
 require(cases>1000 && moved>100 && childs>100 && cues>100,"insufficient source phase/wrap/rollback coverage");std::cout<<"original action4 cases="<<cases<<" movement="<<moved<<" children="<<childs<<" cues="<<cues<<'\n';
}
uint32_t word(const std::array<uint8_t,l3::class60_slot_bytes>& s,size_t off){uint32_t v=0;for(size_t i=0;i<4;++i)v|=uint32_t(s[off+i])<<(i*8);return v;}
void pool_fixture(const char* path,const char* releases){
 std::ifstream f(path);require(bool(f),"child pool fixture missing");std::string line;std::getline(f,line);size_t cases=0;
 while(std::getline(f,line)){if(line.empty())continue;std::istringstream row(line);l3::Class60SourcePool pool;pool.allocated_bitmap=static_cast<uint32_t>(number(row));State s;s.variant_32e=static_cast<int16_t>(number(row));s.behavior.heading_56=static_cast<uint16_t>(number(row));
 const auto sin=static_cast<int16_t>(number(row)),cos=static_cast<int16_t>(number(row));for(auto& v:s.raw_14)v=static_cast<int16_t>(number(row));for(auto& v:s.fixed_1c)v=static_cast<int32_t>(number(row));
 const auto identity=static_cast<uint32_t>(number(row)),bitmap=static_cast<uint32_t>(number(row));const auto sound=number(row);std::array<uint32_t,2> soundwords{};for(auto& v:soundwords)v=static_cast<uint32_t>(number(row));std::array<uint8_t,l3::class60_slot_bytes> expected{};for(auto& v:expected)v=static_cast<uint8_t>(number(row));
 for(auto& slot:pool.slots)for(size_t i=0;i<slot.size();++i)slot[i]=uint8_t(i*17+3);
 l3::ReferenceTables refs;refs.trig.fill({sin,cos});const auto out=l3::class20_03_produce_child_80104078(s,pool,refs,true);
 require(out.status==Status::SourceBackedPartial && out.source_identity==identity && pool.allocated_bitmap==bitmap,"source allocator/initializer identity mismatch");
 if(identity)require(pool.slots[*l3::class60_source_slot(identity)]==expected,"source child initializer byte mutation mismatch");
 require((out.sound_8001a050?*out.sound_8001a050:-1)==sound,"source child178 mismatch");if(sound>=0)require(out.sound_pointed_words && *out.sound_pointed_words==soundwords,"source child178 position mismatch");++cases;
 }
 std::ifstream release(releases);require(bool(release),"child release fixture missing");std::getline(release,line);size_t released=0;
 while(std::getline(release,line)){if(line.empty())continue;std::istringstream row(line);const auto slot=static_cast<size_t>(number(row));l3::Class60SourcePool pool;pool.allocated_bitmap=static_cast<uint32_t>(number(row));pool.pending_release_bitmap=static_cast<uint32_t>(number(row));const auto mask=static_cast<uint32_t>(number(row)),pending=static_cast<uint32_t>(number(row));const auto byte=number(row);const auto bitmap=static_cast<uint32_t>(number(row));pool.slots[slot][0]=255;
 require(l3::class60_mark_release_800318d8(pool,l3::class60_source_pool_base+uint32_t(slot*l3::class60_slot_bytes)),"valid release rejected");require(pool.pending_release_bitmap==pending && pool.slots[slot][0]==byte,"pending release source mismatch");l3::class60_apply_release_mask_80031b3c(pool,mask);require(pool.allocated_bitmap==bitmap && pool.pending_release_bitmap==pending,"batch cleanup source mismatch");++released;
 }
 require(cases==135 && released==32,"missing allocator/release cases");std::cout<<"original child pool cases="<<cases<<" release="<<released<<'\n';
}
void pool_guards(){
 l3::Class60SourcePool p;for(size_t i=0;i<32;++i){const auto id=l3::class60_allocate_80031518(p);require(id==l3::class60_source_pool_base+i*l3::class60_slot_bytes,"allocator slot order changed");p.slots[i][0]=1;}
 require(l3::class60_allocate_80031518(p)==0,"32 slot exhaustion invented capacity");const auto id=l3::class60_source_pool_base+15*l3::class60_slot_bytes;
 require(l3::class60_mark_release_800318d8(p,id) && l3::class60_allocate_80031518(p)==0,"pending release immediately reusable");l3::class60_apply_release_mask_80031b3c(p,p.pending_release_bitmap);require(l3::class60_allocate_80031518(p)==id,"released slot not first-free reused");
 const auto before=p.allocated_bitmap;require(!l3::class60_mark_release_800318d8(p,id+1) && p.allocated_bitmap==before,"unaligned identity admitted");require(!l3::class60_source_slot(0) && !l3::class60_source_slot(l3::class60_source_pool_base+32*l3::class60_slot_bytes),"external pointer admitted");
 State s;l3::ReferenceTables refs;p={};const auto held=l3::class20_03_produce_child_80104078(s,p,refs,false);require(held.status==Status::NeedsSoundConsumer && p.allocated_bitmap==0,"child cue hold allocated slot");
 const auto count=allocations;bool ok=true;for(int i=0;i<640;++i){l3::Class60SourcePool local;auto result=l3::class20_03_produce_child_80104078(s,local,refs,true);ok&=result.source_identity==l3::class60_source_pool_base && word(local.slots[0],0x9c)==32;}
 require(ok && count==allocations,"child scalar producer allocated host heap");
}
}
int main(int argc,char** argv){try{guards();pool_guards();if(argc>1)fixture(argv[1]);if(argc>3)pool_fixture(argv[2],argv[3]);std::cout<<"PASS action4 body, pool and explicit dependency holds\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
